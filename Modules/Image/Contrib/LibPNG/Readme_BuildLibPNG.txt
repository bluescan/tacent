Download linpng official src code from sourceforge.
Download one of the compressed tar files.
Something like libpng-1.6.42.tar.gz

Windows:
========
Open the vsstudio sln in vstudio directory.o
Add x64 architecture.
Select Release_Library (uses the Multithreaded non-dll libs. /MT)
Turn off /GL (whole prog opt in compiler settings)
Turn off /LTCG (link time code gen, in librarian)
Look for the .lib files in vstudio/x64/Release Library

Linux:
------
You can build from WSL Ubuntu in Windows. Just go into the /mnt/c folder.
	ZLIB:
	   apt install zlib1g
	   apt install zlib1g-dev

    ./configure --prefix=/mnt/c/libpngdest
    make check
    make install
    
It will build x64 libs from WSL. Any file in the dest that is 0 in size is a symlink.
Look for lib/libpng16.a static x64 library.

Both:
-----
Grab the following headers. They will be in include/libpng16
png.h
pngconf.h
pnglibconf.h
