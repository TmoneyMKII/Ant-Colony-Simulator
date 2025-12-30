"""Wall system for colony environment with procedural maze generation"""

import pygame
import math
import random
from src.config import WALL_COLOR


class Wall:
    """Represents a wall obstacle in the colony"""
    def __init__(self, x, y, width, height):
        self.rect = pygame.Rect(x, y, width, height)
        self.color = WALL_COLOR
        
    def draw(self, surface):
        """Draw the wall"""
        pygame.draw.rect(surface, self.color, self.rect)
        # Add a border
        pygame.draw.rect(surface, (120, 60, 150), self.rect, width=2)
    
    def distance_to(self, x, y):
        """Get distance from point to nearest edge of wall"""
        closest_x = max(self.rect.left, min(x, self.rect.right))
        closest_y = max(self.rect.top, min(y, self.rect.bottom))
        dx = x - closest_x
        dy = y - closest_y
        return math.sqrt(dx*dx + dy*dy)
    
    def get_closest_point(self, x, y):
        """Get the closest point on the wall to a position"""
        closest_x = max(self.rect.left, min(x, self.rect.right))
        closest_y = max(self.rect.top, min(y, self.rect.bottom))
        return closest_x, closest_y
    
    def is_point_inside(self, x, y, margin=0):
        """Check if point is inside wall (with optional margin)"""
        return (self.rect.left - margin <= x <= self.rect.right + margin and
                self.rect.top - margin <= y <= self.rect.bottom + margin)


class MazeGenerator:
    """Procedural maze generator using recursive backtracking"""
    
    def __init__(self, grid_width, grid_height, cell_size=80):
        self.grid_width = grid_width
        self.grid_height = grid_height
        self.cell_size = cell_size
        # Grid of cells: True = wall, False = passage
        self.grid = [[True for _ in range(grid_width)] for _ in range(grid_height)]
        
    def generate(self, start_x=1, start_y=1):
        """Generate maze using recursive backtracking"""
        stack = [(start_x, start_y)]
        self.grid[start_y][start_x] = False  # Carve starting cell
        
        while stack:
            x, y = stack[-1]
            neighbors = self._get_unvisited_neighbors(x, y)
            
            if neighbors:
                # Choose random neighbor
                nx, ny = random.choice(neighbors)
                # Carve passage (remove wall between current and neighbor)
                wall_x = x + (nx - x) // 2
                wall_y = y + (ny - y) // 2
                self.grid[wall_y][wall_x] = False
                self.grid[ny][nx] = False
                stack.append((nx, ny))
            else:
                stack.pop()
    
    def _get_unvisited_neighbors(self, x, y):
        """Get unvisited neighbors 2 cells away"""
        neighbors = []
        directions = [(0, -2), (0, 2), (-2, 0), (2, 0)]
        
        for dx, dy in directions:
            nx, ny = x + dx, y + dy
            if 0 < nx < self.grid_width - 1 and 0 < ny < self.grid_height - 1:
                if self.grid[ny][nx]:  # Still a wall (unvisited)
                    neighbors.append((nx, ny))
        return neighbors
    
    def add_extra_passages(self, count=5):
        """Add extra passages to make maze less claustrophobic"""
        for _ in range(count):
            x = random.randint(2, self.grid_width - 3)
            y = random.randint(2, self.grid_height - 3)
            self.grid[y][x] = False
            # Also clear neighbors sometimes
            if random.random() < 0.5:
                for dx, dy in [(0, 1), (0, -1), (1, 0), (-1, 0)]:
                    if 0 < x + dx < self.grid_width - 1 and 0 < y + dy < self.grid_height - 1:
                        self.grid[y + dy][x + dx] = False
    
    def clear_area(self, center_x, center_y, radius=3):
        """Clear an area around a point (for colony)"""
        for y in range(max(1, center_y - radius), min(self.grid_height - 1, center_y + radius + 1)):
            for x in range(max(1, center_x - radius), min(self.grid_width - 1, center_x + radius + 1)):
                dist = math.sqrt((x - center_x)**2 + (y - center_y)**2)
                if dist <= radius:
                    self.grid[y][x] = False
    
    def to_walls(self, offset_x, offset_y, wall_thickness=20):
        """Convert grid to Wall objects"""
        walls = []
        
        # Merge adjacent wall cells into larger rectangles for efficiency
        visited = [[False for _ in range(self.grid_width)] for _ in range(self.grid_height)]
        
        for y in range(self.grid_height):
            for x in range(self.grid_width):
                if self.grid[y][x] and not visited[y][x]:
                    # Find horizontal extent
                    width = 0
                    while x + width < self.grid_width and self.grid[y][x + width] and not visited[y][x + width]:
                        width += 1
                    
                    # Find vertical extent (for this width)
                    height = 1
                    can_extend = True
                    while can_extend and y + height < self.grid_height:
                        for wx in range(width):
                            if not self.grid[y + height][x + wx] or visited[y + height][x + wx]:
                                can_extend = False
                                break
                        if can_extend:
                            height += 1
                    
                    # Mark as visited
                    for vy in range(height):
                        for vx in range(width):
                            visited[y + vy][x + vx] = True
                    
                    # Create wall
                    wall_x = offset_x + x * self.cell_size
                    wall_y = offset_y + y * self.cell_size
                    wall_w = width * self.cell_size
                    wall_h = height * self.cell_size
                    walls.append(Wall(wall_x, wall_y, wall_w, wall_h))
        
        return walls


