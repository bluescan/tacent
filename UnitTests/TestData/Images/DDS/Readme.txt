dds files with 'Legacy' do not contain the D.X.1.0. FourCC header.
dds files with 'Modern' do     contain the D.X.1.0. FourCC header.

If you run the unit-tests there will be a bunch of tga's written to the DDS folder. They all take the form
Written_Format_Channels_Flags.tga
The Flags field contains the load-flags when the unit-tests were performed (or an x if not present).
The flags are:
D : Decode to raw 32-bit RGBA (otherwise leaves the data in same format as in dds file).
G : Gamma-correct resulting image data. Generally not needed if src file is in sRGB already.
R : Reverse rows. Useful for binding textures in OpenGL. Never affects quality. If can't do (eg BC7 and no decode) it will tell you.
S : Spead luminance. If set dds files with luminance will spread (dupe) the data to the RGB channels. Uses Red-only otherwise.

Three tools were tried when generating the dds test images (versions as of Sept 20, 2022).
1) NVidia Texture Tools Generator (NVTT). A drag-and-drop closed-source ImGui application.
2) Microsoft's texconv.exe (TEXC) from their DirectXTex tool suite.
3) AMD's Compressonator (COMP).

The compression quality of NVTT beats TEXC. Try encoding the test image to BC1ba. The opaque gradient at the top left
is quite banded with TEXC no matter what settings are used (although there isn't much choice here). NVTT wins.
Note for binary alpha formats you need a -0.5 alpha-bias in NVTT to get a 0.5 alpha cutoff threshold.

NVTT can't generate as many formats though. The legacy 16-bit and unsigned BC6 for example are not available. In these
cases TEXC is being used. Also there is an out-by-one pixel error in the drawing in the preview window. Open in
tacent-view to see that the data is fine.

NVTT erroneously applies a gamma when saving BC6 HDR dds files. The output is too dark. The input when in sRGB-space
looks to be linear in the output, so you need a gamma-correction step after. I checked the bcdec decode to make sure
it wasn't doing anything wrong (it isn't), and both BC6 16 signed and unsigned images generated from TEXC do not have
this issue. Basically the input image should look the same as the output and be in the same colour-space -- whatever
NVTT does during the conversion is up to them. In any case using TEXC for the dds BC6 test images.

The ASTC versions do not seem to have written the alpha channel properly with NVTT -- At least when dragging them back
into the NVTT exporter, only the BC7 ones show the correct alpha channel.

I didn't use Compressonator much, but generating a BC6 HDR image had a couple of issues. The image was the correct
brightness just like TEXC, but if you look at the RGB colour gradient there were vertical dark 'lines' where each
colour component was supposed to be completely saturated -- one line for each of red, green and blue. It looked like
those spectral lines you get from black-body radiation of pure elements -- definitely an issue with values at 1.0.
There were also some 'bleeding' issues between blocks, or the blocks are all out by one or something, because the test
image has the borders between the top-left gradient areas on multiple of 4 pixel boundaries, so there should have been
nice crisp boundaries between them. NVTT and TEXC didn't have this 'bleeding'.

Summary:
Currently using NVTT where possible for the quality, and TEXC for more obscure pixel formats and HDR BC6.
