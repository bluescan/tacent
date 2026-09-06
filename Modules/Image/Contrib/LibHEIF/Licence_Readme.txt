When statically linking an LGPLv3-licensed library to your MIT-licensed library,
the key challenge is that LGPLv3 requires the user to be able to modify the
LGPLv3 library and relink the combined work.

Because static linking creates a single combined binary/library artifact where
the code is bound together, distributing a statically linked combined work
imposes additional obligations under Section 4 of the LGPLv3.

If you fulfill the following obligations, you do NOT need to relicense your own
source code under the LGPLv3—your own code can remain strictly under the MIT
License:

1. Provide Object Code or Source Code for Relinking
To allow users to modify the LGPLv3 library and recombine it with your software,
you must provide:

  Option A (Object Code): Distribute the compiled object files (.o, .a, or
  compiled binary format) of your MIT library alongside build tools/instructions
  so users can re-link your library with a modified version of the LGPLv3
  library.

  Option B (Source Code): Provide the full source code of your MIT library
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
library builds completely pure MIT code by default, and only statically links
the LGPLv3 library if a build flag (e.g., -DENABLE_LGPL_FEATURE=ON) is
explicitly turned on by the consumer. Consumers who compile your code without
the flag get pure MIT software without LGPL requirements.

Dynamic Linking (.so / .dylib / .dll): Dynamically link the LGPLv3 library
instead of statically linking it. Dynamic linking satisfies LGPLv3's relinking
requirements naturally, because users can swap out the shared library file
without needing your object code.

________________________________________________________________________________

All above conditions have been met. I will make this notification more prominent
in a subsequent checkin. Sept 5, 2026.

