# Wall Deterrent System - Documentation

## Overview

The ant colony simulator now includes a **wall deterrent system** that adds physical obstacles to the environment. Ants actively avoid walls as they explore, creating more dynamic and challenging foraging behavior.

## How Walls Work

### Visual Representation
Walls are rendered as **purple/dark magenta rectangles** with bright borders. They're visible throughout the simulation area and create natural barriers for ants to navigate around.

### Ant Behavior
Ants detect walls from a distance and avoid them through a **repulsion field**:
- **Detection Range**: 60 pixels around each wall
- **Avoidance Strength**: Ants steer away before hitting walls
- **Blend Calculation**: 40% wall avoidance + 60% current direction maintains natural movement

### Physics
When an ant approaches a wall:
1. Wall calculates repulsion vector based on closest point on wall surface
2. Repulsion strength decreases with distance (0 at 60px, max at 0px)
3. Ant blends this repulsion with its current direction
4. If collision detected, ant is pushed away from wall surface

## Wall Placement

Current wall layout includes 6 strategic obstacles:
1. **Left side obstacle** - Positioned to divide left foraging area
2. **Right side obstacle** - Forces ants to navigate right side differently
3. **Center-left obstacle** - Creates middle passages
4. **Center-right obstacle** - Additional maze element
5. **Bottom obstacle** - Long horizontal wall creating chokepoint
6. **Top-center obstacle** - Blocks direct routes to some areas

These walls create multiple **foraging pathways** rather than one simple route to food.

## Configuration

Edit `config.py` to customize wall behavior:

```python
# Wall settings
WALL_COLOR = (80, 40, 100)             # Wall color (purple)
WALL_REPEL_RANGE = 60                  # Detection distance in pixels
WALL_REPEL_STRENGTH = 0.8              # Avoidance intensity (0-1)
```

### Adjusting Wall Behavior

- **Increase `WALL_REPEL_RANGE`** (e.g., 80): Ants detect walls from farther away
- **Decrease `WALL_REPEL_RANGE`** (e.g., 40): Walls only affect close ants
- **Increase `WALL_REPEL_STRENGTH`** (e.g., 1.0): Stronger avoidance
- **Decrease `WALL_REPEL_STRENGTH`** (e.g., 0.5): Weaker avoidance, ants more likely to hit walls

## Modifying Wall Placement

To create custom walls, edit `walls.py` in the `_create_walls()` method:

```python
def _create_walls(self):
    """Create wall obstacles in the environment"""
    # Add a new wall
    self.walls.append(Wall(
        x_position,                      # Left edge of wall
        y_position,                      # Top edge of wall
        width,                           # Width in pixels
        height                           # Height in pixels
    ))
```

Example - Add a vertical wall in the center:
```python
self.walls.append(Wall(
    self.area_offset_x + self.area_width // 2 - 20,  # Center X, -20 for width
    self.area_offset_y + 100,                        # Top
    40,                                              # Width
    self.area_height - 200                          # Height
))
```

## How Ants Learn With Walls

### Initial Behavior (Generation 0)
- Ants discover walls through trial and error
- Some genetic lines learn to avoid walls better than others
- Ants that find food around obstacles are rewarded

### Evolved Behavior (Generation 10+)
- Ants with better wall avoidance traits breed more
- **Increased pheromone_sensitivity**: Better at following trails around obstacles
- **Optimized exploration_rate**: Better balance of exploration vs. following trails
- **Improved speed**: Faster ants can navigate complex paths more efficiently

### Emergent Patterns
Over multiple sessions, you'll observe:
- **Multiple food routes**: Ants discover different paths around walls
- **Wall hugging**: Some ants learn to use walls as guides
- **Corridor formation**: Pheromone trails concentrate in narrower passages
- **Adaptive behavior**: Different colonies evolve different strategies

## Visual Indicators

When walls are present, you'll notice:
- **Purple walls** with bright borders throughout the map
- **Ants turning early** as they approach walls (wall avoidance working)
- **More dispersed pheromone trails** as ants navigate around obstacles
- **Longer food routes** compared to empty space (less efficient but more realistic)

## Performance Impact

Walls have minimal performance impact:
- Distance calculation: O(n) where n = number of walls
- Typical: 6 walls × 30-500 ants = negligible CPU cost
- No memory overhead (walls are static, created once)

## Disabling Walls

To temporarily disable walls without modifying code:

Create a modified colony.py with this change:
```python
# Comment out wall manager creation
# self.wall_manager = WallManager(width, height, bounds.left if bounds else 0, bounds.top if bounds else 0)
```

Or remove the wall drawing from colony.py:
```python
# In draw() method, comment out:
# self.wall_manager.draw(surface)
```

## Wall Configuration Examples

### Maze-like Layout
Create more walls, smaller spacing:
```python
# Add multiple walls creating corridors
self.walls.append(Wall(...))
self.walls.append(Wall(...))
# etc.
```

### Open Field with Few Obstacles
Remove walls or reduce `_create_walls()` content:
```python
def _create_walls(self):
    """Create wall obstacles in the environment"""
    # Just one large central obstacle
    self.walls.append(Wall(center_x - 50, center_y - 100, 100, 200))
```

### Ring Configuration
Create walls that form a ring:
```python
# North wall
self.walls.append(Wall(x, y_top, width, height_small))
# South wall
self.walls.append(Wall(x, y_bottom, width, height_small))
# East wall
self.walls.append(Wall(x_right, y, width_small, height))
# West wall
self.walls.append(Wall(x_left, y, width_small, height))
```

## Technical Details

### Repulsion Vector Calculation
```
For each wall and each ant:
1. Find closest point on wall to ant
2. Calculate distance from ant to closest point
3. If distance < WALL_REPEL_RANGE:
   - Strength = (1 - distance/WALL_REPEL_RANGE) × WALL_REPEL_STRENGTH
   - Direction = from wall to ant
   - Apply to ant's movement vector
```

### Collision Avoidance
```
Before moving ant:
1. Calculate intended new position
2. Check if it collides with any wall (within ant radius)
3. If collision detected:
   - Push ant away from wall
   - Strength proportional to repulsion
   - Keep ant on valid side of wall
```

## Integration Points

**Walls are integrated into:**
- `colony.py`: Creates WallManager, draws walls, passes to ants
- `ant.py`: Uses wall repulsion in movement calculations
- `config.py`: Configurable wall parameters
- `walls.py`: NEW MODULE with Wall and WallManager classes

## Testing Walls

Quick test script:
```python
from walls import WallManager

# Create wall manager
walls = WallManager(1000, 1000, 0, 0)

# Test repulsion from point near wall
repel_x, repel_y = walls.get_repel_vector(100, 100)
print(f"Repulsion: {repel_x}, {repel_y}")

# Test collision detection
is_colliding = walls.is_colliding(100, 100, radius=10)
print(f"Colliding: {is_colliding}")

# Test avoidance
safe_x, safe_y = walls.get_avoid_position(100, 100, radius=15)
print(f"Safe position: {safe_x}, {safe_y}")
```

## Summary

Walls make the simulation **more challenging and realistic**:
- ✅ Ants must find routes around obstacles
- ✅ Genetic evolution optimizes for wall-aware navigation
- ✅ Multiple food routes emerge naturally
- ✅ Pheromone trails concentrate in narrow passages
- ✅ Long-term evolution creates sophisticated strategies

The wall system is fully integrated and requires **zero configuration** to start using. Just run `python main.py` and watch ants learn to navigate! 

🧱🐜 **Enjoy your walled colony!**
