find_package("TacentProjectUtilities" REQUIRED)
project(Math VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_library(
	${PROJECT_NAME}
	Src/tColour.cpp
	Src/tGeometry.cpp
	Src/tLinearAlgebra.cpp
	Src/tRandom.cpp
	Src/tSpline.cpp
	Inc/Math/tColour.h
	Inc/Math/tGeometry.h
	Inc/Math/tInterval.h
	Inc/Math/tLinearAlgebra.h
	Inc/Math/tMatrix2.h
	Inc/Math/tMatrix4.h
	Inc/Math/tQuaternion.h
	Inc/Math/tRandom.h
	Inc/Math/tSpline.h
	Inc/Math/tVector2.h
	Inc/Math/tVector3.h
	Inc/Math/tVector4.h
)

tacent_target_include_directories(${PROJECT_NAME})
tacent_target_compile_definitions(${PROJECT_NAME})
tacent_target_compile_options(${PROJECT_NAME})
tacent_target_compile_features(${PROJECT_NAME})
tacent_set_target_properties(${PROJECT_NAME})

# Public means dependent interfaces are propagated. Private means they are not. Here we are saying the
# Tacent Math module depends-on/requires the Foundation module.
target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
		Foundation
)

tacent_install(${PROJECT_NAME})
