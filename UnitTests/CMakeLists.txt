find_package("TacentProjectUtilities" REQUIRED)
project(UnitTests VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_executable(
	${PROJECT_NAME}
	Src/TestPipeline.cpp
	Src/TestFoundation.cpp
	Src/TestImage.cpp
	Src/TestMath.cpp
	Src/TestScene.cpp
	Src/TestSystem.cpp
	Src/UnitTests.cpp
	Src/TestPipeline.h
	Src/TestFoundation.h
	Src/TestImage.h
	Src/TestMath.h
	Src/TestScene.h
	Src/TestSystem.h
	Src/UnitTests.h
)

tacent_target_include_directories(${PROJECT_NAME})
tacent_target_compile_definitions(${PROJECT_NAME})
tacent_target_compile_options(${PROJECT_NAME})
tacent_target_compile_features(${PROJECT_NAME})
tacent_set_target_properties(${PROJECT_NAME})

target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
		Foundation Math System Image Pipeline Scene # shlwapi.lib Dbghelp.lib
)

target_include_directories(
	"${PROJECT_NAME}"
	PRIVATE
		$<TARGET_PROPERTY:Foundation,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:Math,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:System,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:Pipeline,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:Scene,INTERFACE_INCLUDE_DIRECTORIES>
)

# Installation.
install(
	TARGETS ${PROJECT_NAME}
	# EXPORT ${PROJECT_NAME}-targets
	RUNTIME DESTINATION "${CMAKE_BINARY_DIR}/Install"
)
install(DIRECTORY TestData/ DESTINATION "${CMAKE_BINARY_DIR}/Install/TestData")
