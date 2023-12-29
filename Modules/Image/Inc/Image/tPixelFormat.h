// tPixelFormat.h
//
// Pixel formats in Tacent. Not all formats are fully supported. Certainly BC 4, 5, and 7 may not have extensive HW
// support at this time.
//
// Copyright (c) 2004-2006, 2017, 2019, 2022, 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
namespace tImage
{


// Unlike DirectX, which assumes all machines are little-endian, the enumeration below specifies the components in the
// order they appear _in memory_. This means formats commonly called things like B5G6R5 are actually G3R5B5G3. The
// latter is what they are referred to in Tacent, This inconsistent naming gets worse since when things are byte-aligned
// most vendor pixel formats are actually correct for the mempry representation. In any case, in Tacent, it's always the
// in-memory representation that gets named. BC stands for Block Compression.
//
// A note regarding sRGB. We are _not_ indicating via the pixel format what space/profilr the colour encoded by the
// format is in. Tacent separates the encoding (the pixel format) from how the encoded data is to be interpreted. This
// is in contrast to all the MS DXGI formats where they effectively at least double the number of formats unnecessarily.
//
// A way to think of it is as follows -- You have some input data (Din) that gets encoded using a pixel format (Epf)
// resulting in some output data (Dout). Din -> Epf -> Dout. Without changing Din, if changing Epf would result in
// different Dout, it is correct to have separate formats (eg. BCH6_S vs BCH6_U. DXT1 vs DXT1BA). If changing Epf would
// not result in different Dout then the formats are not different and satellite info should be used if what's stored in
// Din (and Dout) has certain properties (eg. sRGB space vs Linear, premultiplied vs not, DXT2 and DXT3 are the same).
//
// This is also why we don't distinguish between UNORM and UINT for example, as this is just a runtime distinction, not
// an encoding difference (For example, UNORM gets converted to a float in [0.0, 1.0] in shaders, UINT doesn't).
//
// The only exception to this rule is the Tacent pixel format _does_ make distinctions between formats based on the
// colour components being represented. It's not ideal, but pixel formats do generally specify R, G, B, A, L etc and
// what order they appear in. In a perfect world (in my perfect world anyways), R8G8B8 would just be C8C8C8 (C8X3) and
// satellite info would describe what the data represented (RGB in this case). Anyway, that's too much of a divergence.
// This exception is why there is a tPixelFormat R8 (Vulkan has one of these), A8, and L8, all 3 with the same internal
// representation.
//
// Summary of Satellite and Pixel-Format information:
//
// Colour Profile (Satellite)
//    A colour profile basically specifies the colour space for the various components. Sometimes
//    the same space is not used for all components. It is common for RGB to be sRGB but alpha to be linear -- there is
//    a profile for that. See tColourProfile and tColourSpace enum in tColour.h.
//
// Component Format (Pixel-Format)
//    The encoding is different for unsigned int, int, unsigned float, and float. Since the encoding is different,
//    this information IS specified by the pixel format. In particular a lower-case suffix is used for the packed
//    pixel-formats if it is not unsigned int:
//    no suffix	-> unsigned int.
//    s			-> signed int (2's complimemt).
//    uf		-> unsigned float (always >= 0.0). No sign bit.
//    f			-> signed float.
//
//    Some non-packed pixel-formats like BC and EAC distinguich between the encoding of signed vs unsigned data. In
//    these cases we use a single capital letter suffix. If a non-packed encoding does not distinguish, no suffix.
//    No Suffix	-> Format does not distinguish.
//    S			-> Signed Variant.
//    U			-> Unsigned Variant.
//
// Channel Type (Satellite)
//    Sometimes it is intended that the data stored with each component is further modified before being used. In
//    particular it may be normalized. ChannelType is additional satellite information that is not entirely specified
//    by the pixel format so it belongs as satellite information here. In particular the part that isn't specified is
//    whether the component data of each colour channel should be normalized or not afterwards. It gets a little tricky
//    here because Vulkan, OpenGL, and DirectX have all decided on variant pixel-format names with channel-type
//    information like UNORM, SNORM, UINT, SINT, and FLOAT. This naming _includes_ both information about how the data
//    is encoded (integer or float, signed or unsigned) as well as whether to normalize after decoding or not. We have a
//    choice here, either ONLY make this satellite info contain whether to normalize of not afterwards, or have a litte
//    redundant information in order to keep the naming as close as possible to UNORM, UINT, etc. I have decided on the
//    latter.
//
//    The reason it is not part of the pixel format is it is quite common for the data to be encoded as, say, an
//    unsigned integer, but 'converted' to a float when it is passed off the video memory by the graphics API so it is
//    available as a float in the fragment/pixel shader. In short the ChannelType indicates intent for what should
//    happen to the value AFTER decoding. For example, UNORM means the data is stored (or decoded for compressed
//    formats) as an unsigned integer (which is already known by looking at the pixel-format) -- it is then converted to
//    a normalized value in [0.0, 1.0]. SNORM means it's stored as a signed integer and then normalized to the
//    [0.0, 1.0] range. The actual number of bits used is NOT specified here -- that is also specified by the
//    pixel-format itself (either explicitly or implicitly by inspecting the compression method used). I bring this up
//    because, for example, the PVR3 filetype 'channel type' field does contain size information, but it doesn't need to
//    (and probably shouldn't).
//
//    Example 1. PixelFormat: G3B5R5G3  ChanelType: UNORM
//    We know the R and B are stored as 5-bit unsigned ints and the G with six bits. We know this from the PixelFormat
//    alone because it does not contain a 's', 'f', or 'uf'. We further know the intent is to 'normalize' it after
//    decoding. R would be in [0, 31] and converted to [0.0, 1.0]. The 'U' part of 'UNORM' is redundant because the
//    pixel-format already told us it was an unsigned integer.
//
//    Example 2. PixelFormat: R11G11B10uf  ChanelType: UFLOAT
//    RG stored as 11-bit unsigned floats (5 exponent, 6 mantissa, no sign bit). B stored as a 10-bit (5,5) float. In
//    this case the ChannelType is completely redundant because we already know we're using unsigned floats from the 'uf'.
//
//    Example 3. PixelFormat: R8G8  ChanelType: UINT
//    RG stored as 8-bit unsigned ints (from pixel-format). In this case the ChannelType indicates _not_ to normalize so
//    each component should be read as an unsigned integer in [0, 255].
//


enum class tPixelFormat
{
	Invalid				= -1,
	Auto				= Invalid,

