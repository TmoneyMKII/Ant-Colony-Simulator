"""Ray-based vision system for ants"""

import math
from typing import List, Tuple, Optional, TYPE_CHECKING

if TYPE_CHECKING:
    from src.ant import Ant

# Vision configuration
NUM_RAYS = 7  # Number of vision rays
FOV_DEGREES = 180  # Field of view in degrees (spread of rays)
RAY_LENGTH = 100  # Maximum ray length in pixels
RAY_LENGTH_SQ = RAY_LENGTH * RAY_LENGTH  # Squared for optimization

# Pre-calculate ray angles (relative to ant's heading)
# Spread rays evenly across FOV, centered on heading direction
_half_fov = math.radians(FOV_DEGREES / 2)
RAY_ANGLES = [
    -_half_fov + (i / (NUM_RAYS - 1)) * FOV_DEGREES * math.pi / 180
    for i in range(NUM_RAYS)
] if NUM_RAYS > 1 else [0.0]


class VisionRay:
    """Single vision ray result"""
    __slots__ = ['wall_dist', 'ant_dist', 'food_dist', 'hit_wall', 'hit_ant', 'hit_food']
    
    def __init__(self):
        self.wall_dist = 1.0  # Normalized distance (1.0 = nothing in range)
        self.ant_dist = 1.0
        self.food_dist = 1.0
        self.hit_wall = False
        self.hit_ant = False
        self.hit_food = False


