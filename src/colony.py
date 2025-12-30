"""Colony management and simulation"""

import pygame
import random
from src.ant import Ant, AntState
from src.pheromone_model import PheromoneModel
from src.save_state import load_colony_state, apply_saved_state_to_colony
from src.walls import WallManager
from src.config import INITIAL_ANT_COUNT, MAX_POPULATION

class FoodSource:
    """A food source on the map"""
    def __init__(self, x, y, amount=100):
        self.x = x
        self.y = y
        self.amount = amount
        self.max_amount = amount
        self.radius = 10
        self.color = (200, 150, 50)
    
    def draw(self, surface):
        # Calculate size based on remaining food
        size_ratio = max(0.3, self.amount / self.max_amount)
        current_radius = int(self.radius * size_ratio)
        
        # Draw food with color intensity based on amount
        color_intensity = int(200 * size_ratio)
        color = (color_intensity, int(150 * size_ratio), 50)
        
        pygame.draw.circle(surface, color, (int(self.x), int(self.y)), current_radius)
        
        # Draw amount indicator
        font = pygame.font.Font(None, 12)
        text = font.render(str(int(self.amount)), True, (255, 255, 255))
        surface.blit(text, (int(self.x) - 6, int(self.y) - 6))

class Colony:
    """Ant colony management"""
    
    def __init__(self, x, y, width, height, bounds=None):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.bounds = bounds  # pygame.Rect for constraining ants
        self.radius = 20
        self.color = (150, 100, 50)
        
        # Colony state
        self.food_stored = 0
        self.max_food = 10000
        self.population = 0
        self.max_population = MAX_POPULATION
        
        # Ants
        self.ants = []
        
        # Simulation - use new PheromoneModel
        self.pheromone_map = PheromoneModel(width, height)
        self.food_sources = []
        self.time = 0
        
        # Create wall manager
        self.wall_manager = WallManager(width, height, bounds.left if bounds else 0, bounds.top if bounds else 0)
        
        # Load saved state if available
        saved_state = load_colony_state()
        if saved_state:
            apply_saved_state_to_colony(self, saved_state)
        
        # Spawn initial ants
        self._spawn_initial_ants(INITIAL_ANT_COUNT)
        self._create_food_sources()
    
    def _spawn_initial_ants(self, count):
        """Spawn initial ant population"""
        for _ in range(count):
            if self.bounds:
                x = random.uniform(self.bounds.left + 20, self.bounds.right - 20)
                y = random.uniform(self.bounds.top + 20, self.bounds.bottom - 20)
            else:
                angle = random.uniform(0, 2 * 3.14159)
                dist = random.uniform(0, self.radius + 10)
                x = self.x + dist * __import__('math').cos(angle)
                y = self.y + dist * __import__('math').sin(angle)
            self.ants.append(Ant(x, y, self))
            self.population += 1
    
    def _create_food_sources(self):
        """Create food sources on the map"""
        if self.bounds:
            x_min, x_max = self.bounds.left + 50, self.bounds.right - 50
            y_min, y_max = self.bounds.top + 50, self.bounds.bottom - 50
        else:
            x_min, x_max = 100, self.width - 100
            y_min, y_max = 100, self.height - 100
        
        for _ in range(12):  # Increased from 5 to 12
            x = random.uniform(x_min, x_max)
            y = random.uniform(y_min, y_max)
            # Random food amount between 50-150
            amount = random.uniform(50, 150)
            self.food_sources.append(FoodSource(x, y, amount))
    
    def add_food_source(self, x, y, amount=50):
        """Add a new food source at the specified position"""
        self.food_sources.append(FoodSource(x, y, amount))
    
    def add_food(self, amount):
        """Add food to colony"""
        self.food_stored = min(self.food_stored + amount, self.max_food)
    
    def consume_food(self):
        """Colony consumes food for survival"""
        consumption = self.population * 0.05
        if self.food_stored >= consumption:
            self.food_stored -= consumption
            return True
        else:
            # Population dies if starving
            self.population = max(0, self.population - int(consumption))
            self.food_stored = 0
            return False
    
    def spawn_ant(self):
        """Spawn a new ant"""
        if self.population < self.max_population and self.food_stored > 100:
            angle = random.uniform(0, 2 * 3.14159)
            dist = random.uniform(0, self.radius + 5)
            x = self.x + dist * __import__('math').cos(angle)
            y = self.y + dist * __import__('math').sin(angle)
            
            new_ant = Ant(x, y, self)
            self.ants.append(new_ant)
            self.population += 1
            self.food_stored -= 20  # Cost to create ant
    
    def update(self):
        """Update colony state"""
        self.time += 1
        
        # Update pheromones
        self.pheromone_map.update()
        
        # Update ants
        dead_ants = []
        for i, ant in enumerate(self.ants):
            if not ant.update(self.pheromone_map, self.width, self.height, self.food_sources, (self.x, self.y), self.ants, self.bounds):
                dead_ants.append(i)
        
        # Remove dead ants
        for i in reversed(dead_ants):
            self.ants.pop(i)
            self.population -= 1
        
        # Consume food for remaining population
        self.consume_food()
        
        # Spawn new ants periodically if well-fed
        if self.time % 60 == 0 and self.food_stored > 500:
            for _ in range(min(5, self.max_population - self.population)):
                self.spawn_ant()
        
        # Remove depleted food sources and spawn new ones
        depleted = [i for i, food in enumerate(self.food_sources) if food.amount <= 0]
        for i in reversed(depleted):
            self.food_sources.pop(i)
        
        # Spawn new food sources to maintain population
        while len(self.food_sources) < 12:
            if self.bounds:
                x = random.uniform(self.bounds.left + 50, self.bounds.right - 50)
                y = random.uniform(self.bounds.top + 50, self.bounds.bottom - 50)
            else:
                x = random.uniform(100, self.width - 100)
                y = random.uniform(100, self.height - 100)
            amount = random.uniform(50, 150)
            self.food_sources.append(FoodSource(x, y, amount))
    
    def draw(self, surface, show_pheromones=True, view_rect=None):
        """Draw colony and all entities"""
        # If view_rect is provided, only draw within that area
        if view_rect is None:
            view_rect = surface.get_rect()
        
        # Draw pheromones (background) - show both trails
        if show_pheromones:
            self.pheromone_map.draw(surface, show_food=True, show_home=True, opacity=180)
        
        # Draw walls
        self.wall_manager.draw(surface)
        
        # Draw food sources
        for food in self.food_sources:
            if view_rect.collidepoint(food.x, food.y):
                food.draw(surface)
        
        # Draw ants
        for ant in self.ants:
            if view_rect.collidepoint(ant.x, ant.y):
                ant.draw(surface)
        
        # Draw colony center
        pygame.draw.circle(surface, self.color, (int(self.x), int(self.y)), self.radius)
        pygame.draw.circle(surface, (200, 150, 100), (int(self.x), int(self.y)), self.radius - 3, 2)
    
    def get_stats(self):
        """Get colony statistics"""
        total_food_collected = sum(ant.food_collected for ant in self.ants)
        total_trips = sum(ant.successful_trips for ant in self.ants)
        
        return {
            'population': self.population,
            'food_stored': self.food_stored,
            'ants_foraging': sum(1 for a in self.ants if a.state == AntState.FORAGING),
            'ants_returning': sum(1 for a in self.ants if a.state == AntState.RETURNING),
            'food_sources_active': sum(1 for f in self.food_sources if f.amount > 5),
            'total_food_collected': total_food_collected,
            'total_trips': total_trips
        }
