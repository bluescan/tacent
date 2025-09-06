find_package("TacentProjectUtilities" REQUIRED)
project(UnitTests VERSION ${TACENT_VERSION} LANGUAGES CXX)
tacent_is_arch_arm(IsArm)

if (NOT IsArm)
	add_executable(
		${PROJECT_NAME}
		Src/UnitTests.h
		Src/UnitTests.cpp
		Src/TestFoundation.h
		Src/TestFoundation.cpp
		Src/TestMath.h
		Src/TestMath.cpp
		Src/TestSystem.h
		Src/TestSystem.cpp
		Src/TestImage.h
		Src/TestImage.cpp
		Src/TestScene.h
		Src/TestScene.cpp
		Src/TestPipeline.h
		Src/TestPipeline.cpp
		Src/TestInput.h
		Src/TestInput.cpp
	)
else()
	add_executable(
		${PROJECT_NAME}
		Src/TestFoundation.h
		Src/TestFoundation.cpp
		Src/TestMath.h
		Src/TestMath.cpp
		Src/TestSystem.h
		Src/TestSystem.cpp
		Src/UnitTests.h
		Src/UnitTests.cpp
	)
endif()

tacent_target_include_directories(${PROJECT_NAME})
tacent_target_compile_definitions(${PROJECT_NAME})
tacent_target_compile_options(${PROJECT_NAME})
tacent_target_compile_features(${PROJECT_NAME})

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"	# Use multithreaded or multithreaded-debug runtime on windows.
)

if (NOT IsArm)
	target_link_libraries(${PROJECT_NAME} PUBLIC
		Foundation Math System Image Pipeline Scene Input
	)
else()
	target_link_libraries(${PROJECT_NAME} PUBLIC
		Foundation Math System
	)
endif()

if (MSVC)
	# Why do we still need this?
	if (CMAKE_BUILD_TYPE MATCHES Debug)
		target_link_options(${PROJECT_NAME} PRIVATE "/NODEFAULTLIB:LIBCMT.lib")
	endif()
	# This was necessary for astcenc library linking, but decided to turn off whole-program optimization.
	#	if (CMAKE_BUILD_TYPE MATCHES Release)
	#		target_link_options(${PROJECT_NAME} PRIVATE "/LTCG")
	#	endif()
endif()

target_include_directories(
	"${PROJECT_NAME}"
	PRIVATE
		$<TARGET_PROPERTY:Foundation,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:Math,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:System,INTERFACE_INCLUDE_DIRECTORIES>
)

if (NOT IsArm)
	target_include_directories(
		"${PROJECT_NAME}"
		PRIVATE
			$<TARGET_PROPERTY:Image,INTERFACE_INCLUDE_DIRECTORIES>
			$<TARGET_PROPERTY:Pipeline,INTERFACE_INCLUDE_DIRECTORIES>
			$<TARGET_PROPERTY:Scene,INTERFACE_INCLUDE_DIRECTORIES>
	)
else()
	message(STATUS "Tacent -- Arm Architecture. Excluding Some Modules.")
endif()


# Installation.
#install(
#	TARGETS ${PROJECT_NAME}
#	RUNTIME DESTINATION "${CMAKE_BINARY_DIR}/Install"
#)
install(DIRECTORY TestData/ DESTINATION "${CMAKE_BINARY_DIR}/UnitTests/TestData")
