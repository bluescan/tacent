Building libtiff

Get it from: http://download.osgeo.org/libtiff/

Windows:
* Run x64 Native Tools Command Prompt for VS 2019
* mkdir buildvs    cd buildvs
* cmake -G"Visual Studio 16 2019" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=Install -Djpeg=OFF -Dzlib=ON -DZLIB_ROOT=C:/Github/tacent/Modules/Image/Contrib/ZLib ..

// Disabled until can get it to work with libjpegturbo.
// * cmake -G"Visual Studio 16 1019" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=Install -Djpeg=ON -DJPEG_INCLUDE_DIR=C:/Github/tacent/Modules/Image/Contrib/LibJPEG/include -DJPEG_LIBRARY=C:/Github/tacent/Modules/Image/Contrib/LibJPEG/lib/jpeg.lib -Dzlib=ON -DZLIB_ROOT=C:/Github/tacent/Modules/Image/Contrib/ZLib ..
* Open tiff.sln is VS2019.
  Set code-gen runtime library to /MT (Multi-threaded)
  Set Config to Release and Platform to x64.
* Build the tiff project. Look for buildvs\libtiff\Release\tiff.lib


Linux (or use WSL)
* For linux I disabled the stuff I don't need built right in the CMakeFiles.txt
# Process subdirectories
#add_subdirectory(port)
add_subdirectory(libtiff)
#add_subdirectory(tools)
#add_subdirectory(test)
#add_subdirectory(contrib)
#add_subdirectory(build)
#add_subdirectory(man)
#add_subdirectory(html)

* mkdir buildwsl    cd buildwsl
* cmake -G"Unix Makefiles" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=Install -Djpeg=OFF -Dzlib=ON -DZLIB_ROOT=/mnt/c/Github/tacent/Modules/Image/Contrib/ZLib ..

//* cmake -G"Unix Makefiles" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=Install -Djpeg=ON -DJPEG_INCLUDE_DIR=/mnt/c/Github/tacent/Modules/Image/Contrib/LibJPEG/include -DJPEG_LIBRARY=/mnt/c/Github/tacent/Modules/Image/Contrib/LibJPEG/lib/jpeg.lib -Dzlib=ON -DZLIB_ROOT=/mnt/c/Github/tacent/Modules/Image/Contrib/ZLib ..
* make install
* Look in Install directory.
