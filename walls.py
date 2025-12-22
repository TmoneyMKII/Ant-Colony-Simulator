"""Wall system for colony environment"""

import pygame
import math
from config import WALL_COLOR, WALL_REPEL_RANGE, WALL_REPEL_STRENGTH


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
        
    def get_repel_vector(self, x, y, repel_range=None):
        """Get repulsion vector from this wall to a point"""
        if repel_range is None:
            repel_range = WALL_REPEL_RANGE
            
        # Find closest point on wall to the ant
        closest_x = max(self.rect.left, min(x, self.rect.right))
        closest_y = max(self.rect.top, min(y, self.rect.bottom))
        
        # Calculate distance
        dx = x - closest_x
        dy = y - closest_y
        distance = math.sqrt(dx*dx + dy*dy)
        
        if distance == 0:
            distance = 0.01
        
        # Only repel if within range
        if distance > repel_range:
            return 0, 0
        
        # Calculate repulsion strength based on distance
        strength = (1 - distance / repel_range) * WALL_REPEL_STRENGTH
        
        # Normalize and apply strength
        if distance > 0:
            dx = (dx / distance) * strength
            dy = (dy / distance) * strength
        
        return dx, dy


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
        """Create wall obstacles in the environment"""
        # Create interesting wall patterns that don't block completely
        
        # Left side obstacle
        self.walls.append(Wall(
            self.area_offset_x + 150,
            self.area_offset_y + 200,
            40,
            300
        ))
        
        # Right side obstacle
        self.walls.append(Wall(
            self.area_offset_x + self.area_width - 200,
            self.area_offset_y + 250,
            40,
            280
        ))
        
        # Center-left obstacle
        self.walls.append(Wall(
            self.area_offset_x + 400,
            self.area_offset_y + 100,
            50,
            200
        ))
        
        # Center-right obstacle
        self.walls.append(Wall(
            self.area_offset_x + self.area_width - 450,
            self.area_offset_y + 400,
            50,
            220
        ))
        
        # Bottom obstacle (maze-like element)
        self.walls.append(Wall(
            self.area_offset_x + 600,
            self.area_offset_y + self.area_height - 250,
            300,
            40
        ))
        
        # Top-center obstacle
        self.walls.append(Wall(
            self.area_offset_x + self.area_width // 2 - 100,
            self.area_offset_y + 80,
            80,
            40
        ))
        
    def get_repel_vector(self, x, y, repel_range=None):
        """Get combined repulsion vector from all walls"""
        total_dx = 0
        total_dy = 0
        
        for wall in self.walls:
            dx, dy = wall.get_repel_vector(x, y, repel_range)
            total_dx += dx
            total_dy += dy
        
        return total_dx, total_dy
    
    def is_colliding(self, x, y, radius=10):
        """Check if a point collides with any wall"""
        for wall in self.walls:
            # Check if point is inside wall (expanded by radius)
            if (wall.rect.left - radius < x < wall.rect.right + radius and
                wall.rect.top - radius < y < wall.rect.bottom + radius):
                return True, wall
        return False, None
    
    def get_avoid_position(self, old_x, old_y, new_x, new_y, radius=15):
        """Block movement if it would enter a wall - return valid position"""
        colliding, wall = self.is_colliding(new_x, new_y, radius)
        
        if not colliding:
            return new_x, new_y
        
        # Find which side of wall we're hitting and block that axis
        # Try moving only in X
        colliding_x, _ = self.is_colliding(new_x, old_y, radius)
        # Try moving only in Y
        colliding_y, _ = self.is_colliding(old_x, new_y, radius)
        
        result_x = new_x
        result_y = new_y
        
        if colliding_x:
            result_x = old_x  # Block X movement
        if colliding_y:
            result_y = old_y  # Block Y movement
            
        # If still colliding, just stay put
        still_colliding, _ = self.is_colliding(result_x, result_y, radius)
        if still_colliding:
            return old_x, old_y
            
        return result_x, result_y
    
    def draw(self, surface):
        """Draw all walls"""
        for wall in self.walls:
            wall.draw(surface)
