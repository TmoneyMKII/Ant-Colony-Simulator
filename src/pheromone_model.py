"""
Pheromone Model - Consolidated pheromone system for ant colony simulation

Two pheromone types:
- FOOD_TRAIL (Green): Deposited by ants returning with food, leads TO food
- HOME_TRAIL (Blue): Deposited by foraging ants, leads TO home/colony
"""

import math
import pygame
from enum import Enum


class PheromoneType(Enum):
    """Types of pheromone trails"""
    FOOD_TRAIL = "food"   # Green - deposited when returning with food
    HOME_TRAIL = "home"   # Blue - deposited when foraging (leaving colony)


class PheromoneLayer:
    """Single layer of pheromone data"""
    
    def __init__(self, grid_width, grid_height):
        self.grid_width = grid_width
        self.grid_height = grid_height
        self.grid = [[0.0 for _ in range(grid_width)] for _ in range(grid_height)]
    
    def deposit(self, gx, gy, amount, max_value):
        """Add pheromone at grid position"""
        if 0 <= gx < self.grid_width and 0 <= gy < self.grid_height:
            self.grid[gy][gx] = min(max_value, self.grid[gy][gx] + amount)
    
    def get(self, gx, gy):
        """Get pheromone value at grid position"""
        if 0 <= gx < self.grid_width and 0 <= gy < self.grid_height:
            return self.grid[gy][gx]
        return 0.0
    
    def evaporate(self, rate):
        """Apply evaporation to all cells"""
        for y in range(self.grid_height):
            for x in range(self.grid_width):
                self.grid[y][x] *= rate
    
    def clear(self):
        """Clear all pheromones"""
        for y in range(self.grid_height):
            for x in range(self.grid_width):
                self.grid[y][x] = 0.0


