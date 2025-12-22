"""Colony management and simulation"""

import pygame
import random
from ant import Ant, AntState
from pheromone import PheromoneMap

class FoodSource:
    """A food source on the map"""
    def __init__(self, x, y, amount=100):
        self.x = x
        self.y = y
        self.amount = amount
        self.radius = 10
        self.color = (200, 150, 50)
    
    def draw(self, surface):
        pygame.draw.circle(surface, self.color, (int(self.x), int(self.y)), self.radius)
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
        self.max_population = 500
        
        # Ants
        self.ants = []
        
        # Simulation
        self.pheromone_map = PheromoneMap(width, height)
        self.food_sources = []
        self.time = 0
        
        # Spawn initial ants
        self._spawn_initial_ants(30)
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
        
        for _ in range(5):
            x = random.uniform(x_min, x_max)
            y = random.uniform(y_min, y_max)
            self.food_sources.append(FoodSource(x, y, 100))
    
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
        """Spawn a new ant if conditions allow"""
        if self.population < self.max_population and self.food_stored > 100:
            angle = random.uniform(0, 2 * 3.14159)
            dist = random.uniform(0, self.radius + 5)
            x = self.x + dist * __import__('math').cos(angle)
            y = self.y + dist * __import__('math').sin(angle)
            self.ants.append(Ant(x, y, self))
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
            if not ant.update(self.pheromone_map, self.width, self.height, self.food_sources, (self.x, self.y)):
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
        
        # Regenerate food sources
        for food in self.food_sources:
            if food.amount < 100:
                food.amount += 0.1
    
    def draw(self, surface, show_pheromones=True, view_rect=None):
        """Draw colony and all entities"""
        # If view_rect is provided, only draw within that area
        if view_rect is None:
            view_rect = surface.get_rect()
        
        # Draw pheromones (background)
        if show_pheromones:
            self.pheromone_map.draw(surface, show_foraging=True, show_returning=True, opacity=40)
        
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
        return {
            'population': self.population,
            'food_stored': self.food_stored,
            'ants_foraging': sum(1 for a in self.ants if a.state == AntState.FORAGING),
            'ants_returning': sum(1 for a in self.ants if a.state == AntState.RETURNING),
            'food_sources_active': sum(1 for f in self.food_sources if f.amount > 5)
        }
