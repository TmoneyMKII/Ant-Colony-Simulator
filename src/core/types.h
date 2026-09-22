/**
 * @file types.h
 * @brief Core type definitions for the ant colony simulator
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

/* ============== Forward Declarations ============== */
typedef struct Colony Colony;
typedef struct Ant Ant;
typedef struct NeuralNetwork NeuralNetwork;
typedef struct PheromoneMap PheromoneMap;
typedef struct WallManager WallManager;
typedef struct VisionRay VisionRay;
typedef struct AntVision AntVision;

/* ============== Enumerations ============== */

/**
 * @brief Ant behavior states
 */
typedef enum {
    ANT_STATE_FORAGING = 0,   /**< Looking for food - follow food trail */
    ANT_STATE_RETURNING = 1,  /**< Carrying food home - follow home trail */
    ANT_STATE_IDLE = 2        /**< Resting at colony */
} AntState;

/**
 * @brief Types of pheromone trails
 */
typedef enum {
    PHEROMONE_FOOD_TRAIL = 0,   /**< Green - leads TO food */
    PHEROMONE_HOME_TRAIL = 1,   /**< Blue - leads TO home/colony */
    PHEROMONE_DANGER = 2        /**< Red - where ants died */
} PheromoneType;

/* ============== 2D Vector ============== */

/**
 * @brief 2D vector for positions and directions
 */
typedef struct {
    float x;
    float y;
} Vec2;

/* ============== Bounding Rectangle ============== */

/**
 * @brief Axis-aligned bounding rectangle
 */
typedef struct {
    int x;
    int y;
    int width;
    int height;
} Rect;

/* ============== Color ============== */

/**
 * @brief RGBA color
 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Color;

/* ============== Vision Ray Result ============== */

/**
 * @brief Single vision ray result
 */
struct VisionRay {
    float wall_dist;    /**< Normalized distance to wall (1.0 = nothing in range) */
    float ant_dist;     /**< Normalized distance to nearest ant */
    float food_dist;    /**< Normalized distance to nearest food */
    bool hit_wall;
    bool hit_ant;
    bool hit_food;
};

/**
 * @brief Vision system for an ant
 */
struct AntVision {
    int num_rays;
    VisionRay rays[NN_NUM_VISION_RAYS];
    float ray_angles[NN_NUM_VISION_RAYS];  /**< Pre-computed angles relative to heading */
};

/* ============== Neural Network ============== */

/**
 * @brief Feedforward neural network for ant decision-making
 * 
 * Architecture: 27 inputs -> 16 hidden (tanh) -> 3 outputs
 */
struct NeuralNetwork {
    /* Weights stored flat for cache efficiency */
    float *weights;         /**< All weights in flat array */
    int total_weights;      /**< Total number of weights */
    
    /* Unpacked weight matrices for fast access */
    float w_ih[NN_INPUT_SIZE][NN_HIDDEN_SIZE];   /**< Input -> Hidden */
    float w_ho[NN_HIDDEN_SIZE][NN_OUTPUT_SIZE];  /**< Hidden -> Output */
    float b_h[NN_HIDDEN_SIZE];                    /**< Hidden biases */
    float b_o[NN_OUTPUT_SIZE];                    /**< Output biases */
    
    /* Cached activations for visualization */
    float last_inputs[NN_INPUT_SIZE];
    float last_hidden[NN_HIDDEN_SIZE];
    float last_outputs[NN_OUTPUT_SIZE];
};

/* ============== Ant ============== */

/**
 * @brief Individual ant agent
 */
struct Ant {
    uint32_t id;            /**< Unique identifier */
    
    /* Position and movement */
    float x;
    float y;
    float direction;        /**< Heading in radians */
    float prev_direction;
    float speed;
    
    /* State */
    AntState state;
    bool alive;
    bool carrying_food;
    float food_amount;
    float energy;
    float max_energy;
    
    /* Memory */
    bool knows_food_location;
    float last_food_x;
    float last_food_y;
    
    /* Target (if any) */
    float target_x;
    float target_y;
    bool has_target;
    
