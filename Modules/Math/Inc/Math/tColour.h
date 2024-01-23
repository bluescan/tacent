// tColour.h
//
// Colour and pixel classes. There are classes for:
// * A 24 bit colour. 3 unsigned 8-bit integer components (rgb).
// * A 32 bit colour. 4 unsigned 8-bit integer components (rgb + alpha).
// * A 48 bit colour. 3 unsigned 16-bit integer components (rgb).
// * A 64 bit colour. 4 unsigned 16-bit integer components (rgb + alpha).
// * A 96 bit colour. 3 32-bit float components.
// * A 128 bit colour. 4 32-bit float components (rgb + alpha).
//
// Copyright (c) 2006, 2011, 2017, 2020, 2022-2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Foundation/tFundamentals.h>
#include "Math/tVector3.h"
#include "Math/tVector4.h"


// We actually don't need all these forward declarations, but they give an overview of the types in this header. Each
// row contains synonyms in the comments. The format is tColourNC where N is the number of components and C is:
// 'b' for byte  components (unsigned 8-bit  integer).
// 's' for short components (unsigned 16-bit integer).
// 'f' for float components (signed 32-bit float).
class tColour3b;	// AKA tPixel3b
class tColour4b;	// AKA tPixel4b
class tColour3s;	// AKA tPixel3s
class tColour4s;	// AKA tPixel4s
class tColour3f;	// AKA tPixel3f
class tColour4f;	// AKA tPixel4f


// Generally floating point colour representations are considered to be in linear colour-space. Linear is where you
// want to do all the work. sRGB (gamma corrected) is probably the space of most textures, especially diffuse textures
// as they were authored on monitors that had a non-linear (gamma) response. Use this enum to indicate
// the colour-space of pixel data you have... if you know it. Unfortunatly you can't in general determine the space
// from, say, the pixel format -- a non sRGB format may contain sRGB data (but an sRGB format should be assumed to
// actually contain sRGB data). In the below enumerants:
// l = linear
// g = gamma
// q = square
// s = standard
// See tColourProfile for an enum that describes which spaces are used for which channels, and whether they are LDR or
// HDR, in image files.
enum class tColourSpace
{
	Unspecified,

	// Linear.
	// Colours (and alpha) represented in this space can be added and multiplied with each other. This is your basic
	// RGB cube. Alphas are also considered linear.
	lRGB, 		lRGBA = lRGB,

	// Gamma.
	// Colours can be multiplied with each other, but not added. This is a common approximation of sRGB-space in
	// which a simple pow function is used with a nominal gamma value of 2.2. If the data contains alpha, the alpha
	// is considered to be linear.
	gRGB, 		gRGBlA = gRGB,

	// Square.
	// This is a lame approximation of gamma-space in which gamma is taken to be 2.0. This allows fast conversion
	// between linear and gamma-square because a square and square-root function are all that's needed.
	qRGB,		qRGBlA = qRGB,

	// Standard.
	// Standard RGB. This is the real-deal and uses the full sRGB spec (https://en.wikipedia.org/wiki/SRGB)
	// Neither mult or add. Most common space of source art.
	sRGB,		sRGBlA = sRGB,

	// I don't think we're going to need it, but we could add sRGBA if needed, where A is also in standard space.
	
	// Hue, Saturation, and Value.
	HSV,

	NumSpaces,

	// Synonyms.
	Invalid		= Unspecified,
	Linear		= lRGB,
	Gamma		= gRGB,
	Square		= qRGB,
	Standard	= sRGB,
	Auto		= NumSpaces
};


// The colour profile specifies the possible variants of how pixel data may be stored. In Tacent the colour profile is
// stored separately to the pixel-format. It specifies how the RGBA components should be intyerpreted, not how they're
// encoded. This includes both what space each component is in as well as range information: LDR (low-dynamic-range) or
// HDR (high-dynamic-range).
//
// For example, many RGBA images have the RGB components in sRGB-space, but the A component in linear. We use the term
// LDR rather than the UNORM term you see in many pixel-formats because UNORM also implies that the data is stored as
// an integer: UNORM -> unsigned normalized integer where all bits 0 represents 0.0 and all bits 1 represents 1.0.
// There's nothing stopping a pure float/half-float encoding being limited to the range 0.0 to 1.0.
// The profile enumerants use the following mnemonics:
//
// LDR		: Low dynamic range. Values restricted to [0.0, 1.0].
// HDR		: High dynamic range. Values are in [0.0, infinity].
// s		: Standard colour space. AKA sRGB.
// g		: Similar to Standard colour space but uses a simple pow function with gamma.
// l		: That's an 'L'. Linear colour space. In practice HDR only ever uses linear.
// RGBA		: These letters are used for the various compenents that may be present.
//
// For cases where a pixel format does not encode all 4 components, the first profile that fits should be used.
// For example a .rgb (Radiance) file does not store alpha, but does store RGB as linear HDR. The profile would be
// HDRlRGB_LDRlA rather than HDRlRGBA. Note also that not all tColourSpaces are represented in the profiles. This is
// because the profiles are used for what is commonly found in image pixel data, which does not include all the spaces
// mentioned in the tColourSpace enum.
enum class tColourProfile
{
	Unspecified				= -1,
	LDRsRGB_LDRlA,			// Equiv ASTC profile: ASTCENC_PRF_LDR_SRGB.      The LDR sRGB colour profile.
	LDRgRGB_LDRlA,			// No ASTC profile equivalent.				      The LDR gRGB colour profile.
	LDRlRGBA,				// Equiv ASTC profile: ASTCENC_PRF_LDR.           The LDR linear colour profile.
	HDRlRGB_LDRlA,			// Equiv ASTC profile: ASTCENC_PRF_HDR_RGB_LDR_A. The HDR RGB with LDR alpha colour profile.
	HDRlRGBA,				// Equiv ASTC profile: ASTCENC_PRF_HDR.           The HDR RGBA colour profile.
	Auto,
	NumProfiles,			// Includes Auto but not Unspecified.

	// Shorter synonyms.
	Invalid					= Unspecified,
	None					= Unspecified,
	sRGB					= LDRsRGB_LDRlA,
	gRGB					= LDRgRGB_LDRlA,			// Currently not found in files, but may be converted to.
	lRGB					= LDRlRGBA,
	HDRa					= HDRlRGB_LDRlA,
	HDRA					= HDRlRGBA,
};
extern const char* tColourProfileNames[int(tColourProfile::NumProfiles)];
extern const char* tColourProfileShortNames[int(tColourProfile::NumProfiles)];
const char* tGetColourProfileName(tColourProfile);
const char* tGetColourProfileShortName(tColourProfile);


// It's the same sort of deal with premultiplied alpha. The data encoded/represented using any particular pixel format
// could be anything. If you have 32-bit RGBA for example, you don't know if the alpha was premultiplied or not. Some
// file formats will tell you, and some legacy DXT formats will tell you (dxt2 dxt4 simply mean you know the alpha was
// premultiplied). In tacent (and much like BC3 doesn't distinguish between dxt4 and dxt5) this sort of satellite info
// is stored outside of the pixel format -- with the data itself.
enum class tAlphaMode
{
	Unspecified				= -1,
	Normal,					// Not premultiplied. Independent alpha channel.
	Premultiplied,
	NumModes,

	// Shorter synonyms.
	Invalid					= Unspecified,
	None					= Unspecified,
	Norm					= Normal,
	Mult					= Premultiplied
};
extern const char* tAlphaModeNames[int(tAlphaMode::NumModes)];
extern const char* tAlphaModeShortNames[int(tAlphaMode::NumModes)];
const char* tGetAlphaModeName(tAlphaMode);
const char* tGetAlphaModeShortName(tAlphaMode);


