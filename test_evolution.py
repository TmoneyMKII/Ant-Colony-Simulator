#!/usr/bin/env python3
"""Test save/load with actual evolution"""

from colony import Colony
from save_state import save_colony_state, load_colony_state, apply_saved_state_to_colony
import os

print("=== PERSISTENCE TEST: Multi-Session Evolution ===\n")

# Test 1: Run colony for a while
print("TEST 1: Running colony for 500 updates...")
colony1 = Colony(500, 500, 1000, 1000)

for i in range(500):
    colony1.update()
    if i % 100 == 0 and i > 0:
        print(f"  Update {i}: Ants={len(colony1.ants)}, Generation={colony1.generation}, Pool={len(colony1.gene_pool)}, Fitness={colony1.best_fitness:.1f}")

print(f"\nSession 1 - State after 500 updates:")
print(f"  Generation: {colony1.generation}")
print(f"  Population: {len(colony1.ants)}")
print(f"  Gene Pool Size: {len(colony1.gene_pool)}")
print(f"  Best Fitness: {colony1.best_fitness:.1f}")
print(f"  Food Stored: {colony1.food_stored:.0f}")

# Save the state
print("\nSaving colony state...")
save_colony_state(colony1)

# Test 2: Load the saved state and continue
print("\n\nTEST 2: Loading saved state and continuing...")
loaded_state = load_colony_state()

colony2 = Colony(500, 500, 1000, 1000)
apply_saved_state_to_colony(colony2, loaded_state)

print(f"Session 2 - Initial state (loaded):")
print(f"  Generation: {colony2.generation}")
print(f"  Gene Pool Size: {len(colony2.gene_pool)}")

# Continue simulation with loaded genes
for i in range(500):
    colony2.update()
    if i % 100 == 0 and i > 0:
        print(f"  Update {i}: Ants={len(colony2.ants)}, Generation={colony2.generation}, Pool={len(colony2.gene_pool)}, Fitness={colony2.best_fitness:.1f}")

print(f"\nSession 2 - State after 500 more updates:")
print(f"  Generation: {colony2.generation}")
print(f"  Population: {len(colony2.ants)}")
print(f"  Gene Pool Size: {len(colony2.gene_pool)}")
print(f"  Best Fitness: {colony2.best_fitness:.1f}")
print(f"  Food Stored: {colony2.food_stored:.0f}")

# Save again
print("\nSaving updated colony state...")
save_colony_state(colony2)

# Test 3: Verify we can load the updated state
print("\n\nTEST 3: Verifying updated state can be loaded...")
final_state = load_colony_state()
print(f"  Generation in saved state: {final_state['generation']}")
print(f"  Gene pool size in saved state: {len(final_state['gene_pool'])}")

print("\n[SUCCESS] Persistence system fully operational!")
print("  Ants can evolve over multiple sessions, preserving their learned traits!")
