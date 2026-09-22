# Ant Colony Simulator - working notes

C11 + SDL2 ant colony simulation. The Python/pygame version in `archive/python/`
is **archived**: read it for reference, never add features there.

## Architecture

```
World (src/core/world.c)
  Nest, FoodSource[], DeathMarker[]        entities
  Ant[]          (ant.c)                   sense -> steer -> move -> interact
    Genome + NNActivations (neural_net.c)  27 -> 16 -> 3, weights stored inline
    VisionRay[7]  (vision.c)               wall / ant / food closeness per ray
  PheromoneMap   (pheromone.c)             food, home, danger grids
  WallSet        (walls.c)                 rects + broadphase grid
  SpatialHash    (spatial_hash.c)          ant positions, rebuilt each tick
  Evolution      (evolution.c)             elites, crossover, mutation
  WorldStats                               rolling samples + per-generation history
```

The frontend (`src/render`, `src/ui`, `src/app`) reads the World and never
owns simulation state.

## Rules that keep the layers apart

- **`src/core` must not include SDL or any UI header.** It is linked into the
  tests and `antsim_headless`, both of which run with no window. If something
  in core needs a colour or a pixel size, it belongs in the renderer instead.
- **Ants do not mutate colony state.** `ant_update()` returns an `AntOutcome`
  (`ANT_DELIVERED`, `ANT_DIED_STARVED`, …) and `world_step()` applies the
  consequence: nest stores, stats, death markers, danger pheromone, respawn.
- **Randomness comes from `w->rng`**, never a global. Each World owns its
  stream so a seed replays exactly, which the determinism test relies on.
  Anything that draws numbers takes an `Rng *`.
- **Tunables go in `SimConfig`** (`sim_config.h`), not new `#define`s, if a
  user might reasonably want to change them while running. Add the field, a
  clamp in `sim_config_sanitize()`, and a slider in the Parameters section.
  `config.h` is for structural constants only (array sizes, network shape).

## Adding things

**A simulation parameter**: field in `SimConfig` → default in
`sim_config_defaults()` → clamp in `sim_config_sanitize()` → slider in
`section_parameters()` (`src/ui/ui_panels.c`). Live parameters are copied into
the world every frame by `sync_config()` in `app.c`.

**A keyboard shortcut**: add an `Action` and a row in the table in
`src/app/input.c`, then handle it in `app_do_action()`. The help panel is
generated from that table, so nothing else needs updating. UI buttons should
call `app_do_action()` rather than duplicating the behaviour.

**A panel or widget**: `src/ui/ui_panels.c`. microui only draws rectangles,
text and icons; `ui_draw_line()` and `ui_draw_disc()` (`src/ui/ui.c`) add
clipped line and circle commands for graphs and the network diagram.

## microui gotchas

- A negative row width means "extend to this far from the right edge", so a
  row of `-1`s stacks controls on top of each other. Use `row_split()`.
- Control ids hash the *pointer*, so two sliders sharing a stack temporary get
  the same id. Wrap those in `mu_push_id(label)` / `mu_pop_id()` — that is what
  `toggle()` and `slider_int()` do.
- A window's rect is only applied when its container is first created. To move
  or resize one later, write to `mu_get_container(ctx, name)->rect`.

## Rendering notes

Ants, food and death markers go through one `SDL_RenderGeometry` call each,
against sprites baked at startup (`bake_sprite()`). Pheromones upload as a
single texture, one pixel per grid cell, stretched with linear filtering.
Adding a per-entity `SDL_RenderCopy` loop would undo that.

The app sets Windows DPI awareness before `SDL_Init`. Do not add
`SDL_WINDOW_ALLOW_HIGHDPI`: it makes renderer pixels differ from the window
coordinates in mouse events, which offsets every click.

## Build and check

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/antsim_headless --ants 500 --ticks 2000 --bench   # timing
```

`tests/test_core.c` covers the broadphase, walls, pheromones, the network,
evolution, and the world (determinism, bounds, generations, reset). Behaviour
changes to the core should come with a test there; it needs no display.