// ChannelType is additional satellite information that is not entirely specified by the pixel format so it belongs as
// satellite information here. In particular the part that isn't specified is whether the component data of each colour
// channel should be normalized or not afterwards. It gets a little tricky here because Vulkan, OpenGL, and DirectX
// have all decided on variant pixel-format names with channel-type information like UNORM, SNORM, UINT, SINT, and
// FLOAT. This naming _includes_ both information about how the data is encoded (integer or float, signed or unsigned)
// as well as whether to normalize after decoding or not. We have a choice here, either ONLY make this satellite info
// contain whether to normalize of not afterwards, or have a litte redundant information in order to keep the naming
// as close as possible to UNORM, UINT, etc. I have decided on the latter.
//
// The reason it is not part of the pixel format is it is quite common for the data to be encoded as, say, an unsigned
// integer, but 'converted' to a float when it is passed off the video memory by the graphics API so it is available as
// a float in the fragment/pixel shader. In short the ChannelType indicates intent for what should happen to the
// value AFTER decoding. For example, UNORM means the data is stored (or decoded for compressed formats) as an unsigned
// integer (which is already known by looking at the pixel-format) -- it is then converted to a normalized value in
// [0.0, 1.0]. SNORM means it's stored as a signed integer and then normalized to the [0.0, 1.0] range. The actual
// number of bits used is NOT specified here -- that is also specified by the pixel-format itself (either explicitly or
// implicitly by inspecting the compression method used). I bring this up because, for example, the PVR3 filetype
// 'channel type' field does contain size information, but it doesn't need to (and probably shouldn't).
//
// Example 1. PixelFormat: G3B5R5G3  ChanelType: UNORM
// We know the R and B are stored as 5-bit unsigned ints and the G with six bits. We know this from the PixelFormat
// alone because it does not contain a 's', 'f', or 'uf'. We further know the intent is to 'normalize' it after
// decoding. R would be in [0, 31] and converted to [0.0, 1.0]. The 'U' part of 'UNORM' is redundant because the
// pixel-format already told us it was an unsigned integer.
//
// Example 2. PixelFormat: R11G11B10uf  ChanelType: UFLOAT
// RG stored as 11-bit unsigned floats (5 exponent, 6 mantissa, no sign bit). B stored as a 10-bit (5,5) float. In this
// case the ChannelType is completely redundant because we already know we're using unsigned floats from the 'uf'.
//
// Example 3. PixelFormat: R8G8  ChanelType: UINT
// RG stored as 8-bit unsigned ints (from pixel-format). In this case the ChannelType indicates not to normalize so
// each component should be read as an unsigned integer in [0, 255].
enum class tChannelType
{
	Unspecified				= -1,
	UnsignedIntNormalized,
	SignedIntNormalized,
	UnsignedInt,
	SignedInt,
	UnsignedFloat,
	SignedFloat,
	NumTypes,

	// Shorter synonyms.
	Invalid					= Unspecified,
	NONE					= Unspecified,
	UNORM					= UnsignedIntNormalized,
	SNORM					= SignedIntNormalized,
	UINT					= UnsignedInt,
	SINT					= SignedInt,
	UFLOAT					= UnsignedFloat,
	SFLOAT					= SignedFloat
};
extern const char* tChannelTypeNames[int(tChannelType::NumTypes)];
extern const char* tChannelTypeShortNames[int(tChannelType::NumTypes)];
const char* tGetChannelTypeName(tChannelType);
const char* tGetChannelTypeShortName(tChannelType);
tChannelType tGetChannelType(const char* nameOrShortName);


namespace tMath
{
	bool tIsProfileLinearInRGB(tColourProfile);		// If RGB not linear it's either sRGB or gRGB.
	bool tIsProfileHDRInRGB(tColourProfile);		// If RGB not HDR it's LDR.

	// Colour space conversions. The integer versions accept angle modes of Degrees and Norm256 only. The angle mode
	// determines the range of the hue. Degrees means angles are in [0, 360). Norm256 means angles are in [0, 256).
	// Saturation and value are both in [0, 256) for the integer conversion functions.
	void tRGBToHSV(int& h, int& s, int& v, int r, int g, int b, tAngleMode = tMath::tAngleMode::Degrees);
	void tHSVToRGB(int& r, int& g, int& b, int h, int s, int v, tMath::tAngleMode = tMath::tAngleMode::Degrees);

	// The floating point colour space conversion functions accept any angle mode. Radians mean angles are in [0.0, 2Pi).
	// Degrees means angles are in [0.0, 360.0). Norm256 means angles are in [0.0, 256.0). NormOne means angles are
	// in [0.0, 1.0].
	void tRGBToHSV(float& h, float& s, float& v, float r, float g, float b, tMath::tAngleMode = tMath::tAngleMode::Radians);
	void tHSVToRGB(float& r, float& g, float& b, float h, float s, float v, tMath::tAngleMode = tMath::tAngleMode::Radians);

	// Convert a standard web colour name (as may be found in rgb.txt for example) into a 32bit RGBA tColour4b.
	tColour4b tGetColour(const char* colourName);

	// Alpha is ignored for these colour difference functions.
	float tColourDiffEuclideanSq(const tColour3b& a, const tColour3b& b);		// Returns value E [0.0, 195075.0]
	float tColourDiffEuclideanSq(const tColour4b& a, const tColour4b& b);		// Returns value E [0.0, 195075.0]
	float tColourDiffEuclidean  (const tColour3b& a, const tColour3b& b);		// Returns value E [0.0, 441.672956]
	float tColourDiffEuclidean  (const tColour4b& a, const tColour4b& b);		// Returns value E [0.0, 441.672956]
	float tColourDiffRedmean    (const tColour3b& a, const tColour3b& b);		// Returns value E [0.0, 764.8340]
	float tColourDiffRedmean    (const tColour4b& a, const tColour4b& b);		// Returns value E [0.0, 764.8340]

	// Some colour-space component conversion functions. Gamma-space is probably more ubiquitous than the more accurate
	// sRGB space. Unless speed is an issue, probably best to stay away from the Square functions (gamma = 2.0).
	//
	// Colours in textures in files may be in 'Gamma-space' and ought to be converted to linear space before lighting
	// calculations are made. They should then be converted back to Gamma space before being displayed. SquareToLinear
	// and LinearToSquare are identical to GammaToLinear and LinearToGamme with a gamma value of 2.0. They're a bit
	// faster because they don't use the tPow function, only square and square-root.
	//
	// SquareToLinear will darken  the image. Gamma = 2.0 (decoding). Gamma expansion.
	// LinearToSquare Will lighten the image. Gamma = 0.5 (encoding). Gamma compression.
	float tSquareToLinear (float squareComponent);
	float tLinearToSquare (float linearComponent);

	// These two are more general versions of the above two functions and use the power function instead of squaring or
	// square-rooting. They support an arbitrary gamma value (default to 2.2). For LinearToGamma you are actually
	// supplying the inverse of the gamma when you supply the ~2.2 gamma. It takes the invGamma and inverts it to get
	// the actual gamma to use.
	//
	// GammaToLinear will darken  the image. Gamma = 2.2   (default/decoding). Gamma expansion.
	// LinearToGamma Will lighten the image. Gamma = 1/2,2 (default/encoding). Gamma compression.
	float tGammaToLinear  (float gammaComponent, float gamma = DefaultGamma);
	float tLinearToGamma  (float linearComponent, float gamma = DefaultGamma);

	// The slowest conversion but for high fidelity, the sRGB space is likely what the image was authored in. sRGB
	// conversions do not use the pow function for the whole domain, and the amplitude is not quite 1, but generally
	// speaking they us a gamma of 2.4 and 1/2.4 respectively.
	//
	// SRGBToLinear will darken  the image. Gamma = ~2.4   (decoding). Gamma expansion.
	// LinearToSRGB Will lighten the image. Gamma = ~1/2.4 (encoding). Gamma compression.
	float tSRGBToLinear   (float srgbComponent);
	float tLinearToSRGB   (float linearComponent);

	// See https://learnopengl.com/Advanced-Lighting/HDR for this simple exposure map.
	float tTonemapExposure(float linearComponent, float exposure = 1.0f);

	// Reinhard tone mapping. Evenly distributes brightness.
	float tTonemapReinhard(float linearComponent);
}


#pragma pack(push, 1)
// The tColour3b class represents a colour in 24 bits and is made of 3 unsigned byte-size integers in the order RGB.
class tColour3b
{
public:
	tColour3b()						/* Does NOT set the colour values. */												{ }
	tColour3b(const tColour3b& c)																						{ Set(c); }
	tColour3b(int r, int g, int b)																						{ Set(r, g, b); }
	tColour3b(uint8 r, uint8 g, uint8 b, uint8 a = 0xFF)																{ Set(r, g, b); }

	void Set(const tColour3b& c)																						{ R = c.R; G = c.G; B = c.B; }
	void Set(int r, int g, int b)																						{ R = tMath::tClamp(r, 0, 0xFF); G = tMath::tClamp(g, 0, 0xFF); B = tMath::tClamp(b, 0, 0xFF); }
	void Set(uint8 r, uint8 g, uint8 b)																					{ R = r; G = g; B = b; }

	// These floating point get methods use a range of [0.0, 255.0] for each component. Denorm means not normalized.
	float GetDenormR() const																							{ return float(R); }
	float GetDenormG() const																							{ return float(G); }
	float GetDenormB() const																							{ return float(B); }
	void GetDenorm(float* dest) const																					{ dest[0] = GetDenormR(); dest[1] = GetDenormG(); dest[2] = GetDenormB(); }
	void GetDenorm(tMath::tVector3& dest) const																			{ dest.x = GetDenormR(); dest.y = GetDenormG(); dest.z = GetDenormB(); }
	void GetDenorm(float& r, float&g, float& b) const																	{ r = GetDenormR(); g = GetDenormG(); b = GetDenormB(); }
	int Intensity() const			/* Returns intensity (average of RGB) in range [0, 255]. */							{ return (int(R)+int(G)+int(B))/3; }

