# Ant Colony Simulator - AI Agent Instructions

## Architecture Overview

**Pygame-based ant colony simulation** with emergent swarm intelligence through pheromones.

```
main.py (game loop) → Colony → Ants ↔ PheromoneModel
                         ↓
                    FoodSources
```

## Project Structure

```
├── main.py              # Entry point, game loop, keyboard controls
├── src/
│   ├── ant.py           # Ant behavior state machine (FORAGING→RETURNING→IDLE)
│   ├── colony.py        # Colony management, spawning
│   ├── config.py        # Colors, constants, RuntimeParams
│   ├── debug.py         # Debug visualization system
│   ├── pheromone_model.py # Dual pheromone system (food/home trails)
│   ├── save_state.py    # JSON persistence to ant_saves/
│   └── walls.py         # Obstacle system
├── ant_saves/           # Save files (auto-created)
└── requirements.txt
```

## Key Patterns

### Ant Attributes (src/ant.py)
Ants have fixed attribute values:
| Attribute | Value | Purpose |
|-----------|-------|---------|
| `speed` | 2.0 | Movement velocity |
| `pheromone_sensitivity` | 0.15 | Trail-following tendency |
| `pheromone_strength` | 1.5 | Trail intensity when depositing |
| `energy_efficiency` | 0.01 | Energy drain per frame |

### Runtime Configuration (src/config.py)
Use `RuntimeParams` class for real-time adjustable parameters:
```python
from src.config import runtime
runtime.wall_repel_range = 120.0  # Modify at runtime
```

Static colors/constants are module-level in `config.py`.

### Save/Load System
- **Auto-saves**: On pause (SPACE) and reset (R)
- **Auto-loads**: On startup via `load_colony_state()` in `Colony.__init__`
- **Location**: `ant_saves/colony_state.json`

### Pheromone System (src/pheromone_model.py)
Two separate trail types:
- `FOOD_TRAIL` (Green): Deposited by returning ants, leads TO food
- `HOME_TRAIL` (Blue): Deposited by foraging ants, leads TO home

Key settings:
- Evaporation rate: 0.995
- Detection threshold: 10.0
- Max value: 200.0

## Controls

| Key | Action |
|-----|--------|
| SPACE | Pause/Resume (auto-saves) |
| P | Toggle pheromone visualization |
| R | Reset colony (auto-saves) |
| G | Toggle grid |
| D | Toggle debug mode |
| 1-6 | Debug levels (OFF/STATS/ANT/PHEROMONE/PATH/FULL) |
| H | Show keybind hints |
| ESC | Quit |

## Running

```bash
python main.py
```

## Common Modification Points

| Task | Location |
|------|----------|
| Change ant behavior | `Ant._forage()`, `Ant._return_to_colony()` in ant.py |
| Adjust wall layout | `WallManager._create_walls()` in walls.py |
| Change food respawn | `Colony._create_food_sources()` in colony.py |
| Modify pheromones | `PheromoneModel` in pheromone_model.py |

## Conventions

- **State machine pattern**: Ant behavior uses `AntState` enum (FORAGING/RETURNING/IDLE)
- **Bounds checking**: Always use `self.bounds` rect for constraining positions
- **Coordinate system**: Pygame (0,0 at top-left), positions stored as floats
