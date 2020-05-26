# tacent
project(squish VERSION 0.1.0)

list (APPEND src
    fastclusterfit.cpp
    fastclusterfit.h
    weightedclusterfit.cpp
    weightedclusterfit.h
    colourblock.cpp
    colourblock.h
    colourfit.cpp
    colourfit.h
    colourset.cpp
    colourset.h
    config.h
    maths.cpp
    maths.h
    simd.h
    simd_sse.h
    simd_ve.h
)

find_package(Properties REQUIRED)