	// These allow tColour3b to be keys in a tMap.
	explicit operator uint32()																							{ return (uint32(R)<<16) | (uint32(G)<<8) | uint32(B); }
	explicit operator uint32() const																					{ return (uint32(R)<<16) | (uint32(G)<<8) | uint32(B); }
	bool operator==(const tColour3b& c) const																			{ return (c.R == R) && (c.G == G) && (c.B == B); }

	union
	{
		struct { uint8 R, G, B; };
		struct { uint8 H, S, V; };
		uint8 E[3];					// Individual elements. Makes it easy to submit colours to OpenGL using glColor3ubv.
	};
};
#pragma pack(pop)
tStaticAssert(sizeof(tColour3b) == 3);
typedef tColour3b tPixel3b;


// The tColour4b class represents a colour in 32 bits and is made of 4 unsigned byte-size integers in the order RGBA.
class tColour4b
{
public:
	tColour4b()												/* Does NOT set the colour values. */						{ }
	tColour4b(const tColour4b& c)																						{ Set(c); }
	tColour4b(const tColour3b& c, int a = 0xFF)																			{ Set(c, a); }
	tColour4b(int r, int g, int b, int a = 0xFF)																		{ Set(r, g, b, a); }
	tColour4b(uint8 r, uint8 g, uint8 b, uint8 a = 0xFF)																{ Set(r, g, b, a); }
	tColour4b(uint32 bits)																								{ Set(bits); }
	tColour4b(const tColour4f& c)																						{ Set(c); }
	tColour4b(const tColour3f& c, uint8 a)																				{ Set(c, a); }
	tColour4b(const tColour3f& c, float a)																				{ Set(c, a); }
	tColour4b(float r, float g, float b, float a = 1.0f)																{ Set(r, g, b, a); }
	tColour4b(const float* src)																							{ Set(src); }

	void Set(const tColour4b& c)																						{ BP = c.BP; }
	void Set(const tColour3b& c, int a = 0xFF);
	void Set(int r, int g, int b, int a = 255)																			{ R = tMath::tClamp(r, 0, 0xFF); G = tMath::tClamp(g, 0, 0xFF); B = tMath::tClamp(b, 0, 0xFF); A = tMath::tClamp(a, 0, 0xFF); }
	void Set(uint8 r, uint8 g, uint8 b, uint8 a = 255)																	{ R = r; G = g; B = b; A = a; }
	void Set(uint32 bits)																								{ BP = bits; }
	void Set(const tColour4f& c);
	void Set(const tColour3f& c, uint8 a);
	void Set(const tColour3f& c, float a);
	void Set(const float* src)																							{ SetR(src[0]); SetG(src[1]); SetB(src[2]); SetA(src[3]); }

	// These leave alpha at whatever value it was at before.
	void SetRGB(int r, int g, int b)																					{ R = tMath::tClamp(r, 0, 0xFF); G = tMath::tClamp(g, 0, 0xFF); B = tMath::tClamp(b, 0, 0xFF); }
	void SetRGB(uint8 r, uint8 g, uint8 b)																				{ R = r; G = g; B = b; }

	// The floating point set methods use a range of [0.0, 1.0] for each component.
	void Set(float r, float g, float b, float a = 1.0f)																	{ SetR(r); SetG(g); SetB(b); SetA(a); }
	void SetR(float r)																									{ R = tMath::tClamp( int(r*256.0f), 0, 0xFF ); }
	void SetG(float g)																									{ G = tMath::tClamp( int(g*256.0f), 0, 0xFF ); }
	void SetB(float b)																									{ B = tMath::tClamp( int(b*256.0f), 0, 0xFF ); }
	void SetA(float a)																									{ A = tMath::tClamp( int(a*256.0f), 0, 0xFF ); }

	// The floating point get methods use a range of [0.0, 1.0] for each component.
	float GetR() const																									{ return float(R) / 255.0f; }
	float GetG() const																									{ return float(G) / 255.0f; }
	float GetB() const																									{ return float(B) / 255.0f; }
	float GetA() const																									{ return float(A) / 255.0f; }
	void Get(float* dest) const																							{ dest[0] = GetR(); dest[1] = GetG(); dest[2] = GetB(); dest[3] = GetA(); }
	void Get(tMath::tVector3& dest) const																				{ dest.x = GetR(); dest.y = GetG(); dest.z = GetB(); }
	void Get(tMath::tVector4& dest) const																				{ dest.x = GetR(); dest.y = GetG(); dest.z = GetB(); dest.w = GetA(); }
	void Get(float& r, float&g, float& b, float& a) const																{ r = GetR(); g = GetG(); b = GetB(); a = GetA(); }

	// These floating point get methods use a range of [0.0, 255.0] for each component.
	float GetDenormR() const																							{ return float(R); }
	float GetDenormG() const																							{ return float(G); }
	float GetDenormB() const																							{ return float(B); }
	float GetDenormA() const																							{ return float(A); }
	void GetDenorm(float* dest) const																					{ dest[0] = GetDenormR(); dest[1] = GetDenormG(); dest[2] = GetDenormB(); dest[3] = GetDenormA(); }
	void GetDenorm(tMath::tVector3& dest) const																			{ dest.x = GetDenormR(); dest.y = GetDenormG(); dest.z = GetDenormB(); }
	void GetDenorm(tMath::tVector4& dest) const																			{ dest.x = GetDenormR(); dest.y = GetDenormG(); dest.z = GetDenormB(); dest.w = GetDenormA(); }
	void GetDenorm(float& r, float&g, float& b, float& a) const															{ r = GetDenormR(); g = GetDenormG(); b = GetDenormB(); a = GetDenormA(); }

	void Get(tColour4b& c) const																						{ c.BP = BP; }

	// Returns intensity (average of chosen components) in range [0, 255].
	int Intensity(comp_t comps = tCompBit_RGB) const;

	void MakeZero()																										{ R = 0x00; G = 0x00; B = 0x00; A = 0x00; }
	void MakeBlack()																									{ R = 0x00; G = 0x00; B = 0x00; A = 0xFF; }
	void MakeWhite()																									{ R = 0xFF; G = 0xFF; B = 0xFF; A = 0xFF; }
	void MakePink()																										{ R = 0xFF; G = 0x80; B = 0x80; A = 0xFF; }

	void MakeRed()																										{ R = 0xFF; G = 0x00; B = 0x00; A = 0xFF; }
	void MakeGreen()																									{ R = 0x00; G = 0xFF; B = 0x00; A = 0xFF; }
	void MakeBlue()																										{ R = 0x00; G = 0x00; B = 0xFF; A = 0xFF; }

	void MakeGrey()																										{ R = 0x80; G = 0x80; B = 0x80; A = 0xFF; }
	void MakeLightGrey()																								{ R = 0xC0; G = 0xC0; B = 0xC0; A = 0xFF; }
	void MakeDarkGrey()																									{ R = 0x40; G = 0x40; B = 0x40; A = 0xFF;}

	void MakeCyan()																										{ R = 0x00; G = 0xFF; B = 0xFF; A = 0xFF; }
	void MakeMagenta()																									{ R = 0xFF; G = 0x00; B = 0xFF; A = 0xFF; }
	void MakeYellow()																									{ R = 0xFF; G = 0xFF; B = 0x00; A = 0xFF; }

	// These querying calls ignore alpha.
	bool IsBlack() const																								{ return ((R == 0x00) && (G == 0x00) && (B == 0x00)) ? true : false; }
	bool IsWhite() const																								{ return ((R == 0xFF) && (G == 0xFF) && (B == 0xFF)) ? true : false; }
	bool IsRed() const																									{ return ((R == 0xFF) && (G == 0x00) && (B == 0x00)) ? true : false; }
	bool IsGreen() const																								{ return ((R == 0x00) && (G == 0xFF) && (B == 0x00)) ? true : false; }
	bool IsBlue() const																									{ return ((R == 0x00) && (G == 0x00) && (B == 0xFF)) ? true : false; }

	// When using the HSV representation of a tColour4b, the hue is in normalized angle units. See tAngleMode::Norm256.
	// Since only one byte is used, we divide the circle into 256 equal parts. All 4 values will be E [0, 255].
	// Consider using a tColoutf object when working in HSV space. It can more accurately represent the hue value
	// without as much loss in precision. See the tRGBToHSV function for retrieval of hue in different angle units.
	// Both of the functions below leave the alpha unchanged.
	void RGBToHSV();										// Assumes current values are RGB.
	void HSVToRGB();										// Assumes current values are HSV.

