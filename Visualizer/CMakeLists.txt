find_package("TacentProjectUtilities" REQUIRED)
project(Visualizer VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_executable(
	${PROJECT_NAME}
	Src/Visualizer.h
	Src/Visualizer.cpp
	../Contrib/imgui/imgui.cpp
	../Contrib/imgui/imgui_demo.cpp
	../Contrib/imgui/imgui_draw.cpp
	../Contrib/imgui/imgui_tables.cpp
	../Contrib/imgui/imgui_widgets.cpp
	../Contrib/imgui/misc/cpp/imgui_stdlib.cpp
	../Contrib/imgui/backends/imgui_impl_glfw.cpp
	../Contrib/imgui/backends/imgui_impl_opengl2.cpp
	../Contrib/glad/src/glad.c
	$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Windows/Visualizer.rc>
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

target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
		Foundation Math System Image Input

		$<$<PLATFORM_ID:Windows>:Dbghelp.lib>
		$<$<PLATFORM_ID:Windows>:opengl32.lib>
		$<$<PLATFORM_ID:Windows>:uxtheme.lib>
		$<$<PLATFORM_ID:Windows>:dwmapi.lib>
		$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/../Contrib/glfw/glfw3.lib>

		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/../Contrib/glfw/libglfw3.a>
		$<$<PLATFORM_ID:Linux>:dl>
		$<$<PLATFORM_ID:Linux>:X11>
		$<$<PLATFORM_ID:Linux>:xcb>		# For clipboard support.
)

if (MSVC)
	target_link_options(
		${PROJECT_NAME}
		PRIVATE
		$<IF:$<BOOL:${TACENT_UTF16_API_CALLS}>,/ENTRY:wmainCRTStartup,/ENTRY:mainCRTStartup>
		#"/SUBSYSTEM:CONSOLE"
		#"/SUBSYSTEM:WINDOWS"
	)
	if (CMAKE_BUILD_TYPE MATCHES Debug)
		target_link_options(
			${PROJECT_NAME}
			PRIVATE "/NODEFAULTLIB:LIBCMT.lib"
		)
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

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
		ARCHITECTURE_X64
		GLFW_INCLUDE_NONE
		$<$<PLATFORM_ID:Linux>:HAVE_PNG_H>
		$<$<CONFIG:Debug>:CONFIG_DEBUG>
		$<$<CONFIG:Release>:CONFIG_RELEASE>
		$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_DEPRECATE>

		$<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS>
		$<$<PLATFORM_ID:Linux>:PLATFORM_LINUX>

		# These shouldn't actually be necessary as there are no direct Windows API calls
		# in TacentView (they are abstracted away by the Tacent libraries). But just in case
		# anything in the viewer were to call an OS-level function, these enable the UTF-16
		# versions if the TACENT_UTF16_API_CALLS option is set.
		$<$<AND:$<PLATFORM_ID:Windows>,$<BOOL:${TACENT_UTF16_API_CALLS}>>:UNICODE>	# C++	UFF-16
		$<$<AND:$<PLATFORM_ID:Windows>,$<BOOL:${TACENT_UTF16_API_CALLS}>>:_UNICODE>	# C		UTF-16
		$<$<AND:$<PLATFORM_ID:Windows>,$<BOOL:${TACENT_UTF16_API_CALLS}>>:TACENT_UTF16_API_CALLS>
)

# Installation.
#install(
#	TARGETS ${PROJECT_NAME}
#	RUNTIME DESTINATION "${CMAKE_BINARY_DIR}/TacentInstall"
#)
install(DIRECTORY Assets/ DESTINATION "${CMAKE_BINARY_DIR}/Visualizer/Assets")
