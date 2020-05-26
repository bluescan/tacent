# tacent
project(mng VERSION 0.1.0)

include_directories(
    ../jpeg
    ../png
    ../zlib
    ../mng
    ../tiff
    ../j2k
    ../jasper/include
)

list (APPEND src
    libmng_callback_xs.c
    libmng_chunk_descr.c
    libmng_chunk_io.c
    libmng_chunk_prc.c
    libmng_chunk_xs.c
    libmng_cms.c
    libmng_display.c
    libmng_dither.c
    libmng_error.c
    libmng_filter.c
    libmng_hlapi.c
    libmng_jpeg.c
    libmng_object_prc.c
    libmng_pixels.c
    libmng_prop_xs.c
    libmng_read.c
    libmng_trace.c
    libmng_write.c
    libmng_zlib.c
    libmng_chunk_descr.h
    libmng_chunk_io.h
    libmng_chunk_prc.h
    libmng_chunks.h
    libmng_cms.h
    libmng_conf.h
    libmng_data.h
    libmng_display.h
    libmng_dither.h
    libmng_error.h
    libmng_filter.h
    libmng_jpeg.h
    libmng_memory.h
    libmng_object_prc.h
    libmng_objects.h
    libmng_pixels.h
    libmng_read.h
    libmng_trace.h
    libmng_types.h
    libmng_write.h
    libmng_zlib.h
    libmng.h
)

add_definitions(-DMNG_SUPPORT_DISPLAY)
add_definitions(-DMNG_SUPPORT_READ)
add_definitions(-DMNG_SUPPORT_WRITE)
add_definitions(-DMNG_ACCESS_CHUNKS)
add_definitions(-DMNG_STORE_CHUNKS)

find_package(Properties REQUIRED)
