"""
Debug System - Comprehensive debugging and visualization for ant colony simulation

Toggle with [D] key. Cycle debug modes with [1-5] keys.
"""

import pygame
import math
import time
from collections import deque
from enum import Enum, auto


class DebugMode(Enum):
    """Available debug visualization modes"""
    OFF = auto()
    STATS = auto()          # Basic statistics overlay
    ANT_DETAILS = auto()    # Individual ant info on hover
    PHEROMONE_DEBUG = auto() # Pheromone grid values
    PATHFINDING = auto()    # Ant direction vectors
    FULL = auto()           # Everything


class DebugLog:
    """Circular buffer for debug messages"""
    
    def __init__(self, max_entries=50):
        self.entries = deque(maxlen=max_entries)
        self.start_time = time.time()
    
    def log(self, category, message):
        """Add a timestamped log entry"""
        elapsed = time.time() - self.start_time
        self.entries.append({
            'time': elapsed,
            'category': category,
            'message': message
        })
    
    def get_recent(self, count=10, category=None):
        """Get recent log entries, optionally filtered by category"""
        entries = list(self.entries)
        if category:
            entries = [e for e in entries if e['category'] == category]
        return entries[-count:]
    
    def clear(self):
        """Clear all entries"""
        self.entries.clear()


class DebugSystem:
    """
    Main debug system for the ant colony simulation.
    
    Usage:
        debug = DebugSystem(screen_width, screen_height)
        debug.set_mode(DebugMode.STATS)
        
        # In update loop:
        debug.track_ant(ant)
        debug.track_pheromone_deposit(x, y, amount, ptype)
        
        # In draw loop:
        debug.draw(surface, colony, mouse_pos)
    """
    
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.mode = DebugMode.OFF
        self.log = DebugLog()
        
        # Fonts
        pygame.font.init()
        self.font_small = pygame.font.Font(None, 18)
        self.font_medium = pygame.font.Font(None, 24)
        self.font_large = pygame.font.Font(None, 32)
        
        # Colors
        self.colors = {
            'bg': (0, 0, 0, 180),
            'text': (255, 255, 255),
            'text_dim': (180, 180, 180),
            'highlight': (255, 255, 0),
            'good': (100, 255, 100),
            'bad': (255, 100, 100),
            'food_trail': (0, 255, 100),
            'home_trail': (100, 150, 255),
            'foraging': (100, 200, 255),
            'returning': (100, 255, 150),
        }
        
        # Tracking data
        self.frame_times = deque(maxlen=60)
        self.last_frame_time = time.time()
        self.selected_ant = None
        self.hover_ant = None
        
        # Pheromone tracking
        self.pheromone_deposits = deque(maxlen=100)  # Recent deposits
        self.total_food_deposits = 0
        self.total_home_deposits = 0
        
        # Ant state tracking
        self.state_counts = {'foraging': 0, 'returning': 0, 'idle': 0}
        self.food_collected_total = 0
        self.trips_completed = 0
        
        # Performance tracking
        self.update_times = deque(maxlen=60)
        self.draw_times = deque(maxlen=60)
    
    def set_mode(self, mode):
        """Set debug visualization mode"""
        self.mode = mode
        self.log.log('DEBUG', f'Mode changed to {mode.name}')
    
    def cycle_mode(self):
        """Cycle to next debug mode"""
        modes = list(DebugMode)
        current_idx = modes.index(self.mode)
        next_idx = (current_idx + 1) % len(modes)
        self.set_mode(modes[next_idx])
        return self.mode
    
    def toggle(self):
        """Toggle debug on/off"""
        if self.mode == DebugMode.OFF:
            self.mode = DebugMode.STATS
        else:
            self.mode = DebugMode.OFF
        return self.mode
    
    # ==================== TRACKING ====================
    
    def track_frame(self):
        """Call at start of each frame to track FPS"""
        now = time.time()
        self.frame_times.append(now - self.last_frame_time)
        self.last_frame_time = now
    
    def track_pheromone_deposit(self, x, y, amount, ptype_name):
        """Track a pheromone deposit event"""
        self.pheromone_deposits.append({
            'x': x, 'y': y, 'amount': amount, 
            'type': ptype_name, 'time': time.time()
        })
        if ptype_name == 'food':
            self.total_food_deposits += 1
        else:
            self.total_home_deposits += 1
    
    def track_food_collected(self, amount):
        """Track food collection"""
        self.food_collected_total += amount
        self.trips_completed += 1
        self.log.log('FOOD', f'Collected {amount:.1f} food (total trips: {self.trips_completed})')
    
    def update_ant_states(self, ants):
        """Update ant state counts"""
        self.state_counts = {'foraging': 0, 'returning': 0, 'idle': 0}
        for ant in ants:
            state_name = ant.state.name.lower()
            if state_name in self.state_counts:
                self.state_counts[state_name] += 1
    
    def find_ant_at(self, x, y, ants, radius=20):
        """Find ant near a position (for mouse hover)"""
        closest = None
        closest_dist = radius
        for ant in ants:
            dist = math.sqrt((ant.x - x)**2 + (ant.y - y)**2)
            if dist < closest_dist:
                closest = ant
                closest_dist = dist
        return closest
    
    # ==================== DRAWING ====================
    
    def draw(self, surface, colony, mouse_pos=None):
        """Main draw method - renders debug overlays based on current mode"""
        if self.mode == DebugMode.OFF:
            return
        
        self.track_frame()
        self.update_ant_states(colony.ants)
        
        # Find hover ant
        if mouse_pos and self.mode in [DebugMode.ANT_DETAILS, DebugMode.FULL]:
            self.hover_ant = self.find_ant_at(mouse_pos[0], mouse_pos[1], colony.ants)
        
        # Draw based on mode
        if self.mode in [DebugMode.STATS, DebugMode.FULL]:
            self._draw_stats_panel(surface, colony)
        
        if self.mode in [DebugMode.ANT_DETAILS, DebugMode.FULL]:
            self._draw_ant_details(surface, colony.ants)
        
        if self.mode in [DebugMode.PHEROMONE_DEBUG, DebugMode.FULL]:
            self._draw_pheromone_debug(surface, colony.pheromone_map)
        
        if self.mode in [DebugMode.PATHFINDING, DebugMode.FULL]:
            self._draw_pathfinding(surface, colony.ants)
        
        # Always show mode indicator
        self._draw_mode_indicator(surface)
    
    def _draw_stats_panel(self, surface, colony):
        """Draw statistics panel in top-left"""
        # Calculate FPS
        if self.frame_times:
            avg_frame = sum(self.frame_times) / len(self.frame_times)
            fps = 1.0 / avg_frame if avg_frame > 0 else 0
        else:
            fps = 0
        
        # Build stats
        stats = colony.get_stats()
        lines = [
            f"FPS: {fps:.1f}",
            f"",
            f"=== COLONY ===",
            f"Population: {stats['population']}",
            f"Food Stored: {stats['food_stored']:.0f}",
            f"",
            f"=== ANT STATES ===",
            f"Foraging: {self.state_counts['foraging']} ({100*self.state_counts['foraging']/max(1,stats['population']):.0f}%)",
            f"Returning: {self.state_counts['returning']} ({100*self.state_counts['returning']/max(1,stats['population']):.0f}%)",
            f"Idle: {self.state_counts['idle']}",
            f"",
            f"=== PHEROMONES ===",
            f"Food deposits: {self.total_food_deposits}",
            f"Home deposits: {self.total_home_deposits}",
            f"",
            f"=== FOOD ===",
            f"Sources: {stats['food_sources_active']}",
            f"Total collected: {stats.get('total_food_collected', 0):.0f}",
            f"Total trips: {stats.get('total_trips', 0)}",
        ]
        
        # Draw background
        panel_width = 220
        panel_height = len(lines) * 18 + 20
        panel_surface = pygame.Surface((panel_width, panel_height), pygame.SRCALPHA)
        panel_surface.fill(self.colors['bg'])
        
        # Draw text
        y = 10
        for line in lines:
            if line.startswith("==="):
                color = self.colors['highlight']
            elif "Foraging" in line:
                color = self.colors['foraging']
            elif "Returning" in line:
                color = self.colors['returning']
            else:
                color = self.colors['text']
            
            text = self.font_small.render(line, True, color)
            panel_surface.blit(text, (10, y))
            y += 18
        
        surface.blit(panel_surface, (10, 10))
    
    def _draw_ant_details(self, surface, ants):
        """Draw details for hovered ant"""
        if not self.hover_ant:
            return
        
        ant = self.hover_ant
        
        # Build info lines
        lines = [
            f"ANT #{id(ant) % 10000}",
            f"State: {ant.state.name}",
            f"Position: ({ant.x:.0f}, {ant.y:.0f})",
            f"Direction: {math.degrees(ant.direction):.0f}°",
            f"Speed: {ant.speed:.2f}",
            f"Energy: {ant.energy:.0f}/{ant.max_energy:.0f}",
            f"Carrying: {ant.food_amount:.1f}" if ant.carrying_food else "Not carrying",
            f"",
            f"=== STATS ===",
            f"Food collected: {ant.food_collected:.1f}",
            f"Trips: {ant.successful_trips}",
        ]
        
        # Position near ant
        panel_x = min(ant.x + 30, self.width - 200)
        panel_y = min(ant.y - 50, self.height - len(lines) * 16 - 20)
        panel_y = max(10, panel_y)
        
        # Draw background
        panel_width = 180
        panel_height = len(lines) * 16 + 20
        panel_surface = pygame.Surface((panel_width, panel_height), pygame.SRCALPHA)
        panel_surface.fill(self.colors['bg'])
        
        # Draw text
        y = 10
        for line in lines:
            if line.startswith("===") or line.startswith("ANT"):
                color = self.colors['highlight']
            else:
                color = self.colors['text']
            text = self.font_small.render(line, True, color)
            panel_surface.blit(text, (10, y))
            y += 16
        
        surface.blit(panel_surface, (panel_x, panel_y))
        
        # Draw highlight circle around ant
        pygame.draw.circle(surface, self.colors['highlight'], (int(ant.x), int(ant.y)), 20, 2)
    
    def _draw_pheromone_debug(self, surface, pheromone_map):
        """Draw pheromone grid values"""
        cell_size = pheromone_map.cell_size
        
        for gy in range(pheromone_map.grid_height):
            for gx in range(pheromone_map.grid_width):
                food_val = pheromone_map.food_trail.get(gx, gy)
                home_val = pheromone_map.home_trail.get(gx, gy)
                
                # Only show cells with significant values
                if food_val > 5 or home_val > 5:
                    cx = gx * cell_size + cell_size // 2
                    cy = gy * cell_size + cell_size // 2
                    
                    # Show values
                    if food_val > 5:
                        text = self.font_small.render(f"{food_val:.0f}", True, self.colors['food_trail'])
                        surface.blit(text, (cx - 10, cy - 14))
                    if home_val > 5:
                        text = self.font_small.render(f"{home_val:.0f}", True, self.colors['home_trail'])
                        surface.blit(text, (cx - 10, cy + 2))
    
    def _draw_pathfinding(self, surface, ants):
        """Draw ant direction vectors and state indicators"""
        for ant in ants:
            # Direction vector
            length = 25
            end_x = ant.x + math.cos(ant.direction) * length
            end_y = ant.y + math.sin(ant.direction) * length
            
            # Color based on state
            if ant.state.name == 'FORAGING':
                color = self.colors['foraging']
            elif ant.state.name == 'RETURNING':
                color = self.colors['returning']
            else:
                color = self.colors['text_dim']
            
            # Draw direction line
            pygame.draw.line(surface, color, (ant.x, ant.y), (end_x, end_y), 1)
            
            # Draw arrowhead
            angle = ant.direction
            arrow_size = 5
            left = (end_x - arrow_size * math.cos(angle - 0.5),
                    end_y - arrow_size * math.sin(angle - 0.5))
            right = (end_x - arrow_size * math.cos(angle + 0.5),
                     end_y - arrow_size * math.sin(angle + 0.5))
            pygame.draw.polygon(surface, color, [(end_x, end_y), left, right])
    
    def _draw_mode_indicator(self, surface):
        """Draw current debug mode in bottom-left"""
        mode_text = f"DEBUG: {self.mode.name} [D to toggle, 1-5 for modes]"
        text = self.font_medium.render(mode_text, True, self.colors['highlight'])
        
        # Background
        bg_rect = text.get_rect()
        bg_rect.bottomleft = (10, self.height - 10)
        bg_surface = pygame.Surface((bg_rect.width + 20, bg_rect.height + 10), pygame.SRCALPHA)
        bg_surface.fill(self.colors['bg'])
        surface.blit(bg_surface, (bg_rect.left - 10, bg_rect.top - 5))
        
        surface.blit(text, bg_rect)
    
    # ==================== LOG DISPLAY ====================
    
    def draw_log(self, surface, x=None, y=None):
        """Draw recent log entries"""
        if x is None:
            x = self.width - 300
        if y is None:
            y = 10
        
        entries = self.log.get_recent(8)
        
        # Background
        panel_surface = pygame.Surface((280, len(entries) * 16 + 20), pygame.SRCALPHA)
        panel_surface.fill(self.colors['bg'])
        
        # Draw entries
        line_y = 10
        for entry in entries:
            time_str = f"{entry['time']:.1f}s"
            text = f"[{time_str}] {entry['category']}: {entry['message']}"
            if len(text) > 40:
                text = text[:37] + "..."
            
            color = self.colors['text_dim']
            if entry['category'] == 'FOOD':
                color = self.colors['good']
            elif entry['category'] == 'ERROR':
                color = self.colors['bad']
            
            rendered = self.font_small.render(text, True, color)
            panel_surface.blit(rendered, (10, line_y))
            line_y += 16
        
        surface.blit(panel_surface, (x, y))


# Global debug instance
_debug_system = None

def get_debug_system(width=1920, height=1080):
    """Get or create the global debug system"""
    global _debug_system
    if _debug_system is None:
        _debug_system = DebugSystem(width, height)
    return _debug_system

def debug_log(category, message):
    """Convenience function to log debug messages"""
    if _debug_system:
        _debug_system.log.log(category, message)