	FirstPacked,
	R8					= FirstPacked,	// 8   bit. Unsigned representing red. Some file-types not supporting A8 or L8 (eg ktx2) will export to this.
	R8G8,								// 16  bit. Unsigned representing red and green. Vulkan has an analagous format.
	R8G8B8,								// 24  bit. Full colour. No alpha. Matches GL_RGB source ordering. Not efficient. Most drivers will swizzle to BGR.
	R8G8B8A8,							// 32  bit. Full alpha. Matches GL_RGBA source ordering. Not efficient. Most drivers will swizzle to ABGR.
	B8G8R8,								// 24  bit. Full colour. No alpha. Matches GL_BGR source ordering. Efficient. Most drivers do not need to swizzle.
	B8G8R8A8,							// 32  bit. Full alpha. Matches GL_BGRA source ordering. Most drivers do not need to swizzle.

	G3B5R5G3,							// 16  bit. No alpha. Incorrectly AKA B5G6R5. The truth is in memory it is GGGBBBBB RRRRRGGG -> this is G3B5R5G3.
	G4B4A4R4,							// 16  bit. 12 colour bits. 4 bit alpha. Incorrectly AKA B4G4R4A4.
	B4A4R4G4,							// 16  bit. 12 colour bits. 4 bit alpha. Incorrectly AKA R4G4B4A4.
	G3B5A1R5G2,							// 16  bit. 15 colour bits. Binary alpha. Incorrectly AKA B5G5R5A1.
	G2B5A1R5G3,							// 16  bit. 15 colour bits. Binary alpha. Incorrectly AKA R5G5B5A1.
	A8L8,								// 16  bit. Alpha and Luminance.
	A8,									// 8   bit. Alpha only.
	L8,									// 8   bit. Luminance only.

