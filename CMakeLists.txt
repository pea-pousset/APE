cmake_minimum_required(VERSION 2.8)
project(APE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(src
	clock.c
	engine.c
	eval.c
	main.c
	move.c
	position.c
	search.c
	transposition.c
	utils.c
	xboard.c
)
file(GLOB headers *.h)

add_executable(APE ${src} ${headers})
