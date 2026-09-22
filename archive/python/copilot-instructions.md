# Ant Colony Simulator - AI Agent Instructions

## Architecture Overview

**Pygame-based ant colony simulation** with emergent swarm intelligence through pheromones and neural network path optimization.

```
main.py (game loop) → Colony → Ants ↔ PheromoneModel
                         ↓         ↘
                    FoodSources   ColonyBrain → AntBrain (Neural Network)
```

## Project Structure

```
├── main.py              # Entry point, game loop, keyboard controls
├── src/
│   ├── ant.py           # Ant behavior state machine (FORAGING→RETURNING→IDLE)
│   ├── colony.py        # Colony management, spawning
│   ├── colony_brain.py  # Collective neural network evolution
│   ├── config.py        # Colors, constants, RuntimeParams
│   ├── debug.py         # Debug visualization system
│   ├── neural_net.py    # Neural network for ant decision-making
│   ├── pheromone_model.py # Dual pheromone system (food/home trails)
│   ├── save_state.py    # JSON persistence to ant_saves/
│   ├── stats_ui.py      # Neural visualization + colony stats overlay
│   ├── vision.py        # Ray-based vision system for ants
│   └── walls.py         # Obstacle system
├── ant_saves/           # Save files (auto-created)
└── requirements.txt
```

## Key Patterns

### Neural Network System (src/neural_net.py, src/colony_brain.py, src/vision.py)
Each ant has an `AntBrain` with a feedforward neural network and ray-based vision:

**Vision System (7 rays × 3 object types = 21 inputs):**
- Rays cast at: -90°, -60°, -30°, 0°, +30°, +60°, +90° from heading
- Each ray detects: Wall distance, Ant distance, Food distance
- Range: 100 pixels, normalized 0-1 (1 = object very close)

**Neural Network Architecture:**
- **Inputs (27)**: 21 vision inputs + 6 state inputs
  - Vision: Wall×7, Ant×7, Food×7 (ray distances)
  - State: Food pheromone, Home pheromone, Colony dist/dir, Carrying, Energy
- **Hidden Layer (16)**: Tanh activation
- **Outputs (3)**: Turn angle, Speed modifier, Exploration tendency

Evolution happens through `ColonyBrain`:
- Tracks elite performers (top 10 ants)
- New ants inherit from crossover + mutation of elites
- Generation evolves every 30 seconds

### Ant Attributes (src/ant.py)
Ants have fixed attribute values:
| Attribute | Value | Purpose |
|-----------|-------|---------|
| `speed` | 2.0 | Movement velocity |
| `pheromone_sensitivity` | 0.15 | Trail-following tendency |
| `pheromone_strength` | 1.5 | Trail intensity when depositing |
| `energy_efficiency` | 0.01 | Energy drain per frame |
| `brain` | AntBrain | Neural network for decisions |

### Runtime Configuration (src/config.py)
Key constants for performance (using squared distances):
```python
ANT_SMELL_RANGE_SQ = 22500      # 150^2 pixels
STUCK_CHECK_INTERVAL = 180      # 3 seconds at 60 FPS
MAX_DEATH_MARKERS = 500
```

### Save/Load System
- **Auto-saves**: On pause (SPACE) and reset (R)
- **Auto-loads**: On startup via `load_colony_state()` in `Colony.__init__`
- **Location**: `ant_saves/colony_state.json`

### Pheromone System (src/pheromone_model.py)
Three trail types:
- `FOOD_TRAIL` (Green): Deposited by returning ants, leads TO food
- `HOME_TRAIL` (Blue): Deposited by foraging ants, leads TO home
- `DANGER` (Red): Deposited where ants die, deters others

## Controls

| Key | Action |
|-----|--------|
| SPACE | Pause/Resume (auto-saves) |
| P | Toggle pheromone visualization |
| N | Toggle neural network UI |
| R | Reset colony (auto-saves) |
| M | Generate new maze |
| G | Toggle grid |
| D | Toggle debug mode |
| ,/. | Speed down/up |
| F+Click | Add food |
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
| Neural network architecture | `NeuralNetwork` class in neural_net.py |
| Vision system | `AntVision` class in vision.py |
| Evolution parameters | `ColonyBrain.create_brain()` in colony_brain.py |
| Stats visualization | `StatsUI` class in stats_ui.py |
| Modify pheromones | `PheromoneModel` in pheromone_model.py |

## Conventions

- **State machine pattern**: Ant behavior uses `AntState` enum (FORAGING/RETURNING/IDLE)
- **Squared distances**: Use `_SQ` suffix constants for performance (avoid sqrt)
- **Bounds checking**: Always use `self.bounds` rect for constraining positions
- **Coordinate system**: Pygame (0,0 at top-left), positions stored as floats
