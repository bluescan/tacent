Clone https://github.com/hepcatjk/openexr.git If using smartgit, make sure to check 'include submodules'. This is --recursive on the command line.
The repo above is a cmake file that includes OpenEXR as well as zlib

The files loadimage.h/cpp are not a part of the library proper, but they are not written by me (see copyright at the top).

====================================================
Windows
====================================================
Open x64 Native Tools Command Prompt for VS 2019
mkdir bld
cd bld
cmake .. -DBUILD_SHARED_LIBS=False -DCMAKE_INSTALL_PREFIX=installdir

This will make a VS sln file (nmake files didn't work as well for me).
For each project (select them all) and choose runtime library /MT for Relase and /MTd for Debug
Open the sln. 
Remove all "throw (IEX_NAMESPACE::MathExc)"

The projects are all set incorrectly to DLL even with the shared libs flag above. 
Select 'Release' configuration. And x64 architecture.
Under CMakePredefinedTargets there will be an 'INSTALL' project. Build it.
The libs will be deposited in bld/installdir with nice lib and include dirs.
IlmImf.lib is the main OpenEXR one. It depends on the others.

Do the same for Debug.

====================================================
Linux
====================================================
mkdir bld
cd bld
cmake .. -DBUILD_SHARED_LIBS=False -DCMAKE_INSTALL_PREFIX=instdir -DCMAKE_BUILD_TYPE=Release
make install
Grab libs/headers from instdir

cmake .. -DBUILD_SHARED_LIBS=False -DCMAKE_INSTALL_PREFIX=instdir -DCMAKE_BUILD_TYPE=Debug
make install
Grab libs/headers from instdir

You will need to build static zlib manually in Linux
cd openexr/zlib
mkdir bld
cd bld
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=False
make

