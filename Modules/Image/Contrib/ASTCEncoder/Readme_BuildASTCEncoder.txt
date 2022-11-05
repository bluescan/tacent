This is the official encoder (and decoder) for ASTC image data from ARM.
https://github.com/ARM-software/astc-encoder
Note that this library can both encode and decode.
Look in Utils\Example\astc_api_example.cpp for a simple example of decoding and encoding.

Building for Windowa:
* Clone the repository.
* Open an x64 Native Tools for VS 2022 command prompt.
* cd to the cloned repository directory.
* mkdir buildwin
* cd buildwin
* The native (no vectorized/SIMD) is quite slow. However pretty much all CPUs have SSE2, so use that:
* cmake -G "Visual Studio 17 2022" -A x64 -DISA_SSE2=ON ..
* OPTIONAL cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON ..
* OPTIONAL 'nmake' to build. Static libs (one for each SIMD variant) will be in Source folder.
* OPTIONAL Goto mkdir and do a new folder, this time exchanging Release with Debug.
* Open in Visual Studio 2022. 
* Build astcenc_sse2_static project in Release x64 configuration, then again in Debug x64 configuration.
* You will find astcenc-sse2-static.lib in buildwin/Source/Release and buildwin/Source/Debug
* You will find astcenc.h in the Source folder (at root of project).
* You will find actcenccli_verion.h in the build folder Source directory. Use it for the lib version as well.

NOTE: The release sse2 windows static lib is so big compared to the other 3
libs because of whole-program-optimization. Optional to turn off in VS2022.
Goto C/C++ => Optimiation -> Uncheck Whole Program Optimization.

Building for Linux
* If using WSL you can just cd into /mnt/c/ and go to the root where you already cloned.
* If running Kubuntu you'll need to clone again with smartgit or whatever.
* In root of clone, mkdir buildlinrel, cd buildlinrel
* I was having some difficulty with LTO. Adding the -DCLI=OFF turns it off.
* cmake -G "Unix Makefiles" -DISA_SSE2=ON -DCMAKE_BUILD_TYPE=Release -DCLI=OFF ..
* make
* Do same thing again but with buildwindeb and Debug for the type.
* Source folder where you just built will have libastcenc-sse2-static.a in it.
* Header is in same place as for windows.
