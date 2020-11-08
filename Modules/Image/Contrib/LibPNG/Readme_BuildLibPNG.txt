Download linpng official src code from sourceforge.

Windows:
========
Download the .zip with windows line endings. Something like lpng1637.zip
Open the vsstudio sln in vstudio directory.
Add x64 architecture.
Select Release_Library (uses the Multithreaded non-dll libs. /MT)
You may need to disable one of the switch warnings.
Turn off /GL (whole prog opt in compiler settings)
Turn off /LTCG (link time code gen, in librarian)
Add ;5045 in advanced compiler warnings to disable.

Look for the .lib files in vstudio/x64/Release Library

Linux:
------
Download one of the compressed tar file. Something like libpng-1.6.37.tar.gz
Configure in Linux does not like windows line endings.

    ./configure --prefix=/mnt/c/libpngdest
    make check
    make install
    
It will build x64 libs from WSL. Any file in the dest that is 0 in size is a symlink.
Look for lib/libpng16.a static x64 library.


Both:
-----
I suggest using the Linux headers for both plats. Windows handles the LF endings gracefully.
They will be in include/libpng16
png.h
pngconf.h
pnglibconf.h