class WallManager:
    """Manages walls in the simulation"""
    def __init__(self, area_width, area_height, area_offset_x=0, area_offset_y=0):
        self.walls = []
        self.area_width = area_width
        self.area_height = area_height
        self.area_offset_x = area_offset_x
        self.area_offset_y = area_offset_y
        self._create_walls()
        
    def _create_walls(self):
        """Create procedural maze walls"""
        # Calculate grid dimensions based on area size
        cell_size = 60  # Size of each maze cell in pixels
        grid_width = self.area_width // cell_size
        grid_height = self.area_height // cell_size
        
        # Ensure odd dimensions for proper maze generation
        if grid_width % 2 == 0:
            grid_width -= 1
        if grid_height % 2 == 0:
            grid_height -= 1
        
        # Create maze generator
        maze = MazeGenerator(grid_width, grid_height, cell_size)
        
        # Generate the maze starting from center-ish position
        start_x = grid_width // 2
        start_y = grid_height // 2
        if start_x % 2 == 0:
            start_x += 1
        if start_y % 2 == 0:
            start_y += 1
        maze.generate(start_x, start_y)
        
        # Clear large area around colony (center of screen)
        center_x = grid_width // 2
        center_y = grid_height // 2
        maze.clear_area(center_x, center_y, radius=4)
        
        # Add extra passages to make it less claustrophobic
        maze.add_extra_passages(count=grid_width * grid_height // 15)
        
        # Clear edges so ants can move around perimeter
        for x in range(grid_width):
            maze.grid[0][x] = False
            maze.grid[1][x] = False
            maze.grid[grid_height - 1][x] = False
            maze.grid[grid_height - 2][x] = False
        for y in range(grid_height):
            maze.grid[y][0] = False
            maze.grid[y][1] = False
            maze.grid[y][grid_width - 1] = False
            maze.grid[y][grid_width - 2] = False
        
        # Convert to wall objects
        self.walls = maze.to_walls(self.area_offset_x, self.area_offset_y, cell_size)
    
    def get_nearest_wall_info(self, x, y, look_range=150):
        """
        Get info about the nearest wall within range.
        Returns: (distance, angle_to_wall, wall) or (None, None, None) if no wall nearby
        """
        nearest_dist = float('inf')
        nearest_wall = None
        nearest_point = None
        
        for wall in self.walls:
            dist = wall.distance_to(x, y)
            if dist < nearest_dist and dist < look_range:
                nearest_dist = dist
                nearest_wall = wall
                nearest_point = wall.get_closest_point(x, y)
        
        if nearest_wall is None:
            return None, None, None
        
        # Calculate angle to wall
        angle_to_wall = math.atan2(nearest_point[1] - y, nearest_point[0] - x)
        return nearest_dist, angle_to_wall, nearest_wall
    
    def get_avoidance_direction(self, x, y, current_direction, look_range=100):
        """
        Calculate the best direction to avoid walls.
        Returns: (should_turn, turn_amount) 
        """
        dist, angle_to_wall, wall = self.get_nearest_wall_info(x, y, look_range)
        
        if dist is None:
            return False, 0
        
        # How much is the ant heading toward the wall?
        angle_diff = angle_to_wall - current_direction
        while angle_diff > math.pi:
            angle_diff -= 2 * math.pi
        while angle_diff < -math.pi:
            angle_diff += 2 * math.pi
        
        # If heading away from wall, no need to turn
        if abs(angle_diff) > math.pi * 0.6:  # More than 108 degrees away
            return False, 0
        
        # Urgency based on distance (closer = stronger turn)
        urgency = 1.0 - (dist / look_range)
        urgency = urgency ** 0.5  # Make it more aggressive when close
        
        # Turn perpendicular to wall - away from it
        # Decide which way to turn (left or right)
        if angle_diff > 0:
            turn = -0.5 * urgency  # Turn left (negative)
        else:
            turn = 0.5 * urgency   # Turn right (positive)
        
        return True, turn
    
    def is_path_blocked(self, x, y, target_x, target_y, check_distance=50):
        """Check if a straight path to target is blocked by a wall"""
        # Sample points along the path
        dx = target_x - x
        dy = target_y - y
        dist = math.sqrt(dx*dx + dy*dy)
        
        if dist == 0:
            return False
        
        # Check along path
        steps = min(5, int(dist / 10))
        for i in range(1, steps + 1):
            t = i / steps
            check_x = x + dx * t
            check_y = y + dy * t
            for wall in self.walls:
                if wall.is_point_inside(check_x, check_y, margin=15):
                    return True
        return False
    
    def is_colliding(self, x, y, radius=10):
        """Check if a point collides with any wall"""
        for wall in self.walls:
            if wall.is_point_inside(x, y, margin=radius):
                return True, wall
        return False, None
    
    def push_out_of_wall(self, x, y, radius=10):
        """If inside a wall, push out to nearest edge"""
        for wall in self.walls:
            if wall.is_point_inside(x, y, margin=0):
                # Find which edge is closest and push out
                dist_left = x - wall.rect.left
                dist_right = wall.rect.right - x
                dist_top = y - wall.rect.top
                dist_bottom = wall.rect.bottom - y
                
                min_dist = min(dist_left, dist_right, dist_top, dist_bottom)
                
                if min_dist == dist_left:
                    return wall.rect.left - radius - 1, y
                elif min_dist == dist_right:
                    return wall.rect.right + radius + 1, y
                elif min_dist == dist_top:
                    return x, wall.rect.top - radius - 1
                else:
                    return x, wall.rect.bottom + radius + 1
        
        return x, y
        
    def draw(self, surface):
        """Draw all walls"""
        for wall in self.walls:
            wall.draw(surface)
