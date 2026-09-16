Building libjxl (static) for JPEG XL decoding
=============================================

Tacent's Image module decodes JPEG XL through libjxl's C API (JxlDecoder*).
libjxl is a single static archive (jxl) plus a few static dependencies, all
compiled "built-in" so no runtime DLLs / .so are needed:

  * libjxl     - the JPEG XL codec (jxl, jxl_cms, jxl_threads)  (BSD-3-Clause)
  * highway    - SIMD runtime-dispatch helpers       (hwy)        (BSD-3-Clause)
  * brotli     - entropy / container compression     (brotli*)    (MIT)
  * skcms      - color management (compiled into jxl_cms)                       (3-clause BSD)

Everything lives in Modules/Image/Contrib/LibJXL/ and is linked together by
Modules/Image/CMakeLists.txt (behind TACENT_ENABLE_JXL).

Source: https://github.com/libjxl/libjxl  (v0.13.0, git describe v0.12-snapshot-14)
Third-party submodules (bundled, so no system deps needed):
  third_party/highway, third_party/brotli, third_party/skcms, third_party/lcms

Key configuration (both platforms)
---------------------------------
  BUILD_SHARED_LIBS=OFF        static libs only
  BUILD_TESTING=OFF            skip gtest-based tests (libjxl 0.13 otherwise
                               hard-requires a PNG lib just for its tests)
  JPEGXL_ENABLE_TOOLS=OFF      no djxl/cjxl binaries
  JPEGXL_ENABLE_BENCHMARK/EXAMPLES/JNI/SJPEG/OPENEXR/FUZZERS/VIEWERS/PLUGINS=OFF
  JPEGXL_BUNDLE_LIBPNG=OFF
  JPEGXL_ENABLE_TRANSCODE_JPEG=OFF   we only decode pixels + metadata, never
                                     reconstruct the embedded JPEG
  JPEGXL_ENABLE_BOXES=ON             REQUIRED: the box container API used to
                                     read EXIF / XMP metadata boxes
  JPEGXL_ENABLE_SKCMS=ON             color management via bundled skcms
  HWY SIMD (AVX2/AVX512/...) left at defaults (runtime-dispatched, safe)

Windows (MSVC 14.51 / VS18 Community + Ninja)
---------------------------------------------
  cmake -S <libjxl> -B <build> -GNinja -DCMAKE_BUILD_TYPE=<Release|Debug> \
      -DCMAKE_INSTALL_PREFIX=<install> <options above>
  cmake --build <build> --target install

Linux (WSL Ubuntu-22.04, clang 14 + Ninja; Release)
----------------------------------------------------
  Same options, -DCMAKE_BUILD_TYPE=Release, -G Ninja. Build out-of-source in a
  native (non-9p) dir for speed, then copy install/{lib,include} onto the C:
  drive. NOTE: the build .sh must have Unix (LF) line endings - a CRLF file
  makes every bash value carry a trailing \r (set -e fails, paths break).

Copy into tacent (Modules/Image/Contrib/LibJXL/)
=================================================
  include/jxl/ : decode.h, encode.h, types.h, color_encoding.h,
                 codestream_header.h, version.h, jxl_export.h, ...
                 (tacent only includes <jxl/decode.h>)
  lib/  (trailing "d" = Windows Debug build):
    Windows Release : jxl.lib, jxl_cms.lib, jxl_threads.lib,
                      brotlienc.lib, brotlidec.lib, brotlicommon.lib, hwy.lib
    Windows Debug   : jxld.lib, jxl_cmsd.lib, jxl_threadsd.lib,
                      brotliencd.lib, brotlidecd.lib, brotlicommond.lib, hwyd.lib
    Linux           : libjxl.a, libjxl_cms.a, libjxl_threads.a,
                      libbrotlienc.a, libbrotlidec.a, libbrotlicommon.a, libhwy.a
                      (tacent links the same .a for all Linux configs, like
                       the ZLib / OpenEXR / LibHEIF contribs)

tacent integration (IMPORTANT - static export macro)
=====================================================
  * Include path: add Contrib/LibJXL/include (so #include <jxl/decode.h> works).
  * Windows: define JXL_STATIC_DEFINE on every TU that includes a jxl header.
    libjxl's generated jxl_export.h defaults JXL_EXPORT to __declspec(dllimport)
    unless JXL_STATIC_DEFINE is set; with a static archive that yields LNK2019
    (unresolved __imp_JxlDecoder*). Same class of issue as LIBHEIF_STATIC_BUILD.
    (Not needed on Linux - no dllimport there.)
  * Link order (Linux ld is order-sensitive; put dependers before dependencies):
        jxl, jxl_cms, jxl_threads, brotlienc, brotlidec, brotlicommon, hwy
    plus system -lpthread and -lm on Linux.
    On Windows the MSVC linker is order-insensitive, but link the Debug set
    (jxld/jxl_cmsd/jxl_threadsd/brotli*d/hwyd) in Debug builds and the Release
    set otherwise.

Licenses: libjxl BSD-3-Clause (Licence_LibJXL_BSD-3-Clause.txt),
highway BSD-3-Clause (Licence_highway_BSD-3-Clause.txt; dual-licensed
Apache-2.0/BSD-3-Clause, we elect BSD-3-Clause), brotli MIT
(Licence_brotli_MIT.txt), skcms 3-Clause BSD (Licence_skcms_3-Clause-BSD.txt).