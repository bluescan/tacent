find_package("TacentProjectUtilities" REQUIRED)
project(Image VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_library(
	${PROJECT_NAME}
	Src/tCubemap.cpp
#	Src/tImageDDS.cpp
#	Src/tImageEXR.cpp
#	Src/tImageGIF.cpp
#	Src/tImageHDR.cpp
#	Src/tImageICO.cpp
#	Src/tImageTGA.cpp
#	Src/tImageWEBP.cpp
#	Src/tLayer.cpp
#	Src/tPicture.cpp
#	Src/tPixelFormat.cpp
#	Src/tTexture.cpp
#	Inc/Image/tCubemap.h
#	Inc/Image/tImageDDS.h
#	Inc/Image/tImageEXR.h
#	Inc/Image/tImageGIF.h
#	Inc/Image/tImageHDR.h
#	Inc/Image/tImageICO.h
#	Inc/Image/tImageTGA.h
#	Inc/Image/tImageWEBP.h
#	Inc/Image/tLayer.h
#	Inc/Image/tPicture.h
#	Inc/Image/tPixelFormat.h
#	Inc/Image/tTexture.h
)

tacent_target_include_directories(${PROJECT_NAME})
tacent_target_compile_definitions(${PROJECT_NAME})
tacent_target_compile_options(${PROJECT_NAME})
tacent_target_compile_features(${PROJECT_NAME})
tacent_set_target_properties(${PROJECT_NAME})

# Dependencies.
target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
		Foundation Math System CxImage
)

tacent_install(${PROJECT_NAME})
