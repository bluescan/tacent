project(Modules VERSION ${TACENT_VERSION})

add_subdirectory(Foundation)
add_subdirectory(Math)
add_subdirectory(System)
add_subdirectory(Image)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	add_subdirectory(Pipeline)
	add_subdirectory(Scene)
endif()
