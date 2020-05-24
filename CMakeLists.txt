cmake_minimum_required(VERSION 3.16.0)
set(CMAKE_BUILD_TYPE_INIT Release)

# Use a Properties common file (location based on top level project)
# to add libraries with common build characteristics
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/CMakeModules")

# Setup tacent build configurations before specifying the project.
find_package("SetupTacentConfigurations")

# Use MultiThreaded libraries
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# tacent is the top level project
project(tacent VERSION 0.1.0 LANGUAGES CXX)
message(STATUS "Tacent -- PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")


# Main sub-projects, consisting of many other projects and sub-projects
#add_subdirectory(Contrib)
add_subdirectory(Modules/Foundation)
