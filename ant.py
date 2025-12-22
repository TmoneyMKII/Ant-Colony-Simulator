"""Ant agent class and behavior"""

import pygame
import math
import random
from enum import Enum
from config import GRID_SIZE
from genetics import AntGenes, FitnessTracker

class AntState(Enum):
    """Ant behavior states"""
    FORAGING = 1
    RETURNING = 2
    IDLE = 3

class Ant:
    """Individual ant agent with AI behavior"""
    
    def __init__(self, x, y, colony, genes=None):
        self.x = x
        self.y = y
        self.colony = colony
        self.radius = 6
        self.color = (220, 120, 180)
        
        # Genetic traits
        self.genes = genes if genes else AntGenes()
        self.fitness = FitnessTracker()
        self.generation = 0
        
        # AI state
        self.state = AntState.FORAGING
        self.energy = 100
        self.max_energy = 100
        self.carrying_food = False
        self.food_amount = 0
        
        # Movement
        self.speed = self.genes.speed
        self.direction = random.uniform(0, 2 * math.pi)
        self.previous_direction = self.direction
        self.target_x = None
        self.target_y = None
        
        # Pheromone tracking
        self.pheromone_strength = self.genes.pheromone_strength
        self.can_deposit = True
        self.deposit_cooldown = 0
        
        # Visual trail for pheromone effect
        self.trail = []  # List of (x, y, age) tuples
        self.max_trail_length = 60  # Increased from 30
        self.trail_update_counter = 0
        
    def update(self, pheromone_map, width, height, food_sources, colony_pos, other_ants=None, bounds=None):
        """Update ant behavior each frame"""
        energy_cost = self.genes.energy_efficiency
        self.energy -= energy_cost
        self.fitness.energy_spent += energy_cost
        self.fitness.lifetime += 1
        
        # Ant dies if out of energy
        if self.energy <= 0:
            return False
        
        # Store bounds for later use
        self.bounds = bounds
        
        # Check for edge proximity and turn away
        if bounds:
            self._avoid_edges(bounds)
        
        # Reduce cooldown for pheromone deposits
        if self.deposit_cooldown > 0:
            self.deposit_cooldown -= 1
        
        # Avoid other ants
        if other_ants:
            self._avoid_other_ants(other_ants)
        
        # State-based behavior
        if self.state == AntState.FORAGING:
            self._forage(pheromone_map, width, height, food_sources, colony_pos)
        elif self.state == AntState.RETURNING:
            self._return_to_colony(pheromone_map, colony_pos)
        elif self.state == AntState.IDLE:
            self._idle_behavior()
        
        # Move ant
        self._move(width, height, pheromone_map)
        
        # Update trail if returning with food
        if self.state == AntState.RETURNING and self.carrying_food:
            self.trail_update_counter += 1
            if self.trail_update_counter >= 2:  # Add trail point every 2 frames
                self.trail.append([self.x, self.y, 0])
                self.trail_update_counter = 0
                if len(self.trail) > self.max_trail_length:
                    self.trail.pop(0)
        
        # Age all trail points (they fade naturally over time)
        for point in self.trail:
            point[2] += 1
        
        # Remove very old trail points
        self.trail = [point for point in self.trail if point[2] < self.max_trail_length * 5]  # 5x max length
        
        # Deposit pheromone if foraging or returning
        if self.state in [AntState.FORAGING, AntState.RETURNING] and self.can_deposit and self.deposit_cooldown == 0:
            pheromone_map.deposit_pheromone(self.x, self.y, self.pheromone_strength, self.state)
            self.deposit_cooldown = 3  # Reduced from 5 to 3
        
        return True
    
    def _avoid_edges(self, bounds):
        """Turn away from edges when getting close"""
        edge_buffer = 35  # Reduced from 40
        turn_strength = 0.3  # Reduced from 0.4
        
        at_edge = False
        
        # Check left edge
        if self.x < bounds.left + edge_buffer:
            at_edge = True
            if math.cos(self.direction) < 0:  # Only if moving toward edge
                self.direction += turn_strength + random.uniform(0, 0.3)
        
        # Check right edge  
        if self.x > bounds.right - edge_buffer:
            at_edge = True
            if math.cos(self.direction) > 0:  # Only if moving toward edge
                self.direction -= turn_strength + random.uniform(0, 0.3)
        
        # Check top edge
        if self.y < bounds.top + edge_buffer:
            at_edge = True
            if math.sin(self.direction) < 0:  # Only if moving toward edge
                self.direction += turn_strength + random.uniform(0, 0.3)
        
        # Check bottom edge
        if self.y > bounds.bottom - edge_buffer:
            at_edge = True
            if math.sin(self.direction) > 0:  # Only if moving toward edge
                self.direction -= turn_strength + random.uniform(0, 0.3)
        
        # If at edge, add random large turn to help escape
        if at_edge and random.random() < 0.4:
            self.direction += random.uniform(-math.pi/2, math.pi/2)
    
    def _avoid_other_ants(self, other_ants):
        """Add small repulsion from nearby ants to prevent clustering"""
        repulsion_radius = 15
        for other in other_ants:
            if other is self:
                continue
            dx = self.x - other.x
            dy = self.y - other.y
            dist = math.sqrt(dx**2 + dy**2)
            if dist < repulsion_radius and dist > 0:
                # Add small random direction change away from other ant
                self.direction += random.uniform(-0.3, 0.3)
    
    def _forage(self, pheromone_map, width, height, food_sources, colony_pos):
        """Foraging behavior - search for food"""
        # Check if ant found food
        for food in food_sources:
            dist = math.sqrt((self.x - food.x)**2 + (self.y - food.y)**2)
            if dist < 15 and food.amount > 0:
                # Take food from source
                food_taken = min(10, food.amount)
                food.amount -= food_taken
                self.carrying_food = True
                self.food_amount = food_taken
                self.state = AntState.RETURNING
                self.direction = self._get_direction_to(colony_pos[0], colony_pos[1])
                return
        
        # Check pheromone trails (follow food pheromone if available)
        nearby_pheromone = pheromone_map.get_pheromone_direction(
            self.x, self.y, AntState.FORAGING, self.direction
        )
        
        if nearby_pheromone is not None:
            # Balance between following trails and independent exploration
            if random.random() < self.genes.pheromone_sensitivity * 1.5:  # Reduced from 3x to 1.5x
                self.direction = nearby_pheromone + random.uniform(-0.4, 0.4)  # More deviation
            else:
                self.direction += random.uniform(-0.5, 0.5)  # More random
        else:
            # Random walk when no pheromone
            if random.random() < self.genes.exploration_rate:
                self.direction += random.uniform(-math.pi / 3, math.pi / 3)
    
    def _return_to_colony(self, pheromone_map, colony_pos):
        """Returning behavior - head back to colony"""
        # Check if reached colony
        dist = math.sqrt((self.x - colony_pos[0])**2 + (self.y - colony_pos[1])**2)
        if dist < 20:
            # Drop food and rest
            self.colony.add_food(self.food_amount)
            self.fitness.food_collected += self.food_amount
            self.fitness.successful_trips += 1
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
            if random.random() < 0.6:  # Reduced from 0.8 to 0.6
                self.direction = nearby_pheromone + random.uniform(-0.2, 0.2)
            else:
                # Head towards colony if not following trail
                self.direction = self._get_direction_to(colony_pos[0], colony_pos[1])
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
        # Smooth direction changes with momentum (prevents jittering)
        momentum = 0.7
        self.direction = self.previous_direction * momentum + self.direction * (1 - momentum)
        self.previous_direction = self.direction
        
        # Calculate new position
        new_x = self.x + math.cos(self.direction) * self.speed
        new_y = self.y + math.sin(self.direction) * self.speed
        
        # Update position
        self.x = new_x
        self.y = new_y
        
        # Hard clamp to bounds - this ensures ants NEVER leave the grid
        if hasattr(self, 'bounds') and self.bounds:
            if self.x <= self.bounds.left:
                self.x = self.bounds.left + 2
                self.direction = random.uniform(0, math.pi)  # Random bounce toward right
            elif self.x >= self.bounds.right:
                self.x = self.bounds.right - 2
                self.direction = random.uniform(math.pi/2, 3*math.pi/2)  # Random bounce toward left
            
            if self.y <= self.bounds.top:
                self.y = self.bounds.top + 2
                self.direction = random.uniform(-math.pi/2, math.pi/2)  # Random bounce downward
            elif self.y >= self.bounds.bottom:
                self.y = self.bounds.bottom - 2
                self.direction = random.uniform(math.pi/2, 3*math.pi/2)  # Random bounce upward
    
    def _get_direction_to(self, target_x, target_y):
        """Calculate direction vector to target"""
        dx = target_x - self.x
        dy = target_y - self.y
        return math.atan2(dy, dx)
    
    def draw(self, surface):
        """Draw ant on surface"""
        color = (100, 255, 150) if self.carrying_food else self.color
        
        # Draw fading pheromone trail (even after dropping food)
        if len(self.trail) > 1:
            for i, point in enumerate(self.trail):
                # Calculate fade based on age (older = more faded)
                age_factor = 1.0 - (point[2] / (self.max_trail_length * 3))
                if age_factor < 0:
                    age_factor = 0
                
                # Create fading yellow-green trail
                alpha = int(180 * age_factor)
                if alpha > 10:  # Only draw if visible
                    # Draw trail segment
                    trail_color = (200, 255, 100, alpha)
                    trail_surface = pygame.Surface((8, 8), pygame.SRCALPHA)
                    pygame.draw.circle(trail_surface, trail_color, (4, 4), 3)
                    surface.blit(trail_surface, (int(point[0]) - 4, int(point[1]) - 4))
        
        # Draw ant body
        pygame.draw.circle(surface, color, (int(self.x), int(self.y)), self.radius)
        
        # Draw direction indicator
        end_x = self.x + math.cos(self.direction) * 12
        end_y = self.y + math.sin(self.direction) * 12
        pygame.draw.line(surface, color, (int(self.x), int(self.y)), (int(end_x), int(end_y)), 2)
