find_package("TacentProjectUtilities" REQUIRED)
project(WoboqGen VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_executable(
	${PROJECT_NAME}
	Src/WoboqGen.cpp
)

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
		Foundation Math System

		# I'm a little surprised cmake doesn't do these for me on Linux. The odd thing is they are
		# unnecessary if using VSCode/Cmake Tools, even in Ubuntu. They are, however, needed when
		# building from bash.
		$<$<PLATFORM_ID:Linux>:m>
		$<$<PLATFORM_ID:Linux>:stdc++>
)

if (MSVC)
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
)
