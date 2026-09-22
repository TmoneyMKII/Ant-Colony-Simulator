"""Configuration and constants for the ant colony simulator"""

import pygame
pygame.init()  # Required for key constants

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
ANT_SMELL_RANGE_SQ = ANT_SMELL_RANGE ** 2  # Squared for fast distance checks
ANT_SMELL_STRENGTH = 0.8        # How strongly ants are drawn to food scent (0-1)
ANT_WANDER_TURN_RATE = 0.15     # How much ants turn when wandering (radians)

# Ant collision/interaction distances (squared for performance)
ANT_FOOD_PICKUP_RANGE = 15      # Distance to pick up food
ANT_FOOD_PICKUP_RANGE_SQ = ANT_FOOD_PICKUP_RANGE ** 2
ANT_COLONY_DROPOFF_RANGE = 25   # Distance to drop off food at colony
ANT_COLONY_DROPOFF_RANGE_SQ = ANT_COLONY_DROPOFF_RANGE ** 2
ANT_REPULSION_RADIUS = 25.0     # Distance for ant-to-ant repulsion
ANT_REPULSION_RADIUS_SQ = ANT_REPULSION_RADIUS ** 2

# Stuck detection settings
STUCK_CHECK_INTERVAL = 180      # Frames between stuck checks (3 sec at 60 FPS)
STUCK_MIN_MOVEMENT = 80         # Min pixels to move to not be considered stuck
STUCK_MIN_MOVEMENT_SQ = STUCK_MIN_MOVEMENT ** 2
MAX_ESCAPE_ATTEMPTS = 5         # Escape attempts before ant dies
WALL_STUCK_DEATH_TIME = 60      # Frames in wall before death (~1 sec)

# Death markers
DEATH_MARKER_DURATION = 600     # Frames death marker visible (10 sec at 60 FPS)
MAX_DEATH_MARKERS = 500         # Maximum death markers to track

# Food placement
CLICK_FOOD_AMOUNT = 50          # Food amount when clicking to add food (hold F + click)
CLICK_FOOD_KEY = pygame.K_f     # Key to hold while clicking to add food

# Pheromone settings are now in src/pheromone_model.py

