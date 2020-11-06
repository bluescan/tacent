Get zlib from https://github.com/madler/zlib
This is a repo that uses cmake to build zlib.

Windows:

* Open a x64 Native Tools Command Prompt for VS 2019
* mkdir buildwin     cd buildwin
* // cmake -DBUILD_SHARED_LIBS=False -DCMAKE_INSTALL_PREFIX=installdir ..       (The -DAMD64 makes it gen asm code for 64bit but is buggy).
* Just use: cmake ..
* Open the zlib.sln
* Set Debug / x64.
* zlibstatic properties. Code Generation. Fix runtime library to be /MTd (for debug) or /MT (for release). The cmake is wrong here and uses DLL versions by mistake.
* Right-click zlibstatic and build. Do same for Release.
* The Debug dir will have zlibd.pdb and zlibstaticd.lib. The Release dir will have zlibstatic.lib
* The headers you'll need are zconf.h in the buildwin folder (should be usable for debug/release for any plat) and the zlib.h up a directory.

Linux:
* mkdir buildwsl      cd buildwsl
* cmake -DBUILD_SHARED_LIBS=False -DCMAKE_BUILD_TYPE=Release ..
* make


///////////////
# This file is here solely so that we can install the zlib lib and header files.
# This path is relative to CMAKE_INSTALL_PREFIX. Do not use an absolute path if you want
# the exports to work properly (the will have bad absolute paths if you do).
set(ZLIB_INSTALL_DIR "TacentInstall/Contrib/ZLib")
install(DIRECTORY include/ DESTINATION "${ZLIB_INSTALL_DIR}/include")
install(DIRECTORY lib/ DESTINATION "${ZLIB_INSTALL_DIR}/lib")