	bool Equal(const tColour4b&, comp_t channels = tCompBit_All) const;
	bool operator==(const tColour4b& c) const																			{ return (BP == c.BP); }
	bool operator!=(const tColour4b& c) const 																			{ return (BP != c.BP); }
	tColour4b& operator=(const tColour4b& c)																			{ BP = c.BP; return *this; }

	tColour4b& operator*=(float f)																						{ R = uint8(float(R)*f); G = uint8(float(G)*f); B = uint8(float(B)*f); A = uint8(float(A)*f); return *this; }
	const tColour4b operator*(float f) const																			{ tColour4b res(*this); res *= f; return res; }
	tColour4b& operator+=(const tColour4b& c)																			{ R += c.R; G += c.G; B += c.B; A += c.A; return *this; }
	const tColour4b operator+(const tColour4b& c) const																	{ tColour4b res(*this); res += c; return res; }

	// These allow tColour4b to be keys in a tMap.
	explicit operator uint32()																							{ return BP; }
	explicit operator uint32() const																					{ return BP; }

	// Predefined colours. Initialized using the C++11 aggregate initializer syntax. These may be used before main()
	// in normally (non-aggregate syntax) constructed objects.
	const static tColour4b black;
	const static tColour4b white;
	const static tColour4b pink;

	const static tColour4b red;
	const static tColour4b green;
	const static tColour4b blue;

	const static tColour4b grey;
	const static tColour4b lightgrey;
	const static tColour4b darkgrey;

	const static tColour4b cyan;
	const static tColour4b magenta;
	const static tColour4b yellow;

	const static tColour4b transparent;

	union
	{
		struct { uint8 R, G, B, A; };
		struct { uint8 H, S, V, O; };		// O for opacity. Some compilers don't like a repeated A.

		// Bit Pattern member.
		// Accessing the colour as a 32 bit value using the BP member means you must take the machine's endianness into
		// account. This explains why the member isn't named something like RGBA. For example, in memory it's always in the
		// RGBA no matter what endianness, but on a little endian machine you'd access the blue component with something
		// like (colour.BP >> 16) % 0xFF
		uint32 BP;

		// Individual elements. Makes it easy to submit colours to OpenGL using glColor3ubv.
		uint8 E[4];
	};
};
tStaticAssert(sizeof(tColour4b) == 4);
typedef tColour4b tPixel4b;


#pragma pack(push, 2)
// The tColour3s class represents a colour in 48 bits and is made of 3 unsigned short (16-bit) integers in the order RGB.
class tColour3s
{
public:
	tColour3s()						/* Does NOT set the colour values. */												{ }
	tColour3s(const tColour3s& c)																						{ Set(c); }
	tColour3s(int r, int g, int b)																						{ Set(r, g, b); }
	tColour3s(uint16 r, uint16 g, uint16 b, uint16 a = 0xFFFF)															{ Set(r, g, b); }

	void Set(const tColour3s& c)																						{ R = c.R; G = c.G; B = c.B; }
	void Set(int r, int g, int b)																						{ R = tMath::tClamp(r, 0, 0xFFFF); G = tMath::tClamp(g, 0, 0xFFFF); B = tMath::tClamp(b, 0, 0xFFFF); }
	void Set(uint16 r, uint16 g, uint16 b)																				{ R = r; G = g; B = b; }

	// These floating point get methods use a range of [0.0, 65535.0] for each component.
	float GetDenormR() const																							{ return float(R); }
	float GetDenormG() const																							{ return float(G); }
	float GetDenormB() const																							{ return float(B); }
	void GetDenorm(float* dest) const																					{ dest[0] = GetDenormR(); dest[1] = GetDenormG(); dest[2] = GetDenormB(); }
	void GetDenorm(tMath::tVector3& dest) const																			{ dest.x = GetDenormR(); dest.y = GetDenormG(); dest.z = GetDenormB(); }
	void GetDenorm(float& r, float&g, float& b) const																	{ r = GetDenormR(); g = GetDenormG(); b = GetDenormB(); }
	int Intensity() const			/* Returns intensity (average of RGB) in range [0, 65535]. */						{ return (int(R)+int(G)+int(B))/3; }

	// These allow tColour3s to be keys in a tMap. Doesn't need to be unique as the tMap deals with hash collisions.
	// This 'hash' function simply places the green 16 bits in the middle overlapping (xor) the red and blue.
	explicit operator uint32()																							{ return (uint32(R)<<16) | (uint32(G)<<8) | uint32(B); }
	explicit operator uint32() const																					{ return (uint32(R)<<16) | (uint32(G)<<8) | uint32(B); }
	bool operator==(const tColour3s& c) const																			{ return (c.R == R) && (c.G == G) && (c.B == B); }

	union
	{
		struct { uint16 R, G, B; };
		struct { uint16 H, S, V; };
		uint16 E[3];					// Individual elements. Makes it easy to submit colours to OpenGL.
	};
};
#pragma pack(pop)
tStaticAssert(sizeof(tColour3s) == 6);
typedef tColour3s tPixel3s;


// The tColour4s class represents a colour in 64 bits and is made of 4 unsigned short (16-bit) integers in the order RGBA.
class tColour4s
{
public:
	tColour4s()												/* Does NOT set the colour values. */						{ }
	tColour4s(const tColour4s& c)																						{ Set(c); }
	tColour4s(const tColour3s& c, int a = 0xFFFF)																		{ Set(c, a); }
	tColour4s(int r, int g, int b, int a = 0xFFFF)																		{ Set(r, g, b, a); }
	tColour4s(uint16 r, uint16 g, uint16 b, uint16 a = 0xFFFF)															{ Set(r, g, b, a); }
	tColour4s(uint64 bits)																								{ Set(bits); }
	tColour4s(const tColour4f& c)																						{ Set(c); }
	tColour4s(const tColour3f& c, uint16 a)																				{ Set(c, a); }
	tColour4s(const tColour3f& c, float a)																				{ Set(c, a); }
	tColour4s(float r, float g, float b, float a = 1.0f)																{ Set(r, g, b, a); }
	tColour4s(const float* src)																							{ Set(src); }

	void Set(const tColour4s& c)																						{ BP = c.BP; }
	void Set(const tColour3s& c, int a = 0xFFFF)																		{ R = c.R; G = c.G; B = c.B; A = a; }
	void Set(int r, int g, int b, int a = 65535)																		{ R = tMath::tClamp(r, 0, 0xFFFF); G = tMath::tClamp(g, 0, 0xFFFF); B = tMath::tClamp(b, 0, 0xFFFF); A = tMath::tClamp(a, 0, 0xFFFF); }
	void Set(uint16 r, uint16 g, uint16 b, uint16 a = 65535)															{ R = r; G = g; B = b; A = a; }
	void Set(uint64 bits)																								{ BP = bits; }
	void Set(const tColour4f& c);
	void Set(const tColour3f& c, uint16 a);
	void Set(const tColour3f& c, float a);
	void Set(const float* src)																							{ SetR(src[0]); SetG(src[1]); SetB(src[2]); SetA(src[3]); }

	// These leave alpha at whatever value it was at before.
	void SetRGB(int r, int g, int b)																					{ R = tMath::tClamp(r, 0, 0xFFFF); G = tMath::tClamp(g, 0, 0xFFFF); B = tMath::tClamp(b, 0, 0xFFFF); }
	void SetRGB(uint16 r, uint16 g, uint16 b)																			{ R = r; G = g; B = b; }

	// The floating point set methods use a range of [0.0, 1.0] for each component.
	void Set(float r, float g, float b, float a = 1.0f)																	{ SetR(r); SetG(g); SetB(b); SetA(a); }
	void SetR(float r)																									{ R = tMath::tClamp( int(r*65536.0f), 0, 0xFFFF ); }
	void SetG(float g)																									{ G = tMath::tClamp( int(g*65536.0f), 0, 0xFFFF ); }
	void SetB(float b)																									{ B = tMath::tClamp( int(b*65536.0f), 0, 0xFFFF ); }
	void SetA(float a)																									{ A = tMath::tClamp( int(a*65536.0f), 0, 0xFFFF ); }

	// The floating point get methods use a range of [0.0, 1.0] for each component.
	float GetR() const																									{ return float(R) / 65535.0f; }
	float GetG() const																									{ return float(G) / 65535.0f; }
	float GetB() const																									{ return float(B) / 65535.0f; }
	float GetA() const																									{ return float(A) / 65535.0f; }
	void Get(float* dest) const																							{ dest[0] = GetR(); dest[1] = GetG(); dest[2] = GetB(); dest[3] = GetA(); }
	void Get(tMath::tVector3& dest) const																				{ dest.x = GetR(); dest.y = GetG(); dest.z = GetB(); }
	void Get(tMath::tVector4& dest) const																				{ dest.x = GetR(); dest.y = GetG(); dest.z = GetB(); dest.w = GetA(); }
	void Get(float& r, float&g, float& b, float& a) const																{ r = GetR(); g = GetG(); b = GetB(); a = GetA(); }

