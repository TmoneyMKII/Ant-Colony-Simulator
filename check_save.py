#!/usr/bin/env python3
"""Verify the save system is working"""

import os
import json

save_file = "ant_saves/colony_state.json"

if os.path.exists(save_file):
    with open(save_file, 'r') as f:
        data = json.load(f)
    print(f"Current save state:")
    print(f"  Generation: {data['generation']}")
    print(f"  Gene Pool Size: {len(data['gene_pool'])}")
    print(f"  Best Fitness: {data['stats']['best_fitness']}")
    print(f"  Timestamp: {data['timestamp']}")
else:
    print("No save file found - will start fresh on next run")
