# tacent
project(zlib VERSION 0.1.0)

list (APPEND src
    adler32.c
    compress.c
    crc32.c
    crc32.h 
    deflate.c
    deflate.h 
    infback.c
    inffast.c
    inffast.h 
    inffixed.h 
    inflate.c
    inflate.h 
    inftrees.c
    inftrees.h 
    trees.c
    trees.h 
    uncompr.c
    zconf.h 
    zlib.h 
    zutil.c
    zutil.h 
)

find_package(Properties REQUIRED)
