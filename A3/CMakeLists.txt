cmake_minimum_required(VERSION 3.20)
project(group_0386 C)

set(CMAKE_C_STANDARD 11)

include_directories(A3)

add_executable(group_0386
        A3/arc.c
        A3/clock.c
        A3/fifo.c
        A3/lru.c
        A3/pagetable.c
        A3/pagetable.h
        A3/rand.c
        A3/sim.c
        A3/sim.h
        A3/swap.c)
