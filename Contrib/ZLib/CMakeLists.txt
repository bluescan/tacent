# This file is here solely so that we can install the zlib lib and header files.
find_package("TacentProjectUtilities" REQUIRED)
project(ZLib VERSION ${TACENT_VERSION} LANGUAGES C CXX)

add_library(ZLib STATIC IMPORTED GLOBAL)

set(ZLIB_INSTALL_DIR "${CMAKE_BINARY_DIR}/TacentInstall/${PROJECT_NAME}")
install(DIRECTORY include/ DESTINATION "${ZLIB_INSTALL_DIR}/include")
install(DIRECTORY lib/ DESTINATION "${ZLIB_INSTALL_DIR}/lib")