	R16f,								// 16  bit. Half-float red/luminance channel only.
	R16G16f,							// 32  bit. Two half-floats per pixel. Red and green.
	R16G16B16A16f,						// 64  bit. Four half-floats per pixel. RGBA.
	R32f,								// 32  bit. Float red/luminance channel only.
	R32G32f,							// 64  bit. Two floats per pixel. Red and green.
	R32G32B32A32f,						// 128 bit. HDR format (linear-space), RGBA in 4 floats.
	R11G11B10uf,						// 32  bit. Unsigned 11-bit floats for RG, and a 10-bit float for B. All use a 5-bit exponent.
	B10G11R11uf,						// 32  bit. Unsigned 10-bit floats for B, and 11-bit floats for GR. All use a 5-bit exponent.
	R9G9B9E5uf,							// 32  bit. Unsigned 14-bit floats for RGB. Always denorm and each share the same 5-bit exponent.
	E5B9G9R9uf,							// 32  bit. Unsigned 14-bit floats for RGB. Always denorm and each share the same 5-bit exponent.
	LastPacked			= E5B9G9R9uf,

	FirstBC,
	BC1DXT1				= FirstBC,		// BC 1, DXT1. No alpha.
	BC1DXT1A,							// BC 1, DXT1. Binary alpha.
	BC2DXT2DXT3,						// BC 2, DXT2 (premult-alpha) and DXT3 share the same format. Large alpha gradients (alpha banding).
	BC3DXT4DXT5,						// BC 3, DXT4 (premult-alpha) and DXT5 share the same format. Variable alpha (smooth).
	BC4ATI1U,							// BC 4. Unsigned. One colour channel only. May not be HW supported.
	BC4ATI1S,							// BC 4. Signed. One colour channel only. May not be HW supported.
	BC5ATI2U,							// BC 5. Unsigned. Two colour channels only. May not be HW supported.
	BC5ATI2S,							// BC 5. Signed. Two colour channels only. May not be HW supported.
	BC6U,								// BC 6 HDR. No alpha. 3 x 16bit unsigned half-floats per pixel.
	BC6S,								// BC 6 HDR. No alpha. 3 x 16bit signed half-floats per pixel.
	BC7,								// BC 7. Full colour. Variable alpha 0 to 8 bits.

	FirstETC,
	ETC1				= FirstETC,		// ETC1. Ericsson Texture Compression. Similar to BC1. RGB-only. No alpha.
	ETC2RGB,							// ETC2. Backwards compatible with ETC1. The sRGB version is the same pixel format.
	ETC2RGBA,							// ETC2. RGBA. sRGB uses the same pixel format.
	ETC2RGBA1,							// ETC2. RGB with binary alpha. sRGB uses the same pixel format.
	LastETC				= ETC2RGBA1,

	FirstEAC,
	EACR11U				= FirstEAC,		// EAC R11. Ericsson. Single channel.
	EACR11S,							// EAC R11. Signed.
	EACRG11U,							// EAC RG11. Ericsson. Two channels.
	EACRG11S,							// EAC RG11. Signed.
	LastEAC				= EACRG11S,
	LastBC				= LastEAC,

