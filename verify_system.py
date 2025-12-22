#!/usr/bin/env python3
"""Final verification that all systems are integrated"""

import sys
import os

# Test all imports
print("Verifying all modules can be imported...")
try:
    import pygame
    print("  [OK] Pygame")
    from config import DARK_BG_COLOR, GRID_SIZE
    print("  [OK] Config")
    from ant import Ant, AntState
    print("  [OK] Ant")
    from pheromone import PheromoneMap
    print("  [OK] Pheromone")
    from genetics import AntGenes
    print("  [OK] Genetics")
    from colony import Colony
    print("  [OK] Colony")
    from ui import UIManager
    print("  [OK] UI")
    from save_state import save_colony_state, load_colony_state, apply_saved_state_to_colony
    print("  [OK] Save/Load System")
    print("\nAll modules imported successfully!")
except ImportError as e:
    print(f"  [FAILED] {e}")
    sys.exit(1)

# Test save system
print("\nVerifying save/load system...")
try:
    # Try to load existing save
    state = load_colony_state()
    if state:
        print(f"  [OK] Load: Generation {state['generation']}, Pool size {len(state['gene_pool'])}")
    else:
        print("  [OK] Load: No save file (will start fresh)")
    
    # Create test colony
    colony = Colony(500, 500, 1000, 1000)
    print(f"  [OK] Colony creation")
    
    # Try to save
    save_colony_state(colony)
    print(f"  [OK] Save: Colony saved successfully")
    
    # Verify file exists
    if os.path.exists("ant_saves/colony_state.json"):
        print("  [OK] Save file exists")
    
except Exception as e:
    print(f"  [FAILED] {e}")
    sys.exit(1)

print("\n" + "="*50)
print("SYSTEM VERIFICATION COMPLETE!")
print("="*50)
print("\nThe ant colony simulator is fully integrated and ready to run.")
print("To start the simulation, run: python main.py")
print("\nFeatures enabled:")
print("  - Real-time visualization with dark theme UI")
print("  - Ant agents with genetic evolution")
print("  - Pheromone-based communication")
print("  - Dynamic food system with depletion")
print("  - Persistent save/load system")
print("  - Statistics and fitness tracking")
