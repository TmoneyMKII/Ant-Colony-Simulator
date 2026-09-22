cmake_minimum_required(VERSION 3.16)
project(AntColonySimulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags for optimization
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native -ffast-math -DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
elseif(MSVC)
    set(CMAKE_C_FLAGS_RELEASE "/O2 /fp:fast /DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "/Od /W4 /Zi")
endif()

# Find SDL2
find_package(SDL2 REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${SDL2_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/main.c
    src/ant.c
    src/colony.c
    src/neural_net.c
    src/pheromone.c
    src/vision.c
    src/walls.c
    src/render.c
    src/utils.c
)

# Create executable
add_executable(ant_simulator ${SOURCES})

# Link libraries
target_link_libraries(ant_simulator ${SDL2_LIBRARIES} m)

# Windows-specific: copy SDL2.dll to build directory
if(WIN32)
    add_custom_command(TARGET ant_simulator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:ant_simulator>
    )
endif()

# Installation
install(TARGETS ant_simulator DESTINATION bin)
