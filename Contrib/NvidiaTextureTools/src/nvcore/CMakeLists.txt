# tacent
cmake_minimum_required(VERSION 3.15.0)
project(nvcore VERSION 0.1.0)

include_directories(
    ..
    ../../project/vs
    ../../src
    ../../extern/poshlib
)

list (APPEND src
    nvcore.h
    Array.h
    Debug.h Debug.cpp
    DefsGnucDarwin.h
    DefsGnucLinux.h
    DefsGnucWin32.h
    DefsVcWin32.h
    FileSystem.h FileSystem.cpp
    ForEach.h
    Memory.h Memory.cpp
    Ptr.h
    RefCounted.h
    StrLib.h StrLib.cpp
    Stream.h
    StdStream.h
    TextWriter.h TextWriter.cpp
    Timer.h Timer.cpp
    Utils.h
)

find_package(Properties REQUIRED)
