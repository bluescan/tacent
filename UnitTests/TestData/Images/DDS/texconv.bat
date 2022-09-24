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