class AntVision:
    """Vision system for a single ant using raycasting"""
    
    def __init__(self, num_rays: int = NUM_RAYS):
        self.num_rays = num_rays
        self.rays: List[VisionRay] = [VisionRay() for _ in range(num_rays)]
        self.ray_angles = RAY_ANGLES[:num_rays]
    
    def cast_rays(self, ant_x: float, ant_y: float, ant_direction: float,
                  wall_manager, ants: List['Ant'], food_sources: list,
                  ant_id: int) -> List[VisionRay]:
        """
        Cast all vision rays and return results.
        
        Args:
            ant_x, ant_y: Ant's position
            ant_direction: Ant's heading in radians
            wall_manager: Wall collision manager
            ants: List of all ants (to detect other ants)
            food_sources: List of food source objects
            ant_id: This ant's ID (to exclude self from detection)
        
        Returns:
            List of VisionRay objects with normalized distances
        """
        for i, angle_offset in enumerate(self.ray_angles):
            ray_angle = ant_direction + angle_offset
            self.rays[i] = self._cast_single_ray(
                ant_x, ant_y, ray_angle, wall_manager, ants, food_sources, ant_id
            )
        
        return self.rays
    
    def _cast_single_ray(self, start_x: float, start_y: float, angle: float,
                         wall_manager, ants: List['Ant'], food_sources: list,
                         exclude_ant_id: int) -> VisionRay:
        """Cast a single ray and find intersections with objects"""
        ray = VisionRay()
        
        # Ray direction vector
        ray_dx = math.cos(angle)
        ray_dy = math.sin(angle)
        
        # Check wall intersections
        if wall_manager is not None:
            wall_dist = self._raycast_walls(start_x, start_y, ray_dx, ray_dy, wall_manager)
            if wall_dist < RAY_LENGTH:
                ray.wall_dist = wall_dist / RAY_LENGTH
                ray.hit_wall = True
        
        # Check ant intersections (use spatial optimization)
        ant_dist = self._raycast_ants(start_x, start_y, ray_dx, ray_dy, ants, exclude_ant_id)
        if ant_dist < RAY_LENGTH:
            ray.ant_dist = ant_dist / RAY_LENGTH
            ray.hit_ant = True
        
        # Check food intersections
        food_dist = self._raycast_food(start_x, start_y, ray_dx, ray_dy, food_sources)
        if food_dist < RAY_LENGTH:
            ray.food_dist = food_dist / RAY_LENGTH
            ray.hit_food = True
        
        return ray
    
    def _raycast_walls(self, start_x: float, start_y: float,
                       ray_dx: float, ray_dy: float, wall_manager) -> float:
        """
        Cast ray against walls using stepped sampling.
        Returns distance to nearest wall hit, or RAY_LENGTH if no hit.
        """
        # Use stepped ray marching for wall detection
        # Step size balances accuracy vs performance
        step_size = 8.0
        num_steps = int(RAY_LENGTH / step_size)
        
        for step in range(1, num_steps + 1):
            dist = step * step_size
            check_x = start_x + ray_dx * dist
            check_y = start_y + ray_dy * dist
            
            # Check if point is inside a wall
            colliding, _ = wall_manager.is_colliding(check_x, check_y, 1)
            if colliding:
                # Binary search for more precise distance
                low = (step - 1) * step_size
                high = dist
                for _ in range(4):  # 4 iterations of binary search
                    mid = (low + high) / 2
                    check_x = start_x + ray_dx * mid
                    check_y = start_y + ray_dy * mid
                    colliding, _ = wall_manager.is_colliding(check_x, check_y, 1)
                    if colliding:
                        high = mid
                    else:
                        low = mid
                return low
        
        return RAY_LENGTH
    
    def _raycast_ants(self, start_x: float, start_y: float,
                      ray_dx: float, ray_dy: float,
                      ants: List['Ant'], exclude_id: int) -> float:
        """
        Cast ray against other ants.
        Uses ray-circle intersection for accurate detection.
        Returns distance to nearest ant hit, or RAY_LENGTH if no hit.
        """
        nearest_dist = RAY_LENGTH
        
        for ant in ants:
            if not ant.alive or ant.id == exclude_id:
                continue
            
            # Quick distance check first (squared distance for performance)
            dx = ant.x - start_x
            dy = ant.y - start_y
            dist_sq = dx * dx + dy * dy
            
            # Skip if ant is too far away
            if dist_sq > RAY_LENGTH_SQ:
                continue
            
            # Ray-circle intersection
            # Ray: P = start + t * dir, Circle: |P - center|^2 = r^2
            # Solve: |start + t*dir - center|^2 = r^2
            
            # Vector from ray start to circle center
            oc_x = start_x - ant.x
            oc_y = start_y - ant.y
            
            ant_radius = ant.radius + 2  # Slightly larger for detection
            
            # Quadratic coefficients: at^2 + bt + c = 0
            a = ray_dx * ray_dx + ray_dy * ray_dy  # Should be 1 for normalized dir
            b = 2 * (oc_x * ray_dx + oc_y * ray_dy)
            c = oc_x * oc_x + oc_y * oc_y - ant_radius * ant_radius
            
            discriminant = b * b - 4 * a * c
            
            if discriminant >= 0:
                # Ray intersects circle
                sqrt_disc = math.sqrt(discriminant)
                t1 = (-b - sqrt_disc) / (2 * a)
                t2 = (-b + sqrt_disc) / (2 * a)
                
                # Find nearest positive intersection
                if t1 > 0 and t1 < nearest_dist:
                    nearest_dist = t1
                elif t2 > 0 and t2 < nearest_dist:
                    nearest_dist = t2
        
        return nearest_dist
    
    def _raycast_food(self, start_x: float, start_y: float,
                      ray_dx: float, ray_dy: float,
                      food_sources: list) -> float:
        """
        Cast ray against food sources.
        Returns distance to nearest food hit, or RAY_LENGTH if no hit.
        """
        nearest_dist = RAY_LENGTH
        
        for food in food_sources:
            if food.amount <= 0:
                continue
            
            # Quick distance check
            dx = food.x - start_x
            dy = food.y - start_y
            dist_sq = dx * dx + dy * dy
            
            if dist_sq > RAY_LENGTH_SQ:
                continue
            
            # Ray-circle intersection (same as ants)
            oc_x = start_x - food.x
            oc_y = start_y - food.y
            
            food_radius = food.radius
            
            a = ray_dx * ray_dx + ray_dy * ray_dy
            b = 2 * (oc_x * ray_dx + oc_y * ray_dy)
            c = oc_x * oc_x + oc_y * oc_y - food_radius * food_radius
            
            discriminant = b * b - 4 * a * c
            
            if discriminant >= 0:
                sqrt_disc = math.sqrt(discriminant)
                t1 = (-b - sqrt_disc) / (2 * a)
                
                if t1 > 0 and t1 < nearest_dist:
                    nearest_dist = t1
        
        return nearest_dist
    
    def get_neural_inputs(self) -> List[float]:
        """
        Convert vision rays to neural network inputs.
        Returns flattened list of normalized distances.
        
        Format: [wall0, wall1, ..., wallN, ant0, ant1, ..., antN, food0, food1, ..., foodN]
        where each value is 0.0 (touching) to 1.0 (nothing in range)
        
        We invert these so 1.0 = object very close, 0.0 = nothing
        This makes more intuitive sense for the NN (high = danger/interest)
        """
        inputs = []
        
        # Wall distances (inverted: 1 = close, 0 = far/nothing)
        for ray in self.rays:
            inputs.append(1.0 - ray.wall_dist)
        
        # Ant distances (inverted)
        for ray in self.rays:
            inputs.append(1.0 - ray.ant_dist)
        
        # Food distances (inverted)  
        for ray in self.rays:
            inputs.append(1.0 - ray.food_dist)
        
        return inputs


# Global vision input size for neural network
VISION_INPUT_SIZE = NUM_RAYS * 3  # 7 rays × 3 types (wall, ant, food) = 21 inputs