    /* Pheromone */
    float pheromone_strength;
    float pheromone_sensitivity;
    bool can_deposit;
    int deposit_cooldown;
    
    /* Stats */
    int food_collected;
    int successful_trips;
    int time_alive;
    
    /* Stuck detection */
    int movement_timer;
    float checkpoint_x;
    float checkpoint_y;
    int stuck_escape_count;
    
    /* Neural network brain */
    NeuralNetwork *brain;
    bool use_neural_net;
    
    /* Vision system */
    AntVision vision;
    
    /* Rendering */
    float radius;
    Color color;
    
    /* Reference to colony */
    Colony *colony;
};

/* ============== Food Source ============== */

/**
 * @brief A food source on the map
 */
typedef struct {
    float x;
    float y;
    float amount;
    float max_amount;
    float radius;
    Color color;
} FoodSource;

/* ============== Death Marker ============== */

/**
 * @brief Visual marker where an ant died
 */
typedef struct {
    float x;
    float y;
    int frames_remaining;
} DeathMarker;

/* ============== Pheromone Layer ============== */

/**
 * @brief Single pheromone layer grid
 */
typedef struct {
    float *grid;            /**< 2D grid stored as 1D array (row-major) */
    int grid_width;
    int grid_height;
} PheromoneLayer;

/**
 * @brief Complete pheromone system with multiple trail types
 */
struct PheromoneMap {
    int width;              /**< World width in pixels */
    int height;             /**< World height in pixels */
    int cell_size;
    int grid_width;         /**< Grid width in cells */
    int grid_height;        /**< Grid height in cells */
    
    PheromoneLayer food_trail;
    PheromoneLayer home_trail;
    PheromoneLayer danger_trail;
    
    float max_pheromone;
    float evaporation_rate;
    float danger_evap_rate;
    float detection_threshold;
};

/* ============== Wall Segment ============== */

/**
 * @brief A wall segment for collision
 */
typedef struct {
    Rect bounds;
} WallSegment;

/**
 * @brief Wall manager with spatial partitioning
 */
struct WallManager {
    WallSegment *walls;
    int wall_count;
    int wall_capacity;
    
    int width;
    int height;
    int offset_x;
    int offset_y;
    
    /* Spatial hash grid for fast collision */
    int *spatial_grid;      /**< Cell -> first wall index */
    int *wall_next;         /**< wall index -> next wall in same cell */
    int spatial_width;
    int spatial_height;
    int spatial_cell_size;
};

/* ============== Colony Brain (Evolution) ============== */

/**
 * @brief Elite ant info for evolution
 */
typedef struct {
    float *weights;
    float fitness;
    int food_collected;
} EliteAnt;

/**
 * @brief Colony brain manages neural network evolution
 */
typedef struct {
    EliteAnt elites[ELITE_COUNT];
    int elite_count;
    int generation;
    int ants_evaluated;
} ColonyBrain;

/* ============== Colony ============== */

/**
 * @brief Main colony structure
 */
struct Colony {
    float x;
    float y;
    int width;
    int height;
    Rect bounds;
    float radius;
    Color color;
    
    /* Resources */
    float food_stored;
    float max_food;
    int population;
    int max_population;
    
    /* Entities */
    Ant *ants;
    int ant_count;
    int ant_capacity;
    
    FoodSource *food_sources;
    int food_count;
    int food_capacity;
    
    DeathMarker *death_markers;
    int death_marker_count;
    int death_marker_capacity;
    
    /* Systems */
    PheromoneMap *pheromone_map;
    WallManager *wall_manager;
    ColonyBrain *brain;
    
    /* Timing */
    int time;
    int generation_timer;
    
    /* Stats */
    int food_collected_this_frame;
};

/* ============== Simulation State ============== */

/**
 * @brief Global simulation state
 */
typedef struct {
    bool running;
    bool paused;
    bool show_pheromones;
    bool show_grid;
    bool show_neural_ui;
    bool show_hints;
    bool show_debug;
    
    float sim_speed;
    int speed_index;
    
    int screen_width;
    int screen_height;
    
    Colony *colony;
} SimState;

#endif /* TYPES_H */
