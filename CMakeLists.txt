cmake_minimum_required(VERSION 3.16)
project(AntColonySimulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler flags for optimization
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
    set(ANTSIM_WARNINGS -Wall -Wextra -Wpedantic)
elseif(MSVC)
    set(CMAKE_C_FLAGS_RELEASE "/O2 /fp:fast /DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "/Od /Zi")
    set(ANTSIM_WARNINGS /W4)
endif()

option(ANTSIM_BUILD_APP   "Build the SDL2 application"        ON)
option(ANTSIM_BUILD_TESTS "Build the simulation core tests"   ON)

# ============== Simulation core (no SDL) ==============

add_library(antsim_core STATIC
    src/core/ant.c
    src/core/evolution.c
    src/core/neural_net.c
    src/core/pheromone.c
    src/core/sim_config.c
    src/core/spatial_hash.c
    src/core/utils.c
    src/core/vision.c
    src/core/walls.c
    src/core/world.c
)
target_include_directories(antsim_core PUBLIC src/core)
target_compile_options(antsim_core PRIVATE ${ANTSIM_WARNINGS})
if(NOT MSVC)
    target_link_libraries(antsim_core PUBLIC m)
endif()

# Headless runner: fast-forward evolution and benchmarking
add_executable(antsim_headless src/app/headless.c)
target_link_libraries(antsim_headless PRIVATE antsim_core)
target_compile_options(antsim_headless PRIVATE ${ANTSIM_WARNINGS})

# ============== Tests ==============

if(ANTSIM_BUILD_TESTS)
    enable_testing()
    add_executable(antsim_tests tests/test_core.c)
    target_link_libraries(antsim_tests PRIVATE antsim_core)
    target_compile_options(antsim_tests PRIVATE ${ANTSIM_WARNINGS})
    add_test(NAME core COMMAND antsim_tests)
endif()

# ============== Application (SDL2 + microui) ==============

if(ANTSIM_BUILD_APP)
    include(FetchContent)

    FetchContent_Declare(SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-2.32.8
        GIT_SHALLOW    TRUE
    )
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON  CACHE BOOL "" FORCE)
    set(SDL_TEST   OFF CACHE BOOL "" FORCE)

    # microui and stb ship no CMakeLists; pointing SOURCE_SUBDIR at a path
    # that has none makes MakeAvailable download them without configuring.
    FetchContent_Declare(microui
        GIT_REPOSITORY https://github.com/rxi/microui.git
        GIT_TAG        0850aba860959c3e75fb3e97120ca92957f9d057
        SOURCE_SUBDIR  no-cmake-here
    )
    FetchContent_Declare(stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG        2c980bb59875b0d32144a71867fbdebb2f77cd20
        SOURCE_SUBDIR  no-cmake-here
    )

    FetchContent_MakeAvailable(SDL2 microui stb)

    add_library(microui STATIC ${microui_SOURCE_DIR}/src/microui.c)
    target_include_directories(microui PUBLIC ${microui_SOURCE_DIR}/src)

    add_executable(ant_simulator
        src/app/main.c
        src/app/app.c
        src/app/input.c
        src/render/render.c
        src/render/font.c
        src/ui/ui.c
        src/ui/ui_panels.c
    )
    target_include_directories(ant_simulator PRIVATE src/app src/render src/ui)
    target_include_directories(ant_simulator SYSTEM PRIVATE ${stb_SOURCE_DIR})
    target_link_libraries(ant_simulator PRIVATE antsim_core microui SDL2::SDL2main SDL2::SDL2-static)
    target_compile_options(ant_simulator PRIVATE ${ANTSIM_WARNINGS})
    target_compile_definitions(ant_simulator PRIVATE
        ANTSIM_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")

    # Ship assets next to the executable so it runs from anywhere
    add_custom_command(TARGET ant_simulator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts
            $<TARGET_FILE_DIR:ant_simulator>/assets/fonts
    )

    install(TARGETS ant_simulator DESTINATION bin)
    install(DIRECTORY assets/fonts DESTINATION bin/assets)
endif()
