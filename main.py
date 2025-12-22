import pygame
import sys
from config import DARK_BG_COLOR, ACCENT_COLOR
from ui import UIManager
from colony import Colony

def main():
    """Initialize and run the ant colony simulation"""
    pygame.init()
    
    # Create fullscreen window with borders
    info = pygame.display.Info()
    screen = pygame.display.set_mode((info.current_w, info.current_h))
    pygame.display.set_caption("Ant Colony Simulator")
    
    clock = pygame.time.Clock()
    width = info.current_w
    height = info.current_h
    
    # Create UI manager
    ui_manager = UIManager(screen)
    
    # Get main simulation area bounds
    main_area_rect = ui_manager.main_area.rect
    
    # Create colony with bounds
    colony = Colony(main_area_rect.centerx, main_area_rect.centery, 
                   main_area_rect.width, main_area_rect.height, 
                   bounds=main_area_rect)
    
    running = True
    show_pheromones = True
    ui_manager.needs_reset = False
    
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                elif event.key == pygame.K_SPACE:
                    ui_manager.simulation_running = not ui_manager.simulation_running
                elif event.key == pygame.K_p:
                    show_pheromones = not show_pheromones
                elif event.key == pygame.K_r:
                    colony = Colony(main_area_rect.centerx, main_area_rect.centery, 
                                  main_area_rect.width, main_area_rect.height, 
                                  bounds=main_area_rect)
                    ui_manager.simulation_running = True
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1:  # Left click
                    mouse_pos = pygame.mouse.get_pos()
                    for button in ui_manager.buttons:
                        if button.rect.collidepoint(mouse_pos):
                            button.handle_click()
        
        # Check if reset was requested
        if ui_manager.needs_reset:
            colony = Colony(main_area_rect.centerx, main_area_rect.centery, 
                          main_area_rect.width, main_area_rect.height, 
                          bounds=main_area_rect)
            ui_manager.needs_reset = False
        
        # Update
        if ui_manager.simulation_running:
            colony.update()
            # Keep ants within bounds
            for ant in colony.ants:
                ant.x = max(main_area_rect.left + 5, min(ant.x, main_area_rect.right - 5))
                ant.y = max(main_area_rect.top + 5, min(ant.y, main_area_rect.bottom - 5))
        ui_manager.update()
        
        # Render
        screen.fill(DARK_BG_COLOR)
        
        # Get main area rect for rendering
        main_area_rect = ui_manager.main_area.rect
        
        # Draw UI panels first (background)
        ui_manager.sidebar.draw(screen)
        ui_manager.main_area.draw(screen)
        
        # Draw simulation background and grid in main area
        main_area_rect = ui_manager.main_area.rect
        pygame.draw.rect(screen, (20, 20, 25), main_area_rect)
        
        # Draw grid in simulation area
        from config import GRID_SIZE, GRID_COLOR
        for x in range(main_area_rect.left, main_area_rect.right, GRID_SIZE):
            pygame.draw.line(screen, GRID_COLOR, (x, main_area_rect.top), (x, main_area_rect.bottom), 1)
        for y in range(main_area_rect.top, main_area_rect.bottom, GRID_SIZE):
            pygame.draw.line(screen, GRID_COLOR, (main_area_rect.left, y), (main_area_rect.right, y), 1)
        
        # Draw colony entities (ants, food, etc.) on top
        colony.draw(screen, show_pheromones=show_pheromones)
        
        # Draw buttons and stats on top
        for button in ui_manager.buttons:
            button.draw(screen)
        ui_manager._draw_stats(screen, colony.get_stats(), ui_manager.sidebar.rect.y + 250)
        
        # Draw status and title
        status = "Running" if ui_manager.simulation_running else "Paused"
        status_font = pygame.font.Font(None, 16)
        status_surface = status_font.render(f"Status: {status}", True, (180, 180, 190))
        screen.blit(status_surface, (ui_manager.sidebar.rect.x + 16, ui_manager.sidebar.rect.y + ui_manager.sidebar.rect.height - 60))
        
        pygame.display.flip()
        clock.tick(60)
    
    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()
