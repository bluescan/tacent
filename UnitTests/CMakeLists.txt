# tacent
project(UnitTests VERSION 0.1.0)

include_directories(
    Inc
    ../Modules/Foundation/Inc
    ../Modules/Image/Inc
    ../Modules/Math/Inc
    ../Modules/Pipeline/Inc
    ../Modules/Scene/Inc
    ../Modules/System/Inc
    )

list (APPEND src
    Src/TestBuild.cpp
    Src/TestFoundation.cpp
    Src/TestImage.cpp
    Src/TestMath.cpp
    Src/TestScene.cpp
    Src/TestSystem.cpp
    Src/UnitTests.cpp
    Src/TestBuild.h
    Src/TestFoundation.h
    Src/TestImage.h
    Src/TestMath.h
    Src/TestScene.h
    Src/TestSystem.h
    Src/UnitTests.h
)

if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-DARCHITECTURE_X64)
    add_definitions(-D_LIB)
else()
    add_definitions(-DPLATFORM_LINUX)
endif(WIN32)

if(CMAKE_BUILD_TYPE MATCHES Debug)

    # Only windows detects the CONFIG itself
    if(NOT WIN32)
        add_definitions(-DCONFIG_DEBUG)
    endif()

else()

    # Non Windows only Release defines and compiler options
    if(NOT WIN32)
        add_definitions(-DCONFIG_RELEASE)
        target_compile_options(${PROJECT_NAME} PRIVATE 
            $<$<CXX_COMPILER_ID:MSVC>:/O2>
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-O2>
        )
    endif()

endif(CMAKE_BUILD_TYPE MATCHES Debug)

add_executable(${PROJECT_NAME} ${src})

# Force C++ 17
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
)

# Set comiler options
target_compile_options(${PROJECT_NAME} PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/W2 /GS /Gy /Zc:wchar_t /Gm- /Zc:inline /fp:precise /WX- /Zc:forScope /Gd /FC>
)

target_link_libraries(${PROJECT_NAME}
    CxImage dcr jasper jpeg mng png psd tiff zlib           # CxImage
    bc6h bc7 nvcore nvimage nvmath nvthread nvtt squish     # nvTextureTools
    Foundation Image Math Pipeline Scene System             # tacent native
    shlwapi.lib Dbghelp.lib
)
