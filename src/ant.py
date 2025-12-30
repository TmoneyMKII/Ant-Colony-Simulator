"""Ant agent class and behavior"""

import pygame
import math
import random
import os
from enum import Enum
from src.config import GRID_SIZE, runtime
from src.genetics import AntGenes, FitnessTracker

# Load ant sprite once at module level
_ant_sprite = None
_ant_sprite_carrying = None

def _load_ant_sprites():
    """Load and composite the black ant sprite parts"""
    global _ant_sprite, _ant_sprite_carrying
    
    if _ant_sprite is not None:
        return
    
    base_path = os.path.join("assets", "spriter_file_png_pieces", "black_ant")
    
    # Create a surface to composite the ant (size based on parts)
    sprite_size = 48
    _ant_sprite = pygame.Surface((sprite_size, sprite_size), pygame.SRCALPHA)
    
    # Load parts and position them to form an ant
    # Ant faces RIGHT by default (direction = 0)
    parts = [
        ("abdomen.png", (30, 20)),    # Back
        ("thorax.png", (18, 20)),      # Middle  
        ("head.png", (6, 20)),         # Front
    ]
    
    for part_file, pos in parts:
        part_path = os.path.join(base_path, part_file)
        if os.path.exists(part_path):
            part = pygame.image.load(part_path).convert_alpha()
            # Scale down parts to fit
            part = pygame.transform.scale(part, (16, 16))
            _ant_sprite.blit(part, pos)
    
    # Scale final sprite
    _ant_sprite = pygame.transform.scale(_ant_sprite, (24, 24))
    
    # Create a green-tinted version for carrying food
    _ant_sprite_carrying = _ant_sprite.copy()
    _ant_sprite_carrying.fill((100, 255, 150, 255), special_flags=pygame.BLEND_RGBA_MULT)

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
        
        # Track time since last food (for speed bonus)
        if hasattr(self, 'time_since_food'):
            self.time_since_food += 1
        else:
            self.time_since_food = 0
        
        # Track previous position for movement detection
        self.prev_x = self.x
        self.prev_y = self.y
        
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
        """Add repulsion from nearby ants to prevent clustering (using runtime params)"""
        repulsion_radius = runtime.ant_repulsion_radius
        repel_strength = runtime.ant_repulsion_strength
        for other in other_ants:
            if other is self:
                continue
            dx = self.x - other.x
            dy = self.y - other.y
            dist = math.sqrt(dx**2 + dy**2)
            if dist < repulsion_radius and dist > 0:
                # Push away from other ant
                push_strength = (repulsion_radius - dist) / repulsion_radius * 0.5
                angle_away = math.atan2(dy, dx)
                self.direction = self.direction * (1 - repel_strength) + angle_away * repel_strength
    
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
                
                # LEARNING INCENTIVE: Track how fast ant found food
                self.fitness.food_discovery_time += self.time_since_food
                self.time_since_food = 0  # Reset timer
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
        # Track consecutive stuck frames
        if not hasattr(self, 'stuck_counter'):
            self.stuck_counter = 0
        
        # Stuck detection and escape mechanism (using runtime params)
        if self.stuck_counter > runtime.stuck_threshold:
            self.direction = random.uniform(0, 2 * math.pi)  # Random new direction
            self.stuck_counter = 0
            # Jump a small distance to break free
            self.x += random.uniform(-10, 10)
            self.y += random.uniform(-10, 10)
        
        # Smooth direction changes with momentum (using runtime param)
        momentum = runtime.momentum
        self.direction = self.previous_direction * momentum + self.direction * (1 - momentum)
        self.previous_direction = self.direction
        
        # Add small random jitter to prevent perfect oscillation (using runtime param)
        jitter = runtime.random_jitter
        self.direction += random.uniform(-jitter, jitter)
        
        # Calculate base movement
        base_dx = math.cos(self.direction) * self.speed
        base_dy = math.sin(self.direction) * self.speed
        
        # Check for wall avoidance if colony has wall manager (using runtime params)
        wall_dx = 0
        wall_dy = 0
        if hasattr(self.colony, 'wall_manager'):
            wall_dx, wall_dy = self.colony.wall_manager.get_repel_vector(self.x, self.y, runtime.wall_repel_range)
            # Scale wall repulsion to be significant (using runtime param)
            wall_dx *= runtime.wall_repel_strength
            wall_dy *= runtime.wall_repel_strength
        
        # Combine movements: 70% base direction, 30% wall repulsion
        new_dx = base_dx * 0.7 + wall_dx * 0.3
        new_dy = base_dy * 0.7 + wall_dy * 0.3
        
        # Calculate new position
        new_x = self.x + new_dx
        new_y = self.y + new_dy
        
        # Check collision with walls and BLOCK movement if hitting wall
        if hasattr(self.colony, 'wall_manager'):
            new_x, new_y = self.colony.wall_manager.get_avoid_position(self.x, self.y, new_x, new_y, radius=self.radius)
            # If blocked, penalize and turn away - LEARNING INCENTIVE
            if new_x == self.x and new_y == self.y:
                self.fitness.wall_hits += 1  # Penalty for hitting wall
                self.energy -= 0.5           # Energy cost for collision
                self.direction = random.uniform(0, 2 * math.pi)  # Complete random turn
                self.stuck_counter += 5  # Big stuck penalty
        
        # Update position
        self.x = new_x
        self.y = new_y
        
        # Track distance traveled - LEARNING INCENTIVE (reward exploration)
        dist = math.sqrt((self.x - self.prev_x)**2 + (self.y - self.prev_y)**2)
        self.fitness.distance_traveled += dist
        
        # Check if stuck - LEARNING INCENTIVE (penalize staying still)
        if dist < 0.5:
            self.fitness.time_stuck += 1
            self.stuck_counter += 1
        else:
            self.stuck_counter = max(0, self.stuck_counter - 1)  # Recover if moving
        
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
        # Load sprites if not already loaded
        _load_ant_sprites()
        
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
        
        # Select sprite based on carrying state
        sprite = _ant_sprite_carrying if self.carrying_food else _ant_sprite
        
        # Rotate sprite to match direction (convert radians to degrees, pygame uses counter-clockwise)
        angle = -math.degrees(self.direction)
        rotated_sprite = pygame.transform.rotate(sprite, angle)
        
        # Get rect centered on ant position
        rect = rotated_sprite.get_rect(center=(int(self.x), int(self.y)))
        surface.blit(rotated_sprite, rect)
