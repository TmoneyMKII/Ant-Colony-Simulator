"""
Neural Network Module for Ant Colony Path Optimization

A feedforward neural network that helps ants make movement decisions
based on ray-based vision inputs (walls, ants, food), pheromones, and state.

The network learns through evolutionary approach - successful ants
(those that collect more food) have their weights favored for reproduction.
"""

import math
import random
from typing import List, Tuple, Optional

# Vision system configuration (must match vision.py)
NUM_VISION_RAYS = 7  # 7 rays across 180° FOV
VISION_INPUTS = NUM_VISION_RAYS * 3  # wall, ant, food per ray = 21 inputs


def sigmoid(x: float) -> float:
    """Sigmoid activation function (bounded 0-1)"""
    try:
        return 1.0 / (1.0 + math.exp(-max(-500, min(500, x))))
    except OverflowError:
        return 0.0 if x < 0 else 1.0


def tanh(x: float) -> float:
    """Tanh activation function (bounded -1 to 1)"""
    return math.tanh(max(-500, min(500, x)))


def relu(x: float) -> float:
    """ReLU activation function"""
    return max(0, x)


class NeuralNetwork:
    """
    Feedforward neural network for ant decision-making with ray-based vision.
    
    Architecture:
        Input Layer (27 neurons):
            Vision rays (21 inputs - 7 rays × 3 types):
                0-6: Wall distances per ray (0=far, 1=close)
                7-13: Ant distances per ray (0=far, 1=close)
                14-20: Food distances per ray (0=far, 1=close)
            
            Pheromone & State (6 inputs):
                21: Food pheromone strength ahead
                22: Home pheromone strength ahead
                23: Distance to colony (normalized)
                24: Direction to colony (normalized -1 to 1)
                25: Carrying food (0 or 1)
                26: Energy level (0 to 1)
        
        Hidden Layer (16 neurons):
            Processes combinations of visual and state inputs
        
        Output Layer (3 neurons):
            0: Turn amount (-1 to 1, mapped to radians)
            1: Speed modifier (0.5 to 1.5)
            2: Exploration tendency (0 to 1)
    """
    
    # 21 vision + 6 state = 27 inputs
    INPUT_SIZE = VISION_INPUTS + 6
    HIDDEN_SIZE = 16  # Larger hidden layer for more complex vision processing
    OUTPUT_SIZE = 3
    
    def __init__(self, weights: Optional[List[float]] = None):
        """Initialize network with random or provided weights"""
        # Calculate total weights needed
        self.weights_ih_count = self.INPUT_SIZE * self.HIDDEN_SIZE  # Input to hidden
        self.weights_ho_count = self.HIDDEN_SIZE * self.OUTPUT_SIZE  # Hidden to output
        self.bias_h_count = self.HIDDEN_SIZE
        self.bias_o_count = self.OUTPUT_SIZE
        self.total_weights = (self.weights_ih_count + self.weights_ho_count + 
                              self.bias_h_count + self.bias_o_count)
        
        if weights is None:
            # Xavier initialization for better convergence
            self.weights = [random.gauss(0, 0.5) for _ in range(self.total_weights)]
        else:
            self.weights = list(weights)
        
        # Unpack weights into matrices for faster access
        self._unpack_weights()
        
        # Track activations for visualization
        self.last_inputs = [0.0] * self.INPUT_SIZE
        self.last_hidden = [0.0] * self.HIDDEN_SIZE
        self.last_outputs = [0.0] * self.OUTPUT_SIZE
    
    def _unpack_weights(self):
        """Unpack flat weight list into structured matrices"""
        idx = 0
        
        # Input -> Hidden weights (INPUT_SIZE x HIDDEN_SIZE)
        self.w_ih = []
        for i in range(self.INPUT_SIZE):
            row = []
            for j in range(self.HIDDEN_SIZE):
                row.append(self.weights[idx])
                idx += 1
            self.w_ih.append(row)
        
        # Hidden -> Output weights (HIDDEN_SIZE x OUTPUT_SIZE)
        self.w_ho = []
        for i in range(self.HIDDEN_SIZE):
            row = []
            for j in range(self.OUTPUT_SIZE):
                row.append(self.weights[idx])
                idx += 1
            self.w_ho.append(row)
        
        # Hidden biases
        self.b_h = self.weights[idx:idx + self.HIDDEN_SIZE]
        idx += self.HIDDEN_SIZE
        
        # Output biases
        self.b_o = self.weights[idx:idx + self.OUTPUT_SIZE]
    
    def forward(self, inputs: List[float]) -> List[float]:
        """
        Forward pass through the network.
        
        Args:
            inputs: List of 8 normalized input values
            
        Returns:
            List of 3 output values [turn, speed_mod, exploration]
        """
        # Store inputs for visualization
        self.last_inputs = list(inputs)
        
        # Hidden layer computation
        hidden = []
        for j in range(self.HIDDEN_SIZE):
            h = self.b_h[j]
            for i in range(self.INPUT_SIZE):
                h += inputs[i] * self.w_ih[i][j]
            hidden.append(tanh(h))  # Tanh for hidden layer
        
        self.last_hidden = hidden
        
        # Output layer computation
        outputs = []
        for k in range(self.OUTPUT_SIZE):
            o = self.b_o[k]
            for j in range(self.HIDDEN_SIZE):
                o += hidden[j] * self.w_ho[j][k]
            outputs.append(o)
        
        # Apply output activations
        final_outputs = [
            tanh(outputs[0]),           # Turn: -1 to 1
            sigmoid(outputs[1]) + 0.5,  # Speed: 0.5 to 1.5
            sigmoid(outputs[2])         # Exploration: 0 to 1
        ]
        
        self.last_outputs = final_outputs
        return final_outputs
    
    def mutate(self, rate: float = 0.1, strength: float = 0.3) -> 'NeuralNetwork':
        """
        Create a mutated copy of this network.
        
        Args:
            rate: Probability of each weight being mutated
            strength: Standard deviation of mutation
            
        Returns:
            New NeuralNetwork with mutated weights
        """
        new_weights = []
        for w in self.weights:
            if random.random() < rate:
                new_weights.append(w + random.gauss(0, strength))
            else:
                new_weights.append(w)
        return NeuralNetwork(new_weights)
    
    def crossover(self, other: 'NeuralNetwork') -> 'NeuralNetwork':
        """
        Create offspring by crossing over with another network.
        
        Args:
            other: Another NeuralNetwork to crossover with
            
        Returns:
            New NeuralNetwork with mixed weights
        """
        new_weights = []
        for w1, w2 in zip(self.weights, other.weights):
            if random.random() < 0.5:
                new_weights.append(w1)
            else:
                new_weights.append(w2)
        return NeuralNetwork(new_weights)
    
    def get_weight_stats(self) -> dict:
        """Get statistics about network weights for visualization"""
        return {
            'mean': sum(self.weights) / len(self.weights),
            'min': min(self.weights),
            'max': max(self.weights),
            'std': math.sqrt(sum((w - sum(self.weights)/len(self.weights))**2 
                                 for w in self.weights) / len(self.weights))
        }
    
    def copy(self) -> 'NeuralNetwork':
        """Create an exact copy of this network"""
        return NeuralNetwork(list(self.weights))
    
    def to_dict(self) -> dict:
        """Serialize network to dictionary for saving"""
        return {'weights': self.weights}
    
    @classmethod
    def from_dict(cls, data: dict) -> 'NeuralNetwork':
        """Deserialize network from dictionary"""
        return cls(data['weights'])


