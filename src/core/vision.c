/**
 * @file vision.c
 * @brief Ray-based vision system implementation
 * 
 * Each ant casts 7 rays across a 180° FOV to detect:
 *   - Walls (obstacles)
 *   - Other ants
 *   - Food sources
 * 
 * Distances are normalized 0-1 where:
 *   - 0 = object at maximum range (100 pixels) or not detected
 *   - 1 = object very close
 * 
 * This inverted scale makes it intuitive for the neural network:
 * higher values = more urgent/closer threats or opportunities.
 */

#include "vision.h"
#include "walls.h"
#include "utils.h"
#include <math.h>
#include <string.h>

/* ============== Pre-computed Ray Angles ============== */

static float precomputed_angles[NN_NUM_VISION_RAYS];
static bool angles_initialized = false;

static void init_ray_angles(void) {
    if (angles_initialized) return;
    
    float half_fov = (VISION_FOV_DEGREES / 2.0f) * (float)M_PI / 180.0f;
    
    for (int i = 0; i < NN_NUM_VISION_RAYS; i++) {
        if (NN_NUM_VISION_RAYS > 1) {
            float t = (float)i / (float)(NN_NUM_VISION_RAYS - 1);
            precomputed_angles[i] = -half_fov + t * 2.0f * half_fov;
        } else {
            precomputed_angles[i] = 0.0f;
        }
    }
    
    angles_initialized = true;
}

/* ============== Ray Casting Helpers ============== */

/**
 * @brief Raycast against walls using stepped sampling
 */
static float raycast_walls(float start_x, float start_y,
                           float ray_dx, float ray_dy,
                           const WallManager *wall_manager) {
    if (!wall_manager) return VISION_RAY_LENGTH;
    
    const float step_size = 8.0f;
    int num_steps = (int)(VISION_RAY_LENGTH / step_size);
    
    for (int step = 1; step <= num_steps; step++) {
        float dist = step * step_size;
        float check_x = start_x + ray_dx * dist;
        float check_y = start_y + ray_dy * dist;
        
        if (walls_is_colliding(wall_manager, check_x, check_y, 1, NULL)) {
            /* Binary search for precise distance */
            float low = (step - 1) * step_size;
            float high = dist;
            
            for (int i = 0; i < 4; i++) {
                float mid = (low + high) / 2.0f;
                float cx = start_x + ray_dx * mid;
                float cy = start_y + ray_dy * mid;
                
                if (walls_is_colliding(wall_manager, cx, cy, 1, NULL)) {
                    high = mid;
                } else {
                    low = mid;
                }
            }
            return low;
        }
    }
    
    return VISION_RAY_LENGTH;
}

/**
 * @brief Ray-circle intersection test
 * @return Distance to intersection, or RAY_LENGTH if no hit
 */
static float ray_circle_intersection(float ray_ox, float ray_oy,
                                     float ray_dx, float ray_dy,
                                     float cx, float cy, float radius) {
    /* Vector from ray origin to circle center */
    float ocx = cx - ray_ox;
    float ocy = cy - ray_oy;
    
    /* Project onto ray direction */
    float t_closest = ocx * ray_dx + ocy * ray_dy;
    
    /* Closest point is behind ray */
    if (t_closest < 0) return VISION_RAY_LENGTH;
    
    /* Distance squared from closest point on ray to circle center */
    float closest_x = ray_ox + ray_dx * t_closest;
    float closest_y = ray_oy + ray_dy * t_closest;
    float dist_sq = (closest_x - cx) * (closest_x - cx) + 
                    (closest_y - cy) * (closest_y - cy);
    
    float radius_sq = radius * radius;
    if (dist_sq > radius_sq) return VISION_RAY_LENGTH;
    
    /* Calculate intersection point */
    float half_chord = sqrtf(radius_sq - dist_sq);
    float t_hit = t_closest - half_chord;
    
    if (t_hit < 0) t_hit = t_closest + half_chord;
    
    return (t_hit > 0 && t_hit < VISION_RAY_LENGTH) ? t_hit : VISION_RAY_LENGTH;
}

/**
 * @brief Raycast against other ants
 */
