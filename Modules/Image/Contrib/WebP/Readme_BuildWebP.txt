Building libwebp

Get it from: https://github.com/webmproject/libwebp/releases

Building static libs is fairly straightforward as that seems to be the default for their premade makefiles. However, the default target does NOT build the 'mux' lib which is needed for saving webp files. Setting the target to all fixed this. Even if it can't build the tools, it will gen the necessary .a and .lib files.

Windows:
* Run x64 Native Tools Command Prompt for VS 2019
* nmake /f Makefile.vc CFG=release-static RTLIBCFG=static OBJDIR=output all
* Look in output folder. (You can share the headers with Linux).

Linux (or use WSL)
* make -f makefile.unix all
* Note, all the deps on packages to load jpg, gif etc are for the conversion tools which we don't need. You can edit the makefile.unix to say you don't have those packages if you like, but in any case, we don't need the tools, so by the time it errors out, the .a files should be there already.
