Building libheif and libde265 (static)
======================================

Get libheif and libde265 from:
* https://github.com/strukturag/libheif
* https://github.com/strukturag/libde265

When configuring, avoid GPL-licensed backends (such as x265). libde265 is used
as the HEVC decoder (LGPL-3.0) and is statically linked.

IMPORTANT - static build macros (Windows / MSVC):
-------------------------------------------------
On Windows, libde265 and libheif use __declspec(dllimport) by default in their
public headers. If the static .lib files are compiled WITHOUT the following
defines, their object files will reference __imp_de265_* and __imp_heif_*
import-thunk symbols, which do NOT exist in a static archive, causing
LNK2019 "unresolved external symbol" errors when tacent links against them.

The macros and where they are required:
  LIBDE265_STATIC_BUILD  - defined when building libde265 (set via
                           -DLIBDE265_STATIC_BUILD=1 on the cmake command)
  LIBDE265_STATIC_BUILD  - ALSO defined when compiling libheif so its object
                           files reference plain de265_* symbols, not thunks
  LIBHEIF_STATIC_BUILD   - defined when compiling libheif so its object files
                           reference plain heif_* symbols, not thunks
  LIBHEIF_STATIC_BUILD   - also defined by tacent's Image CMakeLists.txt
                           (target_compile_definitions), so tacent's own
                           object files reference plain heif_* symbols

The last two are supplied via CMAKE_CXX_FLAGS when configuring libheif.
No changes to tacent sources or CMake are needed.

Windows:
========
* Run x64 Native Tools Command Prompt for VS 2022
* Configure and build libde265 (static):
    cmake -S libde265 -B libde265/build_win \
      -DBUILD_SHARED_LIBS=OFF \
      -DLIBDE265_STATIC_BUILD=1 \
      -DENABLE_SDL=OFF \
      -DENABLE_ENCODER=OFF \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=libde265/install
    cmake --build libde265/build_win --config Release --target install

* Configure and build libheif (static). Pass the static-build macros via
  CMAKE_CXX_FLAGS so libheif's object files use plain C linkage:
    cmake -S libheif -B libheif/build_win \
      -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_BUILD_TYPE=Release \
      -DWITH_EXAMPLES=OFF -DWITH_FUZZERS=OFF -DWITH_TESTING=OFF \
      -DWITH_LIBDE265=ON \
      -DWITH_X265=OFF -DWITH_KVAZAAR=OFF \
      -DWITH_AOM_DECODER=OFF -DWITH_AOM_ENCODER=OFF \
      -DWITH_DAV1D=OFF -DWITH_SvtEnc=OFF -DWITH_RAV1E=OFF \
      -DWITH_JPEG_DECODER=OFF -DWITH_JPEG_ENCODER=OFF \
      -DWITH_OpenJPEG_DECODER=OFF -DWITH_OpenJPEG_ENCODER=OFF \
      -DWITH_FFMPEG_DECODER=OFF -DWITH_OPENJPH_ENCODER=OFF \
      -DENABLE_PLUGIN_LOADING=OFF \
      -DCMAKE_PREFIX_PATH=libde265/install \
      -DCMAKE_INSTALL_PREFIX=libheif/install \
      "-DCMAKE_CXX_FLAGS=/DLIBDE265_STATIC_BUILD /DLIBHEIF_STATIC_BUILD"
    cmake --build libheif/build_win --config Release --target install

* The resulting heif.lib and libde265.lib contain plain de265_* and heif_*
  symbols (no __imp_ thunks) and can be statically linked by tacent.
* Copy heif.lib and libde265.lib into tacent at:
    Modules/Image/Contrib/LibHEIF/lib/

Linux (or use WSL):
-------------------
The __declspec(dllimport) issue is MSVC-specific, so the static-build macros
are not strictly required on Linux. They are included here for consistency.

* Configure and build libde265:
    cmake -S libde265 -B libde265/build_linux \
      -DBUILD_SHARED_LIBS=OFF \
      -DLIBDE265_STATIC_BUILD=1 \
      -DENABLE_SDL=OFF \
      -DENABLE_ENCODER=OFF \
      -DCMAKE_BUILD_TYPE=Release
    make -C libde265/build_linux

* Configure and build libheif:
    cmake -S libheif -B libheif/build_linux \
      -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_BUILD_TYPE=Release \
      -DWITH_EXAMPLES=OFF -DWITH_FUZZERS=OFF -DWITH_TESTING=OFF \
      -DWITH_LIBDE265=ON \
      -DWITH_X265=OFF -DWITH_KVAZAAR=OFF \
      -DWITH_AOM_DECODER=OFF -DWITH_AOM_ENCODER=OFF \
      -DWITH_DAV1D=OFF -DWITH_SvtEnc=OFF -DWITH_RAV1E=OFF \
      -DWITH_JPEG_DECODER=OFF -DWITH_JPEG_ENCODER=OFF \
      -DWITH_OpenJPEG_DECODER=OFF -DWITH_OpenJPEG_ENCODER=OFF \
      -DWITH_FFMPEG_DECODER=OFF -DWITH_OPENJPH_ENCODER=OFF \
      -DENABLE_PLUGIN_LOADING=OFF \
      -DCMAKE_PREFIX_PATH=libde265/build_linux \
      -DCMAKE_CXX_FLAGS="-DLIBDE265_STATIC_BUILD -DLIBHEIF_STATIC_BUILD"
    make -C libheif/build_linux

* Look for libheif.a and libde265.a static libraries.

Both:
-----
* Include all public headers from libheif/include/libheif
  (e.g., heif.h, heif_context.h, heif_decoding.h, heif_image.h, etc.).
* tacent's Image CMakeLists.txt already defines LIBHEIF_STATIC_BUILD for its
  own compilation; no changes to tacent sources are required.
