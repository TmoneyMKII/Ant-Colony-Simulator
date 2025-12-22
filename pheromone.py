"""Pheromone system for ant communication"""

import math
import pygame
from ant import AntState

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
        
        self.evaporation_rate = 0.98  # Pheromones fade over time
        self.max_pheromone = 100.0
        
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
        if best_strength > 10:
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
    
    def draw(self, surface, show_foraging=True, show_returning=True, opacity=50):
        """Draw pheromone map as overlay"""
        pheromone_surface = pygame.Surface((self.width, self.height), pygame.SRCALPHA)
        
        for y in range(self.grid_height):
            for x in range(self.grid_width):
                rect = pygame.Rect(x * self.cell_size, y * self.cell_size, self.cell_size, self.cell_size)
                
                # Draw foraging pheromones in blue
                if show_foraging:
                    strength = self.foraging_pheromones[y][x] / self.max_pheromone
                    if strength > 0.01:
                        color = (100, 150, 255, int(opacity * strength))
                        pygame.draw.rect(pheromone_surface, color, rect)
                
                # Draw returning pheromones in green
                if show_returning:
                    strength = self.returning_pheromones[y][x] / self.max_pheromone
                    if strength > 0.01:
                        color = (100, 255, 150, int(opacity * strength))
                        pygame.draw.rect(pheromone_surface, color, rect)
        
        surface.blit(pheromone_surface, (0, 0))
