Building libtiff (4.7.2)

Get it from: https://gitlab.com/libtiff/libtiff (clone at D:\GitHub\libtiff)

libtiff is built as a static library with all four standard compression codecs
enabled: JPEG, ZLib/deflate, LZW and PackBits (LZW + PackBits are ON by default;
they are confirmed present as tif_jpeg.c.o / tif_zip.c.o / tif_lzw.c.o /
tif_packbits.c.o in every staged archive). JPEG comes from our SIMD libjpeg-turbo
3.2.2 build (IJG API static lib, see the sibling LibJpegTurbo contrib), ZLib from
the tacent ZLib contrib.

SIMD note: libtiff's own codecs (LZW, PackBits, ZLib/deflate) are pure C - there
are no SIMD options to set for them. The only SIMD in the stack is the JPEG path,
which libtiff reaches by calling libjpeg. So JPEG-in-TIFF is SIMD-accelerated ONLY
when the linked libjpeg is the SIMD one - i.e. link the SIMD libjpeg-turbo build
from the sibling LibJpegTurbo contrib (WITH_SIMD=ON; see that readme's "SIMD"
section for how to enable/verify). All three staged libtiff binaries (Windows
Release tiff.lib, Windows Debug tiffd.lib, Linux libtiff.a) link that SIMD libjpeg,
so their JPEG path is SIMD-accelerated.

Windows (see C:\ClinePads\Task20261123_LibJpegTurboTIFF\build_libtiff.ps1):
* Build LibJpegTurbo first (build_libjpegturbo.ps1, Release and Debug).
* For each config (Release => /MT, Debug => /MTd), from the VS x64 dev prompt:
  cmake -S D:\GitHub\libtiff -B <builddir> -GNinja ^
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ^
      -DBUILD_SHARED_LIBS=OFF ^
      -Djpeg=ON -Djpeg-prefer-standard=ON ^
      -DJPEG_INCLUDE_DIR=C:/ClinePads/Build_LibJpegTurbo_Release/install/include ^
      -DJPEG_LIBRARY=C:/ClinePads/Build_LibJpegTurbo_Release/install/lib/jpeg-static.lib ^
      -Dzlib=ON ^
      -DZLIB_INCLUDE_DIR=d:/GitHub/tacent/Modules/Contrib/ZLib/include ^
      -DZLIB_LIBRARY=d:/GitHub/tacent/Modules/Contrib/ZLib/lib/zlibstatic.lib ^
      -Dlibdeflate=OFF -Dlzma=OFF -Dzstd=OFF -Dwebp=OFF -Djpeg12=OFF ^
      -DCMAKE_INSTALL_PREFIX=<builddir>/install
  (Debug: use the Debug LibJpegTurbo install, zlibstaticd.lib, and MultiThreadedDebug.)
  Note: pass forward-slash paths - CMake embeds them in try_compile files where a
  backslash is a string escape character.
* cmake --build <builddir> --target install
* Artifacts: install/lib/tiff.lib (Release) / tiffd.lib (Debug), install/include/*.h

Stage into this folder:
* include/  <- install/include/ (tiff.h tiffconf.h tiffio.h tiffio.hxx tiffvers.h)
* lib/tiff.lib  <- Release install
* lib/tiffd.lib <- Debug install
* lib/libtiff.a <- Linux build (below)

Linux (or use WSL): same flags with forward slashes, e.g.:
* cmake -S /mnt/d/GitHub/libtiff -B <builddir> -GNinja -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_SHARED_LIBS=OFF -Djpeg=ON -Djpeg-prefer-standard=ON ^
    -DJPEG_INCLUDE_DIR=<libjpegturbo linux install>/include ^
    -DJPEG_LIBRARY=<libjpegturbo linux install>/lib/libjpeg.a ^
    -Dzlib=ON -DZLIB_INCLUDE_DIR=/mnt/d/GitHub/tacent/Modules/Contrib/ZLib/include ^
    -DZLIB_LIBRARY=/mnt/d/GitHub/tacent/Modules/Contrib/ZLib/lib/libz.a ^
    -Dlibdeflate=OFF -Dlzma=OFF -Dzstd=OFF -Dwebp=OFF -Djpeg12=OFF ^
    -DCMAKE_INSTALL_PREFIX=<builddir>/install
* cmake --build <builddir> --target install
* Look for install/lib/libtiff.a
