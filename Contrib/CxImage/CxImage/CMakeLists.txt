# tacent
project(CxImage VERSION 0.1.0)

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

find_package(Properties REQUIRED)
