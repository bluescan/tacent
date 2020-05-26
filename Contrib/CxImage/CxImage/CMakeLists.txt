# Setup configurations before specifying the project.
find_package("SetupTacentConfigurations")

# Give the project a name and specify explicitly the languages used.
project(CxImage VERSION ${TacentVersion} LANGUAGES CXX)

# TODO Still need to modernize the stuff below to specify deps (link_libraries) and export 'the modern way'.
# TODO Make the SetupTacentProjectProperties have a functional interface so we can customize the headers.
include_directories(
	../jasper/include
	../jpeg
	../mng
	../png
	../tiff
	../zlib
)

list (APPEND src
	tif_xfile.cpp
	ximabmp.cpp
	ximadsp.cpp
	ximaenc.cpp
	ximaexif.cpp
	xImage.cpp
	ximagif.cpp
	ximahist.cpp
	ximaico.cpp
	ximainfo.cpp
	ximaint.cpp
	ximajas.cpp
	ximajbg.cpp
	ximajpg.cpp
	ximalpha.cpp
	ximalyr.cpp
	ximamng.cpp
	ximapal.cpp
	ximapcx.cpp
	ximapng.cpp
	ximapsd.cpp
	ximaraw.cpp
	ximasel.cpp
	ximaska.cpp
	ximatga.cpp
	ximath.cpp
	ximatif.cpp
	ximatran.cpp
	ximawbmp.cpp
	ximawmf.cpp
	ximawnd.cpp
	xmemfile.cpp
	stdint.h
	xfile.h
	ximabmp.h
	ximacfg.h
	ximadef.h
	ximage.h
	ximagif.h
	ximaico.h
	ximaiter.h
	ximajas.h
	ximajbg.h
	ximajpg.h
	ximamng.h
	ximapcx.h
	ximapng.h
	ximapsd.h
	ximaraw.h
	ximaska.h
	ximatga.h
	ximath.h
	ximatif.h
	ximawbmp.h
	ximawmf.h
	xiofile.h
	xmemfile.h
)

@ TODO This is the older-style module that isn't going to be used.
find_package(Properties REQUIRED)
