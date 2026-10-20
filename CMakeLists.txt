cmake_minimum_required(VERSION 3.16)
project(snes-ds C)

set(CMAKE_C_STANDARD 11)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_FLAGS_RELEASE "-O2")

# on Windows the .exe files carry everything (SDL2, winpthread, the C
# runtime): MSYS2's DLLs are only on the PATH inside its own shells
if(WIN32)
	add_link_options(-static)
endif()

# LakeSnes (MIT), with the snes-ds patches listed in third_party/lakesnes/PATCHES.md
file(GLOB LAKESNES_SRC third_party/lakesnes/*.c)
add_library(lakesnes STATIC ${LAKESNES_SRC})
target_include_directories(lakesnes PUBLIC third_party/lakesnes)
target_compile_options(lakesnes PRIVATE -w)

# the games' profiles: games/games.c lists them, one directory each
file(GLOB GAME_SRC games/games.c games/*/*.c)
add_library(snesds_core STATIC src/core.c src/profiles.c src/mapcap.c src/screen2.c ${GAME_SRC})
target_include_directories(snesds_core PUBLIC src)
find_package(Threads REQUIRED)
target_link_libraries(snesds_core PUBLIC lakesnes Threads::Threads)
target_compile_options(snesds_core PRIVATE -Wall -Wextra)

add_executable(harness tools/harness.c)
target_include_directories(harness PRIVATE third_party)
target_link_libraries(harness PRIVATE snesds_core)
target_compile_options(harness PRIVATE -Wall)

# the app (SDL2)
find_package(SDL2 REQUIRED)
add_executable(snes-ds src/main.c)
if(TARGET SDL2::SDL2main)
	target_link_libraries(snes-ds PRIVATE SDL2::SDL2main)
endif()
target_include_directories(snes-ds PRIVATE third_party)
if(WIN32 AND TARGET SDL2::SDL2-static)
	target_link_libraries(snes-ds PRIVATE snesds_core SDL2::SDL2-static)
else()
	target_link_libraries(snes-ds PRIVATE snesds_core SDL2::SDL2)
endif()
target_compile_options(snes-ds PRIVATE -Wall -Wextra)
