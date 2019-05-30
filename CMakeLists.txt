cmake_minimum_required(VERSION 3.14)
project(testeC C)

set(CMAKE_C_STANDARD 99)

set(GCC_COVERAGE_COMPILE_FLAGS "-lSDL2 -lSDL2_ttf -lpthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

#server
add_executable(testeC  UI_library.c UI_library.h board_library.c board_library.h Server.c gameConnection.c gameConnection.h )

#client
#add_executable(testeC   UI_library.c UI_library.h board_library.c board_library.h gameConnection.c gameConnection.h Client.c)

#MEGABOT
#add_executable(testeC  UI_library.c UI_library.h board_library.c board_library.h MegaBot.c gameConnection.c gameConnection.h )

