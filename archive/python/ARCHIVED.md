# ARCHIVED — Python / pygame implementation

**Status:** Archived on 2026-09-22. Not maintained. Do not add features here.

The active codebase is the C11 / SDL2 implementation at the repository root
(see the top-level `README.md`). This directory is kept for reference only:
the behaviour, tuning values, and UI ideas (neural-net viewer, stats graphs,
debug modes) are the design reference for the C port.

## Why it was archived

- Simulation and rendering were interleaved (every entity had both `update`
  and `draw`), which blocked headless/fast-forward evolution and testing.
- Pure-Python grids and O(n²) ant interactions capped the population well
  below what the C version handles at 60 FPS.
- Maintaining two implementations of the same simulation was not worth it.

## Getting the original, unmodified layout

The last commit where this code lived at the repository root is tagged:

```sh
git checkout python-final
pip install -r requirements.txt
python main.py
```

## Running it from this directory

```sh
cd archive/python
pip install -r requirements.txt
python main.py
```

Asset paths are relative to the working directory, so when run from here the
ant sprites and death-marker image won't be found and the code falls back to
its built-in circle rendering. Saves go to `archive/python/ant_saves/`.

## Contents

| Path | What it was |
| --- | --- |
| `main.py` | Game loop and keyboard controls |
| `src/` | Ant, colony, pheromone, vision, walls, neural net, evolution, UI, debug |
| `README.md` | The original project README (describes the old genetic-trait system in places) |
| `CODEBASE_ANALYSIS.md` | Performance analysis that motivated the move to C |
| `copilot-instructions.md` | Former `.github/copilot-instructions.md` |
| `ant_saves/` | Last saved colony state |
