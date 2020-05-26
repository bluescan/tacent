project(modules VERSION ${TacentVersion})

add_subdirectory(Foundation)
add_subdirectory(Math)
add_subdirectory(System)
#add_subdirectory(Image)

message(STATUS "-- Tacent CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	add_subdirectory(Pipeline)
	add_subdirectory(Scene)
endif()
