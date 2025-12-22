# Ant Colony Simulator - Save/Load System Documentation

## Overview
The ant colony simulator now includes a complete save/load persistence system that allows your evolved colony to be saved between sessions. This means the ants' learned traits (genes) will persist across program restarts.

## How It Works

### Automatic Saving
The colony state is automatically saved when you:
1. **Pause the simulation** (SPACE key)
2. **Reset the simulation** (R key)
3. **Close the application** (ESC key or close window)

### Automatic Loading
When you start the program, it automatically:
1. Checks for a previously saved colony state
2. If found, loads the generation number and gene pool
3. Creates new ants with evolved genes from the loaded gene pool
4. Displays a message: `[LOADED] Generation X, Gene Pool Size: Y`

If no save file exists, it starts fresh with a message: `Starting new colony simulation...`

## File Structure

The save system stores data in:
```
ant_saves/
  └── colony_state.json
```

### JSON Format
```json
{
  "timestamp": "2025-12-23T03:09:00.197815",
  "generation": 42,
  "gene_pool": [
    {
      "speed": 2.5,
      "pheromone_sensitivity": 0.25,
      "exploration_rate": 0.45,
      "pheromone_strength": 85.0,
      "energy_efficiency": 0.08
    },
    ...
  ],
  "stats": {
    "best_fitness": 1250.5,
    "average_fitness": 850.3,
    "population": 45,
    "food_stored": 2500
  }
}
```

## Genetic Traits Preserved

When you save and load, the following evolved traits are preserved:

1. **speed** (1.0-4.0): How fast ants move
2. **pheromone_sensitivity** (0.01-0.5): How strongly ants follow pheromone trails
3. **exploration_rate** (0.0-1.0): How much ants explore vs follow trails
4. **pheromone_strength** (10.0-200.0): How intense pheromone marks are
5. **energy_efficiency** (0.01-0.2): How much energy ants use per frame

## Generation Tracking

Each time you save, the generation counter is preserved. When you continue:
- New ants spawn using genes from the saved gene pool
- The generation counter continues increasing
- Breeding uses the stored best genes, ensuring evolution doesn't restart

## Console Messages

You'll see these messages during normal operation:

- `Starting new colony simulation...` - Fresh start, no previous save
- `[LOADED] Generation X, Gene Pool Size: Y` - Loaded a saved colony
- `[SAVED] Colony saved! Generation X, Gene pool: Y` - Saving progress
- `[APPLIED] Applied saved state: generation X, Y genes in pool` - Applying loaded state

## Tips for Optimal Evolution

1. **Run sessions of different lengths** - Short bursts for quick tests, long sessions for major evolution
2. **Save frequently** - Pause and let it auto-save periodically
3. **Monitor fitness** - Watch the "Avg Fitness" and "Best Fitness" metrics in the UI
4. **Check gene pool size** - A larger pool (up to 50) means more genetic diversity

## Resetting

If you want to start completely fresh:
1. Delete the `ant_saves/colony_state.json` file
2. Next run will start with a new colony

## Technical Details

- **Save location**: `ant_saves/` directory (auto-created)
- **Max genes stored**: 50 (the best performing from all time)
- **Save format**: JSON (human-readable and editable)
- **Save timing**: Automatically on pause/reset
- **Load timing**: Automatic on startup

