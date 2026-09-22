/**
 * @file input.c
 * @brief The key binding table
 */

#include "input.h"

static const Keybind bindings[] = {
    {ACTION_TOGGLE_PAUSE,     SDLK_SPACE,     "Space",  "Pause / resume"},
    {ACTION_STEP_ONCE,        SDLK_RIGHT,     "Right",  "Step one tick (while paused)"},
    {ACTION_SPEED_DOWN,       SDLK_COMMA,     ",",      "Slower"},
    {ACTION_SPEED_UP,         SDLK_PERIOD,    ".",      "Faster"},
    {ACTION_SPEED_MAX,        SDLK_SLASH,     "/",      "Unlimited speed (train fast)"},
    {ACTION_RESET,            SDLK_r,         "R",      "Restart with the same seed"},
    {ACTION_RESET_NEW_SEED,   SDLK_t,         "T",      "Restart with a new seed"},
    {ACTION_NEW_MAZE,         SDLK_m,         "M",      "Regenerate the walls"},
    {ACTION_TOGGLE_TRAILS,    SDLK_p,         "P",      "Show pheromone trails"},
    {ACTION_TOGGLE_DANGER,    SDLK_o,         "O",      "Show danger pheromone"},
    {ACTION_TOGGLE_GRID,      SDLK_g,         "G",      "Show grid"},
    {ACTION_TOGGLE_MARKERS,   SDLK_k,         "K",      "Show death markers"},
    {ACTION_TOGGLE_VISION,    SDLK_v,         "V",      "Show vision rays of the selected ant"},
    {ACTION_TOGGLE_BRAIN,     SDLK_b,         "B",      "Switch brain: neural / classic"},
    {ACTION_TOOL_SELECT,      SDLK_1,         "1",      "Tool: select ants"},
    {ACTION_TOOL_FOOD,        SDLK_2,         "2",      "Tool: place food"},
    {ACTION_FOLLOW_SELECTED,  SDLK_f,         "F",      "Follow the selected ant"},
    {ACTION_DESELECT,         SDLK_ESCAPE,    "Esc",    "Deselect (or quit if nothing selected)"},
    {ACTION_FIT_CAMERA,       SDLK_h,         "H",      "Fit the world to the window"},
    {ACTION_TOGGLE_PANELS,    SDLK_TAB,       "Tab",    "Show / hide panels"},
    {ACTION_TOGGLE_FULLSCREEN, SDLK_F11,      "F11",    "Fullscreen"},
    {ACTION_QUIT,             SDLK_q,         "Q",      "Quit"},
};

const Keybind *input_keybinds(int *count) {
    if (count) *count = (int)(sizeof(bindings) / sizeof(bindings[0]));
    return bindings;
}

Action input_action_for_key(SDL_Keycode key) {
    int count = 0;
    const Keybind *binds = input_keybinds(&count);
    for (int i = 0; i < count; i++) {
        if (binds[i].key == key) return binds[i].action;
    }
    return ACTION_NONE;
}
