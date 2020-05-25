set(TacentVersion "0.8.0")
set(TacentMinCMake "3.16.0")

cmake_minimum_required(VERSION ${TacentMinCMake})
set(CMAKE_BUILD_TYPE_INIT Release)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/CMakeModules")

# Setup tacent build configurations before specifying the project.
find_package("SetupTacentConfigurations")

# Use MultiThreaded libraries
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# tacent is the top level project
project(tacent VERSION ${TacentVersion} LANGUAGES CXX)

# Main sub-projects, consisting of many other projects and sub-projects
#add_subdirectory(Contrib)
add_subdirectory(Modules/Foundation)
#add_subdirectory(Modules/Math)
