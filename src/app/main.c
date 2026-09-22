/**
 * @file main.c
 * @brief Entry point
 *
 * Everything interesting lives in app.c (loop and input), render.c
 * (drawing), ui_panels.c (panels) and src/core (the simulation itself).
 */

#include <stdio.h>
#include "app.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    static App app;
    if (!app_init(&app)) {
        app_shutdown(&app);
        return 1;
    }

    printf("Ant Colony Simulator - seed %u, world %dx%d, %d ants\n",
           app.world->seed, app.world->width, app.world->height, app.world->ant_count);
    printf("Press Tab to hide the panels, Q or Esc to quit.\n");

    app_run(&app);
    app_shutdown(&app);
    return 0;
}
