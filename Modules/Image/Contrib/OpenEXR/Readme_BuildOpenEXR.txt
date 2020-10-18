
The files loadimage.h/cpp are not a part of the library proper, but they are not written by me (see copyright at the top).
Get zlib from https://github.com/madler/zlib

====================================================
Windows
====================================================
Open x64 Native Tools Command Prompt for VS 2019
Use cmake to build zlib.
Use cmake .. -DBUILD_SHARED_LIBS=False -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=installdir
Then use VS to build sln (INSTALL project).
Files will be in installdir

Grab OpenEXR from official GitHub site
Open x64 Native Tools Command Prompt for VS 2019
Trick is to set vars for zlib:
cmake .. -DCMAKE_INSTALL_PREFIX=installdir -DBUILD_SHARED_LIBS=False -DZLIB_LIBRARY=C:\openexr-2.5.3\zlib\zlibstatic.lib -DZLIB_INCLUDE_DIR=C:\openexr-2.5.3\zlib


The projects are all set incorrectly to DLL libs even with the shared libs flag above.
The runtime lib should be /MT (Release) and /MTd (Debug). You can multiselect the projects and change them all at once.

Select 'Release' configuration. And x64 architecture.
Build ALL and then INSTALL projects.
The libs will be deposited in bld/installdir with nice lib and include dirs.
IlmImf.lib is the main OpenEXR one. It depends on the others.

Do the same for Debug. The libs will have _d added to them.


====================================================
Linux
====================================================
Compile zlib similarly (static libz.a)
cd zlib
mkdir bld
cd bld
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=False
make

For OpenEXR
mkdir builddeb
cd builddeb
cmake .. -DBUILD_SHARED_LIBS=False -DCMAKE_INSTALL_PREFIX=instdir -DCMAKE_BUILD_TYPE=Debug
make install
Grab libs/headers from instdir

Do same for Release in a buildrel dir

