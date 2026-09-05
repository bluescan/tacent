Licences
--------
Licensing information for the test images may be see one directory up in the Readme.txt

HDR Test Images
---------------
Three tools were used when generating the dds test images (versions as of Sept 20, 2022).
1) NVidia Texture Tools Generator (NVTT). A drag-and-drop closed-source ImGui application.
2) Microsoft's texconv.exe (TEXC) from their DirectXTex tool suite.
3) AMD's Compressonator (COMP).

The BC6U_RGB_Modern.dds and BC6S_RGB_Modern.dds test images were generated from TacentTestPattern.tga. The test pattern
image is _not_ an HDR image, so there will no HDR data to leverage -- the 32bit RGBs were just be converted to floats,
so you won't get good exposure control for example. These two images were generated with TEXC converter and are in sRGB
(artist-authoring) space. TEXC did not convert them to linear-space, so there is no need to gamma-correct them when
loading / displaying -- they are already in sRGB.

BC6s_HDRRGB_Modern.dds and all of the signed-floating-point dds files _were_ generated from a proper floating-point
source image (Desk.exr from a directory up). These ones will have the extra information needed since they were a
float-to-float conversion. They were all created by NVTT.

The way NVTT works is it looks at the input file-type and assumes that non-floating-point files are in sRGB. When
saving NVTT puts any floating-point output in linear-space.

NVTT does not support BC6U (unsigned). Unfortunately TEXC was unable to load the (perfectly valid) Desk.exr file.
So... to create the BC6U_HDRRGB_Modern.dds, TEXC was used with the R32G32B32 linear-space file and converted it
to the BC6U_HDRRGB_Modern.dds unsigned format. The BC6 is the only HDR format that supports unsigned floats.

Encoding Tools
--------------
dds files with 'Legacy' do not contain the D.X.1.0. FourCC header.
dds files with 'Modern' do     contain the D.X.1.0. FourCC header.

If you run the unit-tests there will be a bunch of tga's written to the DDS folder. They all take the form
Written_Format_Channels_Flags.tga
The Flags field contains the load-flags when the unit-tests were performed (or an x if not present). The flags are:

D : Decode to raw 32-bit RGBA (otherwise leaves the data in same format as in dds file).
G : Gamma-correct resulting image data. Generally not needed if src file is in sRGB already.
R : Reverse rows. Useful for binding textures in OpenGL. Never affects quality. If can't do (eg BC7 and no decode) it
    will tell you.
S : Spead luminance. If set dds files with luminance will spread (dupe) the data to the RGB channels. Uses Red-only
    otherwise.

The compression quality of NVTT beats TEXC. Try encoding the test image to BC1a. The opaque gradient at the top left
is quite banded with TEXC no matter what settings are used (although there isn't much choice here). NVTT wins.
Note for binary alpha formats you need a -0.5 alpha-bias in NVTT to get a 0.5 alpha cutoff threshold.

NVTT can't generate as many formats though. The legacy 16-bit and unsigned BC6 for example are not available. In these
cases TEXC is being used. Also there is an out-by-one pixel error in the drawing in the preview window. Open in
tacentview to see that the data is fine.

Warning: NVTT does not generate a BC1(no-binary-alpha) image even when set to BC1(no-binary-alpha) if the input image
is 32bit and has an alpha channel. That's why there is a 24bit tga of the test pattern.

The ASTC versions do not _seem_ to have written the alpha channel properly with NVTT -- At least when dragging them
back into the NVTT exporter, only the BC7 ones show the correct alpha channel.

NVTT can't write the unsigned BC6 HDR format. TEXC can, but can't read valid EXR files.

I didn't use Compressonator much, but generating a BC6 HDR image had a couple of issues. After converting the test-
pattern the RGB colour gradient (top-left quadrant) had vertical dark 'lines' where each colour component was supposed
to be completely saturated -- one line for each of red, green and blue. It looked like those spectral lines you get from
black-body radiation of pure elements -- definitely an issue with values at 1.0. There were also some 'bleeding' issues
between blocks, or the blocks are all out by one or something. In the test image the gradients are 90 pixels high, so
only every 2nd row will they be on multiple of 4 boundaries... otherwise they fall halfway through a 4x4 block. NVTT and
TEXC didn't have this 'bleeding' issue.

Compressonator (COMP) was used for creating the legacy dds files that use the ETC encoder. There are 4 of these files.
one for each COMP-Supported ETC format: ETC1, ETC2RGB, ETC2RGBA, and ETC2RGBA1. Note that COMP encodes incorrectly. It
swaps the R and B channels. That is, they interpreted the texture-format name literally. The D3DFMT names are wrong and
MS fixed this with their DXGI names. In the case of the 4 COMP-generated test images, the swizzle to swap R and B was
performed before encoding for better-quality results (colour channels are not independant) and correct dds files.

Summary:
Currently using NVTT where possible for the quality, and TEXC for more obscure pixel formats and HDR BC6 unsigned.