	// These floating point get methods use a range of [0.0, 65535.0] for each component.
	float GetDenormR() const																							{ return float(R); }
	float GetDenormG() const																							{ return float(G); }
	float GetDenormB() const																							{ return float(B); }
	float GetDenormA() const																							{ return float(A); }
	void GetDenorm(float* dest) const																					{ dest[0] = GetDenormR(); dest[1] = GetDenormG(); dest[2] = GetDenormB(); dest[3] = GetDenormA(); }
	void GetDenorm(tMath::tVector3& dest) const																			{ dest.x = GetDenormR(); dest.y = GetDenormG(); dest.z = GetDenormB(); }
	void GetDenorm(tMath::tVector4& dest) const																			{ dest.x = GetDenormR(); dest.y = GetDenormG(); dest.z = GetDenormB(); dest.w = GetDenormA(); }
	void GetDenorm(float& r, float&g, float& b, float& a) const															{ r = GetDenormR(); g = GetDenormG(); b = GetDenormB(); a = GetDenormA(); }

	void Get(tColour4s& c) const																						{ c.BP = BP; }

	// Returns intensity (average of chosen components) in range [0, 65535].
	int Intensity(comp_t comps = tCompBit_RGB) const;

	void MakeZero()																										{ R = 0x0000; G = 0x0000; B = 0x0000; A = 0x0000; }
	void MakeBlack()																									{ R = 0x0000; G = 0x0000; B = 0x0000; A = 0xFFFF; }
	void MakeWhite()																									{ R = 0xFFFF; G = 0xFFFF; B = 0xFFFF; A = 0xFFFF; }
	void MakePink()																										{ R = 0xFFFF; G = 0x8000; B = 0x8000; A = 0xFFFF; }

	void MakeRed()																										{ R = 0xFFFF; G = 0x0000; B = 0x0000; A = 0xFFFF; }
	void MakeGreen()																									{ R = 0x0000; G = 0xFFFF; B = 0x0000; A = 0xFFFF; }
	void MakeBlue()																										{ R = 0x0000; G = 0x0000; B = 0xFFFF; A = 0xFFFF; }

	void MakeGrey()																										{ R = 0x8000; G = 0x8000; B = 0x8000; A = 0xFFFF; }
	void MakeLightGrey()																								{ R = 0xC000; G = 0xC000; B = 0xC000; A = 0xFFFF; }
	void MakeDarkGrey()																									{ R = 0x4000; G = 0x4000; B = 0x4000; A = 0xFFFF;}

	void MakeCyan()																										{ R = 0x0000; G = 0xFFFF; B = 0xFFFF; A = 0xFFFF; }
	void MakeMagenta()																									{ R = 0xFFFF; G = 0x0000; B = 0xFFFF; A = 0xFFFF; }
	void MakeYellow()																									{ R = 0xFFFF; G = 0xFFFF; B = 0x0000; A = 0xFFFF; }

	// These querying calls ignore alpha.
	bool IsBlack() const																								{ return ((R == 0x0000) && (G == 0x0000) && (B == 0x0000)) ? true : false; }
	bool IsWhite() const																								{ return ((R == 0xFFFF) && (G == 0xFFFF) && (B == 0xFFFF)) ? true : false; }
	bool IsRed() const																									{ return ((R == 0xFFFF) && (G == 0x0000) && (B == 0x0000)) ? true : false; }
	bool IsGreen() const																								{ return ((R == 0x0000) && (G == 0xFFFF) && (B == 0x0000)) ? true : false; }
	bool IsBlue() const																									{ return ((R == 0x0000) && (G == 0x0000) && (B == 0xFFFF)) ? true : false; }

	bool Equal(const tColour4s&, comp_t channels = tCompBit_All) const;
	bool operator==(const tColour4s& c) const																			{ return (BP == c.BP); }
	bool operator!=(const tColour4s& c) const 																			{ return (BP != c.BP); }
	tColour4s& operator=(const tColour4s& c)																			{ BP = c.BP; return *this; }

	tColour4s& operator*=(float f)																						{ R = uint16(float(R)*f); G = uint16(float(G)*f); B = uint16(float(B)*f); A = uint16(float(A)*f); return *this; }
	const tColour4s operator*(float f) const																			{ tColour4s res(*this); res *= f; return res; }
	tColour4s& operator+=(const tColour4s& c)																			{ R += c.R; G += c.G; B += c.B; A += c.A; return *this; }
	const tColour4s operator+(const tColour4s& c) const																	{ tColour4s res(*this); res += c; return res; }

	// These allow tColour4s to be keys in a tMap. Doesn't need to be unique as the tMap deals with hash collisions.
	// This 'hash' function simply places the green 16 bits in the middle left and the blue in middle-right, overlapping
	// (xor) the red (MSB) and alpha (LSB).
	explicit operator uint32()																							{ return (uint32(R)<<16) | (uint32(G)<<12) | (uint32(B)<<8) | uint32(A); }
	explicit operator uint32() const																					{ return (uint32(R)<<16) | (uint32(G)<<12) | (uint32(B)<<8) | uint32(A); }

	// Predefined colours. Initialized using the C++11 aggregate initializer syntax. These may be used before main()
	// in normally (non-aggregate syntax) constructed objects.
	const static tColour4s black;
	const static tColour4s white;
	const static tColour4s pink;

	const static tColour4s red;
	const static tColour4s green;
	const static tColour4s blue;

	const static tColour4s grey;
	const static tColour4s lightgrey;
	const static tColour4s darkgrey;

	const static tColour4s cyan;
	const static tColour4s magenta;
	const static tColour4s yellow;

	const static tColour4s transparent;

	union
	{
		struct { uint16 R, G, B, A; };
		struct { uint16 H, S, V, O; };		// O for opacity. Some compilers don't like a repeated A.

		// Bit Pattern member.
		// Accessing the colour as a 64 bit value using the BP member means you must take the machine's endianness into
		// account. This explains why the member isn't named something like RGBA. For example, in memory it's always in the
		// RGBA no matter what endianness, but on a little endian machine you'd access the blue component with something
		// like (colour.BP >> 16) % 0xFF
		uint64 BP;

		// Individual elements. Makes it easy to submit colours to OpenGL.
		uint16 E[4];
	};
};
tStaticAssert(sizeof(tColour4s) == 8);
typedef tColour4s tPixel4s;


// The tColour3f class represents a colour in 3 floats and is made of 3 floats in the order RGB.
// The values of each float component are E [0.0, 1.0].
class tColour3f
{
public:
	tColour3f()																											{ }
	tColour3f(const tColour3f& src)																						{ Set(src); }
	tColour3f(float r, float g, float b)																				{ Set(r, g, b); }
	tColour3f(const tMath::tVector3& c)																					{ Set(c); }
	tColour3f(const tMath::tVector4& c)																					{ Set(c); }
	tColour3f(const tColour4b& src)																						{ Set(src); }
	tColour3f(uint8 r, uint8 g, uint8 b)																				{ Set(r, g, b); }
	tColour3f(int r, int g, int b)																						{ Set(r, g, b); }

	void Unset()																										{ R = -1.0f; G = -1.0f; B = -1.0f; }						// An unset colour has value (-1.0f, -1.0f, -1.0f).
	bool IsSet() const																									{ return ((R != -1.0f) || (G != -1.0f) || (B != -1.0f)); }	// Any set component means the whole colour is considered set.
	void Set(const tColour3f& c)																						{ R = c.R; G = c.G; B = c.B; }
	void Set(float r, float g, float b)																					{ R = r; G = g; B = b; }
	void Set(const float* src)																							{ R = src[0]; G = src[1]; B = src[2]; }
	void Set(const tMath::tVector3& c)																					{ R = c.x; G = c.y; B = c.z; }
	void Set(const tMath::tVector4& c)																					{ R = c.x; G = c.y; B = c.z; }
	void Set(const tColour4b& c)																						{ Set(c.R, c.G, c.B); }

	// The integer get and set methods use a range of [0, 255] for each component.
	void Set(int r, int g, int b)																						{ SetR(r); SetG(g); SetB(b); }
	void SetR(int r)																									{ R = float(r)/255.0f; }
	void SetG(int g)																									{ G = float(g)/255.0f; }
	void SetB(int b)																									{ B = float(b)/255.0f; }
	int GetR() const																									{ return tMath::tClamp(int(R*256.0f), 0, 0xFF); }
	int GetG() const																									{ return tMath::tClamp(int(G*256.0f), 0, 0xFF); }
	int GetB() const																									{ return tMath::tClamp(int(B*256.0f), 0, 0xFF); }
	void Get(int* dest) const																							{ dest[0] = GetR(); dest[1] = GetG(); dest[2] = GetB(); }