	FirstPVR,							// PowerVR. Imagination. 8-byte blocks. We do not consider the PVRTC formats to be BC formats because 4 blocks need to be accessed. i.e. The pixels are not 'confined' to the block they are in.
	PVRBPP4				= FirstPVR,		// PVRTC Version 1. 4BPP representing RGB or RGBA channels. One block can encode 4x4 pixels (but needs access to adjacent blocks during decompress).
	PVRBPP2,							// PVRTC Version 1. 2BPP representing RGB or RGBA channels. One block can encode 8x4 pixels.
	PVRHDRBPP8,							// PVRTC Version 1. 8BPP representing HDR RGB.
	PVRHDRBPP6,							// PVRTC Version 1. 6BPP representing HDR RGB.
	PVR2BPP4,							// PVRTC Version 2. 4BPP representing RGB or RGBA channels.
	PVR2BPP2,							// PVRTC Version 2. 2BPP representing RGB or RGBA channels.
	PVR2HDRBPP8,						// PVRTC Version 2. 8BPP representing HDR RGB.
	PVR2HDRBPP6,						// PVRTC Version 2. 6BPP representing HDR RGB.
	LastPVR				= PVR2HDRBPP6,

	FirstASTC,
	ASTC4X4				= FirstASTC,	// 128 bits per 16  pixels. 8    bpp. LDR UNORM.
	ASTC5X4,							// 128 bits per 20  pixels. 6.4  bpp. LDR UNORM.
	ASTC5X5,							// 128 bits per 25  pixels. 5.12 bpp. LDR UNORM.
	ASTC6X5,							// 128 bits per 30  pixels. 4.27 bpp. LDR UNORM.
	ASTC6X6,							// 128 bits per 36  pixels. 3.56 bpp. LDR UNORM.
	ASTC8X5,							// 128 bits per 40  pixels. 3.2  bpp. LDR UNORM.
	ASTC8X6,							// 128 bits per 48  pixels. 2.67 bpp. LDR UNORM.
	ASTC8X8,							// 128 bits per 64  pixels. 2.56 bpp. LDR UNORM.
	ASTC10X5,							// 128 bits per 50  pixels. 2.13 bpp. LDR UNORM.
	ASTC10X6,							// 128 bits per 60  pixels. 2    bpp. LDR UNORM.
	ASTC10X8,							// 128 bits per 80  pixels. 1.6  bpp. LDR UNORM.
	ASTC10X10,							// 128 bits per 100 pixels. 1.28 bpp. LDR UNORM.
	ASTC12X10,							// 128 bits per 120 pixels. 1.07 bpp. LDR UNORM.
	ASTC12X12,							// 128 bits per 144 pixels. 0.89 bpp. LDR UNORM.
	LastASTC			= ASTC12X12,

	FirstVendor,
	RADIANCE			= FirstVendor,	// Radiance HDR.
	OPENEXR,							// OpenEXR HDR.
	LastVendor			= OPENEXR,

	FirstPalette,
	PAL1BIT				= FirstPalette,	// 1-bit indexes to a palette. 2 colour.   1 bpp. Often dithered B/W.
	PAL2BIT,							// 2-bit indexes to a palette. 4 colour.   2 bpp.
	PAL3BIT,							// 3-bit indexes to a palette. 8 colour.   3 bpp.
	PAL4BIT,							// 4-bit indexes to a palette. 16 colour.  4 bpp.
	PAL5BIT,							// 5-bit indexes to a palette. 32 colour.  5 bpp.
	PAL6BIT,							// 6-bit indexes to a palette. 64 colour.  6 bpp.
	PAL7BIT,							// 7-bit indexes to a palette. 128 colour. 7 bpp.
	PAL8BIT,							// 8-bit indexes to a palette. 256 colour. 8 bpp.
	LastPalette			= PAL8BIT,

