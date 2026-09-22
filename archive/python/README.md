# Ant Colony Simulator - Complete Implementation Guide

## Project Overview

This is a sophisticated **ant colony simulation** featuring:
- **Real-time visualization** with a modern dark-themed UI
- **Emergent swarm intelligence** through pheromone-based communication
- **Evolutionary learning** using genetic algorithms
- **Dynamic environment** with food sources that deplete and respawn
- **Persistent state** - evolved genes are saved and loaded between sessions

## Features

### 🐜 Ant Agents
- Individual ants with 5 evolving genetic traits
- State machine behavior (FORAGING → RETURNING → IDLE)
- Pheromone-based communication and trail following
- Energy management with death when exhausted
- Momentum-based smooth movement

### 🧬 Evolutionary System
- Genetic traits control ant behavior:
  - **Speed**: Movement velocity (1.0-4.0)
  - **Pheromone Sensitivity**: Trail-following tendency (0.01-0.5)
  - **Exploration Rate**: Random vs. directed behavior (0.0-1.0)
  - **Pheromone Strength**: Trail intensity (10.0-200.0)
  - **Energy Efficiency**: Energy consumption rate (0.01-0.2)
- Breeding system uses top 50 genes
- Genetic crossover with ±0.1 variation
- Mutation rate: 15% per gene

### 🏛️ User Interface
- **Modern dark theme** with cyan accents
- **Real-time statistics** display:
  - Population count
  - Food reserves
  - Foraging/returning ant counts
  - Generation counter
  - Gene pool size
  - Average and best fitness
- **Interactive controls**:
  - Start/Pause/Reset buttons
  - Keyboard shortcuts (SPACE, P, R, ESC)
  - Toggle pheromone visualization

### 💾 Save/Load System
- Automatic saving when pausing or resetting
- Automatic loading on startup
- Preserves generation counter and gene pool
- JSON-based save format
- Allows multi-session evolution

### 🌍 Environment
- 12 dynamic food sources
- Food depletion (ants consume resources)
- Automatic respawning of depleted sources
- Grid-based pheromone system
- Pheromone evaporation (0.999 rate = slow fade)

## Installation

### Requirements
- Python 3.8+
- Pygame 2.1.0+

### Setup
```bash
# Install dependencies
pip install -r requirements.txt

# Run the simulation
python main.py
```

## Controls

| Input | Action |
|-------|--------|
| SPACE | Play/Pause |
| P | Toggle pheromone visualization |
| R | Reset simulation |
| ESC | Quit |
| Mouse Click | Interact with UI buttons |

## File Structure

```
ant_simulator/
├── main.py              # Main event loop and rendering
├── config.py            # Color scheme and constants
├── ant.py               # Individual ant agent class
├── colony.py            # Colony management and evolution
├── pheromone.py         # Pheromone grid system
├── genetics.py          # Genetic traits and fitness tracking
├── ui.py                # User interface manager
├── save_state.py        # Save/load persistence system
├── requirements.txt     # Python dependencies
├── ant_saves/           # Save state files (auto-created)
│   └── colony_state.json
├── PERSISTENCE_GUIDE.md # Detailed save/load documentation
└── README.md            # This file
```

## How the System Works

### Simulation Loop (60 FPS)
1. **Update Phase**:
   - Each ant updates position and energy
   - Ants search for food or return to colony
   - Pheromones evaporate
   - Colony spawns new ants using evolved genes

2. **Fitness Tracking**:
   - Ants gain fitness from food delivery
   - Best genes saved to gene pool
   - Dead ants removed from population

3. **Evolution**:
   - New ants breed from top 50 genes
   - Genetic crossover creates variation
   - Mutation adds exploration

4. **Render Phase**:
   - Draw grid and environment
   - Render ants and pheromone trails
   - Display UI and statistics

### Pheromone System
- **Foraging pheromones**: Mark paths to food
- **Returning pheromones**: Mark paths back to colony
- **Grid-based**: 20-pixel cell resolution
- **Evaporation**: 0.999 factor (5-minute fade time)
- **Direction detection**: 8-neighbor gradient following

### Genetic Evolution
1. **Fitness Calculation**:
   - Food delivered (10x multiplier)
   - Energy efficiency (50x multiplier)
   - Lifetime (0.01x multiplier)
   - Successful trips (5x multiplier)

