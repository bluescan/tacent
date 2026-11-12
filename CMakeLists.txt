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

# Enable LibHEIF support (AVIF/HEIC image formats) for this build. The option
# defaults to OFF for the Tacent library so that downstream users do not link
# libheif/libde265 into the static library. Since this top-level build also
# builds the UnitTests (which must verify AVIF/HEIC loading), we preset the
# option ON ahead of the modules being configured. An explicit user choice wins.
if (NOT DEFINED TACENT_ENABLE_HEIF)
	set(TACENT_ENABLE_HEIF ON CACHE BOOL "Enable LibHEIF support for AVIF/HEIC image formats (requires LibHEIF library)")
endif()

# All the tacent modules.
add_subdirectory(Modules)

# Grab the Tacent test fixtures (CSV, FileSystem, Images, Script, UTF) from the tacentfixtures
# repository using a SPARSE checkout so we only pull the trees we need at configure time. This uses
# the project-local FetchContent_Sparse_Declare (CMakeModules/FetchContent_Sparse.cmake, the same
# implementation TacentView uses) which supports GIT_SPARSE_CHECKOUT; the standard
# FetchContent_MakeAvailable below then reports the dependency as already populated.
#   GIT_SPARSE_CHECKOUT = where to grab from (the fixture trees in the repo, plus its README.md)
#   SOURCE_DIR          = where the files land (each path is placed there under its own name,
#                         so Data\ ends up looking like the tacentfixtures repository root)
#                       -> ${CMAKE_SOURCE_DIR}/Data/{CSV,FileSystem,Images,Script,UTF,README.md}
# A re-fetch UPDATES the content (changed files only) and leaves any local/generated files
# in Data\ alone; -DFETCHCONTENT_SPARSE_CLEAR=tacentfixtures forces a full mirror instead.
# Only done when Tacent is the top-level project: when we are pulled in as a dependency (e.g. by
# TacentView) the host project fetches the fixtures it needs itself, so we must not double-fetch.
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	include(FetchContent_Sparse)
	FetchContent_Sparse_Declare(
		tacentfixtures
		GIT_REPOSITORY https://github.com/bluescan/tacentfixtures.git
		GIT_TAG master
		GIT_SPARSE_CHECKOUT CSV FileSystem Images Script UTF README.md
		SOURCE_DIR ${CMAKE_SOURCE_DIR}/Data
	)
	FetchContent_MakeAvailable(tacentfixtures)
endif()

# The UnitTests executable.
add_subdirectory(UnitTests)

# The Visualizer executable.
tacent_is_arch_arm(IsArm)
if (IsArm)
	message(STATUS "Tacent -- Arm Architecture. Excluding Visualizer.")
else()
	add_subdirectory(Visualizer)
endif()

# The WoboqGen executable.
if (IsArm)
	message(STATUS "Tacent -- Arm Architecture. Excluding Woboq.")
else()
	add_subdirectory(Woboq)
endif()
