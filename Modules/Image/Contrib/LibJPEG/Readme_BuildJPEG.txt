Get libjpeg from https://github.com/csparker247/jpeg-cmake
This is a repo that uses cmake to build IJG's LibJPEG.

Tacent uses turbo-jpeg for direct loading of jpg files (it is fast). The
standard LibJPEG (this library) from the IJG is needed by LibTIFF for jpeg support.


Windows:

* Open a x64 Native Tools Command Prompt for VS 2019
* mkdir buildwin     cd buildwin
* cmake -G"NMake Makefiles" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=Install ..
* nmake install
* Look in the Install directory.


Linux:
* mkdir buildwsl      cd buildwsl
* cmake -G"Unix Makefiles" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=Install ..
* make install
* Look in the Install directory.
