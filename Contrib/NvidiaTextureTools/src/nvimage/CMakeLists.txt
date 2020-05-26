# tacent
project(nvimage VERSION 0.1.0)

include_directories(
    ..
    ../../project/vs
    ../../src
    ../../extern/poshlib
    ../../extern/stb
)

list (APPEND src
    #KtxFile.cpp
    #KtxFile.h
    BlockDXT.cpp
    BlockDXT.h
    ColorBlock.cpp
    ColorBlock.h
    DirectDrawSurface.cpp
    DirectDrawSurface.h
    ErrorMetric.cpp
    ErrorMetric.h
    Filter.cpp
    Filter.h
    FloatImage.cpp
    FloatImage.h
    Image.cpp
    Image.h
    ImageIO.cpp
    ImageIO.h
    NormalMap.cpp
    NormalMap.h
    nvimage.h
    PixelFormat.h
    PsdFile.h
    TgaFile.h
)

find_package(Properties REQUIRED)
