"""
Stats UI - Transparent Overlay for Colony Statistics and Neural Network Visualization

Provides real-time visualization of:
- Neural network activations (colony average)
- Colony health metrics (food, population, efficiency)
- Rolling graphs of key statistics
- Knowledge and learning progress indicators
"""

import pygame
import math
from typing import List, Tuple, Optional
from collections import deque


class GraphWidget:
    """A simple line graph widget with transparent background"""
    
    def __init__(self, x: int, y: int, width: int, height: int, 
                 title: str, color: Tuple[int, int, int],
                 max_points: int = 150, y_range: Tuple[float, float] = (0, 1)):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.title = title
        self.color = color
        self.max_points = max_points
        self.y_min, self.y_max = y_range
        self.auto_scale = y_range == (0, 0)  # Auto-scale if both are 0
        
        self.data = deque(maxlen=max_points)
        
        # Pre-create surface
        self.surface = pygame.Surface((width, height), pygame.SRCALPHA)
        self.font = pygame.font.Font(None, 16)
        self.title_font = pygame.font.Font(None, 18)
    
    def add_point(self, value: float):
        """Add a data point"""
        self.data.append(value)
    
    def set_data(self, data: List[float]):
        """Set all data at once"""
        self.data.clear()
        for v in data[-self.max_points:]:
            self.data.append(v)
    
    def draw(self, target_surface: pygame.Surface):
        """Draw the graph"""
        self.surface.fill((0, 0, 0, 0))  # Clear with transparency
        
        # Semi-transparent background
        pygame.draw.rect(self.surface, (20, 20, 30, 180), 
                        (0, 0, self.width, self.height), border_radius=5)
        pygame.draw.rect(self.surface, (*self.color, 100), 
                        (0, 0, self.width, self.height), width=1, border_radius=5)
        
        # Title
        title_surf = self.title_font.render(self.title, True, (*self.color, 255))
        self.surface.blit(title_surf, (5, 3))
        
        if len(self.data) < 2:
            target_surface.blit(self.surface, (self.x, self.y))
            return
        
        # Calculate y range
        if self.auto_scale:
            data_min = min(self.data)
            data_max = max(self.data)
            if data_max == data_min:
                data_max = data_min + 1
            y_min, y_max = data_min, data_max
        else:
            y_min, y_max = self.y_min, self.y_max
        
        # Draw grid lines
        graph_top = 20
        graph_bottom = self.height - 5
        graph_height = graph_bottom - graph_top
        
        for i in range(3):
            y_pos = graph_top + (graph_height * i // 2)
            pygame.draw.line(self.surface, (50, 50, 60, 100),
                           (5, y_pos), (self.width - 5, y_pos), 1)
        
        # Draw data line
        points = []
        for i, value in enumerate(self.data):
            x = 5 + (i / max(1, len(self.data) - 1)) * (self.width - 10)
            y_normalized = (value - y_min) / max(0.001, y_max - y_min)
            y = graph_bottom - (y_normalized * graph_height)
            y = max(graph_top, min(graph_bottom, y))
            points.append((x, y))
        
        if len(points) >= 2:
            pygame.draw.lines(self.surface, self.color, False, points, 2)
        
        # Current value
        if self.data:
            current = self.data[-1]
            if current >= 100:
                val_text = f"{int(current)}"
            elif current >= 10:
                val_text = f"{current:.1f}"
            else:
                val_text = f"{current:.2f}"
            val_surf = self.font.render(val_text, True, (200, 200, 210))
            self.surface.blit(val_surf, (self.width - val_surf.get_width() - 5, 3))
        
        target_surface.blit(self.surface, (self.x, self.y))


class NeuralNetworkVisualizer:
    """Visualizes neural network activations as a node graph - optimized for 27 inputs"""
    
    def __init__(self, x: int, y: int, width: int, height: int):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        
        self.surface = pygame.Surface((width, height), pygame.SRCALPHA)
        self.font = pygame.font.Font(None, 10)  # Smaller font for many labels
        self.title_font = pygame.font.Font(None, 18)
        
        # Node positions
        self.input_labels = []
        self.output_labels = []
        
        # Animation
        self.pulse_phase = 0
        
        # Show abbreviated view for large networks
        self.compact_mode = True
    
    def draw(self, target_surface: pygame.Surface, activations: dict,
             input_labels: List[str], output_labels: List[str]):
        """Draw the neural network visualization"""
        self.surface.fill((0, 0, 0, 0))
        self.pulse_phase += 0.05
        
        # Background
        pygame.draw.rect(self.surface, (20, 20, 30, 180),
                        (0, 0, self.width, self.height), border_radius=8)
        pygame.draw.rect(self.surface, (100, 150, 255, 100),
                        (0, 0, self.width, self.height), width=1, border_radius=8)
        
        # Title
        title = self.title_font.render("Colony Neural Network", True, (150, 200, 255))
        self.surface.blit(title, (self.width // 2 - title.get_width() // 2, 5))
        
        inputs = activations.get('inputs', [0] * 27)
        hidden = activations.get('hidden', [0] * 16)
        outputs = activations.get('outputs', [0] * 3)
        
        # Layout for large networks - group inputs by type
        margin = 20
        
        # For 27 inputs, organize in 3 columns (vision types) 
        # Layout: [Input Groups] [Hidden] [Output]
        
        # Draw vision inputs as 3 vertical groups
        vision_rays = 7
        group_height = (self.height - 60) / 3
        
        # Vision input groups: Walls, Ants, Food
        vision_labels = ['Walls', 'Ants', 'Food']
        vision_colors = [(255, 100, 100), (255, 200, 100), (100, 255, 100)]
        
        for group_idx in range(3):
            group_y = 30 + group_idx * group_height
            start_idx = group_idx * vision_rays
            
            # Group label
            group_label = self.font.render(vision_labels[group_idx], True, vision_colors[group_idx])
            self.surface.blit(group_label, (5, group_y))
            
            # Draw rays as horizontal bar
            bar_x = 35
            bar_width = 60
            ray_height = group_height / (vision_rays + 1)
            
            for ray_idx in range(vision_rays):
                ray_y = group_y + 12 + ray_idx * ray_height
                val = inputs[start_idx + ray_idx] if start_idx + ray_idx < len(inputs) else 0
                
                # Draw mini bar for each ray
                intensity = abs(val)
                bar_len = int(bar_width * intensity)
                color = (*vision_colors[group_idx][:3], int(100 + 155 * intensity))
                
                pygame.draw.rect(self.surface, (40, 40, 50), 
                               (bar_x, ray_y, bar_width, ray_height - 2), border_radius=2)
                if bar_len > 0:
                    pygame.draw.rect(self.surface, color[:3],
                                   (bar_x, ray_y, bar_len, ray_height - 2), border_radius=2)
        
        # State inputs (last 6 inputs)
        state_y = self.height - 80
        state_x = 5
        state_labels_short = ['Ph', 'Ho', 'Dst', 'Dir', 'Cary', 'Nrg']
        
        state_label = self.font.render("State:", True, (150, 150, 200))
        self.surface.blit(state_label, (state_x, state_y))
        
        for i in range(min(6, len(inputs) - 21)):
            idx = 21 + i
            val = inputs[idx] if idx < len(inputs) else 0
            x_pos = state_x + 30 + i * 15
            
            # Intensity indicator
            intensity = abs(val)
            color = (100 + int(155 * intensity), 100, 100 + int(100 * intensity))
            pygame.draw.circle(self.surface, color, (x_pos, state_y + 15), 5)
        
        # Hidden layer (simplified - just show activity level)
        hidden_x = self.width // 2 - 20
        hidden_y = 35
        hidden_height = self.height - 70
        
        hidden_label = self.font.render("Hidden", True, (150, 200, 150))
        self.surface.blit(hidden_label, (hidden_x, 22))
        
        # Draw hidden as vertical bar chart
        hidden_bar_width = 40
        node_height = hidden_height / len(hidden)
        
        for j, val in enumerate(hidden):
            y_pos = hidden_y + j * node_height
            intensity = (val + 1) / 2  # Tanh output, map -1,1 to 0,1
            bar_len = int(hidden_bar_width * intensity)
            
            # Background
            pygame.draw.rect(self.surface, (30, 40, 30),
                           (hidden_x, y_pos, hidden_bar_width, node_height - 1), border_radius=2)
            # Value bar
            if val > 0:
                color = (50, 150 + int(100 * val), 50)
            else:
                color = (150 + int(100 * abs(val)), 50, 50)
            
            pygame.draw.rect(self.surface, color,
                           (hidden_x, y_pos, bar_len, node_height - 1), border_radius=2)
        
        # Output layer (3 outputs - detailed)
        output_x = self.width - 55
        output_y = self.height // 2 - 50
        
        output_label = self.font.render("Output", True, (200, 150, 150))
        self.surface.blit(output_label, (output_x, output_y - 15))
        
        output_colors = [(255, 200, 100), (100, 255, 200), (200, 100, 255)]
        output_names = ['Turn', 'Speed', 'Expl']
        
        for k, (val, name) in enumerate(zip(outputs, output_names)):
            y_pos = output_y + k * 35
            
            # Output node (larger)
            intensity = abs(val)
            color = output_colors[k]
            
            pygame.draw.circle(self.surface, (30, 30, 40), (output_x + 20, y_pos + 10), 12)
            pygame.draw.circle(self.surface, color, (output_x + 20, y_pos + 10), 
                             int(4 + 8 * intensity))
            
            # Label and value
            name_surf = self.font.render(name, True, color)
            self.surface.blit(name_surf, (output_x + 35, y_pos + 5))
            
            val_surf = self.font.render(f"{val:.2f}", True, (180, 180, 180))
            self.surface.blit(val_surf, (output_x + 35, y_pos + 15))
        
        target_surface.blit(self.surface, (self.x, self.y))
    
    def _draw_node(self, x: float, y: float, activation: float, label: str, layer_type: str):
        """Draw a single node"""
        radius = 6
        
        # Color based on activation
        if activation > 0:
            intensity = min(255, int(100 + activation * 155))
            color = (50, intensity, 50)  # Green for positive
        else:
            intensity = min(255, int(100 + abs(activation) * 155))
            color = (intensity, 50, 50)  # Red for negative
        
        # Pulse effect for active nodes
        pulse = math.sin(self.pulse_phase) * 0.2 + 0.8
        if abs(activation) > 0.5:
            radius = int(radius * (1 + (abs(activation) - 0.5) * 0.3 * pulse))
        
        # Draw node
        pygame.draw.circle(self.surface, color, (int(x), int(y)), radius)
        pygame.draw.circle(self.surface, (200, 200, 210), (int(x), int(y)), radius, 1)
        
        # Draw label
        if label:
            if layer_type == 'input':
                label_surf = self.font.render(label, True, (180, 180, 190))
                self.surface.blit(label_surf, (x - label_surf.get_width() - radius - 2, y - 4))
            else:
                label_surf = self.font.render(label, True, (180, 180, 190))
                self.surface.blit(label_surf, (x + radius + 2, y - 4))


class HealthBar:
    """A horizontal health/progress bar"""
    
    def __init__(self, x: int, y: int, width: int, height: int,
                 label: str, color: Tuple[int, int, int]):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.label = label
        self.color = color
        self.value = 0.0
        
        self.surface = pygame.Surface((width, height), pygame.SRCALPHA)
        self.font = pygame.font.Font(None, 16)
    
    def set_value(self, value: float):
        """Set value (0 to 1)"""
        self.value = max(0, min(1, value))
    
    def draw(self, target_surface: pygame.Surface):
        """Draw the health bar"""
        self.surface.fill((0, 0, 0, 0))
        
        # Background
        pygame.draw.rect(self.surface, (30, 30, 40, 180),
                        (0, 0, self.width, self.height), border_radius=3)
        
        # Fill
        fill_width = int((self.width - 4) * self.value)
        if fill_width > 0:
            # Gradient effect
            for i in range(fill_width):
                alpha = int(200 + 55 * (i / max(1, fill_width)))
                x_pos = 2 + i
                pygame.draw.line(self.surface, (*self.color, alpha),
                               (x_pos, 2), (x_pos, self.height - 2))
        
        # Border
        pygame.draw.rect(self.surface, (*self.color, 150),
                        (0, 0, self.width, self.height), width=1, border_radius=3)
        
        # Label
        label_surf = self.font.render(self.label, True, (200, 200, 210))
        self.surface.blit(label_surf, (4, (self.height - label_surf.get_height()) // 2))
        
        # Value
        val_text = f"{int(self.value * 100)}%"
        val_surf = self.font.render(val_text, True, (220, 220, 230))
        self.surface.blit(val_surf, (self.width - val_surf.get_width() - 4,
                                     (self.height - val_surf.get_height()) // 2))
        
        target_surface.blit(self.surface, (self.x, self.y))


class StatsUI:
    """
    Main stats UI overlay combining all visualization components.
    
    Layout:
        Top-left: Neural network visualization
        Top-center: Health/metric bars
        Top-right: Rolling graphs
    """
    
    def __init__(self, screen_width: int, screen_height: int):
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.visible = True
        
        # Neural network visualizer (left side)
        self.nn_viz = NeuralNetworkVisualizer(10, 10, 280, 200)
        
        # Health bars (center-top)
        bar_x = 300
        bar_width = 150
        bar_height = 20
        bar_spacing = 25
        
        self.health_bars = {
            'knowledge': HealthBar(bar_x, 15, bar_width, bar_height, 
                                  "Knowledge", (100, 200, 255)),
            'cooperation': HealthBar(bar_x, 15 + bar_spacing, bar_width, bar_height,
                                    "Cooperation", (100, 255, 150)),
            'diversity': HealthBar(bar_x, 15 + bar_spacing * 2, bar_width, bar_height,
                                  "Diversity", (255, 200, 100)),
            'efficiency': HealthBar(bar_x, 15 + bar_spacing * 3, bar_width, bar_height,
                                   "Efficiency", (255, 150, 200)),
        }
        
        # Graphs (right side)
        graph_x = 470
        graph_width = 160
        graph_height = 50
        graph_spacing = 55
        
        self.graphs = {
            'food': GraphWidget(graph_x, 10, graph_width, graph_height,
                               "Food/sec", (100, 255, 150), y_range=(0, 0)),
            'population': GraphWidget(graph_x + graph_width + 10, 10, graph_width, graph_height,
                                     "Population", (255, 200, 100), y_range=(0, 0)),
            'fitness': GraphWidget(graph_x, 10 + graph_spacing, graph_width, graph_height,
                                  "Fitness", (200, 150, 255), y_range=(0, 0)),
            'exploration': GraphWidget(graph_x + graph_width + 10, 10 + graph_spacing, graph_width, graph_height,
                                      "Exploration", (255, 150, 150), y_range=(0, 1)),
        }
        
        # Stats text
        self.stats_font = pygame.font.Font(None, 18)
        self.generation = 0
        self.total_food = 0
        self.best_fitness = 0.0
        
        # Surface for main stats panel
        self.stats_surface = pygame.Surface((180, 100), pygame.SRCALPHA)
    
    def toggle(self):
        """Toggle visibility"""
        self.visible = not self.visible
        return self.visible
    
    def update(self, colony_brain, colony_stats: dict = None):
        """Update all UI components with new data"""
        if not self.visible:
            return
        
        # Get data from colony brain
        stats = colony_brain.get_stats()
        graph_data = colony_brain.get_graph_data()
        nn_data = colony_brain.get_network_visualization_data()
        
        # Update health bars
        self.health_bars['knowledge'].set_value(stats.get('knowledge', 0))
        self.health_bars['cooperation'].set_value(stats.get('cooperation', 0))
        self.health_bars['diversity'].set_value(stats.get('diversity', 0))
        
        # Efficiency based on fitness
        efficiency = min(1.0, stats.get('avg_fitness', 0) / 50.0)
        self.health_bars['efficiency'].set_value(efficiency)
        
        # Update graphs
        self.graphs['food'].set_data(graph_data.get('food', []))
        self.graphs['population'].set_data(graph_data.get('population', []))
        self.graphs['fitness'].set_data(graph_data.get('fitness', []))
        self.graphs['exploration'].set_data(graph_data.get('exploration', []))
        
        # Update stats
        self.generation = stats.get('generation', 0)
        self.total_food = stats.get('total_food', 0)
        self.best_fitness = stats.get('best_fitness', 0)
        
        # Store NN data for drawing
        self.nn_activations = nn_data.get('activations', {})
        self.input_labels = nn_data.get('input_labels', [])
        self.output_labels = nn_data.get('output_labels', [])
    
    def draw(self, surface: pygame.Surface):
        """Draw all UI components"""
        if not self.visible:
            return
        
        # Draw neural network visualization
        self.nn_viz.draw(surface, self.nn_activations, self.input_labels, self.output_labels)
        
        # Draw health bars
        for bar in self.health_bars.values():
            bar.draw(surface)
        
        # Draw graphs
        for graph in self.graphs.values():
            graph.draw(surface)
        
        # Draw stats panel (far right)
        self._draw_stats_panel(surface)
    
    def _draw_stats_panel(self, surface: pygame.Surface):
        """Draw the stats text panel"""
        x = self.screen_width - 190
        y = 10
        
        self.stats_surface.fill((0, 0, 0, 0))
        pygame.draw.rect(self.stats_surface, (20, 20, 30, 180),
                        (0, 0, 180, 100), border_radius=5)
        pygame.draw.rect(self.stats_surface, (100, 150, 200, 100),
                        (0, 0, 180, 100), width=1, border_radius=5)
        
        # Title
        title = self.stats_font.render("Colony Stats", True, (150, 200, 255))
        self.stats_surface.blit(title, (90 - title.get_width() // 2, 5))
        
        # Stats
        stats_text = [
            f"Generation: {self.generation}",
            f"Total Food: {self.total_food}",
            f"Best Fitness: {self.best_fitness:.1f}",
            f"[N] Toggle Neural UI"
        ]
        
        for i, text in enumerate(stats_text):
            text_surf = self.stats_font.render(text, True, (180, 180, 190))
            self.stats_surface.blit(text_surf, (10, 25 + i * 18))
        
        surface.blit(self.stats_surface, (x, y))
