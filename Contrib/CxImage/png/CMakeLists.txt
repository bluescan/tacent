# tacent
project(png VERSION 0.1.0)

include_directories(
    ../zlib
)

list (APPEND src
    png.c
    pngerror.c
    pngget.c
    pngmem.c
    pngpread.c
    pngread.c
    pngrio.c
    pngrtran.c
    pngrutil.c
    pngset.c
    pngtrans.c
    pngwio.c
    pngwrite.c
    pngwtran.c
    pngwutil.c
    png.h
    pngconf.h
)

find_package(Properties REQUIRED)
