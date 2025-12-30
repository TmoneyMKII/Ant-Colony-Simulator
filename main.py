import pygame
import sys
from config import DARK_BG_COLOR, GRID_SIZE, GRID_COLOR
from colony import Colony
from save_state import save_colony_state, load_colony_state

def draw_keybind_hints(screen, font):
    """Draw keybind hints overlay"""
    hints = [
        "[SPACE] Pause/Resume",
        "[P] Toggle Pheromones",
        "[R] Reset Colony",
        "[G] Toggle Grid",
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
        gen = saved_state.get('generation', 0)
        pool_size = len(saved_state.get('gene_pool', []))
        print(f"[LOADED] Generation {gen}, Gene Pool Size: {pool_size}")
    else:
        print("Starting new colony simulation...")
    
    # State
    running = True
    simulation_running = True
    show_pheromones = False
    show_hints = False
    show_grid = True
    
    # Fonts
    hint_font = pygame.font.Font(None, 20)
    small_font = pygame.font.Font(None, 18)
    
    while running:
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
                        print(f"Colony saved at generation {colony.generation}")
                elif event.key == pygame.K_p:
                    show_pheromones = not show_pheromones
                elif event.key == pygame.K_r:
                    save_colony_state(colony)
                    print(f"Colony saved at generation {colony.generation}")
                    colony = Colony(sim_rect.centerx, sim_rect.centery, 
                                  sim_rect.width, sim_rect.height, 
                                  bounds=sim_rect)
                    simulation_running = True
                elif event.key == pygame.K_h:
                    show_hints = not show_hints
                elif event.key == pygame.K_g:
                    show_grid = not show_grid
        
        # Update
        if simulation_running:
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
        
        pygame.display.flip()
        clock.tick(60)
    
    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()