class PheromoneModel:
    """
    Complete pheromone system managing both trail types.
    
    Usage:
        model = PheromoneModel(screen_width, screen_height)
        model.deposit(x, y, amount, PheromoneType.FOOD_TRAIL)
        direction = model.get_trail_direction(x, y, PheromoneType.FOOD_TRAIL)
        model.update()  # Call each frame for evaporation
        model.draw(surface)
    """
    
    def __init__(self, width, height, cell_size=20):
        self.width = width
        self.height = height
        self.cell_size = cell_size
        self.grid_width = width // cell_size
        self.grid_height = height // cell_size
        
        # Two separate pheromone layers
        self.food_trail = PheromoneLayer(self.grid_width, self.grid_height)
        self.home_trail = PheromoneLayer(self.grid_width, self.grid_height)
        
        # Configuration
        self.max_pheromone = 200.0
        self.evaporation_rate = 0.995  # Per frame decay
        self.detection_threshold = 10.0  # Minimum to detect
        
        # Rendering colors (RGBA)
        self.food_color = (0, 255, 100)    # Bright green
        self.home_color = (100, 150, 255)  # Bright blue
        
        # Pre-create surface for efficiency
        self.surface = pygame.Surface((width, height), pygame.SRCALPHA)
    
    def _to_grid(self, x, y):
        """Convert world coordinates to grid coordinates"""
        gx = int(x // self.cell_size)
        gy = int(y // self.cell_size)
        gx = max(0, min(gx, self.grid_width - 1))
        gy = max(0, min(gy, self.grid_height - 1))
        return gx, gy
    
    def _get_layer(self, ptype):
        """Get the appropriate layer for pheromone type"""
        if ptype == PheromoneType.FOOD_TRAIL:
            return self.food_trail
        return self.home_trail
    
    # ==================== DEPOSIT ====================
    
    def deposit(self, x, y, amount, ptype):
        """
        Deposit pheromone at world position.
        
        Args:
            x, y: World coordinates
            amount: Strength to deposit
            ptype: PheromoneType.FOOD_TRAIL or HOME_TRAIL
        """
        gx, gy = self._to_grid(x, y)
        layer = self._get_layer(ptype)
        layer.deposit(gx, gy, amount, self.max_pheromone)
    
    def deposit_food_trail(self, x, y, amount):
        """Convenience: Deposit green food trail (returning ants)"""
        self.deposit(x, y, amount, PheromoneType.FOOD_TRAIL)
    
    def deposit_home_trail(self, x, y, amount):
        """Convenience: Deposit blue home trail (foraging ants)"""
        self.deposit(x, y, amount, PheromoneType.HOME_TRAIL)
    
    # ==================== SENSING ====================
    
    def get_strength(self, x, y, ptype):
        """Get pheromone strength at world position"""
        gx, gy = self._to_grid(x, y)
        return self._get_layer(ptype).get(gx, gy)
    
    def get_trail_direction(self, x, y, ptype, current_dir=None):
        """
        Get direction to follow pheromone trail.
        
        Samples nearby cells and returns direction toward strongest gradient.
        Optionally applies forward bias if current_dir is provided.
        
        Returns:
            Direction in radians, or None if no trail detected
        """
        gx, gy = self._to_grid(x, y)
        layer = self._get_layer(ptype)
        
        # Sample 8 neighboring cells + center
        best_strength = 0.0
        best_dir = None
        
        # Check all 8 directions
        directions = [
            (-1, -1), (0, -1), (1, -1),
            (-1,  0),          (1,  0),
            (-1,  1), (0,  1), (1,  1)
        ]
        
        for dx, dy in directions:
            nx, ny = gx + dx, gy + dy
            strength = layer.get(nx, ny)
            
            if strength < self.detection_threshold:
                continue
            
            # Apply forward bias if we have current direction
            if current_dir is not None:
                target_dir = math.atan2(dy, dx)
                angle_diff = abs(target_dir - current_dir)
                while angle_diff > math.pi:
                    angle_diff = abs(angle_diff - 2 * math.pi)
                
                # Penalize backwards directions (>90 degrees)
                if angle_diff > math.pi / 2:
                    strength *= 0.3
            
            if strength > best_strength:
                best_strength = strength
                best_dir = math.atan2(dy, dx)
        
        return best_dir
    
    def get_food_trail_direction(self, x, y, current_dir=None):
        """Convenience: Get direction toward food (green trail)"""
        return self.get_trail_direction(x, y, PheromoneType.FOOD_TRAIL, current_dir)
    
    def get_home_trail_direction(self, x, y, current_dir=None):
        """Convenience: Get direction toward home (blue trail)"""
        return self.get_trail_direction(x, y, PheromoneType.HOME_TRAIL, current_dir)
    
    # ==================== UPDATE ====================
    
    def update(self):
        """Update pheromones (apply evaporation). Call once per frame."""
        self.food_trail.evaporate(self.evaporation_rate)
        self.home_trail.evaporate(self.evaporation_rate)
    
    def clear(self):
        """Clear all pheromones"""
        self.food_trail.clear()
        self.home_trail.clear()
    
    # ==================== RENDERING ====================
    
    def draw(self, target_surface, show_food=True, show_home=True, opacity=200):
        """
        Draw pheromone visualization - SIMPLE RECT SYSTEM.
        
        Green = Food trail (from returning ants)
        Blue = Home trail (from foraging ants)
        """
        # Draw directly to target - simpler approach
        for gy in range(self.grid_height):
            for gx in range(self.grid_width):
                x = gx * self.cell_size
                y = gy * self.cell_size
                
                food_strength = self.food_trail.get(gx, gy)
                home_strength = self.home_trail.get(gx, gy)
                
                # Skip if both are below threshold
                if food_strength < 5 and home_strength < 5:
                    continue
                
                # Create a surface for this cell
                cell_surface = pygame.Surface((self.cell_size, self.cell_size), pygame.SRCALPHA)
                
                # Draw HOME trail (BLUE) first - underneath
                if show_home and home_strength >= 5:
                    intensity = min(1.0, home_strength / 100.0)
                    alpha = int(150 * intensity)
                    blue_color = (50, 100, 255, alpha)
                    pygame.draw.rect(cell_surface, blue_color, (0, 0, self.cell_size, self.cell_size))
                
                # Draw FOOD trail (GREEN) on top
                if show_food and food_strength >= 5:
                    intensity = min(1.0, food_strength / 100.0)
                    alpha = int(180 * intensity)
                    green_color = (50, 255, 50, alpha)
                    pygame.draw.rect(cell_surface, green_color, (0, 0, self.cell_size, self.cell_size))
                
                target_surface.blit(cell_surface, (x, y))
    
    # ==================== SERIALIZATION ====================
    
    def to_dict(self):
        """Serialize pheromone state for saving"""
        return {
            'food_trail': [[self.food_trail.grid[y][x] for x in range(self.grid_width)] 
                          for y in range(self.grid_height)],
            'home_trail': [[self.home_trail.grid[y][x] for x in range(self.grid_width)] 
                          for y in range(self.grid_height)]
        }
    
    def from_dict(self, data):
        """Restore pheromone state from save data"""
        if 'food_trail' in data:
            for y, row in enumerate(data['food_trail']):
                for x, val in enumerate(row):
                    if y < self.grid_height and x < self.grid_width:
                        self.food_trail.grid[y][x] = val
        if 'home_trail' in data:
            for y, row in enumerate(data['home_trail']):
                for x, val in enumerate(row):
                    if y < self.grid_height and x < self.grid_width:
                        self.home_trail.grid[y][x] = val
