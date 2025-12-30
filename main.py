import pygame
import sys
from src.config import DARK_BG_COLOR, GRID_SIZE, GRID_COLOR, CLICK_FOOD_AMOUNT, CLICK_FOOD_KEY
from src.colony import Colony
from src.save_state import save_colony_state, load_colony_state
from src.debug import DebugSystem, DebugMode

def draw_keybind_hints(screen, font):
    """Draw keybind hints overlay"""
    hints = [
        "[SPACE] Pause/Resume",
        "[,/.] Speed Down/Up",
        "[P] Toggle Pheromones",
        "[R] Reset Colony",
        "[G] Toggle Grid",
        "[D] Debug Mode",
        "[1-5] Debug Levels",
        "[F+Click] Add Food",
        "[H] Hide Hints",
        "[ESC] Quit",
    ]
    
    # Semi-transparent background
    hint_surface = pygame.Surface((220, len(hints) * 25 + 20), pygame.SRCALPHA)
    hint_surface.fill((0, 0, 0, 180))
    
    for i, hint in enumerate(hints):
        text = font.render(hint, True, (200, 200, 210))
        hint_surface.blit(text, (15, 10 + i * 25))
    
    screen.blit(hint_surface, (20, 20))

def main():
    """Initialize and run the ant colony simulation"""
    pygame.init()
    
    # Create fullscreen window
    info = pygame.display.Info()
    screen = pygame.display.set_mode((info.current_w, info.current_h))
    pygame.display.set_caption("Ant Colony Simulator")
    
    clock = pygame.time.Clock()
    width = info.current_w
    height = info.current_h
    
    # Simulation area is full screen
    sim_rect = pygame.Rect(0, 0, width, height)
    
    # Create colony
    colony = Colony(sim_rect.centerx, sim_rect.centery, 
                   sim_rect.width, sim_rect.height, 
                   bounds=sim_rect)
    
    # Check for loaded state
    saved_state = load_colony_state()
    if saved_state:
        print(f"[INFO] Previous save found")
    else:
        print("Starting new colony simulation...")
    
    # State
    running = True
    simulation_running = True
    show_pheromones = False
    show_hints = False
    show_grid = True
    
    # Simulation speed (1 = normal, higher = faster)
    sim_speed = 1
    speed_levels = [0.25, 0.5, 1, 2, 4, 8]
    speed_index = 2  # Start at 1x
    
    # Debug system
    debug = DebugSystem(width, height)
    
    # Fonts
    hint_font = pygame.font.Font(None, 20)
    small_font = pygame.font.Font(None, 18)
    
    while running:
        mouse_pos = pygame.mouse.get_pos()
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                elif event.key == pygame.K_SPACE:
                    simulation_running = not simulation_running
                    if not simulation_running:
                        save_colony_state(colony)
                        print(f"Colony saved")
                elif event.key == pygame.K_p:
                    show_pheromones = not show_pheromones
                elif event.key == pygame.K_r:
                    save_colony_state(colony)
                    print(f"Colony saved")
                    colony = Colony(sim_rect.centerx, sim_rect.centery, 
                                  sim_rect.width, sim_rect.height, 
                                  bounds=sim_rect)
                    simulation_running = True
                    debug.total_food_deposits = 0
                    debug.total_home_deposits = 0
                    debug.food_collected_total = 0
                    debug.trips_completed = 0
                elif event.key == pygame.K_h:
                    show_hints = not show_hints
                elif event.key == pygame.K_g:
                    show_grid = not show_grid
                elif event.key == pygame.K_d:
                    mode = debug.toggle()
                    print(f"Debug mode: {mode.name}")
                elif event.key == pygame.K_1:
                    debug.set_mode(DebugMode.OFF)
                elif event.key == pygame.K_2:
                    debug.set_mode(DebugMode.STATS)
                elif event.key == pygame.K_3:
                    debug.set_mode(DebugMode.ANT_DETAILS)
                elif event.key == pygame.K_4:
                    debug.set_mode(DebugMode.PHEROMONE_DEBUG)
                elif event.key == pygame.K_5:
                    debug.set_mode(DebugMode.PATHFINDING)
                elif event.key == pygame.K_6:
                    debug.set_mode(DebugMode.FULL)
                elif event.key == pygame.K_PERIOD:  # . to speed up
                    speed_index = min(speed_index + 1, len(speed_levels) - 1)
                    sim_speed = speed_levels[speed_index]
                elif event.key == pygame.K_COMMA:  # , to slow down
                    speed_index = max(speed_index - 1, 0)
                    sim_speed = speed_levels[speed_index]
            
            # Mouse click to add food (while holding F key)
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1:  # Left click
                    keys = pygame.key.get_pressed()
                    if keys[CLICK_FOOD_KEY]:
                        # Add food at mouse position (returns False if in wall)
                        if colony.add_food_source(event.pos[0], event.pos[1], CLICK_FOOD_AMOUNT):
                            print(f"Added food source at {event.pos}")
                        else:
                            print(f"Cannot place food in wall at {event.pos}")
        
        # Update (run multiple times for speed > 1)
        if simulation_running:
            updates = int(sim_speed) if sim_speed >= 1 else 1
            for _ in range(updates):
                colony.update()
        
        # Render
        screen.fill(DARK_BG_COLOR)
        
        # Draw grid
        if show_grid:
            for x in range(0, width, GRID_SIZE):
                pygame.draw.line(screen, GRID_COLOR, (x, 0), (x, height), 1)
            for y in range(0, height, GRID_SIZE):
                pygame.draw.line(screen, GRID_COLOR, (0, y), (width, y), 1)
        
        # Draw colony
        colony.draw(screen, show_pheromones=show_pheromones)
        
        # Draw debug overlays
        debug.draw(screen, colony, mouse_pos)
        
        # Draw hint toggle or full hints
        if show_hints:
            draw_keybind_hints(screen, hint_font)
        else:
            hint_text = small_font.render("[H] Keybinds", True, (120, 120, 130))
            screen.blit(hint_text, (15, 15))
        
        # Show paused indicator
        if not simulation_running:
            pause_text = hint_font.render("PAUSED", True, (255, 200, 100))
            screen.blit(pause_text, (width // 2 - 30, 15))
        
        # Show speed indicator in top right
        if sim_speed == 1:
            speed_text = "1x"
            speed_color = (150, 150, 160)
        elif sim_speed < 1:
            speed_text = f"{sim_speed}x"
            speed_color = (100, 180, 255)  # Blue for slow
        else:
            speed_text = f"{int(sim_speed)}x"
            speed_color = (255, 180, 100)  # Orange for fast
        speed_surface = hint_font.render(f"Speed: {speed_text}", True, speed_color)
        screen.blit(speed_surface, (width - 100, 15))
        
        pygame.display.flip()
        # Adjust tick rate for slow motion
        if sim_speed < 1:
            clock.tick(int(60 * sim_speed))
        else:
            clock.tick(60)
    
    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()
