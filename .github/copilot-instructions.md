# Ant Colony Simulator - AI Agent Instructions

## Architecture Overview

This is a **Pygame-based ant colony simulation** with emergent swarm intelligence through pheromones and genetic evolution. Key data flows:

```
main.py (game loop) → Colony → Ants ↔ PheromoneMap
                         ↓         ↘
                    FoodSources    AntGenes/FitnessTracker
```

**Core modules:**
- [main.py](../main.py) - Event loop, rendering, keyboard shortcuts (SPACE/P/R/ESC)
- [colony.py](../colony.py) - Colony management, spawning, evolution, food sources
- [ant.py](../ant.py) - Ant behavior state machine (FORAGING→RETURNING→IDLE)
- [pheromone.py](../pheromone.py) - Grid-based chemical communication (20px cells)
- [genetics.py](../genetics.py) - 5 evolvable traits + fitness calculation
- [save_state.py](../save_state.py) - JSON persistence to `ant_saves/`
- [walls.py](../walls.py) - Obstacle avoidance system

## Key Patterns

### Genetic Traits (genetics.py)
All ant behavior is controlled by these 5 genes with specific ranges:
| Trait | Range | Purpose |
|-------|-------|---------|
| `speed` | 1.0-4.0 | Movement velocity |
| `pheromone_sensitivity` | 0.01-0.5 | Trail-following tendency |
| `exploration_rate` | 0.05-0.4 | Random vs directed behavior |
| `pheromone_strength` | 0.5-3.0 | Trail intensity when depositing |
| `energy_efficiency` | 0.005-0.02 | Energy drain per frame |

When modifying genes, always respect `_clamp_genes()` bounds and maintain the crossover/mutation pattern.

### Runtime Configuration (config.py)
Use `RuntimeParams` class for real-time adjustable parameters:
```python
from config import runtime
runtime.wall_repel_range = 120.0  # Modify at runtime
```

Static colors/constants are module-level in `config.py`.

### Save/Load System
- **Auto-saves**: On pause (SPACE) and reset (R)
- **Auto-loads**: On startup via `load_colony_state()` in `Colony.__init__`
- **Location**: `ant_saves/colony_state.json`
- **Serialization**: `genes_to_dict()`/`dict_to_genes()` for AntGenes objects

### Pheromone System
Two separate layers: `foraging_pheromones` and `returning_pheromones`
- Evaporation rate: 0.999 (slow fade ~5 minutes)
- Detection threshold: strength > 15
- Max value: 200.0

### Fitness Calculation
Located in `FitnessTracker.calculate_fitness()`. Key scoring:
- `food_collected * 15` (primary reward)
- `successful_trips * 20` (trip bonus)
- Penalties for `wall_hits` and `time_stuck`

## Developer Workflow

```bash
# Run simulation
python main.py

# Test save/load system
python test_save.py

# Test evolution mechanics
python test_evolution.py

# Verify all systems
python verify_system.py
```

## Common Modification Points

| Task | Location |
|------|----------|
| Add new genetic trait | `AntGenes.__init__`, `_mutate()`, `_clamp_genes()`, `genes_to_dict()` |
| Change ant behavior | `Ant._forage()`, `Ant._return_to_colony()` in ant.py |
| Modify UI stats | `UIManager.draw()` in ui.py |
| Adjust wall layout | `WallManager._create_walls()` in walls.py |
| Change food respawn | `Colony._create_food_sources()`, `Colony._check_food_respawn()` |

## Conventions

- **State machine pattern**: Ant behavior uses `AntState` enum (FORAGING/RETURNING/IDLE)
- **Bounds checking**: Always use `self.bounds` rect for constraining positions
- **Gene pool**: Top 50 genes preserved in `colony.gene_pool`, used for breeding
- **Coordinate system**: Pygame (0,0 at top-left), positions stored as floats
