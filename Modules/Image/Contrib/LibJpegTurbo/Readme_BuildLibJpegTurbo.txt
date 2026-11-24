Building libjpeg-turbo
======================

Get libjpeg-turbo 3.2.2 from here: https://github.com/libjpeg-turbo/libjpeg-turbo

________________________________________________________________________________
libjpeg-turbo is a fast, SIMD-accelerated JPEG codec that is API-compatible with
IJG libjpeg. Tacent uses two of its static libraries:

* The base libjpeg (IJG API) library:  lib/jpeg-static.lib (or libjpeg.a on Linux)

* The TurboJpeg API library (simpler compress/decompress/transform API; this is
  what Tacent uses for its tImageJPG loader/saver):
  TurboJpeg/lib/turbojpeg-static.lib (or TurboJpeg/lib/libturbojpeg.a on Linux)

The TurboJpeg API library is a drop-in replacement for the base library (it
contains the full IJG API plus the TurboJpeg API), so linking either one is
sufficient; both are staged for flexibility.

Build settings used for the staged binaries:
  - Static libraries (ENABLE_SHARED=OFF, ENABLE_STATIC=ON)
  - TurboJpeg API enabled (WITH_TURBOJPEG=ON); tools/tests/JNA off
  - x86-64 SIMD ENABLED (WITH_SIMD=ON, the 3.x default): SSE2 baseline codelets
    with AVX2/AVX-512 selected at runtime for the host CPU; assembled by NASM
    (NASM must be on the PATH - see the "SIMD" section below)
  - 8/12/16-bit runtime precision selection (the 3.x default)
  - ARGB (12/16-bit alpha) support OFF, matching the previous 2.0.5 build
  - Windows: MSVC static runtime; lib/jpeg-static.lib is /MT (non-Debug
    configs), lib/jpeg-staticd.lib is /MTd (Debug); same for the TurboJpeg libs
  - Linux: Release build (linked for all configs, per Tacent convention)

Windows (Ninja, VS 2026 x64):
  cmake -S <src> -B build -GNinja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded \
        -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DWITH_TURBOJPEG=ON \
        -DWITH_SIMD=ON \
        -DWITH_TOOLS=OFF -DWITH_TESTS=OFF -DCMAKE_INSTALL_PREFIX=build/install
  cmake --build build --target install
  (Repeat with -DCMAKE_BUILD_TYPE=Debug and MultiThreadedDebug for the "d" libs.)

Linux (or WSL):
  cmake -S <src> -B build -GNinja -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DWITH_TURBOJPEG=ON \
        -DWITH_SIMD=ON \
        -DWITH_TOOLS=OFF -DWITH_TESTS=OFF -DCMAKE_INSTALL_PREFIX=build/install
  cmake --build build --target install
  Produces libjpeg.a and libturbojpeg.a.

SIMD - how to enable and verify
  libjpeg-turbo's speed comes from hand-written NASM "codelets" (SSE2 baseline,
  AVX2/AVX-512 picked at runtime for the host CPU). SIMD is ON by default; the
  controlling CMake option is WITH_SIMD:
      -DWITH_SIMD=ON      build the SIMD codelets (DEFAULT - requires NASM)
      -DWITH_SIMD=OFF     force the portable C fallback (slower; no NASM needed)
  NASM is a hard requirement for the codelets: if it is not on the PATH, CMake
  silently falls back to the C codelets (build still succeeds, just slower).
  Put NASM on the PATH before configuring:
      Windows   : nasm 2.16.03    (e.g. add C:\ClinePads\Tools\nasm-2.16.03 to PATH)
      Linux/WSL : nasm 3.x        (apt-get install -y nasm   or   dnf install -y nasm)
  Verify the staged lib really contains the SIMD codelets (jsimd_* / -*-sse*.o):
      Windows : dumpbin /SYMBOLS lib/jpeg-static.lib  |  findstr /I jsimd
      Linux   : nm libjpeg.a | grep -E 'jsimd|sse2'
  Both staged JPEG libs share the same SIMD core, so libjpeg.a/.lib (IJG API)
  and libturbojpeg.a/turbojpeg-static.lib (tj API) are both SIMD-accelerated.

License: see ../../../../Licences/Licence_LicJpegTurbo.txt
