find_package("TacentProjectUtilities" REQUIRED)
project(Image VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_library(
	${PROJECT_NAME}
	Src/tCubemap.cpp
	Src/tImageDDS.cpp
	Src/tImageEXR.cpp
	Src/tImageGIF.cpp
	Src/tImageHDR.cpp
	Src/tImageICO.cpp
	Src/tImageTGA.cpp
	Src/tImageJPG.cpp
	Src/tImageWEBP.cpp
	Src/tImageXPM.cpp
	Src/tLayer.cpp
	Src/tPicture.cpp
	Src/tPixelFormat.cpp
	Src/tTexture.cpp
	Inc/Image/tCubemap.h
	Inc/Image/tImageDDS.h
	Inc/Image/tImageEXR.h
	Inc/Image/tImageGIF.h
	Inc/Image/tImageHDR.h
	Inc/Image/tImageICO.h
	Inc/Image/tImageTGA.h
	Inc/Image/tImageJPG.h
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
	"${PROJECT_NAME}" PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Contrib
	${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/include
	${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/include/OpenEXR
)

tacent_target_compile_definitions(${PROJECT_NAME})
tacent_target_compile_options(${PROJECT_NAME})
tacent_target_compile_features(${PROJECT_NAME})
tacent_set_target_properties(${PROJECT_NAME})

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

		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/debug/Half.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/Half.lib>
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/debug/Iex.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/Iex.lib>
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/debug/IexMath.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/IexMath.lib>
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/debug/IlmImf.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/IlmImf.lib>
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/debug/IlmThread.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/IlmThread.lib>
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/debug/Imath.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/Imath.lib>
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/debug/zlibstatic.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/zlibstatic.lib>

		// Only linking to release libs in Linux.
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/libIlmImf.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/libHalf.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/libIex.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/libIexMath.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/libIlmThread.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/libImath.a>
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/Contrib/OpenEXR/lib/release/libz.a>
)

target_include_directories(
	"${PROJECT_NAME}"
	PRIVATE
		$<TARGET_PROPERTY:CxImage,INTERFACE_INCLUDE_DIRECTORIES>
)

tacent_install(${PROJECT_NAME})
