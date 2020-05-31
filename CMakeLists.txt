cmake_minimum_required(VERSION "3.16.0")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/CMakeModules")

# Set default build-type (AKA the configuration in other IDEs).
set(CMAKE_BUILD_TYPE_INIT Release)

# Setup tacent build configurations before specifying the project.
find_package("TacentConfigurations")

# Tacent is the top level project.
set(TACENT_VERSION "0.8.0")
project(Tacent VERSION ${TACENT_VERSION} LANGUAGES CXX)

# Sub-projects.
add_subdirectory(Contrib)
add_subdirectory(Modules)

# The UnitTests executable, the only non-library.
add_subdirectory(UnitTests)