	void Get(tMath::tVector3& dest) const																				{ dest.x = R; dest.y = G; dest.z = B; }
	void Get(tMath::tVector4& dest) const																				{ dest.x = R; dest.y = G; dest.z = B; dest.w = 1.0f; }
	void Get(float& r, float&g, float& b) const																			{ r = R; g = G; b = B; }
	void Get(tColour3f& c) const																						{ c.R = R; c.G = G; c.B = B; }

	void Saturate()																										{ tMath::tiSaturate(R); tMath::tiSaturate(G); tMath::tiSaturate(B); }
	float Intensity() const			/* Returns intensity (average of RGB) in range [0.0f, 1.0f]. */						{ return (R+G+B)/3.0f; }

	void MakeBlack()																									{ R = 0.0f; G = 0.0f; B = 0.0f; }
	void MakeWhite()																									{ R = 1.0f; G = 1.0f; B = 1.0f; }
	void MakePink()																										{ R = 1.0f; G = 0.5f; B = 0.5f; }

	void MakeRed()																										{ R = 1.0f; G = 0.0f; B = 0.0f; }
	void MakeGreen()																									{ R = 0.0f; G = 1.0f; B = 0.0f; }
	void MakeBlue()																										{ R = 0.0f; G = 0.0f; B = 1.0f; }

	void MakeGrey()																										{ R = 0.5f; G = 0.5f; B = 0.5f; }
	void MakeLightGrey()																								{ R = 0.75f; G = 0.75f; B = 0.75f; }
	void MakeDarkGrey()																									{ R = 0.25f; G = 0.25f; B = 0.25f; }

	void MakeCyan()																										{ R = 0.0f; G = 1.0f; B = 1.0f; }
	void MakeMagenta()																									{ R = 1.0f; G = 0.0f; B = 1.0f; }
	void MakeYellow()																									{ R = 1.0f; G = 1.0f; B = 0.0f; }

	// These querying calls ignore alpha.
	bool IsBlack() const																								{ return ((R == 0.0f) && (G == 0.0f) && (B == 0.0f)); }
	bool IsWhite() const																								{ return ((R == 1.0f) && (G == 1.0f) && (B == 1.0f)); }
	bool IsRed() const																									{ return ((R == 1.0f) && (G == 0.0f) && (B == 0.0f)); }
	bool IsGreen() const																								{ return ((R == 0.0f) && (G == 1.0f) && (B == 0.0f)); }
	bool IsBlue() const																									{ return ((R == 0.0f) && (G == 0.0f) && (B == 1.0f)); }

	// Colours in textures in files may be in Gamma space and ought to be converted to linear space before
	// lighting calculations are made. They should then be converted back to Gamma space before being displayed.
	// Gamma-space here should really be sRGB but we're just using an approximation by squaring (gamma=2) when the
	// average sRGB gamma should be 2.2. To do the conversion properly, the gamma varies with intensity from 1 to 2.4,
	// but, again, we're only approximating here.
	void ToLinearSpaceApprox()																							{ R *= R; G *= G; B *= B; }
	void ToGammaSpaceApprox()																							{ R = tMath::tSqrt(R); G = tMath::tSqrt(G); B = tMath::tSqrt(B); }

	// When using the HSV representation of a tColour4f, the hue is in NormOne angle mode. See the tRGBToHSV and
	// tHSVToRGB functions if you wish to use different angle units. All the components (h, s, v, r, g, b, a) are in
	// [0.0, 1.0]. Both of the functions below leave the alpha unchanged.
	void RGBToHSV();										// Assumes current values are RGB.
	void HSVToRGB();										// Assumes current values are HSV.

	bool operator==(const tColour4f& c) const;
	bool operator!=(const tColour4f& c) const;
	tColour3f& operator=(const tColour3f& c)																			{ R = c.R; G = c.G; B = c.B; return *this; }

	// Predefined colours.
	const static tColour3f invalid;
	const static tColour3f black;
	const static tColour3f white;
	const static tColour3f hotpink;

	const static tColour3f red;
	const static tColour3f green;
	const static tColour3f blue;

	const static tColour3f grey;
	const static tColour3f lightgrey;
	const static tColour3f darkgrey;

	const static tColour3f cyan;
	const static tColour3f magenta;
	const static tColour3f yellow;

	union
	{
		struct { float R, G, B; };
		struct { float H, S, V; };
		float E[3];
	};
};
tStaticAssert(sizeof(tColour3f) == 12);
typedef tColour3f tPixel3f;


// The tColour4f class represents a colour in 4 floats and is made of 4 floats in the order RGBA. The values of each
// float component are E [0.0, 1.0]. tColour4f is _usually_ considered to be in linear space rather than sRGB or Gamma.
// Even in these cases, however, the alpha if often LDR (0..1) and in linear-space.
class tColour4f
{
public:
	tColour4f()																											{ }
	tColour4f(const tColour4f& src)																						{ Set(src); }
	tColour4f(const tColour3f& src, float a = 1.0f)																		{ Set(src, a); }
	tColour4f(float r, float g, float b, float a = 1.0f)																{ Set(r, g, b, a); }
	tColour4f(const tMath::tVector3& c, float a = 1.0f)																	{ Set(c, a); }
	tColour4f(const tMath::tVector4& ca)																				{ Set(ca); }
	tColour4f(const tColour4b& src)																						{ Set(src); }
	tColour4f(const tColour4s& src)																						{ Set(src); }
	tColour4f(uint8 r, uint8 g, uint8 b, uint8 a = 0xFF)																{ Set(r, g, b, a); }
	tColour4f(int r, int g, int b, int a = 255)																			{ Set(r, g, b, a); }

	void Unset()																										{ R = -1.0f; G = -1.0f; B = -1.0f; A = -1.0f; }														// An unset colour has value (-1.0f, -1.0f, -1.0f, -1.0f).
	bool IsSet() const																									{ if ((R != -1.0f) || (G != -1.0f) || (B != -1.0f) || (A != -1.0f)) return true; return false; }	// Any set component means the whole colour is considered set.
	void Set(const tColour4f& c)																						{ BP0 = c.BP0; BP1 = c.BP1; }
	void Set(const tColour3f& c, float a = 1.0f);
	void Set(float r, float g, float b, float a = 1.0f)																	{ R = r; G = g; B = b; A = a; }
	void Set(const float* src)																							{ R = src[0]; G = src[1]; B = src[2]; A = src[3]; }
	void Set(const tMath::tVector3& c, float a = 1.0f)																	{ R = c.x; G = c.y; B = c.z; A = a; }
	void Set(const tMath::tVector4& ca)																					{ R = ca.x; G = ca.y; B = ca.z; A = ca.w; }
	void Set(const tColour4b& c)																						{ Set(c.R, c.G, c.B, c.A); }
	void Set(const tColour4s& c)																						{ Set16(c.R, c.G, c.B, c.A); }

	// The integer get and set methods use a range of [0, 255] for each component.
	// @wip Can we use uint8's and get rid of the 16 suffix?
	void Set(int r, int g, int b, int a = 255)																			{ SetR(r); SetG(g); SetB(b); SetA(a); }
	void Set16(uint16 r, uint16 g, uint16 b, uint16 a = 65535)															{ SetR16(r); SetG16(g); SetB16(b); SetA16(a); }
	void SetR(int r)																									{ R = float(r)/255.0f; }
	void SetG(int g)																									{ G = float(g)/255.0f; }
	void SetB(int b)																									{ B = float(b)/255.0f; }
	void SetA(int a)																									{ A = float(a)/255.0f; }
	void SetR16(uint16 r)																								{ R = float(r)/65535.0f; }
	void SetG16(uint16 g)																								{ G = float(g)/65535.0f; }
	void SetB16(uint16 b)																								{ B = float(b)/65535.0f; }
	void SetA16(uint16 a)																								{ A = float(a)/65535.0f; }
	int GetR() const																									{ return tMath::tClamp(int(R*256.0f), 0, 0xFF); }
	int GetG() const																									{ return tMath::tClamp(int(G*256.0f), 0, 0xFF); }
	int GetB() const																									{ return tMath::tClamp(int(B*256.0f), 0, 0xFF); }
	int GetA() const																									{ return tMath::tClamp(int(A*256.0f), 0, 0xFF); }
	void Get(int* dest) const																							{ dest[0] = GetR(); dest[1] = GetG(); dest[2] = GetB(); dest[3] = GetA(); }

