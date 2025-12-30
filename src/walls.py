"""Wall system for colony environment"""

import pygame
import math
from src.config import WALL_COLOR


class Wall:
    """Represents a wall obstacle in the colony"""
    def __init__(self, x, y, width, height):
        self.rect = pygame.Rect(x, y, width, height)
        self.color = WALL_COLOR
        
    def draw(self, surface):
        """Draw the wall"""
        pygame.draw.rect(surface, self.color, self.rect)
        # Add a border
        pygame.draw.rect(surface, (120, 60, 150), self.rect, width=2)
    
    def distance_to(self, x, y):
        """Get distance from point to nearest edge of wall"""
        closest_x = max(self.rect.left, min(x, self.rect.right))
        closest_y = max(self.rect.top, min(y, self.rect.bottom))
        dx = x - closest_x
        dy = y - closest_y
        return math.sqrt(dx*dx + dy*dy)
    
    def get_closest_point(self, x, y):
        """Get the closest point on the wall to a position"""
        closest_x = max(self.rect.left, min(x, self.rect.right))
        closest_y = max(self.rect.top, min(y, self.rect.bottom))
        return closest_x, closest_y
    
    def is_point_inside(self, x, y, margin=0):
        """Check if point is inside wall (with optional margin)"""
        return (self.rect.left - margin <= x <= self.rect.right + margin and
                self.rect.top - margin <= y <= self.rect.bottom + margin)


class WallManager:
    """Manages walls in the simulation"""
    def __init__(self, area_width, area_height, area_offset_x=0, area_offset_y=0):
        self.walls = []
        self.area_width = area_width
        self.area_height = area_height
        self.area_offset_x = area_offset_x
        self.area_offset_y = area_offset_y
        self._create_walls()
        
    def _create_walls(self):
        """Create wall obstacles - positioned to not block colony access"""
        center_x = self.area_offset_x + self.area_width // 2
        center_y = self.area_offset_y + self.area_height // 2
        
        # Left side obstacle (far from center)
        self.walls.append(Wall(
            self.area_offset_x + 120,
            self.area_offset_y + 180,
            35,
            280
        ))
        
        # Right side obstacle (far from center)
        self.walls.append(Wall(
            self.area_offset_x + self.area_width - 160,
            self.area_offset_y + 220,
            35,
            260
        ))
        
        # Top-left obstacle
        self.walls.append(Wall(
            self.area_offset_x + 350,
            self.area_offset_y + 80,
            45,
            180
        ))
        
        # Bottom-right obstacle
        self.walls.append(Wall(
            self.area_offset_x + self.area_width - 400,
            self.area_offset_y + self.area_height - 280,
            45,
            200
        ))
        
        # Bottom horizontal bar (with gap in middle)
        self.walls.append(Wall(
            self.area_offset_x + 550,
            self.area_offset_y + self.area_height - 200,
            250,
            35
        ))
        
        # Top horizontal bar
        self.walls.append(Wall(
            self.area_offset_x + center_x - 150,
            self.area_offset_y + 60,
            70,
            35
        ))
    
    def get_nearest_wall_info(self, x, y, look_range=150):
        """
        Get info about the nearest wall within range.
        Returns: (distance, angle_to_wall, wall) or (None, None, None) if no wall nearby
        """
        nearest_dist = float('inf')
        nearest_wall = None
        nearest_point = None
        
        for wall in self.walls:
            dist = wall.distance_to(x, y)
            if dist < nearest_dist and dist < look_range:
                nearest_dist = dist
                nearest_wall = wall
                nearest_point = wall.get_closest_point(x, y)
        
        if nearest_wall is None:
            return None, None, None
        
        # Calculate angle to wall
        angle_to_wall = math.atan2(nearest_point[1] - y, nearest_point[0] - x)
        return nearest_dist, angle_to_wall, nearest_wall
    
    def get_avoidance_direction(self, x, y, current_direction, look_range=100):
        """
        Calculate the best direction to avoid walls.
        Returns: (should_turn, turn_amount) 
        """
        dist, angle_to_wall, wall = self.get_nearest_wall_info(x, y, look_range)
        
        if dist is None:
            return False, 0
        
        # How much is the ant heading toward the wall?
        angle_diff = angle_to_wall - current_direction
        while angle_diff > math.pi:
            angle_diff -= 2 * math.pi
        while angle_diff < -math.pi:
            angle_diff += 2 * math.pi
        
        # If heading away from wall, no need to turn
        if abs(angle_diff) > math.pi * 0.6:  # More than 108 degrees away
            return False, 0
        
        # Urgency based on distance (closer = stronger turn)
        urgency = 1.0 - (dist / look_range)
        urgency = urgency ** 0.5  # Make it more aggressive when close
        
        # Turn perpendicular to wall - away from it
        # Decide which way to turn (left or right)
        if angle_diff > 0:
            turn = -0.5 * urgency  # Turn left (negative)
        else:
            turn = 0.5 * urgency   # Turn right (positive)
        
        return True, turn
    
    def is_path_blocked(self, x, y, target_x, target_y, check_distance=50):
        """Check if a straight path to target is blocked by a wall"""
        # Sample points along the path
        dx = target_x - x
        dy = target_y - y
        dist = math.sqrt(dx*dx + dy*dy)
        
        if dist == 0:
            return False
        
        # Check along path
        steps = min(5, int(dist / 10))
        for i in range(1, steps + 1):
            t = i / steps
            check_x = x + dx * t
            check_y = y + dy * t
            for wall in self.walls:
                if wall.is_point_inside(check_x, check_y, margin=15):
                    return True
        return False
    
    def is_colliding(self, x, y, radius=10):
        """Check if a point collides with any wall"""
        for wall in self.walls:
            if wall.is_point_inside(x, y, margin=radius):
                return True, wall
        return False, None
    
    def push_out_of_wall(self, x, y, radius=10):
        """If inside a wall, push out to nearest edge"""
        for wall in self.walls:
            if wall.is_point_inside(x, y, margin=0):
                # Find which edge is closest and push out
                dist_left = x - wall.rect.left
                dist_right = wall.rect.right - x
                dist_top = y - wall.rect.top
                dist_bottom = wall.rect.bottom - y
                
                min_dist = min(dist_left, dist_right, dist_top, dist_bottom)
                
                if min_dist == dist_left:
                    return wall.rect.left - radius - 1, y
                elif min_dist == dist_right:
                    return wall.rect.right + radius + 1, y
                elif min_dist == dist_top:
                    return x, wall.rect.top - radius - 1
                else:
                    return x, wall.rect.bottom + radius + 1
        
        return x, y
        
    def draw(self, surface):
        """Draw all walls"""
        for wall in self.walls:
            wall.draw(surface)
