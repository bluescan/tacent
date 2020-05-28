find_package("TacentProjectUtilities" REQUIRED)
project(Scene VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_library(
	${PROJECT_NAME}
	Src/tAttribute.cpp
	Src/tCamera.cpp
	Src/tInstance.cpp
	Src/tLight.cpp
	Src/tLodGroup.cpp
	Src/tMaterial.cpp
	Src/tMesh.cpp
	Src/tObject.cpp
	Src/tPath.cpp
	Src/tPolyModel.cpp
	Src/tSelection.cpp
	Src/tSkeleton.cpp
	Src/tWorld.cpp
	Inc/Scene/tAttribute.h
	Inc/Scene/tCamera.h
	Inc/Scene/tInstance.h
	Inc/Scene/tLight.h
	Inc/Scene/tLodGroup.h
	Inc/Scene/tMaterial.h
	Inc/Scene/tMesh.h
	Inc/Scene/tObject.h
	Inc/Scene/tPath.h
	Inc/Scene/tPolyModel.h
	Inc/Scene/tSelection.h
	Inc/Scene/tSkeleton.h
	Inc/Scene/tWorld.h
)

tacent_target_include_directories(${PROJECT_NAME})
tacent_target_compile_definitions(${PROJECT_NAME})
tacent_target_compile_options(${PROJECT_NAME})
tacent_target_compile_features(${PROJECT_NAME})
tacent_set_target_properties(${PROJECT_NAME})

# Note that since Math happens to also depend on Foundation, we could have left Foundation out and it would work.
target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
		Foundation Math System
)

tacent_install(${PROJECT_NAME})
