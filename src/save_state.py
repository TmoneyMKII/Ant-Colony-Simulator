"""Save/Load system for ant colony state"""

import json
import os
from datetime import datetime
from src.genetics import AntGenes

SAVE_DIR = "ant_saves"
SAVE_FILE = os.path.join(SAVE_DIR, "colony_state.json")

def ensure_save_dir():
    """Create save directory if it doesn't exist"""
    if not os.path.exists(SAVE_DIR):
        os.makedirs(SAVE_DIR)

def genes_to_dict(genes):
    """Convert AntGenes to dictionary for JSON serialization"""
    return {
        'speed': genes.speed,
        'pheromone_sensitivity': genes.pheromone_sensitivity,
        'exploration_rate': genes.exploration_rate,
        'pheromone_strength': genes.pheromone_strength,
        'energy_efficiency': genes.energy_efficiency,
    }

def dict_to_genes(gene_dict):
    """Convert dictionary back to AntGenes"""
    genes = AntGenes()
    genes.speed = gene_dict['speed']
    genes.pheromone_sensitivity = gene_dict['pheromone_sensitivity']
    genes.exploration_rate = gene_dict['exploration_rate']
    genes.pheromone_strength = gene_dict['pheromone_strength']
    genes.energy_efficiency = gene_dict['energy_efficiency']
    return genes

def save_colony_state(colony):
    """Save colony state including gene pool"""
    ensure_save_dir()
    
    state = {
        'timestamp': datetime.now().isoformat(),
        'generation': colony.generation,
        'gene_pool': [genes_to_dict(genes) for genes in colony.gene_pool],
        'stats': {
            'best_fitness': colony.best_fitness,
            'average_fitness': colony.average_fitness,
            'population': colony.population,
            'food_stored': colony.food_stored,
        }
    }
    
    with open(SAVE_FILE, 'w') as f:
        json.dump(state, f, indent=2)
    
    print(f"[SAVED] Colony saved! Generation {colony.generation}, Gene pool: {len(colony.gene_pool)}")
    return True

def load_colony_state():
    """Load colony state from save file"""
    ensure_save_dir()
    
    if not os.path.exists(SAVE_FILE):
        print("No save file found. Starting fresh.")
        return None
    
    try:
        with open(SAVE_FILE, 'r') as f:
            state = json.load(f)
        
        # Reconstruct gene pool
        gene_pool = [dict_to_genes(g) for g in state.get('gene_pool', [])]
        
        result = {
            'generation': state.get('generation', 0),
            'gene_pool': gene_pool,
            'stats': state.get('stats', {}),
            'timestamp': state.get('timestamp', 'unknown'),
        }
        
        print(f"[LOADED] Colony loaded! Generation {result['generation']}, Gene pool: {len(gene_pool)}")
        return result
    
    except Exception as e:
        print(f"Error loading save file: {e}")
        return None

def apply_saved_state_to_colony(colony, saved_state):
    """Apply loaded state to a colony"""
    if not saved_state:
        return
    
    colony.generation = saved_state['generation']
    colony.gene_pool = saved_state['gene_pool']
    print(f"[APPLIED] Applied saved state: generation {colony.generation}, {len(colony.gene_pool)} genes in pool")
