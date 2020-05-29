project(png VERSION ${TacentVersion})

# TODO zlib will end up being a dependent library using target_link_libraries.
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
