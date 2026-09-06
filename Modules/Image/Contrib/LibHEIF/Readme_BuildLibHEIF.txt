Building libheif, libde265 and dav1d (static) for AVIF + HEIC
================================================================

Tacent's Image module decodes AVIF and HEIC through libheif, which delegates
pixel decoding to two static decoder libs (both compiled "built-in", so no
runtime plugin DLLs / .so are needed):

  * libde265  - HEVC decoder -> HEIC   (LGPL-3.0)
  * dav1d     - AV1  decoder -> AVIF   (BSD-2-Clause)

The three static libs (libheif + libde265 + dav1d) live in
Modules/Image/Contrib/LibHEIF/lib/ and are linked together by
Modules/Image/CMakeLists.txt.

Sources:
  * https://github.com/strukturag/libheif
  * https://github.com/strukturag/libde265
  * https://code.videolan.org/videolan/dav1d

Key configuration (all three libs, both platforms, Debug + Release)
-------------------------------------------------------------------
  BUILD_SHARED_LIBS=OFF      static libs only
  ENABLE_PLUGIN_LOADING=OFF  decoders are "built-in"; libheif emits de265_* /
                             dav1d_* undefined symbols that we resolve by
                             linking libde265 and dav1d (no runtime plugins)
  WITH_LIBDE265=ON           enable the HEIC (HEVC) decoder
  WITH_DAV1D=ON              enable the AVIF (AV1) decoder
  (all other codecs OFF: x265, kvazaar, aom, svt, rav1e, jpeg, openjpeg,
   ffmpeg, openh264, x264, openjph, uvg266, vvenc, vvdec)

A correct libheif configure prints, in the "Supported formats" summary:
  AVIF  decoding YES ; HEIC decoding YES
  Dav1d AV1 decoder: + built-in ; libde265 HEVC decoder: + built-in

Windows (MSVC) - IMPORTANT static-build macros
----------------------------------------------
libde265/libheif headers use __declspec(dllimport) by default. On Windows the
static .lib files MUST be compiled with these defines, else their objects
reference __imp_de265_* / __imp_heif_* import-thunk symbols that do not exist
in a static archive, causing LNK2019 "unresolved external symbol" at tacent link:
  LIBDE265_STATIC_BUILD  - when building libde265 AND when compiling libheif
  LIBHEIF_STATIC_BUILD   - when compiling libheif (also defined by tacent's own
                           Image CMakeLists.txt for tacent's objects)
Pass them via CMAKE_CXX_FLAGS when configuring libheif. MSVC-only (not needed on Linux).

Build order (for each of Release, then Debug):
  1) dav1d (meson) -> libdav1d.lib / libdav1d.a, dav1d.h, dav1d.pc
  2) libde265 (cmake) -> libde265.lib / libde265.a, de265.h
       cmake -S libde265 -B libde265/build_win -G "NMake Makefiles" \
         -DBUILD_SHARED_LIBS=OFF -DLIBDE265_STATIC_BUILD=1 -DENABLE_SDL=OFF \
         -DENABLE_ENCODER=OFF -DCMAKE_BUILD_TYPE=<Release|Debug> \
         -DCMAKE_INSTALL_PREFIX=<libde265>/install_win_<cfg>
       cmake --build libde265/build_win --target install
  3) libheif (cmake) -> heif.lib
       cmake -S libheif -B libheif/build_win -G "NMake Makefiles" \
         -DBUILD_SHARED_LIBS=OFF -DENABLE_PLUGIN_LOADING=OFF \
         -DWITH_LIBDE265=ON -DWITH_DAV1D=ON <all other WITH_*=OFF> \
         -DCMAKE_BUILD_TYPE=<Release|Debug> \
         -DCMAKE_PREFIX_PATH=<dav1d>/install_win_<cfg>;<libde265>/install_win_<cfg> \
         "-DCMAKE_CXX_FLAGS=/DLIBDE265_STATIC_BUILD /DLIBHEIF_STATIC_BUILD"
       cmake --build libheif/build_win

Linux (WSL; Release + Debug): same three steps. dav1d via meson
(--default-library=static), libde265 + libheif via cmake -G Ninja, using
CMAKE_PREFIX_PATH pointing at the dav1d and libde265 install prefixes.
  NOTE (bash): -DCMAKE_PREFIX_PATH contains a ';' list separator. In bash the
  WHOLE -D argument must be quoted ("...;..."), else bash splits on ';' and
  silently drops a decoder (e.g. libde265). In cmd.exe / PowerShell ';' is
  literal, so no quoting is needed there.

Copy into tacent (Modules/Image/Contrib/LibHEIF/)
==================================================
lib/  (trailing "d" = Windows Debug build):
  Windows Release : heif.lib (build_win_release/libheif/heif.lib),
                    libde265.lib, libdav1d.lib
  Windows Debug   : heifd.lib (build_win_debug/libheif/heif.lib),
                    libde265d.lib, libdav1dd.lib
  Linux           : libheif.a (build_linux_release/libheif/libheif.a),
                    libde265.a, libdav1d.a
                    (tacent links the release .a for all Linux configs, like the
                     ZLib / OpenEXR contribs)
include/libheif/ : heif.h, heif_context.h, heif_decoding.h, heif_image.h,
                   heif_version.h, ... (tacent only includes <libheif/heif.h>;
                   dav1d / de265 headers are not needed by tacent's sources)

tacent integration
==================
Modules/Image/CMakeLists.txt already defines LIBHEIF_STATIC_BUILD, adds
Contrib/LibHEIF/include to the include path, and (behind TACENT_ENABLE_HEIF)
links the three static libs per platform, and on Windows per config:
  Debug  -> heifd.lib, libde265d.lib, libdav1dd.lib
  others -> heif.lib,  libde265.lib,  libdav1d.lib
  Linux  -> libheif.a, libde265.a,  libdav1d.a

Licenses: libheif LGPL-3.0 (Licence_LibHEIF.txt), libde265 LGPL-3.0
(Licence_LibDE265_LGPLv3.txt), dav1d BSD-2-Clause
(Licence_dav1d_BSD-2-Clause.txt).