class AntBrain:
    """
    Wrapper that provides ant-specific interface to the neural network.
    Handles vision input processing and output interpretation.
    """
    
    def __init__(self, network: Optional[NeuralNetwork] = None):
        self.network = network or NeuralNetwork()
        self.fitness = 0.0  # Accumulated fitness score
        self.decisions_made = 0
    
    def decide(self,
               vision_inputs: List[float],
               food_pheromone_ahead: float,
               home_pheromone_ahead: float,
               colony_distance: float,
               colony_direction: float,
               carrying_food: bool,
               energy: float,
               max_pheromone: float = 200.0,
               max_distance: float = 1000.0) -> Tuple[float, float, float]:
        """
        Make a movement decision based on vision and state inputs.
        
        Args:
            vision_inputs: 21 values from ray vision (walls, ants, food)
            food_pheromone_ahead: Pheromone strength in forward direction
            home_pheromone_ahead: Home trail strength in forward direction
            colony_distance: Distance to home colony
            colony_direction: Angle to colony relative to heading
            carrying_food: Whether ant is carrying food
            energy: Current energy level (0-100)
            
        Returns:
            Tuple of (turn_angle, speed_modifier, exploration_tendency)
            - turn_angle: Radians to turn (-pi/2 to pi/2)
            - speed_modifier: Multiplier for base speed (0.5 to 1.5)
            - exploration: How much to explore vs follow trails (0 to 1)
        """
        # Build full input vector: 21 vision + 6 state = 27 inputs
        inputs = list(vision_inputs)  # Already normalized 0-1 from vision system
        
        # Add state inputs (normalized)
        inputs.extend([
            min(1.0, food_pheromone_ahead / max_pheromone),
            min(1.0, home_pheromone_ahead / max_pheromone),
            min(1.0, colony_distance / max_distance),
            colony_direction / math.pi,  # -1 to 1
            1.0 if carrying_food else 0.0,
            min(1.0, energy / 100.0)  # Normalized energy
        ])
        
        outputs = self.network.forward(inputs)
        self.decisions_made += 1
        
        # Convert outputs to usable values
        turn_angle = outputs[0] * (math.pi / 2)  # -90 to +90 degrees
        speed_mod = outputs[1]  # Already 0.5 to 1.5
        exploration = outputs[2]  # 0 to 1
        
        return turn_angle, speed_mod, exploration
    
    def add_fitness(self, amount: float):
        """Add to fitness score (called when ant does something good)"""
        self.fitness += amount
    
    def get_activations(self) -> dict:
        """Get current network activations for visualization"""
        return {
            'inputs': self.network.last_inputs,
            'hidden': self.network.last_hidden,
            'outputs': self.network.last_outputs
        }
    
    def mutate(self, rate: float = 0.1, strength: float = 0.3) -> 'AntBrain':
        """Create mutated copy"""
        return AntBrain(self.network.mutate(rate, strength))
    
    def crossover(self, other: 'AntBrain') -> 'AntBrain':
        """Create offspring with another brain"""
        return AntBrain(self.network.crossover(other.network))
    
    def copy(self) -> 'AntBrain':
        """Create exact copy"""
        return AntBrain(self.network.copy())
