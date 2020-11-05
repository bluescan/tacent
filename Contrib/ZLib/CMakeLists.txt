# This file is here solely so that we can install the zlib lib and header files.
# This path is relative to CMAKE_INSTALL_PREFIX. Do not use an absolute path if you want
# the exports to work properly (the will have bad absolute paths if you do).
set(ZLIB_INSTALL_DIR "TacentInstall/Contrib/ZLib")
install(DIRECTORY include/ DESTINATION "${ZLIB_INSTALL_DIR}/include")
install(DIRECTORY lib/ DESTINATION "${ZLIB_INSTALL_DIR}/lib")
