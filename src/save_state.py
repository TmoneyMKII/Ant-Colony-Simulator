"""Save/Load system for ant colony state"""

import json
import os
from datetime import datetime

SAVE_DIR = "ant_saves"
SAVE_FILE = os.path.join(SAVE_DIR, "colony_state.json")

def ensure_save_dir():
    """Create save directory if it doesn't exist"""
    if not os.path.exists(SAVE_DIR):
        os.makedirs(SAVE_DIR)

def save_colony_state(colony):
    """Save colony state"""
    ensure_save_dir()
    
    state = {
        'timestamp': datetime.now().isoformat(),
        'stats': {
            'population': colony.population,
            'food_stored': colony.food_stored,
        }
    }
    
    with open(SAVE_FILE, 'w') as f:
        json.dump(state, f, indent=2)
    
    print(f"[SAVED] Colony saved! Population: {colony.population}, Food: {colony.food_stored:.0f}")
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
        
        result = {
            'stats': state.get('stats', {}),
            'timestamp': state.get('timestamp', 'unknown'),
        }
        
        print(f"[LOADED] Colony loaded!")
        return result
    
    except Exception as e:
        print(f"Error loading save file: {e}")
        return None

def apply_saved_state_to_colony(colony, saved_state):
    """Apply loaded state to a colony"""
    if not saved_state:
        return
    
    # Just log that we loaded - don't apply anything since we start fresh
    print(f"[INFO] Previous save found from {saved_state.get('timestamp', 'unknown')}")
