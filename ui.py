"""UI Management system for the ant colony simulator"""

import pygame
from config import (
    WINDOW_WIDTH, WINDOW_HEIGHT, TEXT_PRIMARY, TEXT_SECONDARY,
    DARK_SECONDARY, ACCENT_COLOR, UI_PADDING, BORDER_COLOR, UI_CORNER_RADIUS, UI_BORDER_WIDTH
)
from save_state import load_colony_state
import config

class Slider:
    """Modern slider component for real-time parameter adjustment"""
    def __init__(self, x, y, width, height, label, min_val, max_val, initial_val, callback=None):
        self.rect = pygame.Rect(x, y, width, height)
        self.label = label
        self.min_val = min_val
        self.max_val = max_val
        self.value = initial_val
        self.callback = callback
        self.dragging = False
        self.font = pygame.font.Font(None, 14)
        
        # Knob settings
        self.knob_radius = 8
        self.track_height = 4
        
    def get_knob_x(self):
        """Get knob x position based on current value"""
        ratio = (self.value - self.min_val) / (self.max_val - self.min_val)
        return self.rect.x + 10 + ratio * (self.rect.width - 20)
    
    def update(self, mouse_pos, mouse_pressed):
        knob_x = self.get_knob_x()
        knob_rect = pygame.Rect(knob_x - self.knob_radius, self.rect.y + 15, 
                                self.knob_radius * 2, self.knob_radius * 2)
        
        if mouse_pressed[0]:  # Left mouse button
            if self.dragging or knob_rect.collidepoint(mouse_pos) or \
               (self.rect.collidepoint(mouse_pos) and self.rect.y + 10 < mouse_pos[1] < self.rect.y + 35):
                self.dragging = True
                # Calculate new value from mouse x position
                rel_x = mouse_pos[0] - (self.rect.x + 10)
                ratio = max(0, min(1, rel_x / (self.rect.width - 20)))
                self.value = self.min_val + ratio * (self.max_val - self.min_val)
                
                if self.callback:
                    self.callback(self.value)
        else:
            self.dragging = False
            
    def draw(self, surface):
        # Draw label and value
        label_text = f"{self.label}: {self.value:.1f}"
        label_surface = self.font.render(label_text, True, TEXT_PRIMARY)
        surface.blit(label_surface, (self.rect.x, self.rect.y))
        
        # Draw track
        track_y = self.rect.y + 20
        track_rect = pygame.Rect(self.rect.x + 10, track_y, self.rect.width - 20, self.track_height)
        pygame.draw.rect(surface, BORDER_COLOR, track_rect, border_radius=2)
        
        # Draw filled portion
        knob_x = self.get_knob_x()
        filled_rect = pygame.Rect(self.rect.x + 10, track_y, knob_x - (self.rect.x + 10), self.track_height)
        pygame.draw.rect(surface, ACCENT_COLOR, filled_rect, border_radius=2)
        
        # Draw knob
        knob_color = (150, 220, 255) if self.dragging else ACCENT_COLOR
        pygame.draw.circle(surface, knob_color, (int(knob_x), track_y + 2), self.knob_radius)
        pygame.draw.circle(surface, TEXT_PRIMARY, (int(knob_x), track_y + 2), self.knob_radius - 2)

class Button:
    """Modern button component"""
    def __init__(self, x, y, width, height, text, callback=None):
        self.rect = pygame.Rect(x, y, width, height)
        self.text = text
        self.callback = callback
        self.hovered = False
        self.font = pygame.font.Font(None, 16)
        
    def update(self, mouse_pos):
        self.hovered = self.rect.collidepoint(mouse_pos)
        
    def draw(self, surface):
        # Draw background
        color = ACCENT_COLOR if self.hovered else DARK_SECONDARY
        pygame.draw.rect(surface, color, self.rect, border_radius=UI_CORNER_RADIUS)
        
        # Draw border
        pygame.draw.rect(surface, BORDER_COLOR, self.rect, width=UI_BORDER_WIDTH, border_radius=UI_CORNER_RADIUS)
        
        # Draw text
        text_surface = self.font.render(self.text, True, TEXT_PRIMARY)
        text_rect = text_surface.get_rect(center=self.rect.center)
        surface.blit(text_surface, text_rect)
        
    def handle_click(self):
        if self.callback:
            self.callback()