	NumPixelFormats,
	NumPackedFormats	= LastPacked	- FirstPacked	+ 1,
	NumBCFormats		= LastBC		- FirstBC		+ 1,
	NumPVRFormats		= LastPVR		- FirstPVR		+ 1,
	NumASTCFormats		= LastASTC		- FirstASTC		+ 1,
	NumVendorFormats	= LastVendor	- FirstVendor	+ 1,
	NumPaletteFormats	= LastPalette	- FirstPalette	+ 1
};


// Simple RGB and RGBA formats with different numbers of bits per component and different orderings.
bool tIsPackedFormat	(tPixelFormat);

// Is the format a 4x4 BC (Block Compression) format. This includes ETC and EAC formats. These 4x4
// blocks use various numbers of bits per block.
bool tIsBCFormat		(tPixelFormat);

// Returns true if the format is an ETC BC format. EAC is not considered part of ETC for this function.
// ETC formats are a subset of tIsBCFormat.
bool tIsETCFormat		(tPixelFormat);

// Returns true if the format is an EAC BC format. EAC formats are a subset of tIsBCFormat.
bool tIsEACFormat		(tPixelFormat);

// Is it one of the PVR formats.
bool tIsPVRFormat		(tPixelFormat);

// Is it one of the ASTC (Adaptive Scalable Texture Compression) block formats. Block sizes are avail from 4x4 up
// to 12x12. The 4x4 ASTC variant is not considered a BC format by tIsBCFormat.
bool tIsASTCFormat		(tPixelFormat);

bool tIsVendorFormat	(tPixelFormat);
bool tIsPaletteFormat	(tPixelFormat);
bool tIsAlphaFormat		(tPixelFormat);
bool tIsOpaqueFormat	(tPixelFormat);
bool tIsHDRFormat		(tPixelFormat);
bool tIsLDRFormat		(tPixelFormat);
bool tIsLuminanceFormat	(tPixelFormat);				// Single-channel luminance formats. Includes red-only formats. Does not include alpha only.

// Gets the width/height in pixels of a block in the specified pixel-format. BC blocks are all 4x4. PVR blocks are
// either 4x4 or 8x4. ASTC blocks have varying width/height depending on specific ASTC format -- they vary from 4x4 to
// 12x12. Packed, Vendor, and Palette formats return 1 for width and height. Invalid pixel-formats return 0.
int tGetBlockWidth		(tPixelFormat);
int tGetBlockHeight		(tPixelFormat);

// Given a block-width or block-height and how may pixels you need to store (image-width or image-height), returns the
// number of blocks you will need in that dimension.
int tGetNumBlocks		(int blockWH, int imageWH);

// Only applies to formats that can guarantee an integer number of bits per pixel. In particular does not apply to ASTC
// formats (even if the particular ASTC format has an integer number of bits-per-pixel). We report in bits (not bytes)
// because some formats (i.e. BC1) are only half a byte per pixel. Palette formats do not consider the palette entry,
// size, but rather the size of the index as there is one index per pixel. Returns 0 for non-integral bpp formats and
// all ASTC formats.
int tGetBitsPerPixel(tPixelFormat);

// Works for any pixel format, even if a non-integral number of bits per pixel. In particular does work for ASTC
// formats. Returns 0.0f if pixel format is invalid.
float tGetBitsPerPixelFloat(tPixelFormat);

// This function must be given a BC format, a PVR format, an ASTC format, or a packed format.
// BC formats		: 4x4 with different number of bytes per block.
// PVR formats		: 4x4 or 8x4 for the LDR PVR formats but always 8 bytes. Unknown for the HDR variants.
// ASTC formats		: Varying MxN but always 16 bytes.
// Packed Formats	: Considered 1x1 with varying number of bytes per pixel.
// Returns 0 otherwise.
int tGetBytesPerBlock(tPixelFormat);

const char* tGetPixelFormatName(tPixelFormat);

// Gets the pixel format from its name. Case sensitive. Slow. Use for testing/unit-tests only.
tPixelFormat tGetPixelFormat(const char* name);


// I've decided to put generic aspect ratio stuff in here as well. It doesn't really warrant its own header. This enum
// is for commonly encountered aspect ratios on-screen and in print. The name array may be indexed by the enum values.
// They are ordered from largest to smallest.
enum class tAspectRatio
{
	Invalid,						// Must be 0.
	Free			= Invalid,
	First_Valid,
	First_Screen	= First_Valid,
	Screen_3_1		= First_Screen,	// 3.0
	Screen_2_1,						// 2.0
	Screen_16_9,					// 1.7777777
	Screen_5_3,						// 1.6666666
	Screen_16_10,					// 1.6			Reduces to 8_5
	Screen_8_5,						// 1.6
	Screen_3_2,						// 1.5
	Screen_16_11,					// 1.4545454
	Screen_7_5,						// 1.4
	Screen_4_3,						// 1.3333333
	Screen_22_17,					// 1.2941176
	Screen_14_11,					// 1.2727272
	Screen_5_4,						// 1.25
	Screen_1_1,						// 1.0
	Screen_4_5,						// 0.8
	Screen_11_14,					// 0.7857142
	Screen_17_22,					// 0.7727272
	Screen_3_4,						// 0.75
	Screen_5_7,						// 0.7142857
	Screen_11_16,					// 0.6875
	Screen_2_3,						// 0.6666666
	Screen_5_8,						// 0.625
	Screen_10_16,					// 0.625		Reduces to 5_8
	Screen_3_5,						// 0.6
	Screen_9_16,					// 0.5625
	Screen_1_2,						// 0.5
	Screen_1_3,						// 0.3333333
	Last_Screen		= Screen_1_3,
	NumScreenRatios	= Last_Screen,

