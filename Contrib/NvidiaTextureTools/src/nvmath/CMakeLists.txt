# tacent
project(nvmath VERSION 0.1.0)

include_directories(
    ..
    ../../project/vs
    ../../src
    ../../extern/poshlib
    ../../extern/stb
)

list (APPEND src
    Box.h
    Box.inl
    Color.h
    Color.inl
    Fitting.cpp
    Fitting.h
    Half.cpp
    Half.h
    Matrix.h
    nvmath.h
    Plane.cpp
    Plane.h
    Plane.inl
    SimdVector_SSE.h
    SimdVector_VE.h
    SimdVector.h
    SphericalHarmonic.cpp
    SphericalHarmonic.h
    Vector.h
    Vector.inl
)

find_package(Properties REQUIRED)
