project(dcr VERSION ${TacentVersion})

list (APPEND src 
	libdcr.c
	libdcr.h
)

find_package(Properties REQUIRED)
