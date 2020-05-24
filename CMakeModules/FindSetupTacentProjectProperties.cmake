if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
    add_definitions(-DJAS_WIN_MSVC_BUILD)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-DARCHITECTURE_X64)
    add_definitions(-D_LIB)
else()
    add_definitions(-DPLATFORM_LINUX)
endif()

# Add the library itself
add_library(${PROJECT_NAME} ${src})

# Make adjustment based on build type
if(CMAKE_BUILD_TYPE MATCHES Debug)

    if(NOT CMAKE_DEBUG_POSTFIX)
        set(CMAKE_DEBUG_POSTFIX "d")
    endif()

    # Only windows detects the CONFIG itself
    if(NOT WIN32)
        add_definitions(-DCONFIG_DEBUG)
    endif()

else()

    # Non Windows only Debug and Release defines
    if(NOT WIN32)
        add_definitions(-DCONFIG_RELEASE)
        target_compile_options(
			${PROJECT_NAME} PRIVATE 
            $<$<CXX_COMPILER_ID:MSVC>:/O2>
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-O2>
        )
        endif()

endif()

# Set comiler options
target_compile_options(
	${PROJECT_NAME} PRIVATE 
	$<$<CXX_COMPILER_ID:MSVC>:/W2 /GS /Gy /Zc:wchar_t /Gm- /Zc:inline /fp:precise /WX- /Zc:forScope /Gd /FC>
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# And now set the properties for the output of the library
#set_target_properties(
#	${PROJECT_NAME} PROPERTIES
#    CXX_STANDARD 17
#    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
#    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
#    # OUTPUT_NAME "${PROJECT_NAME}$<PLATFORM_ID>"
#)
