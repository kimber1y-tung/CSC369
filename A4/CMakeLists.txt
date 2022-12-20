cmake_minimum_required(VERSION 3.20)
project(group_0648 C)

set(CMAKE_C_STANDARD 11)

include_directories(A4/out/inc)
include_directories(A4/out/src)

add_executable(group_0648
        A4/out/inc/ext2umfs.h
        A4/out/src/e2fs.c
        A4/out/src/e2fs.h
        A4/out/src/ext2.h
        A4/out/src/ext2fsal.c
        A4/out/src/ext2fsal.h
        A4/out/src/ext2fsal_cp.c
        A4/out/src/ext2fsal_ln_hl.c
        A4/out/src/ext2fsal_ln_sl.c
        A4/out/src/ext2fsal_mkdir.c
        A4/out/src/ext2fsal_rm.c)
