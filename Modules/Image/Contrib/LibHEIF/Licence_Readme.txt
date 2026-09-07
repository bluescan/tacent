Both LibHEIF and LibDE265 are licensed under LGPLv3

________________________________________________________________________________
Requirements

When statically linking an LGPLv3-licensed library to your ISC-licensed library,
the key challenge is that LGPLv3 requires the user to be able to modify the
LGPLv3 library and relink the combined work.

Because static linking creates a single combined binary/library artifact where
the code is bound together, distributing a statically linked combined work
imposes additional obligations under Section 4 of the LGPLv3.

If you fulfill the following obligations, you do NOT need to relicense your own
source code under the LGPLv3—your own code can remain strictly under the ISC
License:

1. Provide Object Code or Source Code for Relinking
To allow users to modify the LGPLv3 library and recombine it with your software,
you must provide:

  Option A (Object Code): Distribute the compiled object files (.o, .a, or
  compiled binary format) of your ISC library alongside build tools/instructions
  so users can re-link your library with a modified version of the LGPLv3
  library.

  Option B (Source Code): Provide the full source code of your ISC library
  (which you are already doing) along with instructions on how to build and link
  it against the LGPLv3 library.

2. Retain & Deliver License Notices
Include a copy of both the GNU Lesser General Public License (LGPLv3) and the
GNU General Public License (GPLv3) in your distribution (LGPLv3 incorporates
GPLv3 by reference).

Prominently display a notice in your code/documentation stating that the LGPLv3
library is used and protected under the LGPLv3.

3. Provide LGPLv3 Source Code
Provide the complete, corresponding source code of the LGPLv3 library (or a
written offer/direct link to where the exact version of the source code can be
downloaded).

Best Alternative: Dynamic Linking or Optional Build Flags
Since you mentioned the LGPLv3 library is an optional dependency, the cleanest
legal and technical paths to avoid static linking complications are:

Conditional Build / Preprocessor Flags: Structure your build system so that your
library builds completely pure ISC code by default, and only statically links
the LGPLv3 library if a build flag (e.g., -DENABLE_LGPL_FEATURE=ON) is
explicitly turned on by the consumer. Consumers who compile your code without
the flag get pure ISC software without LGPL requirements.

Dynamic Linking (.so / .dylib / .dll): Dynamically link the LGPLv3 library
instead of statically linking it. Dynamic linking satisfies LGPLv3's relinking
requirements naturally, because users can swap out the shared library file
without needing your object code.

________________________________________________________________________________
Compliance

All above conditions have been met. By complying we are able to keep
distributing Tacent under ISC even if we link to LibHEIF and LibDE265.

1) Option B. Full source code and build instructions are provided at
   https://github.com/bluescan/tacent. To link LibHEIF and LibDE265 pass
   -DTACENT_ENABLE_HEIF.

2) The Licences are available as Licence_LibDE265_LGPLv3.txt and
   Licence_LibHEIF.txt. The GPL which is referenced is found in Licences as
   Standard_GPLV3.txt.

   The main site GitHub README contains a notice of optional LGPLv3 linking for
   these libraries.

3) Links for the exact versions of the libraries' source code are (which was
   not modified) are:

   https://github.com/strukturag/libde265
   FullCommitID: b85929dcf812fecb353ff90d690f917aafbe3b27

   https://github.com/strukturag/libheif
   FullCommitID: 2f3bb8e48c24ea42a0dbc6fc180f44f0d745d811

________________________________________________________________________________
Optional Linking

By default Tacent does not link with LGPLv3 libraries. Building of the UnitTests
does link with LibHEIF and LibDE265.

From Modules/Image/CMakeLists.txt:

# LibHEIF and LibDE265 are supplied as static archives (heif.lib / libde265.lib
# on Windows, libheif.a / libde265.a on Linux). Without this, heif_export.h
# expands LIBHEIF_API to __declspec(dllimport) on Windows, so the compiler
# emits __imp_heif_* references that the static archive does not provide
# (LNK2019). Defining LIBHEIF_STATIC_BUILD collapses LIBHEIF_API to plain C
# linkage so the static symbols resolve correctly.
#
# These are only needed (and the LibHEIF code path in the image classes is only
# compiled) when HEIF support is enabled. HEIF support defaults to OFF so that
# the Tacent static library does not link libheif/libde265 in by default; it is
# turned ON for the in-repo test build (see the top-level CMakeLists.txt).


