"""UI Management system for the ant colony simulator"""

import pygame
from config import (
    WINDOW_WIDTH, WINDOW_HEIGHT, TEXT_PRIMARY, TEXT_SECONDARY,
    DARK_SECONDARY, ACCENT_COLOR, UI_PADDING, BORDER_COLOR, UI_CORNER_RADIUS, UI_BORDER_WIDTH
)

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
        
        # Create buttons
        self.buttons = [
            Button(self.sidebar.rect.x + UI_PADDING, self.sidebar.rect.y + 80, 
                   self.sidebar.rect.width - 2 * UI_PADDING, 40, "Start", self.start_simulation),
            Button(self.sidebar.rect.x + UI_PADDING, self.sidebar.rect.y + 130,
                   self.sidebar.rect.width - 2 * UI_PADDING, 40, "Pause", self.pause_simulation),
            Button(self.sidebar.rect.x + UI_PADDING, self.sidebar.rect.y + 180,
                   self.sidebar.rect.width - 2 * UI_PADDING, 40, "Reset", self.reset_simulation),
        ]
        
        self.simulation_running = True
        
    def set_colony_reference(self, colony):
        """Store reference to colony for reset"""
        self.colony_ref = colony
        
    def start_simulation(self):
        self.simulation_running = True
        
    def pause_simulation(self):
        self.simulation_running = False
        
    def reset_simulation(self):
        self.simulation_running = True
        self.needs_reset = True
        
    def update(self):
        mouse_pos = pygame.mouse.get_pos()
        for button in self.buttons:
            button.update(mouse_pos)
            
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
            
        # Draw status text
        status = "Running" if self.simulation_running else "Paused"
        status_font = pygame.font.Font(None, 16)
        status_surface = status_font.render(f"Status: {status}", True, TEXT_SECONDARY)
        surface.blit(status_surface, (self.sidebar.rect.x + UI_PADDING, self.sidebar.rect.y + self.sidebar.rect.height - 60))
        
        # Draw statistics if provided
        if stats:
            stats_y = self.sidebar.rect.y + 250
            self._draw_stats(surface, stats, stats_y)
        if stats:
            stats_y = self.sidebar.rect.y + 250
            self._draw_stats(surface, stats, stats_y)
            
    def _draw_stats(self, surface, stats, start_y):
        """Draw colony statistics"""
        line_height = 25
        
        stat_items = [
            f"Population: {stats['population']}",
            f"Food Stored: {stats['food_stored']:.0f}",
            f"Foraging: {stats['ants_foraging']}",
            f"Returning: {stats['ants_returning']}",
            f"Food Sources: {stats['food_sources_active']}",
        ]
        
        for i, stat_text in enumerate(stat_items):
            text_surface = self.stat_font.render(stat_text, True, TEXT_PRIMARY)
            surface.blit(text_surface, (self.sidebar.rect.x + UI_PADDING, start_y + i * line_height))