static float raycast_ants(float start_x, float start_y,
                          float ray_dx, float ray_dy,
                          const Ant *ants, int ant_count,
                          uint32_t exclude_id) {
    float nearest_dist = VISION_RAY_LENGTH;
    
    for (int i = 0; i < ant_count; i++) {
        const Ant *ant = &ants[i];
        
        if (!ant->alive || ant->id == exclude_id) continue;
        
        /* Quick distance check first */
        float dx = ant->x - start_x;
        float dy = ant->y - start_y;
        float dist_sq = dx * dx + dy * dy;
        
        if (dist_sq > VISION_RAY_LENGTH_SQ) continue;
        
        /* Ray-circle intersection */
        float hit_dist = ray_circle_intersection(
            start_x, start_y, ray_dx, ray_dy,
            ant->x, ant->y, ant->radius
        );
        
        if (hit_dist < nearest_dist) {
            nearest_dist = hit_dist;
        }
    }
    
    return nearest_dist;
}

/**
 * @brief Raycast against food sources
 */
static float raycast_food(float start_x, float start_y,
                          float ray_dx, float ray_dy,
                          const FoodSource *food_sources, int food_count) {
    float nearest_dist = VISION_RAY_LENGTH;
    
    for (int i = 0; i < food_count; i++) {
        const FoodSource *food = &food_sources[i];
        
        if (food->amount <= 0) continue;
        
        /* Quick distance check */
        float dx = food->x - start_x;
        float dy = food->y - start_y;
        float dist_sq = dx * dx + dy * dy;
        
        if (dist_sq > VISION_RAY_LENGTH_SQ) continue;
        
        /* Ray-circle intersection */
        float hit_dist = ray_circle_intersection(
            start_x, start_y, ray_dx, ray_dy,
            food->x, food->y, food->radius
        );
        
        if (hit_dist < nearest_dist) {
            nearest_dist = hit_dist;
        }
    }
    
    return nearest_dist;
}

/* ============== Public Functions ============== */

void vision_init(AntVision *vision) {
    init_ray_angles();
    
    vision->num_rays = NN_NUM_VISION_RAYS;
    memcpy(vision->ray_angles, precomputed_angles, sizeof(precomputed_angles));
    
    vision_reset(vision);
}

void vision_reset(AntVision *vision) {
    for (int i = 0; i < vision->num_rays; i++) {
        vision->rays[i].wall_dist = 1.0f;
        vision->rays[i].ant_dist = 1.0f;
        vision->rays[i].food_dist = 1.0f;
        vision->rays[i].hit_wall = false;
        vision->rays[i].hit_ant = false;
        vision->rays[i].hit_food = false;
    }
}

void vision_cast_rays(AntVision *vision,
                      float ant_x, float ant_y, float ant_direction,
                      const WallManager *wall_manager,
                      const Ant *ants, int ant_count,
                      const FoodSource *food_sources, int food_count,
                      uint32_t exclude_id) {
    
    for (int i = 0; i < vision->num_rays; i++) {
        float ray_angle = ant_direction + vision->ray_angles[i];
        float ray_dx = cosf(ray_angle);
        float ray_dy = sinf(ray_angle);
        
        VisionRay *ray = &vision->rays[i];
        
        /* Cast against each object type */
        float wall_dist = raycast_walls(ant_x, ant_y, ray_dx, ray_dy, wall_manager);
        float ant_dist = raycast_ants(ant_x, ant_y, ray_dx, ray_dy, 
                                       ants, ant_count, exclude_id);
        float food_dist = raycast_food(ant_x, ant_y, ray_dx, ray_dy,
                                        food_sources, food_count);
        
        /* Normalize and invert (1 = close, 0 = far) */
        ray->wall_dist = 1.0f - (wall_dist / VISION_RAY_LENGTH);
        ray->ant_dist = 1.0f - (ant_dist / VISION_RAY_LENGTH);
        ray->food_dist = 1.0f - (food_dist / VISION_RAY_LENGTH);
        
        ray->hit_wall = wall_dist < VISION_RAY_LENGTH;
        ray->hit_ant = ant_dist < VISION_RAY_LENGTH;
        ray->hit_food = food_dist < VISION_RAY_LENGTH;
    }
}

void vision_get_inputs(const AntVision *vision, float *inputs) {
    /* Layout: [wall×7, ant×7, food×7] */
    int idx = 0;
    
    /* Wall distances */
    for (int i = 0; i < vision->num_rays; i++) {
        inputs[idx++] = vision->rays[i].wall_dist;
    }
    
    /* Ant distances */
    for (int i = 0; i < vision->num_rays; i++) {
        inputs[idx++] = vision->rays[i].ant_dist;
    }
    
    /* Food distances */
    for (int i = 0; i < vision->num_rays; i++) {
        inputs[idx++] = vision->rays[i].food_dist;
    }
}
