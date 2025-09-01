find_package("TacentProjectUtilities" REQUIRED)
project(InputVis VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_executable(
	${PROJECT_NAME}
	Src/InputVisualizer.h
	Src/InputVisualizer.cpp
)

option(TACENT_UTF16_API_CALLS "Build Tacent With UTF16 API Calls" Off)

tacent_target_include_directories(${PROJECT_NAME})
tacent_target_compile_definitions(${PROJECT_NAME})
tacent_target_compile_options(${PROJECT_NAME})
tacent_target_compile_features(${PROJECT_NAME})

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"	# Use multithreaded or multithreaded-debug runtime on windows.
)

target_link_libraries(${PROJECT_NAME} PUBLIC
	Foundation Math System Image Input
)

if (MSVC)
	# Why do we still need this?
	if (CMAKE_BUILD_TYPE MATCHES Debug)
		target_link_options(${PROJECT_NAME} PRIVATE "/NODEFAULTLIB:LIBCMT.lib")
	endif()
endif()

target_include_directories(
	"${PROJECT_NAME}"
	PRIVATE
		$<TARGET_PROPERTY:Foundation,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:Math,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:System,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:Image,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:Input,INTERFACE_INCLUDE_DIRECTORIES>
		${CMAKE_CURRENT_SOURCE_DIR}/Src
		${CMAKE_CURRENT_SOURCE_DIR}/../Contrib/imgui
		${CMAKE_CURRENT_SOURCE_DIR}/../Contrib/imgui/misc/cpp
		${CMAKE_CURRENT_SOURCE_DIR}/../Contrib/imgui/backends
		${CMAKE_CURRENT_SOURCE_DIR}/../Contrib/glad/include
		${CMAKE_CURRENT_SOURCE_DIR}/../Contrib/glfw/include
)

# Installation.
install(
	TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION "${CMAKE_BINARY_DIR}/TacentInstall"
)
# install(DIRECTORY TestData/ DESTINATION "${CMAKE_BINARY_DIR}/TacentInstall/TestData")
