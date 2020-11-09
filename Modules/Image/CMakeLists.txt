find_package("TacentProjectUtilities" REQUIRED)
project(Image VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_subdirectory(Contrib/CxImage)

add_library(
	${PROJECT_NAME}
	Src/tCubemap.cpp
	Src/tImageAPNG.cpp
	Src/tImageDDS.cpp
	Src/tImageEXR.cpp
	Src/tImageGIF.cpp
	Src/tImageHDR.cpp
	Src/tImageICO.cpp
	Src/tImageJPG.cpp
	Src/tImagePNG.cpp
	Src/tImageTGA.cpp
	Src/tImageWEBP.cpp
	Src/tImageXPM.cpp
	Src/tLayer.cpp
	Src/tPicture.cpp
	Src/tPixelFormat.cpp
	Src/tTexture.cpp
	Inc/Image/tCubemap.h
	Inc/Image/tImageAPNG.h
	Inc/Image/tImageDDS.h
	Inc/Image/tImageEXR.h
	Inc/Image/tImageGIF.h
	Inc/Image/tImageHDR.h
	Inc/Image/tImageICO.h
	Inc/Image/tImageJPG.h
	Inc/Image/tImagePNG.h
	Inc/Image/tImageTGA.h
	Inc/Image/tImageWEBP.h
	Inc/Image/tImageXPM.h
	Inc/Image/tLayer.h
	Inc/Image/tPicture.h
	Inc/Image/tPixelFormat.h
	Inc/Image/tTexture.h
	
	# OpenEXR
	Contrib/OpenEXR/loadImage.h
	Contrib/OpenEXR/loadImage.cpp
)

tacent_target_include_directories(${PROJECT_NAME})
target_include_directories(
	"${PROJECT_NAME}"
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ZLib/include
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/include
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/include/OpenEXR
		${CMAKE_CURRENT_SOURCE_DIR}/Contrib/LibPNG/include
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
		CxImage

		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/WebP/Linux/libwebp.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/WebP/Linux/libwebpdemux.a>
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/WebP/Windows/release-static/x64/lib/libwebp.lib>
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/WebP/Windows/release-static/x64/lib/libwebpdemux.lib>
		
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/TurboJpeg/Linux/libturbojpeg.a>
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/TurboJpeg/Windows/turbojpeg-static.lib>

		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/LibPNG/lib/libpng16.a>
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/LibPNG/lib/libpng16.lib>

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

		# Link ZLib. Shared by a few different image formats.
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ZLib/lib/zlibstaticd.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ZLib/lib/zlibstatic.lib>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/ZLib/lib/libz.a>
)

tacent_install(${PROJECT_NAME})
