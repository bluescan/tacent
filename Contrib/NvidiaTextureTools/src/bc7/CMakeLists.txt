# tacent
project(bc7 VERSION 0.1.0)

include_directories(
    ..
    ../../project/vs
    ../../src
    ../../extern/poshlib
)

list (APPEND src
    avpcl_mode0.cpp
    avpcl_mode1.cpp
    avpcl_mode2.cpp
    avpcl_mode3.cpp
    avpcl_mode4.cpp
    avpcl_mode5.cpp
    avpcl_mode6.cpp
    avpcl_mode7.cpp
    avpcl_utils.cpp
    avpcl_utils.h
    avpcl.cpp
    avpcl.h
    bits.h
    endpts.h
    shapes_three.h
    shapes_two.h
    tile.h
)

find_package(Properties REQUIRED)
