"""Pheromone system for ant communication"""

import math
import pygame
from src.ant import AntState

class PheromoneMap:
    """Manages pheromone trails left by ants"""
    
    def __init__(self, width, height, cell_size=20):
        self.width = width
        self.height = height
        self.cell_size = cell_size
        self.grid_width = width // cell_size
        self.grid_height = height // cell_size
        
        # Store pheromones: [foraging, returning]
        self.foraging_pheromones = [[0.0 for _ in range(self.grid_width)] for _ in range(self.grid_height)]
        self.returning_pheromones = [[0.0 for _ in range(self.grid_width)] for _ in range(self.grid_height)]
        
        self.evaporation_rate = 0.999  # Much slower evaporation (was 0.995)
        self.max_pheromone = 200.0  # Increased max to allow stronger trails
        
    def deposit_pheromone(self, x, y, strength, state):
        """Deposit pheromone at position"""
        grid_x = int(x // self.cell_size)
        grid_y = int(y // self.cell_size)
        
        # Clamp to grid
        grid_x = max(0, min(grid_x, self.grid_width - 1))
        grid_y = max(0, min(grid_y, self.grid_height - 1))
        
        if state == AntState.FORAGING:
            self.foraging_pheromones[grid_y][grid_x] = min(
                self.max_pheromone,
                self.foraging_pheromones[grid_y][grid_x] + strength
            )
        elif state == AntState.RETURNING:
            self.returning_pheromones[grid_y][grid_x] = min(
                self.max_pheromone,
                self.returning_pheromones[grid_y][grid_x] + strength
            )
    
    def get_pheromone_direction(self, x, y, state, current_direction):
        """Get direction towards strongest pheromone"""
        grid_x = int(x // self.cell_size)
        grid_y = int(y // self.cell_size)
        
        # Clamp to grid
        grid_x = max(0, min(grid_x, self.grid_width - 1))
        grid_y = max(0, min(grid_y, self.grid_height - 1))
        
        pheromones = self.foraging_pheromones if state == AntState.FORAGING else self.returning_pheromones
        
        # Check surrounding cells
        best_strength = 0
        best_direction = None
        
        for dy in [-1, 0, 1]:
            for dx in [-1, 0, 1]:
                if dx == 0 and dy == 0:
                    continue
                
                check_x = grid_x + dx
                check_y = grid_y + dy
                
                # Wrap around
                check_x = check_x % self.grid_width
                check_y = check_y % self.grid_height
                
                strength = pheromones[check_y][check_x]
                
                if strength > best_strength:
                    best_strength = strength
                    best_direction = math.atan2(dy, dx)
        
        # Only return direction if pheromone is strong enough
        if best_strength > 15:  # Balanced threshold (was 5, originally 20)
            return best_direction
        
        return None
    
    def get_pheromone_value(self, x, y, state):
        """Get pheromone strength at position"""
        grid_x = int(x // self.cell_size) % self.grid_width
        grid_y = int(y // self.cell_size) % self.grid_height
        
        if state == AntState.FORAGING:
            return self.foraging_pheromones[grid_y][grid_x]
        else:
            return self.returning_pheromones[grid_y][grid_x]
    
    def update(self):
        """Evaporate pheromones"""
        for y in range(self.grid_height):
            for x in range(self.grid_width):
                self.foraging_pheromones[y][x] *= self.evaporation_rate
                self.returning_pheromones[y][x] *= self.evaporation_rate
    
    def draw(self, surface, show_foraging=True, show_returning=True, opacity=120):
        """Draw pheromone map as overlay"""
        pheromone_surface = pygame.Surface((self.width, self.height), pygame.SRCALPHA)
        
        for y in range(self.grid_height):
            for x in range(self.grid_width):
                px = x * self.cell_size
                py = y * self.cell_size
                
                # Only draw if within bounds
                if px >= self.width or py >= self.height:
                    continue
                
                # Draw returning pheromones (food trail) in bright green/yellow
                if show_returning:
                    strength = self.returning_pheromones[y][x] / self.max_pheromone
                    if strength > 0.05:
                        # Bright green-yellow gradient for food trail
                        green_val = int(255 * min(1.0, strength * 1.5))
                        color = (200, green_val, 50, int(opacity * strength))
                        # Draw smaller circles instead of rectangles to avoid artifacts
                        center_x = px + self.cell_size // 2
                        center_y = py + self.cell_size // 2
                        radius = max(2, int(self.cell_size * strength * 0.4))
                        pygame.draw.circle(pheromone_surface, color, (center_x, center_y), radius)
                
                # Draw foraging pheromones in subtle blue (optional)
                if show_foraging:
                    strength = self.foraging_pheromones[y][x] / self.max_pheromone
                    if strength > 0.05:
                        color = (80, 120, 200, int(opacity * 0.5 * strength))
                        center_x = px + self.cell_size // 2
                        center_y = py + self.cell_size // 2
                        radius = max(2, int(self.cell_size * strength * 0.3))
                        pygame.draw.circle(pheromone_surface, color, (center_x, center_y), radius)
        
        surface.blit(pheromone_surface, (0, 0))
