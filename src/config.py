"""Configuration and constants for the ant colony simulator"""

import pygame

# Display settings
WINDOW_WIDTH = 1920
WINDOW_HEIGHT = 1080

# Modern dark theme colors
DARK_BG_COLOR = (15, 15, 20)           # Deep dark background
DARK_SECONDARY = (25, 25, 35)          # Slightly lighter for panels
ACCENT_COLOR = (100, 200, 255)         # Bright cyan accent
ACCENT_HOVER = (150, 220, 255)         # Brighter accent on hover
TEXT_PRIMARY = (240, 240, 245)         # Light text
TEXT_SECONDARY = (180, 180, 190)       # Dimmer text
BORDER_COLOR = (60, 60, 75)            # Subtle borders

# Grid settings
GRID_SIZE = 20  # pixels per cell
GRID_COLOR = (40, 40, 50)

# Wall settings
WALL_COLOR = (80, 40, 100)             # Purple/dark magenta walls
WALL_REPEL_RANGE = 120                 # How far ants sense walls (increased for better avoidance)
WALL_REPEL_STRENGTH = 3.0              # How strongly ants avoid walls (increased)

# UI settings
UI_PADDING = 16
UI_CORNER_RADIUS = 8
UI_BORDER_WIDTH = 1

# Animation
FPS = 60

# Simulation settings
INITIAL_ANT_COUNT = 100          # Number of ants to start with
MAX_POPULATION = 500            # Maximum ant population

# Ant senses
ANT_SMELL_RANGE = 150           # How far ants can smell food (pixels)
ANT_SMELL_STRENGTH = 0.8        # How strongly ants are drawn to food scent (0-1)
ANT_WANDER_TURN_RATE = 0.15     # How much ants turn when wandering (radians)

# Pheromone settings are now in src/pheromone_model.py

# =====================
# REAL-TIME ADJUSTABLE PARAMETERS
# These can be modified via UI sliders
# =====================
class RuntimeParams:
    """Parameters that can be adjusted in real-time via UI"""
    wall_repel_range = 120.0
    wall_repel_strength = 3.0
    ant_repulsion_radius = 25.0
    momentum = 0.5
    stuck_threshold = 15
    random_jitter = 0.1
    ant_repulsion_strength = 0.3

# Global instance
runtime = RuntimeParams()
