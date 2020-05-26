# tacent
project(nvthread VERSION 0.1.0)

include_directories(
    ..
    ../../project/vs
    ../../src
    ../../extern/poshlib
    ../../extern/stb
)

list (APPEND src
    Atomic.h
    Event.cpp
    Event.h
    Mutex.cpp
    Mutex.h
    nvthread.cpp
    nvthread.h
    ParallelFor.cpp
    ParallelFor.h
    Thread.cpp
    Thread.h
    ThreadPool.cpp
    ThreadPool.h
)

find_package(Properties REQUIRED)