	// Print sizes listed by lower of the two dimensions and ordered by the lower size.
	// L means landscape.
	First_Print,
	Print_2x3		= First_Print,	// 0.6666666	Same as 2_3. Wallet size.
	Print_2x3_L,					// 1.5			Same as 3_2. Wallet size.
	Print_3x5,						// 0.6			Same as 3_5.
	Print_3x5_L,					// 1.6666666	Same as 5_3.
	Print_4x4,						// 1.0			Same as 1_1.
	Print_4x6,						// 0.6666666	Same as 2_3.
	Print_4x6_L,					// 1.5			Same as 3_2.
	Print_5x7,						// 0.7142857	Same as 5_7.
	Print_5x7_L,					// 1.4			Same as 7_5.
	Print_5x15,						// 0.3333333	Same as 1_3.
	Print_5x15_L,					// 3.0			Same as 3_1.
	Print_8x8,						// 1.0			Same as 1_1.
	Print_8x10,						// 0.8			Same as 4_5.
	Print_8x10_L,					// 1.25			Same as 5_4.
	Print_8x24,						// 0.3333333	Same as 1_3.
	Print_8x24_L,					// 3.0			Same as 3_1.
	Print_8p5x11,					// 0.7727272	Same as 17_22.
	Print_8p5x11_L,					// 1.2941176	Same as 22_17.
	Print_9x16,						// 0.5625		Same as 9_16.
	Print_9x16_L,					// 1.7777777	Same as 16_9.
	Print_11x14,					// 0.7857142	Same as 11_14.
	Print_11x14_L,					// 1.2727272	Same as 14_11.
	Print_11x16,					// 0.6875		Same as 11_16.
	Print_11x16_L,					// 1.4545454	Same as 16_11.
	Print_12x12,					// 1.0			Same as 1_1.
	Print_12x18,					// 0.6666666	Same as 2_3.
	Print_12x18_L,					// 1.5			Same as 3_2.
	Print_12x36,					// 0.3333333	Same as 1_3.
	Print_12x36_L,					// 3.0			Same as 3_1.
	Print_16x20,					// 0.8			Same as 4_5.
	Print_16x20_L,					// 1.25			Same as 5_4.
	Print_18x24,					// 0.75			Same as 3_4.
	Print_18x24_L,					// 1.3333333	Same as 4_3.
	Print_20x30,					// 0.6666666	Same as 2_3.
	Print_20x30_L,					// 1.5			Same as 3_2.
	Print_24x36,					// 0.6666666	Same as 2_3.
	Print_24x36_L,					// 1.5			Same as 3_2.
	Last_Print		= Print_24x36_L,
	Last_Valid		= Last_Print,
	NumRatios,						// Including Invalid.
	User			= NumRatios
};

// The 'User' aspect ratio name is included in this array as the last item.
extern const char* tAspectRatioNames[int(tAspectRatio::NumRatios)+1];

bool tIsScreenRatio(tAspectRatio);
bool tIsPrintRatio(tAspectRatio);
bool tisValidRatio(tAspectRatio);

// Returns 0.0f for Invalid/Free. Returns -1.0f for User.
float tGetAspectRatioFloat(tAspectRatio);

// If Invalid/Free/User is passed in, returns false and sets numerator and denominator to 0.
// Otherwise returns true and fills in numerator and denominator in reduced form (16:10 -> 8:5).
bool tGetAspectRatioFrac(int& numerator, int& denominator, tAspectRatio);

// Returns the aspect ratio given numerator and denominator. Returns Invalid if either numerator or denominator
// are <= 0. Returns User if the ratio doesn't exist in the enum. Returns the most reduced Screen_ ratio
// otherwise. For example tGetAspectRatio(32,20) returns Screen_8_5 rather than Screen_16_10. Does not return
// any of the Print_ enumerants.
tAspectRatio tGetAspectRatio(int numerator, int denominator);

// Gets the most reduced screen enumerant given a valid aspect ratio. Returns Invalid if Invalid passed in.
// Returns User if User passed in. The function tReduceAspectRatio does the same thing, just different syntax/calling.
tAspectRatio tGetReducedAspectRatio(tAspectRatio);
void tReduceAspectRatio(tAspectRatio&);

}


