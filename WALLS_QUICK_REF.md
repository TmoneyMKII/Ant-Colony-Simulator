# Wall System - Quick Reference

## What Changed?
✅ Added **purple obstacles** (walls) that ants actively avoid

## How to Use
Just run normally - **walls are automatic**:
```bash
python main.py
```

## What You'll See
- Purple walls throughout the map
- Ants steering around them
- Multiple food routes
- More intelligent behavior

## Adjust Walls

Edit `config.py`:

**Make walls stronger deterrent:**
```python
WALL_REPEL_STRENGTH = 1.0    # Was 0.8
WALL_REPEL_RANGE = 100        # Was 60
```

**Make walls weaker:**
```python
WALL_REPEL_STRENGTH = 0.3     # Was 0.8
WALL_REPEL_RANGE = 30         # Was 60
```

**Change wall color:**
```python
WALL_COLOR = (255, 0, 0)      # Red instead of purple
```

## Add/Remove Walls

Edit `walls.py` in `_create_walls()`:

```python
# Add a new wall
self.walls.append(Wall(x, y, width, height))

# Remove a wall by deleting its line
```

## Evolution Effect

Ants will evolve to:
- Navigate walls better
- Find optimal routes
- Develop more speed
- Use pheromones effectively

Over multiple sessions, your colony becomes **uniquely adapted** to its wall layout!

## Files Changed
- `walls.py` (NEW)
- `config.py` (added wall config)
- `ant.py` (wall avoidance logic)
- `colony.py` (wall rendering)

## Performance
✅ Still 60 FPS
✅ Minimal memory overhead
✅ No noticeable slowdown

## Questions?
See:
- `WALLS_GUIDE.md` - Detailed guide
- `WALLS_IMPLEMENTATION.md` - Technical details
- `WALLS_SUMMARY.md` - Complete overview

---

**That's it!** Walls are live and automatic. Just run and watch! 🧱🐜
