Building libktx from Khronos

Windows:
* Clone the repo from https://github.com/KhronosGroup/KTX-Software
* You must clone and not download as a zip otherwise it won't grab the version number correctly.
* Open 'x64 Native Tools Command Prompt for VS 2022' from the start menu. This will give access to cmake etc.
* cd to where you cloned the repo to (eg. C:\GitHub\KTX-Software\)
* mkdir buildwinrel
* cmake . -B buildwinrel -GNinja -DKTX_FEATURE_STATIC_LIBRARY=ON -DCMAKE_BUILD_TYPE=Release
* (To build debug, leave the build type define out).
* cmake --build buildwinrel
* In folder buildwinrel you will find ktx.lib and ktx.version
* Note: ktx_read.lib is a readling library only (no saving). We are using the full lib (libktx) so we can add save-support later.
* In folder 'include' you will find ktx.h


Linux (or use WSL):
* apt-get install libtool
* apt-get install libtool-bin
* apt-get install libvulkan1 libvulkan-dev											// may be optional
* apt-get install ninja-build 													// optional
* cd /mnt/c/GitHub/KTX-Software
* mkdir buildlinrel
* cmake . -B buildlinrel -G"Unix Makefiles" -DKTX_FEATURE_STATIC_LIBRARY=ON -DCMAKE_BUILD_TYPE=Release
* cmake . -B buildlinrel -G"Ninja" -DKTX_FEATURE_STATIC_LIBRARY=ON -DCMAKE_BUILD_TYPE=Release			// optional

* cmake --build buildlinrel --config Release --target ktx
* In folder buildlinrel you will find ktx.lib and ktx.version
