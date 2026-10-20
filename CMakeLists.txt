cmake_minimum_required(VERSION 3.16)
project(snes-ds C)

set(CMAKE_C_STANDARD 11)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
# -O3 and LTO: about 6% faster than -O2 on the emulation (tools/harness bench)
set(SNESDS_OPT "-O3" CACHE STRING "Optimisation flags for Release builds")
set(CMAKE_C_FLAGS_RELEASE "${SNESDS_OPT}")
option(SNESDS_LTO "Link-time optimisation" ON)
if(SNESDS_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# on Windows the .exe files carry everything (SDL2, winpthread, the C
# runtime): MSYS2's DLLs are only on the PATH inside its own shells
if(WIN32)
	add_link_options(-static)
	# the process' code page is UTF-8, so file names with accents work
	enable_language(RC)
	set(SNESDS_WIN_RC ${CMAKE_SOURCE_DIR}/cmake/win/utf8.rc)
endif()

# LakeSnes (MIT), with the snes-ds patches listed in third_party/lakesnes/PATCHES.md
file(GLOB LAKESNES_SRC third_party/lakesnes/*.c)
add_library(lakesnes STATIC ${LAKESNES_SRC})
target_include_directories(lakesnes PUBLIC third_party/lakesnes)
target_compile_options(lakesnes PRIVATE -w)
find_package(Threads REQUIRED)
target_link_libraries(lakesnes PUBLIC Threads::Threads)

# the games' profiles: games/games.c lists them, one directory each
file(GLOB GAME_SRC games/games.c games/*/*.c)
# kuba/zip + miniz (public domain), for ROMs in .zip files
add_library(zip STATIC third_party/zip/zip.c)
target_include_directories(zip PUBLIC third_party/zip)
target_compile_options(zip PRIVATE -w)

add_library(snesds_core STATIC src/core.c src/profiles.c src/mapcap.c src/screen2.c src/romfile.c src/library.c
	src/settings.c src/i18n.c src/fileutil.c ${GAME_SRC})
target_include_directories(snesds_core PUBLIC src)
target_link_libraries(snesds_core PRIVATE zip)
find_package(Threads REQUIRED)
target_link_libraries(snesds_core PUBLIC lakesnes Threads::Threads)
target_compile_options(snesds_core PRIVATE -Wall -Wextra)

add_executable(harness tools/harness.c ${SNESDS_WIN_RC})
target_include_directories(harness PRIVATE third_party)
target_link_libraries(harness PRIVATE snesds_core)
target_compile_options(harness PRIVATE -Wall)

# the app (SDL2); without SDL2 only the harness is built
find_package(SDL2 QUIET)
if(NOT SDL2_FOUND)
	message(STATUS "SDL2 not found: building the harness only")
	return()
endif()
file(READ VERSION SNESDS_VERSION)
string(STRIP "${SNESDS_VERSION}" SNESDS_VERSION)
add_library(stb STATIC src/stb_impl.c)
target_include_directories(stb PUBLIC third_party)
target_compile_options(stb PRIVATE -w)
add_executable(snes-ds src/main.c src/video.c src/pad.c src/game.c src/ui/gfx.c src/ui/ui.c ${SNESDS_WIN_RC})
target_compile_definitions(snes-ds PRIVATE SNESDS_VERSION="${SNESDS_VERSION}")
target_include_directories(snes-ds PRIVATE src/ui)
if(TARGET SDL2::SDL2main)
	target_link_libraries(snes-ds PRIVATE SDL2::SDL2main)
endif()
target_include_directories(snes-ds PRIVATE third_party)
if(WIN32 AND TARGET SDL2::SDL2-static)
	target_link_libraries(snes-ds PRIVATE snesds_core stb SDL2::SDL2-static)
else()
	target_link_libraries(snes-ds PRIVATE snesds_core stb SDL2::SDL2 m)
endif()
target_compile_options(snes-ds PRIVATE -Wall -Wextra)
# the fonts go beside the program
add_custom_command(TARGET snes-ds POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:snes-ds>/assets)
# and each game's icon, as assets/games/<id>.png
file(GLOB GAME_ICONS ${CMAKE_SOURCE_DIR}/games/*/icon.png)
foreach(icon ${GAME_ICONS})
	get_filename_component(dir ${icon} DIRECTORY)
	get_filename_component(id ${dir} NAME)
	add_custom_command(TARGET snes-ds POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:snes-ds>/assets/games
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${icon} $<TARGET_FILE_DIR:snes-ds>/assets/games/${id}.png)
endforeach()
