# tacent
project(bc6h VERSION 0.1.0)

include_directories(
    ..
    ../../project/vs
    ../../src
    ../../extern/poshlib
)

list (APPEND src
    bits.h
    shapes_two.h
    tile.h
    zoh_utils.cpp
    zoh_utils.h
    zoh.cpp
    zoh.h
    zohone.cpp
    zohtwo.cpp
)

find_package(Properties REQUIRED)
