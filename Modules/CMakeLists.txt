project(Modules VERSION ${TACENT_VERSION})

add_subdirectory(Foundation)
add_subdirectory(Math)
add_subdirectory(System)

tacent_is_arch_arm(IsArm)
if (IsArm)
	message(STATUS "Tacent -- Arm Architecture. Excluding Some Modules.")
else()
	add_subdirectory(Image)
	add_subdirectory(Pipeline)
	add_subdirectory(Scene)
	add_subdirectory(Input)
endif()
