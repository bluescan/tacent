# tacent
project(jpeg VERSION 0.1.0)

include_directories(
    include
)

list (APPEND src
    jaricom.c
    Jcapimin.c
    Jcapistd.c
    jcarith.c
    Jccoefct.c
    Jccolor.c
    Jcdctmgr.c
    Jchuff.c
    # Jchuff.h
    Jcinit.c
    Jcmainct.c
    Jcmarker.c
    Jcmaster.c
    Jcomapi.c
    Jconfig.h
    Jcparam.c
    Jcprepct.c
    Jcsample.c
    Jctrans.c
    Jdapimin.c
    Jdapistd.c
    jdarith.c
    Jdatadst.c
    Jdatasrc.c
    Jdcoefct.c
    Jdcolor.c
    Jdct.h
    Jddctmgr.c
    Jdhuff.c
    # Jdhuff.h
    Jdinput.c
    Jdmainct.c
    Jdmarker.c
    Jdmaster.c
    Jdmerge.c
    Jdpostct.c
    Jdsample.c
    Jdtrans.c
    Jerror.c
    Jerror.h
    Jfdctflt.c
    Jfdctfst.c
    Jfdctint.c
    Jidctflt.c
    Jidctfst.c
    Jidctint.c
    Jinclude.h
    Jmemmgr.c
    Jmemnobs.c
    Jmemsys.h
    Jmorecfg.h
    Jpegint.h
    Jpeglib.h
    Jquant1.c
    Jquant2.c
    Jutils.c
    Jversion.h
)

add_definitions(-DEXCLUDE_JPG_SUPPORT)

find_package(Properties REQUIRED)
