#!/usr/bin/env python3
"""Test save/load functionality"""

from colony import Colony
from save_state import save_colony_state, load_colony_state, apply_saved_state_to_colony
import os

# Create a test colony
print("Creating test colony...")
colony1 = Colony(500, 500, 1000, 1000)

# Advance a few generations manually
for i in range(50):
    colony1.update()
    if i % 10 == 0:
        print(f"Update {i}: Population={len(colony1.ants)}, Generation={colony1.generation}")

print(f"\nColony 1 - Before save:")
print(f"  Generation: {colony1.generation}")
print(f"  Population: {len(colony1.ants)}")
print(f"  Gene Pool Size: {len(colony1.gene_pool)}")
print(f"  Best Fitness: {colony1.best_fitness:.1f}")
print(f"  Food Stored: {colony1.food_stored:.0f}")

# Save the colony
print("\nSaving colony...")
save_colony_state(colony1)
print(f"Saved to: ant_saves/colony_state.json")

# Verify file exists
if os.path.exists('ant_saves/colony_state.json'):
    print("Save file created successfully")
    
    # Load it back
    print("\nLoading saved state...")
    loaded_state = load_colony_state()
    
    if loaded_state:
        print("State loaded successfully")
        print(f"  Generation: {loaded_state['generation']}")
        print(f"  Gene Pool Size: {len(loaded_state['gene_pool'])}")
        
        # Create a new colony and apply the loaded state
        print("\nCreating new colony and applying loaded state...")
        colony2 = Colony(500, 500, 1000, 1000)
        apply_saved_state_to_colony(colony2, loaded_state)
        
        print(f"\nColony 2 - After loading:")
        print(f"  Generation: {colony2.generation}")
        print(f"  Population: {len(colony2.ants)}")
        print(f"  Gene Pool Size: {len(colony2.gene_pool)}")
        print(f"  Best Fitness: {colony2.best_fitness:.1f}")
        
        # Verify they match
        if colony2.generation == colony1.generation and len(colony2.gene_pool) == len(colony1.gene_pool):
            print("\nSave/Load system working correctly!")
        else:
            print("\n- Mismatch detected")
    else:
        print("- Failed to load state")
else:
    print("- Save file not created")
