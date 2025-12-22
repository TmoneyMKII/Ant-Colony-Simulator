"""Genetic system for ant evolution"""

import random
import math

class AntGenes:
    """Genetic traits that define ant behavior"""
    
    def __init__(self, parent1=None, parent2=None):
        if parent1 and parent2:
            # Breed from two parents
            self.speed = self._crossover(parent1.speed, parent2.speed)
            self.pheromone_sensitivity = self._crossover(parent1.pheromone_sensitivity, parent2.pheromone_sensitivity)
            self.exploration_rate = self._crossover(parent1.exploration_rate, parent2.exploration_rate)
            self.pheromone_strength = self._crossover(parent1.pheromone_strength, parent2.pheromone_strength)
            self.energy_efficiency = self._crossover(parent1.energy_efficiency, parent2.energy_efficiency)
        else:
            # Random initial genes
            self.speed = random.uniform(1.5, 3.5)
            self.pheromone_sensitivity = random.uniform(0.05, 0.35)
            self.exploration_rate = random.uniform(0.1, 0.3)
            self.pheromone_strength = random.uniform(0.8, 2.5)
            self.energy_efficiency = random.uniform(0.008, 0.015)
        
        # Apply mutations
        self._mutate()
        
        # Clamp values to valid ranges
        self._clamp_genes()
    
    def _crossover(self, gene1, gene2):
        """Combine two parent genes with some randomness"""
        # Average with slight bias toward better performer
        return (gene1 + gene2) / 2 + random.uniform(-0.1, 0.1)
    
    def _mutate(self):
        """Random mutations for genetic diversity"""
        mutation_rate = 0.15
        mutation_strength = 0.2
        
        if random.random() < mutation_rate:
            self.speed += random.uniform(-mutation_strength, mutation_strength)
        
        if random.random() < mutation_rate:
            self.pheromone_sensitivity += random.uniform(-0.05, 0.05)
        
        if random.random() < mutation_rate:
            self.exploration_rate += random.uniform(-0.05, 0.05)
        
        if random.random() < mutation_rate:
            self.pheromone_strength += random.uniform(-0.3, 0.3)
        
        if random.random() < mutation_rate:
            self.energy_efficiency += random.uniform(-0.002, 0.002)
    
    def _clamp_genes(self):
        """Keep genes within valid ranges"""
        self.speed = max(1.0, min(4.0, self.speed))
        self.pheromone_sensitivity = max(0.01, min(0.5, self.pheromone_sensitivity))
        self.exploration_rate = max(0.05, min(0.4, self.exploration_rate))
        self.pheromone_strength = max(0.5, min(3.0, self.pheromone_strength))
        self.energy_efficiency = max(0.005, min(0.02, self.energy_efficiency))
    
    def copy(self):
        """Create a copy of these genes"""
        new_genes = AntGenes()
        new_genes.speed = self.speed
        new_genes.pheromone_sensitivity = self.pheromone_sensitivity
        new_genes.exploration_rate = self.exploration_rate
        new_genes.pheromone_strength = self.pheromone_strength
        new_genes.energy_efficiency = self.energy_efficiency
        return new_genes


class FitnessTracker:
    """Track ant fitness for evolution"""
    
    def __init__(self):
        self.food_collected = 0
        self.energy_spent = 0
        self.lifetime = 0
        self.successful_trips = 0
        # Learning incentives
        self.wall_hits = 0           # Penalty for hitting walls
        self.distance_traveled = 0   # Reward for exploration
        self.time_stuck = 0          # Penalty for getting stuck
        self.food_discovery_time = 0 # Reward for finding food fast
    
    def calculate_fitness(self):
        """Calculate overall fitness score with learning incentives"""
        # Positive rewards
        food_score = self.food_collected * 15           # Strong food reward
        efficiency_score = (self.food_collected / max(1, self.energy_spent)) * 50
        survival_score = self.lifetime * 0.01
        trip_score = self.successful_trips * 20         # Strong trip reward
        exploration_score = self.distance_traveled * 0.01  # Reward movement
        
        # Penalties (negative incentives)
        wall_penalty = self.wall_hits * 5               # Punish wall hits
        stuck_penalty = self.time_stuck * 0.5           # Punish getting stuck
        
        # Speed bonus - faster food finding is better
        speed_bonus = 0
        if self.successful_trips > 0 and self.food_discovery_time > 0:
            avg_time = self.food_discovery_time / self.successful_trips
            speed_bonus = max(0, 100 - avg_time) * 0.5  # Bonus for quick trips
        
        # Calculate total fitness
        total = (food_score + efficiency_score + survival_score + 
                 trip_score + exploration_score + speed_bonus - 
                 wall_penalty - stuck_penalty)
        
        return max(0, total)  # Never negative