// Implementation below this line.


inline bool tImage::tIsPackedFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstPacked) && (format <= tPixelFormat::LastPacked))
		return true;

	return false;
}


inline bool tImage::tIsBCFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstBC) && (format <= tPixelFormat::LastBC))
		return true;

	return false;
}


inline bool tImage::tIsETCFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstETC) && (format <= tPixelFormat::LastETC))
		return true;

	return false;
}


inline bool tImage::tIsEACFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstEAC) && (format <= tPixelFormat::LastEAC))
		return true;

	return false;
}


inline bool tImage::tIsPVRFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstPVR) && (format <= tPixelFormat::LastPVR))
		return true;

	return false;
}


inline bool tImage::tIsASTCFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstASTC) && (format <= tPixelFormat::LastASTC))
		return true;

	return false;
}


inline bool tImage::tIsVendorFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstVendor) && (format <= tPixelFormat::LastVendor))
		return true;

	return false;
}


inline bool tImage::tIsPaletteFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstPalette) && (format <= tPixelFormat::LastPalette))
		return true;

	return false;
}


inline bool tImage::tIsAlphaFormat(tPixelFormat format)
{
	switch (format)
	{
		case tPixelFormat::R8G8B8A8:
		case tPixelFormat::B8G8R8A8:
		case tPixelFormat::G4B4A4R4:
		case tPixelFormat::B4A4R4G4:
		case tPixelFormat::G3B5A1R5G2:
		case tPixelFormat::G2B5A1R5G3:
		case tPixelFormat::A8L8:
		case tPixelFormat::R16G16B16A16f:
		case tPixelFormat::R32G32B32A32f:
		case tPixelFormat::BC1DXT1A:
		case tPixelFormat::BC2DXT2DXT3:
		case tPixelFormat::BC3DXT4DXT5:
		case tPixelFormat::BC7:
		case tPixelFormat::OPENEXR:

		// For palettized the palette may have an entry that can be considered alpha. However for only 1-bit
		// palettes we consider it dithered (ColourA/ColourB) and not to have an alpha.
		case tPixelFormat::PAL2BIT:
		case tPixelFormat::PAL3BIT:
		case tPixelFormat::PAL4BIT:
		case tPixelFormat::PAL5BIT:
		case tPixelFormat::PAL6BIT:
		case tPixelFormat::PAL7BIT:
		case tPixelFormat::PAL8BIT:
			return true;
	}

	// Not quite sure how to handle ASTC formats, but they usually contain an alpha.
	if (tIsASTCFormat(format))
		return true;

	// PVR non-HDR formats all support alpha.
	if (tIsPVRFormat(format) && tIsLDRFormat(format))
		return true;

	return false;
}


