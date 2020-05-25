# Add the library itself
add_library(${PROJECT_NAME} ${src})

# Set properties on the target. Include files here. Note:
# PRIVATE is for building only. Populates INCLUDE_DIRECTORIES
# INTERFACE is for users of the library. Populates INTERFACE_INCLUDE_DIRECTORIES
# PUBLIC means both building and for users of the lib. Populates both.
target_include_directories(
	${PROJECT_NAME}
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Inc>
		$<INSTALL_INTERFACE:Inc>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

# Set -D defines  based on configuration and platform.
target_compile_definitions(${PROJECT_NAME}
	PUBLIC
		ARCHITECTURE_X64
		$<$<CONFIG:Debug>:CONFIG_DEBUG>
		$<$<CONFIG:Release>:CONFIG_RELEASE>
		$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_DEPRECATE _LIB JAS_WIN_MSVC_BUILD>
		$<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS>
		$<$<PLATFORM_ID:Linux>:PLATFORM_LINUX>
)

# Make adjustment based on build type
if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(NOT CMAKE_DEBUG_POSTFIX)
        set(CMAKE_DEBUG_POSTFIX "d")
    endif()
endif()

# Set compiler option flags based on specific compiler and configuration.
target_compile_options(${PROJECT_NAME}
	PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/W2 /GS /Gy /Zc:wchar_t /Gm- /Zc:inline /fp:precise /WX- /Zc:forScope /Gd /FC>
		$<$<AND:$<CONFIG:Debug>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-O0>
		$<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:/O0>
		$<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-O2>
		$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2>
)

target_compile_features(${PROJECT_NAME}
	PRIVATE
		cxx_std_17
)


#
# Installation instructions
#
set(TACENT_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/Out")
message(STATUS "Tacent -- TACENT_INSTALL_DIR: ${TACENT_INSTALL_DIR}")

install(
	TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${TACENT_INSTALL_DIR}
    ARCHIVE DESTINATION ${TACENT_INSTALL_DIR}
)

install(DIRECTORY Inc/ DESTINATION "${TACENT_INSTALL_DIR}/Inc")

install(
	EXPORT ${PROJECT_NAME}-targets
	FILE
		${PROJECT_NAME}Targets.cmake
	NAMESPACE
		Tacent::
	DESTINATION
		${TACENT_INSTALL_DIR}
)
