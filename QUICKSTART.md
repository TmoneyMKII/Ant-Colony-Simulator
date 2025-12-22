# Quick Start Guide - Ant Colony Simulator

## Installation (First Time Only)

```bash
# Install required packages
pip install pygame>=2.1.0

# Or use requirements.txt
pip install -r requirements.txt
```

## Running the Simulation

```bash
python main.py
```

The window will open in fullscreen with a dark theme and cyan controls.

## Basic Controls

| Key | Action |
|-----|--------|
| SPACE | Play / Pause |
| P | Toggle pheromone trails visibility |
| R | Reset to new colony (saves current state) |
| ESC | Quit application |

Or click the buttons on the left sidebar:
- **Start** - Resume simulation
- **Pause** - Pause simulation (auto-saves)
- **Reset** - Start new colony (saves old one first)

## What to Watch For

### First Run (0-1 minute)
- Ants spawn in the center
- They start exploring randomly
- Food sources appear (colored circles)
- Pheromone trails show their movement (green-yellow when P is pressed)

### Early Evolution (1-5 minutes)
- Ants discover food sources
- Trails form to food
- Successful ants breed new ants
- Generation counter increases
- Gene pool grows with successful traits

### Mature Colony (5+ minutes)
- Clear pheromone highways visible
- Population stabilizes or grows
- Fitness metrics improve
- Food consumption patterns visible
- Different behavioral patterns emerge

## Save System (Automatic!)

### Your Progress is Automatically Saved When:
- ✓ You pause (SPACE key)
- ✓ You reset (R key)
- ✓ You quit (ESC key)

### Loading is Automatic:
- ✓ Start the app → loads last saved colony
- ✓ Keep same generation counter
- ✓ Keep gene pool (evolved genes)
- ✓ Ants spawn with evolved traits

### If You Want a Fresh Start:
```bash
# Delete the save file
rm ant_saves/colony_state.json

# Or just leave it - reset (R) will save current and start fresh with evolved genes
```

## Understanding the Display

### Left Sidebar (Controls & Stats)
```
GENERATION: 42        <- Which breeding cycle
GENE POOL: 15         <- Number of stored successful genes
AVG FITNESS: 850.5    <- Average ant performance
BEST: 1250.3          <- Best performing ant
```

### Main Area (Simulation)
- **Grid**: Black/dark background
- **Brown circles**: Colony (starting position)
- **Colored circles**: Food sources (brighter = more food)
- **Pink/magenta dots**: Individual ants
- **Green-yellow dots**: Pheromone trails (when P is pressed)

### Status Bar
- Shows if simulation is "Running" or "Paused"

## Tips for Getting Good Results

### To See Evolution:
1. Let it run for 2-3 minutes
2. Press P to watch pheromone trails form
3. You'll see highways to food emerge
4. Reset (R) keeps the genes, starts fresh - they'll be faster!

### To Get a Huge Population:
1. Let it run for 10+ minutes
2. Pause occasionally (auto-saves)
3. Watch population bar grow
4. Reset to let evolved ants re-colonize

### To Study Specific Behaviors:
1. Pause (SPACE) to freeze action
2. Press P to see pheromone trails
3. Look for patterns in movement
4. Resume to watch behavior change

## Common Questions

**Q: Why do ants sometimes bunch at walls?**
A: They're exploring edges. This gets better as they evolve. Try a reset to see improved wall avoidance!

**Q: How do I speed up the simulation?**
A: The simulation runs at 60 FPS fixed. Closing other apps helps.

**Q: Do my ants get faster each run?**
A: Yes! With each session, ants evolve to be faster and smarter. The gene pool accumulates better traits.

**Q: Can I watch them for hours?**
A: Absolutely! The simulation will keep running. Pause to save periodically.

**Q: What happens if I reset?**
A: The old colony is saved, new colony starts with the evolved genes. You can continue from where you were.

**Q: Can I delete the save file?**
A: Yes. Delete `ant_saves/colony_state.json` to start completely fresh next time. Or just keep running - each session builds on the last!

## What's Actually Happening

The ants use **pheromones** (chemical signals) to communicate. When an ant finds food:
1. It marks a trail back to the colony
2. Other ants smell the trail
3. More ants follow, reinforcing it
4. Over generations, good trails become highways

The **evolution** happens because:
1. Ants that find more food reproduce
2. Their genes get stored
3. New ants inherit those genes
4. Traits like speed and trail-following improve
5. Next session builds on learned traits

## Enjoy Your Ant Colony!

The goal is to watch emergent intelligence develop. Each run teaches the colony a bit more about efficient foraging. By session 5-10, you should see clearly intelligent behavior emerge!

