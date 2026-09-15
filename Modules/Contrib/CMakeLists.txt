# Contrib.
#
# Third-party libraries that are shared by more than one Tacent module (they are
# not image-specific). Modules link the targets defined here, eg.
#   target_link_libraries(${PROJECT_NAME} PRIVATE TinyXML2 ZLib)

find_package("TacentProjectUtilities" REQUIRED)
project(Contrib VERSION ${TACENT_VERSION} LANGUAGES CXX)

# TinyXML2. An XML parser used by a few modules. It is compiled here as a small
# static library so that any module can link it. The xmltest.cpp program from the
# TinyXML2 distribution is a standalone test tool and is not part of this build.
add_library(
	TinyXML2 STATIC
	TinyXML2/tinyxml2.h
	TinyXML2/tinyxml2.cpp
)
target_include_directories(
	"TinyXML2"

	# PRIVATE is for building only. Populates INCLUDE_DIRECTORIES
	# INTERFACE is for users of the library. Populates INTERFACE_INCLUDE_DIRECTORIES
	# PUBLIC means both building and for users of the lib. Populates both.
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/TinyXML2>
		$<INSTALL_INTERFACE:Contrib/TinyXML2>
)
tacent_target_compile_definitions(TinyXML2)
tacent_target_compile_options(TinyXML2)
tacent_target_compile_features(TinyXML2)
tacent_set_target_properties(TinyXML2)


# ZLib. Shared by a few different modules and formats. It is supplied as prebuilt
# static libraries (see Readme_BuildZLib.txt). An INTERFACE target exposes the
# headers and the correct library file for each configuration and platform.
add_library(ZLib INTERFACE)
target_include_directories(
	"ZLib"
	INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ZLib/include>
		$<INSTALL_INTERFACE:Contrib/ZLib/include>
)
target_link_libraries(
	"ZLib"
	INTERFACE
		# Windows: Debug and Release static libs are kept separate (MSVC ABI); the "d" suffix is Debug.
		$<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/ZLib/lib/zlibstaticd.lib>
		$<$<AND:$<CONFIG:Develop>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/ZLib/lib/zlibstatic.lib>
		$<$<AND:$<CONFIG:Profile>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/ZLib/lib/zlibstatic.lib>
		$<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/ZLib/lib/zlibstatic.lib>
		$<$<AND:$<CONFIG:Ship>,$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/ZLib/lib/zlibstatic.lib>
		# Linux: only the release .a lib is linked for all configs (matches the other Linux contribs).
		$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/ZLib/lib/libz.a>
)


# Installation.
# This path is relative to CMAKE_INSTALL_PREFIX. Do not use an absolute path if you want
# the exports to work properly (the will have bad absolute paths if you do).
set(TACENT_INSTALL_RELDIR "Install")
message(STATUS "Tacent -- Contrib will be installed to ${TACENT_INSTALL_RELDIR}")

install(
	TARGETS TinyXML2 ZLib
	EXPORT Contrib-targets
	ARCHIVE DESTINATION ${TACENT_INSTALL_RELDIR}
)
install(FILES TinyXML2/tinyxml2.h DESTINATION "Contrib/TinyXML2")
install(FILES ZLib/include/zlib.h ZLib/include/zconf.h DESTINATION "Contrib/ZLib/include")

install(
	EXPORT Contrib-targets
	FILE
		ContribTargets.cmake
	NAMESPACE
		Tacent::
	DESTINATION
		${TACENT_INSTALL_RELDIR}
)