class Panel:
    """Modern panel component for UI sections"""
    def __init__(self, x, y, width, height, title=""):
        self.rect = pygame.Rect(x, y, width, height)
        self.title = title
        self.font_title = pygame.font.Font(None, 20)
        self.font_text = pygame.font.Font(None, 14)
        
    def draw(self, surface):
        # Draw background
        pygame.draw.rect(surface, DARK_SECONDARY, self.rect, border_radius=UI_CORNER_RADIUS)
        
        # Draw border
        pygame.draw.rect(surface, BORDER_COLOR, self.rect, width=UI_BORDER_WIDTH, border_radius=UI_CORNER_RADIUS)
        
        # Draw title if provided
        if self.title:
            title_surface = self.font_title.render(self.title, True, ACCENT_COLOR)
            surface.blit(title_surface, (self.rect.x + UI_PADDING, self.rect.y + UI_PADDING))

class UIManager:
    """Manages all UI elements"""
    def __init__(self, screen):
        self.screen = screen
        self.width = screen.get_width()
        self.height = screen.get_height()
        
        # Initialize UI elements
        self.title_font = pygame.font.Font(None, 48)
        self.subtitle_font = pygame.font.Font(None, 24)
        self.stat_font = pygame.font.Font(None, 14)
        
        # Create panels
        self.sidebar = Panel(UI_PADDING, UI_PADDING, 250, self.height - 2 * UI_PADDING, "Controls")
        self.main_area = Panel(
            UI_PADDING + 250 + UI_PADDING,
            UI_PADDING,
            self.width - (UI_PADDING * 3 + 250),
            self.height - 2 * UI_PADDING,
            "Colony Simulation"
        )
        
        # Store reference to colony for reset
        self.colony_ref = None
        self.main_area_rect = self.main_area.rect
        
        # Check if we're loading a saved state
        saved_state = load_colony_state()
        self.loaded_state = saved_state
        
        # Create buttons
        self.buttons = [
            Button(self.sidebar.rect.x + UI_PADDING, self.sidebar.rect.y + 80, 
                   self.sidebar.rect.width - 2 * UI_PADDING, 40, "Start", self.start_simulation),
            Button(self.sidebar.rect.x + UI_PADDING, self.sidebar.rect.y + 130,
                   self.sidebar.rect.width - 2 * UI_PADDING, 40, "Pause", self.pause_simulation),
            Button(self.sidebar.rect.x + UI_PADDING, self.sidebar.rect.y + 180,
                   self.sidebar.rect.width - 2 * UI_PADDING, 40, "Reset", self.reset_simulation),
        ]
        
        # Create sliders for real-time parameter adjustment
        slider_x = self.sidebar.rect.x + UI_PADDING
        slider_width = self.sidebar.rect.width - 2 * UI_PADDING
        slider_start_y = self.sidebar.rect.y + 500  # Below stats
        
        self.sliders = [
            Slider(slider_x, slider_start_y, slider_width, 35,
                   "Wall Repel Range", 20, 200, config.runtime.wall_repel_range,
                   lambda v: setattr(config.runtime, 'wall_repel_range', v)),
            Slider(slider_x, slider_start_y + 45, slider_width, 35,
                   "Wall Repel Strength", 0.5, 10.0, config.runtime.wall_repel_strength,
                   lambda v: setattr(config.runtime, 'wall_repel_strength', v)),
            Slider(slider_x, slider_start_y + 90, slider_width, 35,
                   "Ant Repel Radius", 5, 50, config.runtime.ant_repulsion_radius,
                   lambda v: setattr(config.runtime, 'ant_repulsion_radius', v)),
            Slider(slider_x, slider_start_y + 135, slider_width, 35,
                   "Momentum", 0.0, 1.0, config.runtime.momentum,
                   lambda v: setattr(config.runtime, 'momentum', v)),
            Slider(slider_x, slider_start_y + 180, slider_width, 35,
                   "Stuck Threshold", 5, 50, config.runtime.stuck_threshold,
                   lambda v: setattr(config.runtime, 'stuck_threshold', v)),
            Slider(slider_x, slider_start_y + 225, slider_width, 35,
                   "Random Jitter", 0.0, 0.5, config.runtime.random_jitter,
                   lambda v: setattr(config.runtime, 'random_jitter', v)),
            Slider(slider_x, slider_start_y + 270, slider_width, 35,
                   "Ant Repel Strength", 0.0, 1.0, config.runtime.ant_repulsion_strength,
                   lambda v: setattr(config.runtime, 'ant_repulsion_strength', v)),
        ]
        
        self.simulation_running = True
        
    def start_simulation(self):
        self.simulation_running = True
        
    def pause_simulation(self):
        from save_state import save_colony_state
        self.simulation_running = False
        if self.colony_ref:
            save_colony_state(self.colony_ref)
        
    def reset_simulation(self):
        from save_state import save_colony_state
        self.simulation_running = True
        self.needs_reset = True
        if self.colony_ref:
            save_colony_state(self.colony_ref)
        
    def update(self):
        mouse_pos = pygame.mouse.get_pos()
        mouse_pressed = pygame.mouse.get_pressed()
        
        for button in self.buttons:
            button.update(mouse_pos)
        
        # Update sliders
        for slider in self.sliders:
            slider.update(mouse_pos, mouse_pressed)
            
    def draw(self, surface, stats=None):
        """Draw UI and stats"""
        # Draw panels
        self.sidebar.draw(surface)
        self.main_area.draw(surface)
        
        # Draw title in sidebar
        title_surface = self.title_font.render("Ant Colony", True, ACCENT_COLOR)
        surface.blit(title_surface, (self.sidebar.rect.x + UI_PADDING, self.sidebar.rect.y + UI_PADDING))
        
        subtitle_surface = self.subtitle_font.render("Simulator", True, TEXT_SECONDARY)
        surface.blit(subtitle_surface, (self.sidebar.rect.x + UI_PADDING, self.sidebar.rect.y + 45))
        
        # Draw buttons
        for button in self.buttons:
            button.draw(surface)
            
        # Draw sliders
        for slider in self.sliders:
            slider.draw(surface)
            
        # Draw status text
        status = "Running" if self.simulation_running else "Paused"
        status_font = pygame.font.Font(None, 16)
        status_surface = status_font.render(f"Status: {status}", True, TEXT_SECONDARY)
        surface.blit(status_surface, (self.sidebar.rect.x + UI_PADDING, self.sidebar.rect.y + self.sidebar.rect.height - 60))
        
        # Draw statistics if provided
        if stats:
            stats_y = self.sidebar.rect.y + 250
            self._draw_stats(surface, stats, stats_y)
            
    def _draw_stats(self, surface, stats, start_y):
        """Draw colony statistics"""
        line_height = 22
        
        stat_items = [
            f"Population: {stats['population']}",
            f"Food: {stats['food_stored']:.0f}",
            f"Foraging: {stats['ants_foraging']}",
            f"Returning: {stats['ants_returning']}",
            "",
            f"== Evolution ==",
            f"Generation: {stats['generation']}",
            f"Gene Pool: {stats['gene_pool_size']}",
            f"Avg Fitness: {stats['avg_fitness']:.1f}",
            f"Best: {stats['best_fitness']:.1f}",
        ]
        
        for i, stat_text in enumerate(stat_items):
            if stat_text == "== Evolution ==":
                color = ACCENT_COLOR
            elif stat_text == "":
                continue
            else:
                color = TEXT_PRIMARY
            text_surface = self.stat_font.render(stat_text, True, color)
            surface.blit(text_surface, (self.sidebar.rect.x + UI_PADDING, start_y + i * line_height))