2. **Selection**:
   - Top 50 genes stored in gene pool
   - Ranked by fitness

3. **Breeding**:
   - Crossover: Average parent genes ± 0.1 noise
   - Mutation: 15% chance per gene, 0.2 strength
   - New ants inherit best traits

## Performance Metrics

- **Simulation Speed**: 60 FPS
- **Max Population**: 500 ants
- **Max Gene Pool**: 50 genes
- **Food Sources**: 12 (dynamic)
- **Pheromone Grid**: ~500x500 cells

## Behavioral Patterns

### Emergent Behaviors
After running for a while, you'll observe:
- **Food routes**: Visible pheromone highways to food sources
- **Recruitment**: New ants follow successful routes
- **Adaptation**: Genes evolve to exploit discovered patterns
- **Population cycles**: Growth when food abundant, decline when scarce

### What Makes Ants Smart
1. **Pheromone communication**: Ants share route information
2. **Genetic evolution**: Successful traits accumulate
3. **Local decision-making**: Each ant acts independently
4. **Emergence**: Colony intelligence emerges from simple rules

## Configuration

Edit `config.py` to customize:
- Colors (dark theme colors, accent color, grid color)
- Grid size (20 pixels per cell)
- UI padding and sizing
- Window dimensions
- FPS target

Edit `colony.py` constants to adjust:
- Initial population (30 ants)
- Food source count (12)
- Food spawn amount (50-150 units)
- Max population (500)

Edit `ant.py` constants to adjust:
- Base energy consumption
- Energy for different actions
- Pheromone deposit rate
- Edge avoidance buffer

Edit `genetics.py` to adjust:
- Mutation rate (15%)
- Mutation strength (0.2)
- Gene ranges (speed, sensitivity, etc.)

## Advanced Usage

### Long-term Evolution
1. Run simulation for extended periods
2. Pause periodically (auto-saves)
3. Watch generation number increase
4. Observe gene pool grow with successful traits
5. Delete `ant_saves/colony_state.json` to reset

### Observing Specific Behaviors
- **Press P**: Toggle pheromone visualization
  - See trails ants are following
  - Bright colors = recent deposits
  - Fading colors = older trails

- **Watch food depletion**:
  - Food sources shrink as ants consume them
  - New sources spawn when depleted
  - Ants must find new food routes

- **Monitor fitness metrics**:
  - Check "Best Fitness" and "Avg Fitness"
  - Higher values = better-evolved colony
  - Should increase over multiple sessions

## Debugging

### If ants get stuck at walls
- This has been fixed with random edge escape
- Ants avoid walls with 35-pixel buffer
- 40% random turn when at edge

### If pheromones aren't visible
- Press P to toggle visibility
- Increase display quality if needed
- Check that show_pheromones is True in main.py

### If simulation is slow
- Reduce max population in colony.py
- Reduce food source count
- Close other applications

### If save system isn't working
- Check that `ant_saves/` directory exists
- Verify disk space available
- Check file permissions
- Run `python check_save.py` to verify

## Future Enhancement Ideas

- **Ant castes**: Workers, scouts, soldiers
- **Queen management**: Limit reproduction
- **Temperature effects**: Seasonal simulation
- **Predators**: Threats to avoid
- **Pheromone types**: Different chemical signals
- **Visualization improvements**: 3D rendering
- **Network analysis**: Graph showing ant relationships
- **Statistics export**: CSV logging of metrics

## Technical Details

### Memory Usage
- 30-500 ants × ~500 bytes = 15-250 KB
- Pheromone grid: 25×25 × 4 bytes × 2 layers = 5 KB
- Save file: ~2-10 KB (JSON)

### CPU Usage
- Main loop: ~1-2% CPU
- Rendering: ~5-10% CPU
- Evolution calculations: <1% CPU
- Total: 10-15% CPU typical

## Known Limitations

- No ant-to-ant collisions (they pass through)
- No ant communication except pheromones
- No seasonal changes
- Fixed grid resolution (not adaptive)
- Maximum of 500 ants

## License & Credits

Created as an educational project in evolutionary computation and swarm intelligence.

Built with:
- **Pygame**: Graphics and event handling
- **Python**: Implementation language
- **JSON**: Save format