	void Get(tMath::tVector3& dest) const																				{ dest.x = R; dest.y = G; dest.z = B; }
	void Get(tMath::tVector4& dest) const																				{ dest.x = R; dest.y = G; dest.z = B; dest.w = A; }
	void Get(float& r, float&g, float& b, float& a) const																{ r = R; g = G; b = B; a = A; }
	void Get(tColour4f& c) const																							{ c.BP0 = BP0; c.BP1 = BP1;}

	void Saturate()																										{ tMath::tiSaturate(R); tMath::tiSaturate(G); tMath::tiSaturate(B); tMath::tiSaturate(A); }
	float Intensity() const			/* Returns intensity (average of RGB) in range [0.0f, 1.0f]. */						{ return (R+G+B)/3.0f; }

	void MakeBlack()																									{ R = 0.0f; G = 0.0f; B = 0.0f; A = 1.0f; }
	void MakeWhite()																									{ R = 1.0f; G = 1.0f; B = 1.0f; A = 1.0f; }
	void MakePink()																										{ R = 1.0f; G = 0.5f; B = 0.5f; A = 1.0f; }

	void MakeRed()																										{ R = 1.0f; G = 0.0f; B = 0.0f; A = 1.0f; }
	void MakeGreen()																									{ R = 0.0f; G = 1.0f; B = 0.0f; A = 1.0f; }
	void MakeBlue()																										{ R = 0.0f; G = 0.0f; B = 1.0f; A = 1.0f; }

	void MakeGrey()																										{ R = 0.5f; G = 0.5f; B = 0.5f; A = 1.0f; }
	void MakeLightGrey()																								{ R = 0.75f; G = 0.75f; B = 0.75f; A = 1.0f; }
	void MakeDarkGrey()																									{ R = 0.25f; G = 0.25f; B = 0.25f; A = 1.0f;}

	void MakeCyan()																										{ R = 0.0f; G = 1.0f; B = 1.0f; A = 1.0f; }
	void MakeMagenta()																									{ R = 1.0f; G = 0.0f; B = 1.0f; A = 1.0f; }
	void MakeYellow()																									{ R = 1.0f; G = 1.0f; B = 0.0f; A = 1.0f; }

	// These querying calls ignore alpha.
	bool IsBlack() const																								{ return ((R == 0.0f) && (G == 0.0f) && (B == 0.0f)) ? true : false; }
	bool IsWhite() const																								{ return ((R == 1.0f) && (G == 1.0f) && (B == 1.0f)) ? true : false; }
	bool IsRed() const																									{ return ((R == 1.0f) && (G == 0.0f) && (B == 0.0f)) ? true : false; }
	bool IsGreen() const																								{ return ((R == 0.0f) && (G == 1.0f) && (B == 0.0f)) ? true : false; }
	bool IsBlue() const																									{ return ((R == 0.0f) && (G == 0.0f) && (B == 1.0f)) ? true : false; }

	// Returns true if any component (including alpha currently) is above 1.0f.
	bool IsHDR() const																									{ return ((R > 1.0f) || (G > 1.0f) || (B > 1.0f) || (A > 1.0f)); }

	// Colours in textures in files may be in 'Gamma-space' and ought to be converted to linear space before lighting
	// calculations are made. They should then be converted back to Gamma space before being displayed. SquareToLinear
	// and LinearToSquare are identical to GammaToLinear and LinearToGamme with a gamma value of 2.0. They're a bit
	// faster because they don't use the tPow function, only square and square-root.
	//
	// SquareToLinear will darken  the image. Gamma = 2.0 (decoding). Gamma expansion.
	// LinearToSquare Will lighten the image. Gamma = 0.5 (encoding). Gamma compression.
	void SquareToLinear(comp_t = tCompBit_RGB);
	void LinearToSquare(comp_t = tCompBit_RGB);

	// These two are more general versions of the above two functions and use the power function instead of squaring or
	// square-rooting. They support an arbitrary gamma value (default to 2.2). For LinearToGamma you are actually
	// supplying the inverse of the gamma when you supply the ~2.2 gamma. It takes the invGamma and inverts it to get
	// the actual gamma to use.
	//
	// GammaToLinear will darken  the image. Gamma = 2.2   (default/decoding). Gamma expansion.
	// LinearToGamma Will lighten the image. Gamma = 1/2,2 (default/encoding). Gamma compression.
	void GammaToLinear(float gamma    = tMath::DefaultGamma, comp_t = tCompBit_RGB);
	void LinearToGamma(float invGamma = tMath::DefaultGamma, comp_t = tCompBit_RGB);

	// The slowest conversion but for high fidelity, the sRGB space is likely what the image was authored in. sRGB
	// conversions do not use the pow function for the whole domain, and the amplitude is not quite 1, but generally
	// speaking they us a gamma of 2.4 and 1/2.4 respectively.
	//
	// SRGBToLinear will darken  the image. Gamma = ~2.4   (decoding). Gamma expansion.
	// LinearToSRGB Will lighten the image. Gamma = ~1/2.4 (encoding). Gamma compression.
	void SRGBToLinear(comp_t = tCompBit_RGB);
	void LinearToSRGB(comp_t = tCompBit_RGB);

	// Simple exposure tonemapping.
	void TonemapExposure(float exposure, comp_t = tCompBit_RGB);

	// Evently distributes brightness.
	void TonemapReinhard(comp_t = tCompBit_RGB);

	// When using the HSV representation of a tColour4f, the hue is in NormOne angle mode. See the tRGBToHSV and
	// tHSVToRGB functions if you wish to use different angle units. All the components (h, s, v, r, g, b, a) are in
	// [0.0, 1.0]. Both of the functions below leave the alpha unchanged.
	void RGBToHSV();					// Assumes current values are RGB.
	void HSVToRGB();					// Assumes current values are HSV.

	bool operator==(const tColour4f& c) const																			{ return ((BP0 == c.BP0) && (BP1 == c.BP1)); }
	bool operator!=(const tColour4f& c) const 																			{ return ((BP0 != c.BP0) || (BP1 != c.BP1)); }
	tColour4f& operator=(const tColour4f& c)																			{ BP0 = c.BP0; BP1 = c.BP1; return *this; }
	tColour4f& operator*=(float f)																						{ R *= f; G *= f; B *= f; A *= f; return *this; }
	const tColour4f operator*(float f) const																			{ tColour4f res(*this); res *= f; return res; }
	tColour4f& operator+=(const tColour4f& c)																			{ R += c.R; G += c.G; B += c.B; A += c.A; return *this; }
	const tColour4f operator+(const tColour4f& c) const																	{ tColour4f res(*this); res += c; return res; }

	// Predefined colours.
	const static tColour4f invalid;
	const static tColour4f black;
	const static tColour4f white;
	const static tColour4f hotpink;

	const static tColour4f red;
	const static tColour4f green;
	const static tColour4f blue;

	const static tColour4f grey;
	const static tColour4f lightgrey;
	const static tColour4f darkgrey;

	const static tColour4f cyan;
	const static tColour4f magenta;
	const static tColour4f yellow;

	const static tColour4f transparent;

	union
	{
		struct { float R, G, B, A; };
		struct { float H, S, V, O; };
		struct { uint64 BP0, BP1; };						// Bit Pattern.
		float E[4];
	};
};
tStaticAssert(sizeof(tColour4f) == 16);
typedef tColour4f tPixel4f;


// Implementation below this line.


inline bool tMath::tIsProfileLinearInRGB(tColourProfile profile)
{
	switch (profile)
	{
		case tColourProfile::LDRlRGBA:
		case tColourProfile::HDRlRGB_LDRlA:
		case tColourProfile::HDRlRGBA:
			return true;
	}
	return false;
}


inline bool tMath::tIsProfileHDRInRGB(tColourProfile profile)
{
	switch (profile)
	{
		case tColourProfile::HDRlRGB_LDRlA:
		case tColourProfile::HDRlRGBA:
			return true;
	}
	return false;
}


inline float tMath::tSquareToLinear(float squareComponent)
{
	return squareComponent*squareComponent;
}


inline float tMath::tLinearToSquare(float linearComponent)
{
	return tMath::tSqrt(linearComponent);
}


inline float tMath::tGammaToLinear(float gammaComponent, float gamma)
{
	return tMath::tPow(gammaComponent, gamma);
}


inline float tMath::tLinearToGamma(float linearComponent, float gamma)
{
	return tMath::tPow(linearComponent, 1.0f/gamma);
}


inline float tMath::tSRGBToLinear(float srgbComponent)
{
	// See https://en.wikipedia.org/wiki/SRGB
	float linear =
		(srgbComponent <= 0.04045f)			?
		(srgbComponent / 12.92f)			:
		tMath::tPow((srgbComponent + 0.055f)/1.055f, 2.4f);

	return tMath::tSaturate(linear);
}


