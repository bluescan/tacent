cmake_minimum_required(VERSION 3.16.0)
set(CMAKE_BUILD_TYPE_INIT Release)

include("CMakeModules/SetupTacentConfigurations.cmake")

# Use MultiThreaded libraries
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# tacent is the top level project
project(tacent VERSION 0.1.0 LANGUAGES CXX)

# Use a Properties common file (location based on top level project)
# to add libraries with common build characteristics
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/CMakeModules")

# Main sub-projects, consisting of many other projects and sub-projects
#add_subdirectory(Contrib)
add_subdirectory(Modules/Foundation)
