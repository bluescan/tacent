# tacent
project(nvtt VERSION 0.1.0)

add_subdirectory(squish)

include_directories(
    ..
    ../../project/vs
    ../../src
    ../../extern/poshlib
)

list (APPEND src
    BlockCompressor.cpp
    ClusterFit.cpp
    CompressionOptions.cpp
    CompressorDX10.cpp
    CompressorDX11.cpp
    CompressorDX9.cpp
    CompressorDXT1.cpp
    CompressorDXT5_RGBM.cpp
    CompressorRGB.cpp
    Context.cpp
    CubeSurface.cpp
    cuda/CudaCompressorDXT.cpp
    cuda/CudaUtils.cpp
    InputOptions.cpp
    nvtt_wrapper.cpp
    nvtt.cpp
    OptimalCompressDXT.cpp
    OutputOptions.cpp
    QuickCompressDXT.cpp
    SingleColorLookup.cpp
    Surface.cpp
    TaskDispatcher.cpp
    BlockCompressor.h
    ClusterFit.h
    CompressionOptions.h
    Compressor.h
    CompressorDX10.h
    CompressorDX11.h
    CompressorDX9.h
    CompressorDXT1.h
    CompressorDXT5_RGBM.h
    CompressorRGB.h
    Context.h
    CubeSurface.h
    cuda/CudaCompressorDXT.h
    cuda/CudaMath.h
    cuda/CudaUtils.h
    InputOptions.h
    nvtt_wrapper.h
    nvtt.h
    OptimalCompressDXT.h
    OutputOptions.h
    QuickCompressDXT.h
    SingleColorLookup.h
    Surface.h
    TaskDispatcher.h
)

# SQW verify these defines
add_definitions(-D_WINDOWS)
# add_definitions(-D_USRDLL)
add_definitions(-DNVTT_EXPORTS)
add_definitions(-DNVTT_SHARED)
# add_definitions(-DHAVE_CUDA)
add_definitions(-D__SSE2__)
add_definitions(-D__SSE__)
add_definitions(-D__MMX__)
add_definitions(-DNV_NO_ASSERT)

find_package(Properties REQUIRED)
