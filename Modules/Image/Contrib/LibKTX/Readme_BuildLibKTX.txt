Building libktx from Khronos

NEW INSTRUCTIONS
* Sync the bluescan KTX-Software fork in GitHub to pickup the latest changes from the Khronos official repo.
* Find the latest TAG in the official repo and duplicate it at the head of the bluescan fork. The TAG must be a full (annotated) version tag.
* Copy all headers from the fork over the existing tacent LibKTX headers.

WINDOWS
Open the folder in VSCode and go to the cmake extension settings. Under Cmake: Configure Args add
-DKTX_FEATURE_STATIC_LIBRARY=ON
-DASTCENC_ISA_SSE2=ON
Choose the DEBUG build variant and hit F7
Choose the RELEASE build variant and hit F7
Copy the generated libs over to Tacent. You may need to rename the debug lib as it needs the 'd' in it.

LINUX
Open the folder in VSCode WSL and go to the cmake extension settings. Under Cmake: Configure Args add
-DKTX_FEATURE_STATIC_LIBRARY=ON
-DASTCENC_ISA_SSE2=ON
Choose the RELEASE build variant and hit F7
Copy the generated lib over to Tacent.


OLD INSTRUCTIONS

Windows:
* Clone the repo from https://github.com/KhronosGroup/KTX-Software
* You must clone and not download as a zip otherwise it won't grab the version number correctly.
* Open 'x64 Native Tools Command Prompt for VS 2022' from the start menu. This will give access to cmake etc.
* cd to where you cloned the repo to (eg. C:\GitHub\KTX-Software\)
* mkdir buildwinrel
* cmake . -A x64 -B buildwinrel -G "Visual Studio 17 2022" -DKTX_FEATURE_STATIC_LIBRARY=ON -DCMAKE_BUILD_TYPE=Release
* // cmake . -B buildwinrel -GNinja -DKTX_FEATURE_STATIC_LIBRARY=ON -DCMAKE_BUILD_TYPE=Release
* (To build debug, leave the build type define out).
* // cmake --build buildwinrel
*
* You MUST use the VS generator, otherwise it does not build a proper static library. Using Ninja will work, but the
  lib will not use the proper runtime libraries.
* Open KTX-Software.sln in VS2022. Set to Release configurarion / x64. Then, under the ktx project, select properties
  and go to C/C++ -> Code Generation -> Runtime Library and select Multi-threaded (/MT) (NOT multi-threaded DLL)
* Right-click on the ktk project and select Build or Build Clean.
* In buildwinrel/Release you will find ktx.lib.
* In folder 'include' you will find ktx.h
* In lib\dfdutils\vulkan you will find vulkan_core.h and vk_platform.h (Needed for Vulkan pixelformats)
* In lib you will find gl_format.h (A standalone header needed for OpenGL pixel formats. Not generated.) This also requires vkformat_enum.h (odd) and formatsize.h from the same folder.
* In lib you will find version.h
* Note: ktx_read.lib is a readling library only (no saving). We are using the full lib (libktx) so we can add
  save-support later.
* Note: For getting the version you'll need something dumb like:
#include "LibKTX/include/version.h"
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
const char* LibKTXVersionString = TOSTRING(LIBKTX_VERSION);

* IF YOU WANT TO BUILD THE LOADTEST APPS: These require the Vulkan SDK for windows from https://vulkan.lunarg.com/
* Download the installer and install. Version as of this readme is VulkanSDK-1.3.231.1-Installer.exe
* Use default settings.
* The configure command looks like:
C:\GitHub\KTX-Software>cmake . -A x64 -B buildtest -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DVulkan_LIBRARY=C:\VulkanSDK\1.3.231.1\Lib\vulkan-1.lib -DVulkan_INCLUDE_DIR=C:\VulkanSDK\1.3.231.1\Include -DKTX_FEATURE_LOADTEST_APPS=ON
* You will now see the vkloadtests project when you open the sln.

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
* In folder buildlinrel you will find libktx.a
* See the windows notes above for the version include and how to parse.