inline bool tImage::tIsOpaqueFormat(tPixelFormat format)
{
	return !tImage::tIsAlphaFormat(format);
}


inline bool tImage::tIsHDRFormat(tPixelFormat format)
{
	switch (format)
	{
		case tPixelFormat::R16f:
		case tPixelFormat::R16G16f:
		case tPixelFormat::R16G16B16A16f:
		case tPixelFormat::R32f:
		case tPixelFormat::R32G32f:
		case tPixelFormat::R32G32B32A32f:
		case tPixelFormat::R11G11B10uf:
		case tPixelFormat::B10G11R11uf:
		case tPixelFormat::R9G9B9E5uf:
		case tPixelFormat::E5B9G9R9uf:
		case tPixelFormat::BC6U:
		case tPixelFormat::BC6S:
		case tPixelFormat::RADIANCE:
		case tPixelFormat::OPENEXR:
		case tPixelFormat::PVRHDRBPP8:
		case tPixelFormat::PVRHDRBPP6:
		case tPixelFormat::PVR2HDRBPP8:
		case tPixelFormat::PVR2HDRBPP6:
			return true;
	}

	// ASTCNxM can be LDR or HDR, but since they are not guaranteed to be HDR we return false for them.
	return false;
}


inline bool tImage::tIsLDRFormat(tPixelFormat format)
{
	return !tImage::tIsHDRFormat(format);
}


inline bool tImage::tIsLuminanceFormat(tPixelFormat format)
{
	switch (format)
	{
		case tPixelFormat::L8:
		case tPixelFormat::R8:
		case tPixelFormat::R16f:
		case tPixelFormat::R32f:
		case tPixelFormat::BC4ATI1U:
		case tPixelFormat::BC4ATI1S:
		case tPixelFormat::EACR11U:
		case tPixelFormat::EACR11S:
			return true;
	}

	return false;
}


inline int tImage::tGetNumBlocks(int blockWH, int imageWH)
{
	tAssert(blockWH > 0);
	return (imageWH + blockWH - 1) / blockWH;
}


inline bool tImage::tIsScreenRatio(tAspectRatio ratio)
{
	return (int(ratio) >= int(tAspectRatio::First_Screen)) && (int(ratio) <= int(tAspectRatio::Last_Screen));
}


inline bool tImage::tIsPrintRatio(tAspectRatio ratio)
{
	return (int(ratio) >= int(tAspectRatio::First_Print)) && (int(ratio) <= int(tAspectRatio::Last_Print));
}


inline bool tImage::tisValidRatio(tAspectRatio ratio)
{
	return (int(ratio) >= int(tAspectRatio::First_Valid)) && (int(ratio) <= int(tAspectRatio::Last_Valid));
}


inline void tImage::tReduceAspectRatio(tAspectRatio& aspect)
{
	aspect = tGetReducedAspectRatio(aspect);
}


inline float tImage::tGetAspectRatioFloat(tAspectRatio aspect)
{
	tReduceAspectRatio(aspect);
	switch (aspect)
	{
		case tAspectRatio::Invalid:			return 0.0f;
		case tAspectRatio::User:			return -1.0f;
	}

	int numerator, denominator;
	bool ok = tGetAspectRatioFrac(numerator, denominator, aspect);
	if (!ok)
		return 0.0f;

	return float(numerator) / float(denominator);
}
