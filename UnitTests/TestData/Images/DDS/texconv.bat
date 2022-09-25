goto lastimg

rem BC6H
texconv.exe -f BC6H_SF16 -dx10 -y TacentTestPattern.tga
del BC6Hs_HDRRGB_Modern.dds
ren TacentTestPattern.dds BC6Hs_HDRRGB_Modern.dds

texconv.exe -f BC6H_UF16 -dx10 -y TacentTestPattern.tga
del BC6Hu_HDRRGB_Modern.dds
ren TacentTestPattern.dds BC6Hu_HDRRGB_Modern.dds

rem B5G6R5
texconv.exe -f B5G6R5_UNORM -dx10 -y TacentTestPattern.tga
del B5G6R5_RGB_Modern.dds
ren TacentTestPattern.dds B5G6R5_RGB_Modern.dds

texconv.exe -f B5G6R5_UNORM -dx9 -y TacentTestPattern.tga
del B5G6R5_RGB_Legacy.dds
ren TacentTestPattern.dds B5G6R5_RGB_Legacy.dds

:lastimg
rem B4G4R4A4
texconv.exe -f B4G4R4A4_UNORM -dx10 -y TacentTestPattern.tga
del B4G4R4A4_RGBA_Modern.dds
ren TacentTestPattern.dds B4G4R4A4_RGBA_Modern.dds

texconv.exe -f B4G4R4A4_UNORM -dx9 -y TacentTestPattern.tga
del B4G4R4A4_RGBA_Legacy.dds
ren TacentTestPattern.dds B4G4R4A4_RGBA_Legacy.dds

rem B5G5R5A1
texconv.exe -f B5G5R5A1_UNORM -dx10 -y TacentTestPattern.tga
del B5G5R5A1_RGBA_Modern.dds
ren TacentTestPattern.dds B5G5R5A1_RGBA_Modern.dds

texconv.exe -f B5G5R5A1_UNORM -dx9 -y TacentTestPattern.tga
del B5G5R5A1_RGBA_Legacy.dds
ren TacentTestPattern.dds B5G5R5A1_RGBA_Legacy.dds

