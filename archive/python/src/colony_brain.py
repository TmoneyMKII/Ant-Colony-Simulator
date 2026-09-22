"""
Colony Brain - Collective Intelligence System

Manages the neural network evolution for the entire colony.
Tracks the best-performing ant brains and evolves them over generations.
Provides colony-wide statistics and learning metrics.
"""

import random
import math
from typing import List, Optional, Tuple
from collections import deque
from src.neural_net import NeuralNetwork, AntBrain


class ColonyBrain:
    """
    Manages collective learning for the ant colony.
    
    Uses an evolutionary approach where successful ants' neural networks
    are more likely to be used as templates for new ants.
    """
    
    def __init__(self, population_size: int = 100):
        self.population_size = population_size
        
        # Elite brains (top performers)
        self.elite_brains: List[AntBrain] = []
        self.max_elites = 10
        
        # Colony-wide network (averaged from elites)
        self.colony_network = NeuralNetwork()
        
        # Statistics tracking
        self.generation = 0
        self.total_food_collected = 0
        self.total_trips_completed = 0
        
        # Historical data for graphs (rolling windows)
        self.food_history = deque(maxlen=300)  # 5 seconds at 60fps
        self.fitness_history = deque(maxlen=300)
        self.population_history = deque(maxlen=300)
        self.exploration_history = deque(maxlen=300)
        
        # Learning metrics
        self.avg_fitness = 0.0
        self.best_fitness = 0.0
        self.diversity_score = 0.0  # How different the brains are
        
        # Aggregated activations for visualization
        self.avg_activations = {
            'inputs': [0.0] * NeuralNetwork.INPUT_SIZE,
            'hidden': [0.0] * NeuralNetwork.HIDDEN_SIZE,
            'outputs': [0.0] * NeuralNetwork.OUTPUT_SIZE
        }
        self.activation_samples = 0
        
        # Knowledge metrics
        self.knowledge_score = 0.0  # How well colony "knows" the environment
        self.path_efficiency = 0.0  # Average path efficiency
        self.cooperation_score = 0.0  # How well ants work together
    
    def create_brain(self) -> AntBrain:
        """
        Create a new brain for a new ant.
        Uses elite brains as templates with mutation.
        """
        if len(self.elite_brains) == 0:
            # No elites yet, create random brain
            return AntBrain()
        
        if len(self.elite_brains) == 1:
            # Only one elite, mutate it
            return self.elite_brains[0].mutate(rate=0.2, strength=0.4)
        
        # Pick two random elites and crossover + mutate
        parent1 = random.choice(self.elite_brains)
        parent2 = random.choice(self.elite_brains)
        
        child = parent1.crossover(parent2)
        child = child.mutate(rate=0.15, strength=0.3)
        
        return child
    
    def report_ant_performance(self, brain: AntBrain, food_collected: int, 
                                trips: int, time_alive: int):
        """
        Report an ant's performance (called when ant dies or periodically).
        Updates elite list if this ant performed well.
        """
        # Calculate fitness
        if time_alive > 0:
            efficiency = (food_collected * 100 + trips * 50) / (time_alive / 60.0)
        else:
            efficiency = 0
        
        brain.fitness = efficiency
        
        # Update totals
        self.total_food_collected += food_collected
        self.total_trips_completed += trips
        
        # Check if this brain qualifies as elite
        if len(self.elite_brains) < self.max_elites:
            self.elite_brains.append(brain.copy())
            self.elite_brains.sort(key=lambda b: b.fitness, reverse=True)
        elif efficiency > self.elite_brains[-1].fitness:
            # Replace worst elite
            self.elite_brains[-1] = brain.copy()
            self.elite_brains.sort(key=lambda b: b.fitness, reverse=True)
        
        # Update best fitness
        if efficiency > self.best_fitness:
            self.best_fitness = efficiency
    
    def update(self, ants: list, food_collected_this_frame: int = 0):
        """
        Update colony brain state each frame.
        Aggregates activations and updates statistics.
        """
        if not ants:
            return
        
        # Reset activation aggregation
        input_sums = [0.0] * NeuralNetwork.INPUT_SIZE
        hidden_sums = [0.0] * NeuralNetwork.HIDDEN_SIZE
        output_sums = [0.0] * NeuralNetwork.OUTPUT_SIZE
        count = 0
        
        total_fitness = 0.0
        total_exploration = 0.0
        
        for ant in ants:
            if hasattr(ant, 'brain') and ant.brain is not None:
                activations = ant.brain.get_activations()
                
                for i, v in enumerate(activations['inputs']):
                    input_sums[i] += v
                for i, v in enumerate(activations['hidden']):
                    hidden_sums[i] += v
                for i, v in enumerate(activations['outputs']):
                    output_sums[i] += v
                
                total_fitness += ant.brain.fitness
                total_exploration += activations['outputs'][2] if len(activations['outputs']) > 2 else 0
                count += 1
        
        if count > 0:
            self.avg_activations = {
                'inputs': [s / count for s in input_sums],
                'hidden': [s / count for s in hidden_sums],
                'outputs': [s / count for s in output_sums]
            }
            self.avg_fitness = total_fitness / count
            avg_exploration = total_exploration / count
        else:
            avg_exploration = 0.5
        
        # Update history for graphs
        self.food_history.append(food_collected_this_frame)
        self.fitness_history.append(self.avg_fitness)
        self.population_history.append(len(ants))
        self.exploration_history.append(avg_exploration)
        
        # Calculate knowledge score (based on elite performance)
        if self.elite_brains:
            avg_elite_fitness = sum(b.fitness for b in self.elite_brains) / len(self.elite_brains)
            self.knowledge_score = min(1.0, avg_elite_fitness / 100.0)
        
        # Calculate diversity (variance in elite weights)
        self._calculate_diversity()
        
        # Calculate cooperation (based on pheromone following)
        if count > 0:
            self.cooperation_score = 1.0 - avg_exploration
    
    def _calculate_diversity(self):
        """Calculate genetic diversity among elite brains"""
        if len(self.elite_brains) < 2:
            self.diversity_score = 1.0
            return
        
        # Calculate average pairwise weight difference
        total_diff = 0.0
        pairs = 0
        
        for i, brain1 in enumerate(self.elite_brains):
            for brain2 in self.elite_brains[i+1:]:
                diff = sum(abs(w1 - w2) for w1, w2 in 
                          zip(brain1.network.weights, brain2.network.weights))
                total_diff += diff / len(brain1.network.weights)
                pairs += 1
        
        if pairs > 0:
            self.diversity_score = min(1.0, total_diff / pairs)
    
    def evolve_generation(self):
        """
        Called periodically to advance the generation.
        Refines elite brains and updates colony network.
        """
        self.generation += 1
        
        # Update colony network to average of elites
        if self.elite_brains:
            avg_weights = []
            for i in range(len(self.elite_brains[0].network.weights)):
                avg = sum(b.network.weights[i] for b in self.elite_brains) / len(self.elite_brains)
                avg_weights.append(avg)
            self.colony_network = NeuralNetwork(avg_weights)
        
        # Decay elite fitness to encourage fresh blood
        for brain in self.elite_brains:
            brain.fitness *= 0.95
    
    def get_stats(self) -> dict:
        """Get current colony statistics for display"""
        return {
            'generation': self.generation,
            'total_food': self.total_food_collected,
            'total_trips': self.total_trips_completed,
            'avg_fitness': self.avg_fitness,
            'best_fitness': self.best_fitness,
            'knowledge': self.knowledge_score,
            'diversity': self.diversity_score,
            'cooperation': self.cooperation_score,
            'elite_count': len(self.elite_brains)
        }
    
    def get_graph_data(self) -> dict:
        """Get historical data for graphs"""
        return {
            'food': list(self.food_history),
            'fitness': list(self.fitness_history),
            'population': list(self.population_history),
            'exploration': list(self.exploration_history)
        }
    
    def get_network_visualization_data(self) -> dict:
        """Get data for neural network visualization"""
        # Build vision ray labels (7 rays × 3 types = 21 vision inputs)
        input_labels = []
        ray_angles = ['-90°', '-60°', '-30°', '0°', '+30°', '+60°', '+90°']
        
        # Wall rays (0-6)
        for angle in ray_angles:
            input_labels.append(f'Wall {angle}')
        
        # Ant rays (7-13)
        for angle in ray_angles:
            input_labels.append(f'Ant {angle}')
        
        # Food rays (14-20)
        for angle in ray_angles:
            input_labels.append(f'Food {angle}')
        
        # State inputs (21-26)
        input_labels.extend([
            'Food Pher ↑',
            'Home Pher ↑',
            'Colony Dist',
            'Colony Dir',
            'Carrying',
            'Energy'
        ])
        
        return {
            'activations': self.avg_activations,
            'input_labels': input_labels,
            'output_labels': [
                'Turn',
                'Speed',
                'Explore'
            ],
            'weights': self.colony_network.get_weight_stats() if self.colony_network else None
        }
    
    def to_dict(self) -> dict:
        """Serialize for saving"""
        return {
            'generation': self.generation,
            'total_food': self.total_food_collected,
            'total_trips': self.total_trips_completed,
            'elite_brains': [{'weights': b.network.weights, 'fitness': b.fitness} 
                           for b in self.elite_brains],
            'best_fitness': self.best_fitness
        }
    
    @classmethod
    def from_dict(cls, data: dict, population_size: int = 100) -> 'ColonyBrain':
        """Deserialize from saved data"""
        brain = cls(population_size)
        brain.generation = data.get('generation', 0)
        brain.total_food_collected = data.get('total_food', 0)
        brain.total_trips_completed = data.get('total_trips', 0)
        brain.best_fitness = data.get('best_fitness', 0)
        
        for elite_data in data.get('elite_brains', []):
            elite = AntBrain(NeuralNetwork(elite_data['weights']))
            elite.fitness = elite_data.get('fitness', 0)
            brain.elite_brains.append(elite)
        
        return brain