inline float tMath::tLinearToSRGB(float linearComponent)
{
	// See https://en.wikipedia.org/wiki/SRGB
	float srgb =
		(linearComponent <= 0.0031308f)		?
		(12.92f*linearComponent)			:
		1.055f*tMath::tPow(linearComponent, 1.0f/2.4f) - 0.055f;

	return tMath::tSaturate(srgb);
}


inline float tMath::tTonemapExposure(float linearComponent, float exposure)
{
	return 1.0f - tMath::tExp(-linearComponent * exposure);
}


inline float tMath::tTonemapReinhard(float linearComponent)
{
	return linearComponent / (linearComponent + 1.0f);
}


inline float tMath::tColourDiffEuclideanSq(const tColour3b& aa, const tColour3b& bb)
{
	tVector3 a; aa.GetDenorm(a);
	tVector3 b; bb.GetDenorm(b);
	return tDistBetweenSq(a, b);
}


inline float tMath::tColourDiffEuclideanSq(const tColour4b& aa, const tColour4b& bb)
{
	tVector3 a; aa.GetDenorm(a);
	tVector3 b; bb.GetDenorm(b);
	return tDistBetweenSq(a, b);
}


inline float tMath::tColourDiffEuclidean(const tColour3b& aa, const tColour3b& bb)
{
	tVector3 a; aa.GetDenorm(a);
	tVector3 b; bb.GetDenorm(b);
	return tDistBetween(a, b);
}


inline float tMath::tColourDiffEuclidean(const tColour4b& aa, const tColour4b& bb)
{
	tVector3 a; aa.GetDenorm(a);
	tVector3 b; bb.GetDenorm(b);
	return tDistBetween(a, b);
}


inline void tColour4b::Set(const tColour3b& c, int a)
{
	R = c.R; G = c.G; B = c.B; A = tMath::tClamp(a, 0, 0xFF);
}


inline void tColour4b::Set(const tColour4f& c)
{
	Set(c.R, c.G, c.B, c.A);
}


inline void tColour4b::Set(const tColour3f& c, uint8 a)
{
	SetR(c.R); SetG(c.G); SetB(c.B); A = a;
}


inline void tColour4b::Set(const tColour3f& c, float a)
{
	Set(c.R, c.G, c.B, a);
}


inline int tColour4b::Intensity(comp_t comps) const
{
	int sum = 0; int count = 0;
	if (comps & tCompBit_R) { sum += int(R); count++; }
	if (comps & tCompBit_G) { sum += int(G); count++; }
	if (comps & tCompBit_B) { sum += int(B); count++; }
	if (comps & tCompBit_A) { sum += int(A); count++; }
	if (!count) return -1;
	return sum/count;
}


inline void tColour4b::RGBToHSV()
{
	int r = R;
	int g = G;
	int b = B;
	int h, s, v;
	tRGBToHSV(h, s, v, r, g, b, tMath::tAngleMode::Norm256);
	H = h;
	S = s;
	V = v;
}


inline void tColour4b::HSVToRGB()
{
	int h = H;
	int s = S;
	int v = V;
	int r, g, b;
	tHSVToRGB(r, g, b, h, s, v, tMath::tAngleMode::Norm256);
	R = r;
	G = g;
	B = b;
}


inline bool tColour4b::Equal(const tColour4b& colour, comp_t channels) const
{
	if ((channels & tCompBit_R) && (R != colour.R))
		return false;

	if ((channels & tCompBit_G) && (G != colour.G))
		return false;

	if ((channels & tCompBit_B) && (B != colour.B))
		return false;

	if ((channels & tCompBit_A) && (A != colour.A))
		return false;

	return true;
}


inline void tColour4s::Set(const tColour4f& c)
{
	Set(c.R, c.G, c.B, c.A);
}


inline void tColour4s::Set(const tColour3f& c, uint16 a)
{
	SetR(c.R); SetG(c.G); SetB(c.B); A = a;
}


inline void tColour4s::Set(const tColour3f& c, float a)
{
	Set(c.R, c.G, c.B, a);
}


inline int tColour4s::Intensity(comp_t comps) const
{
	int sum = 0; int count = 0;
	if (comps & tCompBit_R) { sum += int(R); count++; }
	if (comps & tCompBit_G) { sum += int(G); count++; }
	if (comps & tCompBit_B) { sum += int(B); count++; }
	if (comps & tCompBit_A) { sum += int(A); count++; }
	if (!count) return -1;
	return sum/count;
}


inline bool tColour4s::Equal(const tColour4s& colour, comp_t channels) const
{
	if ((channels & tCompBit_R) && (R != colour.R))
		return false;

	if ((channels & tCompBit_G) && (G != colour.G))
		return false;

	if ((channels & tCompBit_B) && (B != colour.B))
		return false;

	if ((channels & tCompBit_A) && (A != colour.A))
		return false;

	return true;
}


inline bool tColour3f::operator==(const tColour4f& c) const
{
	return ((R == c.R) && (G == c.G) && (B == c.B));
}


inline bool tColour3f::operator!=(const tColour4f& c) const
{
	return ((R != c.R) || (G != c.G) || (B != c.B));
}


inline void tColour4f::Set(const tColour3f& c, float a)
{
	Set(c.R, c.G, c.B, a);
}


inline void tColour4f::SquareToLinear(comp_t chans)
{
	if (chans & tCompBit_R) R = tMath::tSquareToLinear(R);
	if (chans & tCompBit_G) G = tMath::tSquareToLinear(G);
	if (chans & tCompBit_B) B = tMath::tSquareToLinear(B);
	if (chans & tCompBit_A) A = tMath::tSquareToLinear(A);
}


inline void tColour4f::LinearToSquare(comp_t chans)
{
	if (chans & tCompBit_R) R = tMath::tLinearToSquare(R);
	if (chans & tCompBit_G) G = tMath::tLinearToSquare(G);
	if (chans & tCompBit_B) B = tMath::tLinearToSquare(B);
	if (chans & tCompBit_A) A = tMath::tLinearToSquare(A);
}


inline void tColour4f::GammaToLinear(float gamma, comp_t chans)
{
	if (chans & tCompBit_R) R = tMath::tGammaToLinear(R, gamma);
	if (chans & tCompBit_G) G = tMath::tGammaToLinear(G, gamma);
	if (chans & tCompBit_B) B = tMath::tGammaToLinear(B, gamma);
	if (chans & tCompBit_A) A = tMath::tGammaToLinear(A, gamma);
}


inline void tColour4f::LinearToGamma(float gamma, comp_t chans)
{
	if (chans & tCompBit_R) R = tMath::tLinearToGamma(R, gamma);
	if (chans & tCompBit_G) G = tMath::tLinearToGamma(G, gamma);
	if (chans & tCompBit_B) B = tMath::tLinearToGamma(B, gamma);
	if (chans & tCompBit_A) A = tMath::tLinearToGamma(A, gamma);
}


inline void tColour4f::SRGBToLinear(comp_t chans)
{
	if (chans & tCompBit_R) R = tMath::tSRGBToLinear(R);
	if (chans & tCompBit_G) G = tMath::tSRGBToLinear(G);
	if (chans & tCompBit_B) B = tMath::tSRGBToLinear(B);
	if (chans & tCompBit_A) A = tMath::tSRGBToLinear(A);
}


inline void tColour4f::LinearToSRGB(comp_t chans)
{
	if (chans & tCompBit_R) R = tMath::tLinearToSRGB(R);
	if (chans & tCompBit_G) G = tMath::tLinearToSRGB(G);
	if (chans & tCompBit_B) B = tMath::tLinearToSRGB(B);
	if (chans & tCompBit_A) A = tMath::tLinearToSRGB(A);
}


inline void tColour4f::TonemapExposure(float exposure, comp_t chans)
{
	if (chans & tCompBit_R) R = tMath::tTonemapExposure(R, exposure);
	if (chans & tCompBit_G) G = tMath::tTonemapExposure(G, exposure);
	if (chans & tCompBit_B) B = tMath::tTonemapExposure(B, exposure);
	if (chans & tCompBit_A) A = tMath::tTonemapExposure(A, exposure);
}


inline void tColour4f::TonemapReinhard(comp_t chans)
{
	if (chans & tCompBit_R) R = tMath::tTonemapReinhard(R);
	if (chans & tCompBit_G) G = tMath::tTonemapReinhard(G);
	if (chans & tCompBit_B) B = tMath::tTonemapReinhard(B);
	if (chans & tCompBit_A) A = tMath::tTonemapReinhard(A);
}


inline void tColour4f::RGBToHSV()
{
	float r = R;
	float g = G;
	float b = B;
	tRGBToHSV(H, S, V, r, g, b, tMath::tAngleMode::NormOne);
}


inline void tColour4f::HSVToRGB()
{
	float h = H;
	float s = S;
	float v = V;
	tHSVToRGB(R, G, B, h, s, v, tMath::tAngleMode::NormOne);
}
