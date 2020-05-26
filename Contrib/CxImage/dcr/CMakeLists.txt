# tacent
project(dcr VERSION 0.1.0)

list (APPEND src 
    libdcr.c
    libdcr.h
)

find_package(Properties REQUIRED)
