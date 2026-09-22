/**
 * @file input.h
 * @brief Named actions and the key bindings that trigger them
 *
 * Keys, UI buttons and the help panel all go through this one table, so a
 * binding never drifts from the shortcut list shown to the user.
 */

#ifndef INPUT_H
#define INPUT_H

#include <SDL.h>

typedef enum {
    ACTION_NONE = 0,
    ACTION_QUIT,
    ACTION_TOGGLE_PAUSE,
    ACTION_STEP_ONCE,
    ACTION_SPEED_UP,
    ACTION_SPEED_DOWN,
    ACTION_SPEED_MAX,
    ACTION_RESET,
    ACTION_RESET_NEW_SEED,
    ACTION_NEW_MAZE,
    ACTION_TOGGLE_TRAILS,
    ACTION_TOGGLE_DANGER,
    ACTION_TOGGLE_GRID,
    ACTION_TOGGLE_MARKERS,
    ACTION_TOGGLE_VISION,
    ACTION_TOGGLE_BRAIN,
    ACTION_TOGGLE_PANELS,
    ACTION_TOGGLE_FULLSCREEN,
    ACTION_FIT_CAMERA,
    ACTION_FOLLOW_SELECTED,
    ACTION_DESELECT,
    ACTION_TOOL_SELECT,
    ACTION_TOOL_FOOD,
    ACTION_COUNT
} Action;

typedef struct {
    Action action;
    SDL_Keycode key;
    const char *key_label;      /**< How the shortcut is written in the UI */
    const char *description;
} Keybind;

/** @brief The binding table (used for dispatch and for the help panel) */
const Keybind *input_keybinds(int *count);

/** @brief Action bound to a key, or ACTION_NONE */
Action input_action_for_key(SDL_Keycode key);

#endif /* INPUT_H */
