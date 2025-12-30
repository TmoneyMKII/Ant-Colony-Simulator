"""Ant agent class and behavior"""

import pygame
import math
import random
import os
from enum import Enum
from src.config import GRID_SIZE, ANT_SMELL_RANGE, ANT_SMELL_STRENGTH, ANT_WANDER_TURN_RATE
from src.pheromone_model import PheromoneType

# Pheromone deposit amount
PHEROMONE_DEPOSIT = 8.0

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
    """Ant behavior states - determines which pheromone to follow"""
    FORAGING = 1    # Looking for food - follow GREEN (food trail) pheromone
    RETURNING = 2   # Carrying food home - follow BLUE (home trail) pheromone
    IDLE = 3        # Resting at colony

# Default ant attributes (fixed values instead of genetics)
DEFAULT_SPEED = 2.0
DEFAULT_PHEROMONE_SENSITIVITY = 0.15
DEFAULT_PHEROMONE_STRENGTH = 1.5
DEFAULT_ENERGY_EFFICIENCY = 0.01


class Ant:
    """Individual ant agent with AI behavior"""
    
    def __init__(self, x, y, colony):
        self.x = x
        self.y = y
        self.colony = colony
        self.radius = 6
        self.color = (220, 120, 180)
        self.alive = True  # For wall-stuck detection
        
        # ANT STATUS SYSTEM
        self.state = AntState.FORAGING  # Current behavior mode
        self.carrying_food = False       # Has food to carry
        self.food_amount = 0             # Amount of food carrying
        self.knows_food_location = False # Has found food before
        self.last_food_x = None          # Remember where food was
        self.last_food_y = None
        
        # Energy
        self.energy = 100
        self.max_energy = 100
        
        # Movement (fixed values)
        self.speed = DEFAULT_SPEED
        self.direction = random.uniform(0, 2 * math.pi)
        self.previous_direction = self.direction
        self.target_x = None
        self.target_y = None
        
        # Pheromone tracking (fixed values)
        self.pheromone_strength = DEFAULT_PHEROMONE_STRENGTH
        self.pheromone_sensitivity = DEFAULT_PHEROMONE_SENSITIVITY
        self.can_deposit = True
        self.deposit_cooldown = 0
        
        # Simple stats tracking
        self.food_collected = 0
        self.successful_trips = 0
        
        # Visual trail for pheromone effect
        self.trail = []  # List of (x, y, age) tuples
        self.max_trail_length = 60
        self.trail_update_counter = 0
        
        # Movement tracking for stuck detection
        self.movement_check_interval = 180  # Check every 3 seconds (60 FPS * 3)
        self.min_movement_distance = 80     # Must move at least 80 pixels (~4 grid spaces)
        self.movement_timer = 0
        self.checkpoint_x = x
        self.checkpoint_y = y
        self.stuck_escape_count = 0         # Track how many times we've tried to escape
        self.max_escape_attempts = 5        # After this many failed escapes, ant dies
        
    def update(self, pheromone_map, width, height, food_sources, colony_pos, other_ants=None, bounds=None):
        """Update ant behavior each frame"""
        # Check if ant is dead (e.g., stuck in wall)
        if not self.alive:
            return False
        
        # Movement-based stuck detection
        self.movement_timer += 1
        if self.movement_timer >= self.movement_check_interval:
            # Calculate distance moved since last checkpoint
            dist_moved = math.sqrt((self.x - self.checkpoint_x)**2 + (self.y - self.checkpoint_y)**2)
            if dist_moved < self.min_movement_distance:
                # Ant hasn't moved enough - try to escape with random direction
                self.stuck_escape_count += 1
                if self.stuck_escape_count >= self.max_escape_attempts:
                    # Too many failed escape attempts - ant dies
                    self.alive = False
                    return False
                else:
                    # Random direction escape attempt
                    self.direction = random.uniform(0, 2 * math.pi)
                    # Small random teleport to break free from corners
                    self.x += random.uniform(-15, 15)
                    self.y += random.uniform(-15, 15)
            else:
                # Ant is moving fine - reset escape counter
                self.stuck_escape_count = 0
            # Reset checkpoint
            self.checkpoint_x = self.x
            self.checkpoint_y = self.y
            self.movement_timer = 0
        
        energy_cost = DEFAULT_ENERGY_EFFICIENCY
        self.energy -= energy_cost
        
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
        
        # DUAL PHEROMONE SYSTEM:
        # - HOME_TRAIL (BLUE): deposited when foraging (marks path back to colony)
        # - FOOD_TRAIL (GREEN): deposited when returning with food (marks path to food)
        colony_dist = math.sqrt((self.x - colony_pos[0])**2 + (self.y - colony_pos[1])**2)
        
        if self.deposit_cooldown == 0:
            if self.state == AntState.RETURNING and self.carrying_food:
                # FOOD_TRAIL (GREEN) - tells others where food is
                if colony_dist > 50:
                    pheromone_map.deposit_food_trail(self.x, self.y, PHEROMONE_DEPOSIT * self.pheromone_strength)
                    self.deposit_cooldown = 2
            elif self.state == AntState.FORAGING:
                # HOME_TRAIL (BLUE) - marks path back to colony
                if colony_dist > 50:
                    pheromone_map.deposit_home_trail(self.x, self.y, PHEROMONE_DEPOSIT * self.pheromone_strength)
                    self.deposit_cooldown = 2
        
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
        """Add repulsion from nearby ants to prevent clustering"""
        repulsion_radius = 25.0
        repel_strength = 0.3
        for other in other_ants:
            if other is self:
                continue
            dx = self.x - other.x
            dy = self.y - other.y
            dist = math.sqrt(dx**2 + dy**2)
            if dist < repulsion_radius and dist > 0:
                # Push away from other ant
                angle_away = math.atan2(dy, dx)
                self.direction = self.direction * (1 - repel_strength) + angle_away * repel_strength
    
    def _forage(self, pheromone_map, width, height, food_sources, colony_pos):
        """
        FORAGING STATE: Ant is looking for food
        - Priority 1: Pick up food if touching it
        - Priority 2: Smell food directly (within range)
        - Priority 3: Follow GREEN pheromone trail (food trail from returning ants)
        - Priority 4: Wander randomly to explore
        """
        # PRIORITY 1: Check if ant is touching food (pickup range)
        for food in food_sources:
            dist = math.sqrt((self.x - food.x)**2 + (self.y - food.y)**2)
            if dist < 15 and food.amount > 0:
                # Take 1 unit of food from source
                food.amount -= 1
                self.carrying_food = True
                self.food_amount = 1
                self.state = AntState.RETURNING  # SWITCH STATE
                
                # Remember food location
                self.knows_food_location = True
                self.last_food_x = food.x
                self.last_food_y = food.y
                
                # Turn toward colony
                self.direction = self._get_direction_to(colony_pos[0], colony_pos[1])
                
                self.time_since_food = 0
                return
        
        # PRIORITY 2: Smell food directly within range
        closest_food = None
        closest_dist = ANT_SMELL_RANGE
        
        for food in food_sources:
            if food.amount <= 0:
                continue
            dist = math.sqrt((self.x - food.x)**2 + (self.y - food.y)**2)
            if dist < closest_dist:
                closest_dist = dist
                closest_food = food
        
        if closest_food:
            # Ant smells food - move toward it
            food_direction = self._get_direction_to(closest_food.x, closest_food.y)
            smell_intensity = 1.0 - (closest_dist / ANT_SMELL_RANGE)
            blend_factor = ANT_SMELL_STRENGTH * smell_intensity
            
            angle_diff = food_direction - self.direction
            while angle_diff > math.pi:
                angle_diff -= 2 * math.pi
            while angle_diff < -math.pi:
                angle_diff += 2 * math.pi
            
            self.direction += angle_diff * blend_factor
            self.direction += random.uniform(-0.05, 0.05)
            return
        
        # Check distance from colony - don't follow pheromones near colony (prevents clustering)
        colony_dist = math.sqrt((self.x - colony_pos[0])**2 + (self.y - colony_pos[1])**2)
        
        # PRIORITY 3: Follow GREEN pheromone (food trail) - only far from colony
        if colony_dist > 150:  # Only follow trails when well away from colony
            food_trail = pheromone_map.get_food_trail_direction(self.x, self.y, self.direction)
            
            if food_trail is not None:
                # Follow the food trail (deposited by ants returning with food)
                angle_diff = food_trail - self.direction
                while angle_diff > math.pi:
                    angle_diff -= 2 * math.pi
                while angle_diff < -math.pi:
                    angle_diff += 2 * math.pi
                
                # Only follow if it's roughly forward (not a U-turn)
                if abs(angle_diff) < math.pi * 0.6:
                    self.direction += angle_diff * 0.25 * self.pheromone_sensitivity
                    self.direction += random.uniform(-0.1, 0.1)
                    return
        
        # PRIORITY 4: No trail or near colony - wander to explore
        # Add outward bias when near colony to encourage spreading
        if colony_dist < 250:
            outward_dir = math.atan2(self.y - colony_pos[1], self.x - colony_pos[0])
            bias = (250 - colony_dist) / 250 * 0.2  # Stronger when closer
            angle_diff = outward_dir - self.direction
            while angle_diff > math.pi:
                angle_diff -= 2 * math.pi
            while angle_diff < -math.pi:
                angle_diff += 2 * math.pi
            self.direction += angle_diff * bias
        
        self.direction += random.uniform(-ANT_WANDER_TURN_RATE, ANT_WANDER_TURN_RATE)
    
    def _return_to_colony(self, pheromone_map, colony_pos):
        """
        RETURNING STATE: Ant is carrying food back to colony
        - Priority 1: Drop food if at colony
        - Priority 2: Follow BLUE pheromone trail (home trail from foraging ants)
        - Priority 3: Head directly toward colony (ants know where home is)
        """
        # PRIORITY 1: Check if reached colony
        dist = math.sqrt((self.x - colony_pos[0])**2 + (self.y - colony_pos[1])**2)
        if dist < 25:
            # Drop food at colony
            self.colony.add_food(self.food_amount)
            self.food_collected += self.food_amount
            self.successful_trips += 1
            self.carrying_food = False
            self.food_amount = 0
            self.energy = min(self.energy + 20, self.max_energy)
            self.state = AntState.FORAGING  # SWITCH STATE
            
            # Turn around to go back out foraging - spread out more
            self.direction += math.pi + random.uniform(-0.8, 0.8)
            return
        
        # PRIORITY 2: Head directly toward colony (ants know where home is)
        # This is more reliable than following trails when returning
        target_direction = self._get_direction_to(colony_pos[0], colony_pos[1])
        angle_diff = target_direction - self.direction
        while angle_diff > math.pi:
            angle_diff -= 2 * math.pi
        while angle_diff < -math.pi:
            angle_diff += 2 * math.pi
        self.direction += angle_diff * 0.4
        self.direction += random.uniform(-0.05, 0.05)
    
    def _idle_behavior(self):
        """Idle behavior - rest near colony"""
        if self.energy > self.max_energy * 0.8:
            self.state = AntState.FORAGING
        else:
            self.direction += random.uniform(-0.2, 0.2)
    
    def _move(self, width, height, pheromone_map):
        """Move ant in current direction with proactive wall avoidance"""
        # Track consecutive stuck frames
        if not hasattr(self, 'stuck_counter'):
            self.stuck_counter = 0
        if not hasattr(self, 'wall_stuck_time'):
            self.wall_stuck_time = 0
        
        # Check if currently inside a wall
        inside_wall = False
        if hasattr(self.colony, 'wall_manager'):
            inside_wall, _ = self.colony.wall_manager.is_colliding(self.x, self.y, self.radius * 0.5)
        
        # Track wall stuck time - if inside wall too long, mark for death
        if inside_wall:
            self.wall_stuck_time += 1
            if self.wall_stuck_time > 60:  # ~1 second at 60 FPS
                self.alive = False  # Kill the ant
                return
            # Try aggressive push out
            self.x, self.y = self.colony.wall_manager.push_out_of_wall(self.x, self.y, self.radius)
        else:
            self.wall_stuck_time = 0  # Reset if not in wall
        
        # General stuck detection and escape mechanism
        if self.stuck_counter > 30:
            self.direction = random.uniform(0, 2 * math.pi)  # Random new direction
            self.stuck_counter = 0
            # Push out of wall if inside
            if hasattr(self.colony, 'wall_manager'):
                self.x, self.y = self.colony.wall_manager.push_out_of_wall(self.x, self.y, self.radius)
        
        # PROACTIVE WALL AVOIDANCE - "see" walls ahead and turn before hitting
        if hasattr(self.colony, 'wall_manager'):
            should_turn, turn_amount = self.colony.wall_manager.get_avoidance_direction(
                self.x, self.y, self.direction, look_range=80
            )
            if should_turn:
                self.direction += turn_amount
        
        # DANGER PHEROMONE AVOIDANCE - avoid areas where ants have died
        should_avoid, avoid_turn = pheromone_map.get_danger_avoidance(self.x, self.y, self.direction)
        if should_avoid:
            self.direction += avoid_turn
        
        # Smooth direction changes with momentum
        momentum = 0.7
        self.direction = self.previous_direction * momentum + self.direction * (1 - momentum)
        self.previous_direction = self.direction
        
        # Add small random jitter to prevent oscillation
        self.direction += random.uniform(-0.05, 0.05)
        
        # Calculate movement
        dx = math.cos(self.direction) * self.speed
        dy = math.sin(self.direction) * self.speed
        
        new_x = self.x + dx
        new_y = self.y + dy
        
        # Check collision with walls - if about to enter, stop and turn
        if hasattr(self.colony, 'wall_manager'):
            colliding, wall = self.colony.wall_manager.is_colliding(new_x, new_y, self.radius)
            if colliding:
                # Push out if inside
                new_x, new_y = self.colony.wall_manager.push_out_of_wall(new_x, new_y, self.radius)
                # Turn perpendicular to wall
                wall_center_x = wall.rect.centerx
                wall_center_y = wall.rect.centery
                away_angle = math.atan2(self.y - wall_center_y, self.x - wall_center_x)
                self.direction = away_angle + random.uniform(-0.5, 0.5)
                self.stuck_counter += 2
                # Don't move into wall
                new_x = self.x
                new_y = self.y
        
        # Update position
        self.x = new_x
        self.y = new_y
        
        # Track distance traveled for stuck detection
        dist = math.sqrt((self.x - self.prev_x)**2 + (self.y - self.prev_y)**2)
        
        # Check if stuck
        if dist < 0.5:
            self.stuck_counter += 1
        else:
            self.stuck_counter = max(0, self.stuck_counter - 1)  # Recover if moving
        
        # Hard clamp to bounds
        if hasattr(self, 'bounds') and self.bounds:
            if self.x <= self.bounds.left:
                self.x = self.bounds.left + 2
                self.direction = random.uniform(-math.pi/2, math.pi/2)  # Face right
            elif self.x >= self.bounds.right:
                self.x = self.bounds.right - 2
                self.direction = random.uniform(math.pi/2, 3*math.pi/2)  # Face left
            
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
