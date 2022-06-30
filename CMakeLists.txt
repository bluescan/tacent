cmake_minimum_required(VERSION "3.16.0")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/CMakeModules")

# Set default build-type (AKA the configuration in other IDEs).
set(CMAKE_BUILD_TYPE_INIT Release)

# Setup tacent build configurations before specifying the project.
find_package("TacentConfigurations")

# Tacent is the top level project. This include sets the version string variable.
include("Modules/Foundation/Inc/Foundation/tVersion.cmake.h")
project(Tacent VERSION "${TACENT_VERSION}" LANGUAGES C CXX)
message(STATUS "Tacent -- ProjectName ${PROJECT_NAME} ProjectVersion ${PROJECT_VERSION}")

# These are options that are shared between all modules. Thanks to CMP0077 the setting here is ignored
# if a user of Tacent sets the option before doing the FetchContent.
option(TACENT_UTF16_API_CALLS "Build Tacent With UTF16 API Calls" Off)
if (TACENT_UTF16_API_CALLS)
	message(STATUS "Tacent -- UTF16 API Calls.")
else()
	message(STATUS "Tacent -- UTF8 API Calls.")
endif()

# We want a better default for install prefix. It is bad form to be modifying
# system files from a cmake build of anything. Really quite surprised someone
# thinks the cmake defaults are good.
# message(STATUS "init=${CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT}")
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "..." FORCE)
endif()
message(STATUS "Tacent -- InstallPrefix ${CMAKE_INSTALL_PREFIX}")

# All the tacent modules.
add_subdirectory(Modules)

# The UnitTests executable.
add_subdirectory(UnitTests)

# The WoboqGen executable.
tacent_is_arch_arm(IsArm)
if (IsArm)
	message(STATUS "Tacent -- Arm Architecture. Excluding Woboq.")
else()
	add_subdirectory(Woboq)
endif()
