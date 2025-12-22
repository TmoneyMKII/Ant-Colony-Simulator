"""Ant agent class and behavior"""

import pygame
import math
import random
from enum import Enum
from config import GRID_SIZE

class AntState(Enum):
    """Ant behavior states"""
    FORAGING = 1
    RETURNING = 2
    IDLE = 3

class Ant:
    """Individual ant agent with AI behavior"""
    
    def __init__(self, x, y, colony):
        self.x = x
        self.y = y
        self.colony = colony
        self.radius = 6
        self.color = (220, 120, 180)
        
        # AI state
        self.state = AntState.FORAGING
        self.energy = 100
        self.max_energy = 100
        self.carrying_food = False
        self.food_amount = 0
        
        # Movement
        self.speed = 2
        self.direction = random.uniform(0, 2 * math.pi)
        self.target_x = None
        self.target_y = None
        
        # Pheromone tracking
        self.pheromone_strength = 0.5
        self.can_deposit = True
        self.deposit_cooldown = 0
        
    def update(self, pheromone_map, width, height, food_sources, colony_pos):
        """Update ant behavior each frame"""
        self.energy -= 0.01
        
        # Ant dies if out of energy
        if self.energy <= 0:
            return False
        
        # Reduce cooldown for pheromone deposits
        if self.deposit_cooldown > 0:
            self.deposit_cooldown -= 1
        
        # State-based behavior
        if self.state == AntState.FORAGING:
            self._forage(pheromone_map, width, height, food_sources, colony_pos)
        elif self.state == AntState.RETURNING:
            self._return_to_colony(pheromone_map, colony_pos)
        elif self.state == AntState.IDLE:
            self._idle_behavior()
        
        # Move ant
        self._move(width, height, pheromone_map)
        
        # Deposit pheromone if foraging or returning
        if self.state in [AntState.FORAGING, AntState.RETURNING] and self.can_deposit and self.deposit_cooldown == 0:
            pheromone_map.deposit_pheromone(self.x, self.y, self.pheromone_strength, self.state)
            self.deposit_cooldown = 5
        
        return True
    
    def _forage(self, pheromone_map, width, height, food_sources, colony_pos):
        """Foraging behavior - search for food"""
        # Check if ant found food
        for food in food_sources:
            dist = math.sqrt((self.x - food.x)**2 + (self.y - food.y)**2)
            if dist < 15:
                self.carrying_food = True
                self.food_amount = 10
                self.state = AntState.RETURNING
                self.direction = self._get_direction_to(colony_pos[0], colony_pos[1])
                return
        
        # Check pheromone trails (follow food pheromone if available)
        nearby_pheromone = pheromone_map.get_pheromone_direction(
            self.x, self.y, AntState.FORAGING, self.direction
        )
        
        if nearby_pheromone is not None:
            # Bias towards pheromone but add some randomness
            if random.random() < 0.7:
                self.direction = nearby_pheromone
            else:
                self.direction += random.uniform(-0.5, 0.5)
        else:
            # Random walk when no pheromone
            if random.random() < 0.1:
                self.direction += random.uniform(-math.pi / 4, math.pi / 4)
    
    def _return_to_colony(self, pheromone_map, colony_pos):
        """Returning behavior - head back to colony"""
        # Check if reached colony
        dist = math.sqrt((self.x - colony_pos[0])**2 + (self.y - colony_pos[1])**2)
        if dist < 20:
            # Drop food and rest
            self.colony.add_food(self.food_amount)
            self.carrying_food = False
            self.food_amount = 0
            self.energy = min(self.energy + 20, self.max_energy)
            self.state = AntState.FORAGING
            return
        
        # Follow return pheromone trail if available
        nearby_pheromone = pheromone_map.get_pheromone_direction(
            self.x, self.y, AntState.RETURNING, self.direction
        )
        
        if nearby_pheromone is not None:
            if random.random() < 0.8:
                self.direction = nearby_pheromone
        else:
            # Head towards colony if no trail
            self.direction = self._get_direction_to(colony_pos[0], colony_pos[1])
    
    def _idle_behavior(self):
        """Idle behavior - rest near colony"""
        if self.energy > self.max_energy * 0.8:
            self.state = AntState.FORAGING
        else:
            self.direction += random.uniform(-0.2, 0.2)
    
    def _move(self, width, height, pheromone_map):
        """Move ant in current direction"""
        # Calculate new position
        new_x = self.x + math.cos(self.direction) * self.speed
        new_y = self.y + math.sin(self.direction) * self.speed
        
        # Wrap around edges
        self.x = new_x % width
        self.y = new_y % height
    
    def _get_direction_to(self, target_x, target_y):
        """Calculate direction vector to target"""
        dx = target_x - self.x
        dy = target_y - self.y
        return math.atan2(dy, dx)
    
    def draw(self, surface):
        """Draw ant on surface"""
        color = (100, 255, 150) if self.carrying_food else self.color
        pygame.draw.circle(surface, color, (int(self.x), int(self.y)), self.radius)
        
        # Draw direction indicator
        end_x = self.x + math.cos(self.direction) * 12
        end_y = self.y + math.sin(self.direction) * 12
        pygame.draw.line(surface, color, (int(self.x), int(self.y)), (int(end_x), int(end_y)), 2)
