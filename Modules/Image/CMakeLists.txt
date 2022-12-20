find_package("TacentProjectUtilities" REQUIRED)
project(Image VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_library(
	${PROJECT_NAME}
	Src/tCubemap.cpp
	Src/tImageAPNG.cpp
	Src/tImageASTC.cpp
	Src/tImageBMP.cpp
	Src/tImageDDS.cpp
	Src/tImageEXR.cpp
	Src/tImageGIF.cpp
	Src/tImageHDR.cpp
	Src/tImageICO.cpp
	Src/tImageJPG.cpp
	Src/tImageKTX.cpp
	Src/tImagePNG.cpp
	Src/tImageQOI.cpp
	Src/tImageTGA.cpp
	Src/tImageTIFF.cpp
	Src/tImageWEBP.cpp
	Src/tImageXPM.cpp
	Src/tLayer.cpp
	Src/tMetaData.cpp
	Src/tPaletteImage.cpp
	Src/tPicture.cpp
	Src/tPixelFormat.cpp
	Src/tPixelUtil.cpp
	Src/tQuantizeSpatial.cpp
	Src/tQuantizeNeu.cpp
	Src/tTexture.cpp
	Src/tResample.cpp
	Inc/Image/tBaseImage.h
	Inc/Image/tCubemap.h
	Inc/Image/tFrame.h
	Inc/Image/tImageAPNG.h
	Inc/Image/tImageASTC.h
	Inc/Image/tImageBMP.h
	Inc/Image/tImageDDS.h
	Inc/Image/tImageEXR.h
	Inc/Image/tImageGIF.h
	Inc/Image/tImageHDR.h
	Inc/Image/tImageICO.h
	Inc/Image/tImageJPG.h
	Inc/Image/tImageKTX.h
	Inc/Image/tImagePNG.h
	Inc/Image/tImageQOI.h
	Inc/Image/tImageTGA.h
	Inc/Image/tImageTIFF.h
	Inc/Image/tImageWEBP.h
	Inc/Image/tImageXPM.h
	Inc/Image/tLayer.h
	Inc/Image/tMetaData.h
	Inc/Image/tPaletteImage.h
	Inc/Image/tPicture.h
	Inc/Image/tPixelFormat.h
	Inc/Image/tPixelUtil.h
	Inc/Image/tResample.h
	Inc/Image/tTexture.h
	
	# Contributed source including image loaders.
	Contrib/ApngAsm/apngasm.h
	Contrib/ApngAsm/apngasm.cpp
	Contrib/ApngDis/apngdis.h
	Contrib/ApngDis/apngdis.cpp
	Contrib/bcdec/bcdec.h
	Contrib/ASTCEncoder/include/astcenc.h
	Contrib/GifLoad/gif_load.h
	Contrib/GifSave/gif.h
	Contrib/LibKTX/include/ktx.h
	Contrib/LibPNG/include/png.h
	Contrib/LibTIFF/include/tiff.h
	Contrib/OpenEXR/loadImage.h
	Contrib/OpenEXR/loadImage.cpp
	Contrib/QOI/qoi.h
	Contrib/TinyEXIF/TinyEXIF.h
	Contrib/TinyEXIF/TinyEXIF.cpp
	Contrib/TinyXML2/tinyxml2.h
	Contrib/TinyXML2/tinyxml2.cpp
	Contrib/TurboJpeg/turbojpeg.h
	Contrib/WebP/include/mux.h
	Contrib/WebP/include/demux.h
	Contrib/WebP/include/encode.h
	Contrib/ZLib/include/zlib.h
)

tacent_target_include_directories(${PROJECT_NAME})
target_include_directories(
	"${PROJECT_NAME}"

	# PRIVATE is for building only. Populates INCLUDE_DIRECTORIES
	# INTERFACE is for users of the library. Populates INTERFACE_INCLUDE_DIRECTORIES
	# PUBLIC means both building and for users of the lib. Populates both.
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Contrib>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/TinyXML2>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ZLib/include>
		$<INSTALL_INTERFACE:Contrib>
		$<INSTALL_INTERFACE:Contrib/TinyXML2>
		$<INSTALL_INTERFACE:Contrib/ZLib/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ApngDis
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ApngAsm
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/bcdec
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ASTCEncoder/include
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/GifLoad
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/GifSave
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/LibKTX/include
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/LibPNG/include
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/LibTIFF/include
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/include
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/include/OpenEXR
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/TinyEXIF
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/TurboJpeg
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/TurboJpeg/Windows>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/TurboJpeg/Linux>
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/WebP/include
)

tacent_target_compile_definitions(${PROJECT_NAME})
tacent_target_compile_options(${PROJECT_NAME})
tacent_target_compile_features(${PROJECT_NAME})
tacent_set_target_properties(${PROJECT_NAME})
tacent_target_link_options(${PROJECT_NAME})

# Dependencies.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# @todo Setup a tacent_function (var arg) to allow easy specification of deps including dependent headers.
target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
		Foundation
		Math
		System
	PRIVATE
		Threads::Threads

		# LibWEBP
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/WebP/lib/libwebp.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/WebP/lib/libwebpdemux.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/WebP/lib/libwebpmux.a>
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/WebP/lib/libwebp.lib>
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/WebP/lib/libwebpdemux.lib>
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/WebP/lib/libwebpmux.lib>

		# ASTCEncoder
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Linux>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ASTCEncoder/lib/debug/libastcenc-sse2-static.a>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Linux>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ASTCEncoder/lib/release/libastcenc-sse2-static.a>
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ASTCEncoder/lib/debug/astcenc-sse2-static.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ASTCEncoder/lib/release/astcenc-sse2-static.lib>

		# LibKTX
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/LibKTX/lib/libktx.a>
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/LibKTX/lib/ktx.lib>

		# LibPNG
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/LibPNG/lib/libpng16.a>
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/LibPNG/lib/libpng16.lib>

		# LibEXR
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/Half-2_5_d.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/Half-2_5.lib>
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/Iex-2_5_d.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/Iex-2_5.lib>
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/IexMath-2_5_d.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/IexMath-2_5.lib>
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/IlmImf-2_5_d.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/IlmImf-2_5.lib>
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/IlmThread-2_5_d.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/IlmThread-2_5.lib>
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/Imath-2_5_d.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/Imath-2_5.lib>
		# Only linking to release libs in Linux.
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/libIlmImf-2_5.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/libHalf-2_5.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/libIex-2_5.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/libIexMath-2_5.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/libIlmThread-2_5.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/libImath-2_5.a>

		# ZLib. Shared by a few different image formats.
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ZLib/lib/zlibstaticd.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ZLib/lib/zlibstatic.lib>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ZLib/lib/libz.a>

		# LibTIFF.
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/LibTIFF/lib/libtiff.a>
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/LibTIFF/lib/tiff.lib>

		# LibTurboJPEG
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/TurboJpeg/Linux/libturbojpeg.a>
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/TurboJpeg/Windows/turbojpeg-static.lib>
)

tacent_install(${PROJECT_NAME})
