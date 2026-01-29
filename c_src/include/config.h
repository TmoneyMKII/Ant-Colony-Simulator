/**
 * @file config.h
 * @brief Configuration constants for the ant colony simulator
 * 
 * All compile-time configuration values. Many are squared for
 * faster distance comparisons (avoids sqrt).
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ============== Display Settings ============== */
#define WINDOW_TITLE        "Ant Colony Simulator"
#define TARGET_FPS          60
#define FRAME_TIME_MS       (1000 / TARGET_FPS)

/* ============== Color Definitions (RGBA) ============== */
// Modern dark theme
#define COLOR_BG_R          15
#define COLOR_BG_G          15
#define COLOR_BG_B          20
#define COLOR_BG_A          255

#define COLOR_GRID_R        40
#define COLOR_GRID_G        40
#define COLOR_GRID_B        50

#define COLOR_WALL_R        80
#define COLOR_WALL_G        40
#define COLOR_WALL_B        100

#define COLOR_ACCENT_R      100
#define COLOR_ACCENT_G      200
#define COLOR_ACCENT_B      255

/* ============== Grid Settings ============== */
#define GRID_CELL_SIZE      20  /* pixels per cell */

/* ============== Simulation Settings ============== */
#define INITIAL_ANT_COUNT   100
#define MAX_POPULATION      500

/* ============== Ant Configuration ============== */
// Senses (squared for fast distance checks)
#define ANT_SMELL_RANGE         150.0f
#define ANT_SMELL_RANGE_SQ      (ANT_SMELL_RANGE * ANT_SMELL_RANGE)
#define ANT_SMELL_STRENGTH      0.8f
#define ANT_WANDER_TURN_RATE    0.15f

// Collision distances (squared)
#define ANT_FOOD_PICKUP_RANGE       15.0f
#define ANT_FOOD_PICKUP_RANGE_SQ    (ANT_FOOD_PICKUP_RANGE * ANT_FOOD_PICKUP_RANGE)
#define ANT_COLONY_DROPOFF_RANGE    25.0f
#define ANT_COLONY_DROPOFF_RANGE_SQ (ANT_COLONY_DROPOFF_RANGE * ANT_COLONY_DROPOFF_RANGE)
#define ANT_REPULSION_RADIUS        25.0f
#define ANT_REPULSION_RADIUS_SQ     (ANT_REPULSION_RADIUS * ANT_REPULSION_RADIUS)

// Default attributes (fixed values)
#define DEFAULT_ANT_SPEED               2.0f
#define DEFAULT_PHEROMONE_SENSITIVITY   0.15f
#define DEFAULT_PHEROMONE_STRENGTH      1.5f
#define DEFAULT_ENERGY_EFFICIENCY       0.01f

// Stuck detection
#define STUCK_CHECK_INTERVAL    180     /* frames (3 sec at 60 FPS) */
#define STUCK_MIN_MOVEMENT      80.0f
#define STUCK_MIN_MOVEMENT_SQ   (STUCK_MIN_MOVEMENT * STUCK_MIN_MOVEMENT)
#define MAX_ESCAPE_ATTEMPTS     5
#define WALL_STUCK_DEATH_TIME   60      /* frames (~1 sec) */

// Pheromone deposit
#define PHEROMONE_DEPOSIT       8.0f

/* ============== Death Markers ============== */
#define DEATH_MARKER_DURATION   600     /* frames (10 sec at 60 FPS) */
#define MAX_DEATH_MARKERS       500

/* ============== Food ============== */
#define CLICK_FOOD_AMOUNT       50
#define MIN_FOOD_SPAWN          50.0f
#define MAX_FOOD_SPAWN          150.0f
#define FOOD_SOURCE_COUNT       12

/* ============== Pheromone System ============== */
#define PHEROMONE_CELL_SIZE     20
#define PHEROMONE_MAX_VALUE     200.0f
#define PHEROMONE_EVAP_RATE     0.995f
#define PHEROMONE_DANGER_EVAP   0.998f
#define PHEROMONE_DETECT_THRESHOLD  10.0f

/* ============== Neural Network ============== */
#define NN_NUM_VISION_RAYS      7
#define NN_VISION_INPUTS        (NN_NUM_VISION_RAYS * 3)  /* wall, ant, food per ray */
#define NN_STATE_INPUTS         6
#define NN_INPUT_SIZE           (NN_VISION_INPUTS + NN_STATE_INPUTS)  /* 27 */
#define NN_HIDDEN_SIZE          16
#define NN_OUTPUT_SIZE          3

/* ============== Vision System ============== */
#define VISION_FOV_DEGREES      180.0f
#define VISION_RAY_LENGTH       100.0f
#define VISION_RAY_LENGTH_SQ    (VISION_RAY_LENGTH * VISION_RAY_LENGTH)

/* ============== Colony Brain / Evolution ============== */
#define ELITE_COUNT             10
#define GENERATION_INTERVAL     (60 * 30)   /* frames (30 sec at 60 FPS) */
#define MUTATION_RATE           0.1f
#define MUTATION_STRENGTH       0.3f

/* ============== Speed Control ============== */
static const float SPEED_LEVELS[] = {0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f};
#define SPEED_LEVELS_COUNT      6
#define DEFAULT_SPEED_INDEX     2

#endif /* CONFIG_H */
