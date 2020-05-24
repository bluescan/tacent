cmake_minimum_required(VERSION 3.16)

# I prefer this relative path to setting up the modules path in the root CMakeLists because
# it allows this project to be built completely independently.
#include("../../CMakeModules/SetupTacentConfigurations.cmake")
include("SetupTacentConfigurations")

# Give the project a name and specify explicitely the launguages used.
project(libtacentfoundation VERSION 0.7.0 LANGUAGES CXX)

# Here are the source files. No globbing. We include the headers here
# so that some generators can include them properly, but they are not required.
list(
	APPEND foundationSourceFiles
    Src/tAssert.cpp
    Src/tBitArray.cpp
    Src/tMemory.cpp
    Src/tPlatform.cpp
    Src/tPool.cpp
    Src/tStandard.cpp
    Src/tString.cpp
    Src/tUnits.cpp
    Inc/Foundation/tArray.h
    Inc/Foundation/tAssert.h
    Inc/Foundation/tBitArray.h
    Inc/Foundation/tBitField.h
    Inc/Foundation/tFixInt.h
    Inc/Foundation/tList.h
    Inc/Foundation/tMemory.h
    Inc/Foundation/tPlatform.h
    Inc/Foundation/tPool.h
    Inc/Foundation/tPriorityQueue.h
    Inc/Foundation/tRingBuffer.h
    Inc/Foundation/tSort.h
    Inc/Foundation/tStandard.h
    Inc/Foundation/tString.h
    Inc/Foundation/tUnits.h
    Inc/Foundation/tVersion.h
)

# We want to generate a static library.
add_library(Foundation ${foundationSourceFiles})

# Set properties on the target. Include files here. Note:
# PRIVATE is for building only. Populates INCLUDE_DIRECTORIES
# INTERFACE is for users of the library. Populates INTERFACE_INCLUDE_DIRECTORIES
# PUBLIC means both building and for users of the lib. Populates both.
target_include_directories(
	Foundation
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Inc>
		$<INSTALL_INTERFACE:Inc>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

#target_compile_options(foundation PRIVATE -Werror)

target_compile_features(Foundation PRIVATE cxx_std_17)

# If we had dependencies, we would use target_link_libraries

#
# Installation instructions
#

set(TACENT_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/Out")
message(STATUS "Tacent -- TACENT_INSTALL_DIR: ${TACENT_INSTALL_DIR}")

install(
	TARGETS Foundation
    EXPORT Foundation-targets
    LIBRARY DESTINATION ${TACENT_INSTALL_DIR}
    ARCHIVE DESTINATION ${TACENT_INSTALL_DIR}
)

install(DIRECTORY Inc/ DESTINATION "${TACENT_INSTALL_DIR}/Inc")

install(
	EXPORT Foundation-targets
	FILE
		FoundationTargets.cmake
	NAMESPACE
		Tacent::
	DESTINATION
		${TACENT_INSTALL_DIR}
)
