find_package("TacentProjectUtilities" REQUIRED)
project(CxImage VERSION ${TACENT_VERSION} LANGUAGES C CXX)

add_library(
	${PROJECT_NAME}
	Src/tif_xfile.cpp
	Src/ximabmp.cpp
	Src/ximadsp.cpp
	Src/ximaenc.cpp
	Src/ximaexif.cpp
	Src/ximage.cpp
	Src/ximagif.cpp
	Src/ximahist.cpp
	Src/ximaico.cpp
	Src/ximainfo.cpp
	Src/ximaint.cpp
	Src/ximajas.cpp
	Src/ximajbg.cpp
	Src/ximajpg.cpp
	Src/ximalpha.cpp
	Src/ximalyr.cpp
	Src/ximamng.cpp
	Src/ximapal.cpp
	Src/ximapcx.cpp
	Src/ximapng.cpp
	Src/ximapsd.cpp
	Src/ximaraw.cpp
	Src/ximasel.cpp
	Src/ximaska.cpp
	Src/ximatga.cpp
	Src/ximath.cpp
	Src/ximatif.cpp
	Src/ximatran.cpp
	Src/ximawbmp.cpp
	Src/ximawmf.cpp
	Src/ximawnd.cpp
	Src/xmemfile.cpp
	Inc/xfile.h
	Inc/ximabmp.h
	Inc/ximacfg.h
	Inc/ximadef.h
	Inc/ximage.h
	Inc/ximagif.h
	Inc/ximaico.h
	Inc/ximaiter.h
	Inc/ximajas.h
	Inc/ximajbg.h
	Inc/ximajpg.h
	Inc/ximamng.h
	Inc/ximapcx.h
	Inc/ximapng.h
	Inc/ximapsd.h
	Inc/ximaraw.h
	Inc/ximaska.h
	Inc/ximatga.h
	Inc/ximath.h
	Inc/ximatif.h
	Inc/ximawbmp.h
	Inc/ximawmf.h
	Inc/xiofile.h
	Inc/xmemfile.h

	# Png
	Src/png/png.c
	Src/png/pngerror.c
	Src/png/pngget.c
	Src/png/pngmem.c
	Src/png/pngpread.c
	Src/png/pngread.c
	Src/png/pngrio.c
	Src/png/pngrtran.c
	Src/png/pngrutil.c
	Src/png/pngset.c
	Src/png/pngtrans.c
	Src/png/pngwio.c
	Src/png/pngwrite.c
	Src/png/pngwtran.c
	Src/png/pngwutil.c
	Src/png/png.h
	Src/png/pngconf.h

	# Tiff
	Src/tiff/tif_aux.c
	Src/tiff/tif_close.c
	Src/tiff/tif_codec.c
	Src/tiff/tif_color.c
	Src/tiff/tif_compress.c
	Src/tiff/tif_dir.c
	Src/tiff/tif_dirinfo.c
	Src/tiff/tif_dirread.c
	Src/tiff/tif_dirwrite.c
	Src/tiff/tif_dumpmode.c
	Src/tiff/tif_error.c
	Src/tiff/tif_fax3.c
	Src/tiff/tif_fax3sm.c
	Src/tiff/tif_flush.c
	Src/tiff/tif_getimage.c
	Src/tiff/tif_jpeg.c
	Src/tiff/tif_luv.c
	Src/tiff/tif_lzw.c
	Src/tiff/tif_next.c
	Src/tiff/tif_ojpeg.c
	Src/tiff/tif_open.c
	Src/tiff/tif_packbits.c
	Src/tiff/tif_pixarlog.c
	Src/tiff/tif_predict.c
	Src/tiff/tif_print.c
	Src/tiff/tif_read.c
	Src/tiff/tif_strip.c
	Src/tiff/tif_swab.c
	Src/tiff/tif_thunder.c
	Src/tiff/tif_tile.c
	Src/tiff/tif_version.c
	Src/tiff/tif_warning.c
	Src/tiff/tif_write.c
	Src/tiff/tif_zip.c
	Src/tiff/t4.h
	Src/tiff/tif_dir.h
	Src/tiff/tif_fax3.h
	Src/tiff/tif_predict.h
	Src/tiff/tiff.h
	Src/tiff/tiffconf.h
	Src/tiff/tiffio.h
	Src/tiff/tiffiop.h
	Src/tiff/uvcode.h
)

target_include_directories(
	"${PROJECT_NAME}"
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Inc>
		$<INSTALL_INTERFACE:Inc>
	PRIVATE
		${CMAKE_SOURCE_DIR}/Contrib/ZLib/include
		${CMAKE_CURRENT_SOURCE_DIR}/Inc
		${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_compile_definitions(
	${PROJECT_NAME}
	PUBLIC
		ARCHITECTURE_X64
		#EXCLUDE_JPG_SUPPORT
		$<$<CONFIG:Debug>:CONFIG_DEBUG>
		$<$<CONFIG:Release>:CONFIG_RELEASE>
		$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_DEPRECATE _LIB JAS_WIN_MSVC_BUILD>
		$<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS>
		$<$<PLATFORM_ID:Linux>:PLATFORM_LINUX _LINUX>
)

tacent_target_compile_options(${PROJECT_NAME})
tacent_target_compile_features(${PROJECT_NAME})
tacent_set_target_properties(${PROJECT_NAME})

# This path is relative to CMAKE_INSTALL_PREFIX. Do not use an absolute path if you want
# the exports to work properly (the will have bad absolute paths if you do).
set(TACENT_INSTALL_RELDIR "TacentInstall")
install(
	TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}-targets
	LIBRARY DESTINATION ${TACENT_INSTALL_RELDIR}
	ARCHIVE DESTINATION ${TACENT_INSTALL_RELDIR}
)

install(DIRECTORY Inc/ DESTINATION "${TACENT_INSTALL_RELDIR}/Inc/CxImage")

install(
	EXPORT ${PROJECT_NAME}-targets
	FILE
		${PROJECT_NAME}Targets.cmake
	NAMESPACE
		Tacent::
	DESTINATION
		${TACENT_INSTALL_RELDIR}
)
