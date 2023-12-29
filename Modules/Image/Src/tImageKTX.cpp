// tImageKTX.cpp
//
// This knows how to load/save KTX files. It knows the details of the ktx and ktx2 file format and loads the data into
// multiple tPixel arrays, one for each frame (KTKs may be animated). These arrays may be 'stolen' by tPictures.
//
// Copyright (c) 2022, 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tString.h>
#include <Foundation/tSmallFloat.h>
#include <System/tMachine.h>
#include "Image/tImageKTX.h"
#include "Image/tPixelUtil.h"
#include "Image/tPicture.h"
#include "bcdec/bcdec.h"
#include "etcdec/etcdec.h"
#include "astcenc.h"
#define KHRONOS_STATIC
#include "LibKTX/include/ktx.h"
#include "LibKTX/include/vulkan_core.h"
#include "LibKTX/include/gl_format.h"
namespace tImage
{


// Helper functions, enums, and types for parsing KTX files.
namespace tKTX
{
	// These figure out the pixel-format, colour-profile, alpha-mode, and channe-type. tPixelFormat does not specify
	// ancilllary properties of the data -- it specified the encoding of the data. The extra information, like the
	// colour-profile itcwas authored in, is stored in tColourProfile, tAlphaMode, and tChannelType. In many cases this
	// satellite information cannot be determined, in which case they get set the their 'unspecified' enumerants.
	void GetFormatInfo_FromGLFormat(tPixelFormat&, tColourProfile&, tAlphaMode&, tChannelType&, uint32 glType, uint32 glFormat, uint32 glInternalFormat);
	void GetFormatInfo_FromVKFormat(tPixelFormat&, tColourProfile&, tAlphaMode&, tChannelType&, uint32 vkFormat);
}


void tKTX::GetFormatInfo_FromGLFormat(tPixelFormat& format, tColourProfile& profile, tAlphaMode& alphaMode, tChannelType& chanType, uint32 glType, uint32 glFormat, uint32 glInternalFormat)
{
	// For colour profile (the space of the data) we try to make an educated guess. In general only the asset author
	// knows the colour space/profile. For most (non-HDR) pixel formats for colours, we assume the data is sRGB.
	// Floating-point formats are likewise assumed to be in linear-space (and are usually used for HDR images). In
	// addition when the data is probably not colour data (like ATI1/2) we assume it's in linear.
	format		= tPixelFormat::Invalid;
	profile		= tColourProfile::sRGB;
	alphaMode	= tAlphaMode::None;
	chanType	= tChannelType::NONE;

	// First deal with compressed formats. For these the internal-format must be specified and can be used to determine
	// the format of the data in the ktx file. See https://registry.khronos.org/KTX/specs/1.0/ktxspec_v1.html
	// In cases where it's not a compressed format, the internal-format should not be queried to determine the format
	// of the ktx file data because it represents the desired format the data should be converted to when binding --
	// all we care about is the format of the actual data, and glType/glFormat can be used to determine that.
	// Exception: The internal format is used to determine some non-compressed formats.
	//
	// Regardiing colour profiles. In many cases there are two similar GL pixel formats that differ only in that one of
	// them specifies using sRGB for the RGB channels. For example:
	// SRGB_ALPHA_S3TC_DXT3_EXT	: sRGB for RGB, linear for A.
	// RGBA_S3TC_DXT3_EXT		: No-sRGB variant.
	// The implication is that RGBA_S3TC_DXT3_EXT should therefore be in linear RGBA space. Unfortunately most files in
	// the wild that have these non-sRGB variants are usually still authored in sRGB-space. Tacent keeps them by default
	// in sRGB-space but you will see a commented-out lRGB tag in the switch below (indicating how it 'should' be).
	#define C(c) case GL_##c
	#define CC(c) case GL_COMPRESSED_##c
	#define F(f) format = tPixelFormat::f;
	#define P(p) profile = tColourProfile::p;
	#define M(m) alphaMode = tAlphaMode::m;
	#define T(t) chanType = tChannelType::t;
	switch (glInternalFormat)
	{
		//
		// BC formats.
		//
		CC(RGB_S3TC_DXT1_EXT):					F(BC1DXT1)			/*P(lRGB)*/	M(None)		T(NONE)		break;
		CC(SRGB_S3TC_DXT1_EXT):					F(BC1DXT1)												break;
		CC(RGBA_S3TC_DXT1_EXT):					F(BC1DXT1A)			/*P(lRGB)*/							break;
		CC(SRGB_ALPHA_S3TC_DXT1_EXT):			F(BC1DXT1A)												break;
		CC(RGBA_S3TC_DXT3_EXT):					F(BC2DXT2DXT3)		/*P(lRGB)*/							break;
		CC(SRGB_ALPHA_S3TC_DXT3_EXT):			F(BC2DXT2DXT3)											break;
		CC(RGBA_S3TC_DXT5_EXT):					F(BC3DXT4DXT5)		/*P(lRGB)*/							break;
		CC(SRGB_ALPHA_S3TC_DXT5_EXT):			F(BC3DXT4DXT5)											break;
		CC(RED_RGTC1):							F(BC4ATI1U)			P(lRGB)					T(UNORM)	break;
		CC(SIGNED_RED_RGTC1):					F(BC4ATI1S)			P(lRGB)					T(SNORM)	break;
		CC(RG_RGTC2):							F(BC5ATI2U)			P(lRGB)					T(UNORM)	break;
		CC(SIGNED_RG_RGTC2):					F(BC5ATI2S)			P(lRGB)					T(SNORM)	break;
		CC(RGB_BPTC_UNSIGNED_FLOAT):			F(BC6U)				P(HDRa)					T(UFLOAT)	break;
		CC(RGB_BPTC_SIGNED_FLOAT):				F(BC6S)				P(HDRa)					T(SFLOAT)	break;

		// BPTC AKA BC7 is designed for UNORM data.
		CC(RGBA_BPTC_UNORM):					F(BC7)				/*P(lRGB)*/				T(UNORM)	break;
		CC(SRGB_ALPHA_BPTC_UNORM):				F(BC7)										T(UNORM)	break;

		//
		// ETC and EAC formats.
		//
		C(ETC1_RGB8_OES):						F(ETC1)													break;
		CC(RGB8_ETC2):							F(ETC2RGB)			/*P(lRGB)*/							break;
		CC(SRGB8_ETC2):							F(ETC2RGB)												break;
		CC(RGBA8_ETC2_EAC):						F(ETC2RGBA)			/*P(lRGB)*/							break;
		CC(SRGB8_ALPHA8_ETC2_EAC):				F(ETC2RGBA)												break;
		CC(RGB8_PUNCHTHROUGH_ALPHA1_ETC2):		F(ETC2RGBA1)		/*P(lRGB)*/							break;
		CC(SRGB8_PUNCHTHROUGH_ALPHA1_ETC2):		F(ETC2RGBA1)											break;

		// Leaving the R and RG formats in sRGB space.
		CC(R11_EAC):							F(EACR11U)									T(UINT)		break;
		CC(SIGNED_R11_EAC):						F(EACR11S)									T(SFLOAT)	break;
		CC(RG11_EAC):							F(EACRG11U)									T(UINT)		break;
		CC(SIGNED_RG11_EAC):					F(EACRG11S)									T(SFLOAT)	break;

		//
		// For ASTC formats we assume HDR-linear space if SRGB not specified.
		//
		// We chose HDR as the default profile because it can load LDR blocks. The other way around doesn't work with
		// with the test images -- the LDR profile doesn't appear capable of loading HDR blocks (they become magenta).
		//
		CC(RGBA_ASTC_4x4_KHR):					F(ASTC4X4)			P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_4x4_KHR):			F(ASTC4X4)												break;
		CC(RGBA_ASTC_5x4_KHR):					F(ASTC5X4)			P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_5x4_KHR):			F(ASTC5X4)												break;
		CC(RGBA_ASTC_5x5_KHR):					F(ASTC5X5)			P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_5x5_KHR):			F(ASTC5X5)												break;
		CC(RGBA_ASTC_6x5_KHR):					F(ASTC6X5)			P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_6x5_KHR):			F(ASTC6X5)												break;
		CC(RGBA_ASTC_6x6_KHR):					F(ASTC6X6)			P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_6x6_KHR):			F(ASTC6X6)												break;
		CC(RGBA_ASTC_8x5_KHR):					F(ASTC8X5)			P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_8x5_KHR):			F(ASTC8X5)												break;
		CC(RGBA_ASTC_8x6_KHR):					F(ASTC8X6)			P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_8x6_KHR):			F(ASTC8X6)												break;
		CC(RGBA_ASTC_8x8_KHR):					F(ASTC8X8)			P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_8x8_KHR):			F(ASTC8X8)												break;
		CC(RGBA_ASTC_10x5_KHR):					F(ASTC10X5)			P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_10x5_KHR):			F(ASTC10X5)												break;
		CC(RGBA_ASTC_10x6_KHR):					F(ASTC10X6)			P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_10x6_KHR):			F(ASTC10X6)												break;
		CC(RGBA_ASTC_10x8_KHR):					F(ASTC10X8)			P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_10x8_KHR):			F(ASTC10X8)												break;
		CC(RGBA_ASTC_10x10_KHR):				F(ASTC10X10)		P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_10x10_KHR):		F(ASTC10X10)											break;
		CC(RGBA_ASTC_12x10_KHR):				F(ASTC12X10)		P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_12x10_KHR):		F(ASTC12X10)											break;
		CC(RGBA_ASTC_12x12_KHR):				F(ASTC12X12)		P(HDRa)								break;
		CC(SRGB8_ALPHA8_ASTC_12x12_KHR):		F(ASTC12X12)											break;

		// Decided to specify the channel-type as full floats rather than half. Ref implementations of
		// B10G11R11uf and E5B9G9R9uf all convert to float rather than half. They are both unsigned.
		C(R11F_G11F_B10F):						F(B10G11R11uf)		P(HDRa)					T(UFLOAT)	break;
		C(RGB9_E5):								F(E5B9G9R9uf)		P(HDRa)					T(UFLOAT)	break;
	}

	if (format != tPixelFormat::Invalid)
		return;

	// Not a compressed format. Look at glFormat.
	switch (glFormat)
	{
		C(LUMINANCE):
			switch (glType)
			{
				C(UNSIGNED_BYTE):				F(L8)										T(UINT)		break;
			}
			break;

		C(ALPHA):
			switch (glType)
			{
				C(UNSIGNED_BYTE):				F(A8)				P(lRGB)					T(UINT)		break;
			}
			break;

		// It's ok to also include the 'INTEGER' versions here as it just restricts what the glType
		// may be. For example R32f would still be GL_RED with type GL_FLOAT, and R8 could be either
		// GL_RED/GL_UNSIGNED_BYTE or GL_RED_INTEGER/GL_UNSIGNED_BYTE.
		C(RED):
		C(RED_INTEGER):
			switch (glType)
			{
				C(UNSIGNED_BYTE):				F(R8)										T(UINT)		break;
				C(HALF_FLOAT):					F(R16f)				P(HDRa)					T(SFLOAT)	break;
				C(FLOAT):						F(R32f)				P(HDRa)					T(SFLOAT)	break;
			}
			break;
		
		C(RG):
		C(RG_INTEGER):
			switch (glType)
			{
				C(UNSIGNED_BYTE):				F(R8G8)										T(UINT)		break;
				C(HALF_FLOAT):					F(R16G16f)			P(HDRa)					T(SFLOAT)	break;
				C(FLOAT):						F(R32G32f)			P(HDRa)					T(SFLOAT)	break;
			}
			break;

		C(RGB):
		C(RGB_INTEGER):
			switch (glType)
			{
				// They are all T(UINT8). The short refers to total pixel size but each component would still
				// be accessed as a uint8 in a shader program. GL is a bit inconsistent here as the BYTE type
				// does not refer to the total pixel size like SHORT does.
				C(UNSIGNED_BYTE):				F(R8G8B8)									T(UINT)		break;
				C(UNSIGNED_SHORT_5_6_5_REV):	F(G3B5R5G3)									T(UINT)		break;
			}
			break;

		C(RGBA):
		C(RGBA_INTEGER):
			switch (glType)
			{
				C(UNSIGNED_BYTE):				F(R8G8B8A8)									T(UINT)		break;
				C(HALF_FLOAT):					F(R16G16B16A16f)	P(HDRa)					T(SFLOAT)	break;
				C(FLOAT):						F(R32G32B32A32f)	P(HDRa)					T(SFLOAT)	break;
			}
			break;

		C(BGR):
		C(BGR_INTEGER):
			switch (glType)
			{
				// See comment above.
				C(UNSIGNED_BYTE):				F(B8G8R8)									T(UINT)		break;
				C(UNSIGNED_SHORT_5_6_5):		F(G3B5R5G3)									T(UINT)		break;
			}
			break;

		C(BGRA):
		C(BGRA_INTEGER):
			switch (glType)
			{
				// See comment above.
				C(UNSIGNED_BYTE):				F(B8G8R8A8)									T(UINT)		break;
				C(UNSIGNED_SHORT_4_4_4_4):		F(G4B4A4R4)									T(UINT)		break;
				C(UNSIGNED_SHORT_5_5_5_1):		F(G3B5A1R5G2)								T(UINT)		break;
			}
			break;
	}

	if (format == tPixelFormat::Invalid)
		P(Unspecified)

	#undef C
	#undef CC
	#undef F
	#undef P
	#undef M
	#undef T
}


void tKTX::GetFormatInfo_FromVKFormat(tPixelFormat& format, tColourProfile& profile, tAlphaMode& alphaMode, tChannelType& chanType, uint32 vkFormat)
{
	// For colour profile (the space of the data) we try to make an educated guess. In general only the asset author
	// knows the colour space/profile. For most (non-HDR) pixel formats for colours, we assume the data is sRGB.
	// Floating-point formats are likewise assumed to be in linear-space (and are usually used for HDR images). In
	// addition when the data is probably not colour data (like ATI1/2) we assume it's in linear.
	format		= tPixelFormat::Invalid;
	profile		= tColourProfile::sRGB;
	alphaMode	= tAlphaMode::None;
	chanType	= tChannelType::NONE;

	// The VK formats conflate the format with the data. The colour-space is not part of the format in tacent and is
	// returned in a separate variable. UNORM means E [0.0, 1.0].
	//
	// Regardiing colour profiles. In many cases there are two similar VK pixel formats that differ only in that one of
	// them specifies using sRGB for the RGB channels. For example:
	// VK_FORMAT_R8G8B8A8_SRGB		: sRGB for RGB, linear for A.
	// VK_FORMAT_R8G8B8A8_UNORM		: No-sRGB variant.
	// The implication is that VK_FORMAT_R8G8B8A8_UNORM should therefore be in linear RGBA space. Unfortunately most
	// files in the wild that have these non-sRGB variants are usually still authored in sRGB-space. Tacent keeps them
	// by default in sRGB-space but you will see a commented-out lRGB tag below (indicating how it 'should' be).
	#define C(c) case VK_FORMAT_##c
	#define F(f) format = tPixelFormat::f;
	#define P(p) profile = tColourProfile::p;
	#define M(m) alphaMode = tAlphaMode::m;
	#define T(t) chanType = tChannelType::t;
	switch (vkFormat)
	{
		//
		// Packed formats.
		//
		// NVTT can export ktx2 as A8. There is no A8 in VkFormat so it uses R8 instead.
		// There is no difference in storage between UNORM (unsigned normalized) and UINT. The only difference is
		// when the texture is bound, the UNORM textures get their component values converted to floats in [0.0, 1.0],
		// whereas the UINT textures would just have the int returned by a shader texture sampler. Commented-out
		// case statements are simply not implemented yet.
		//
		//C(R8_SNORM):
		//C(R8_USCALED):
		//C(R8_SSCALED):
		//C(R8_SINT):
		C(R8_UNORM):							F(R8)				/*P(lRGB)*/	M(None)		T(UNORM)	break;
		C(R8_UINT):								F(R8)				/*P(lRGB)*/				T(UINT)		break;
		C(R8_SRGB):								F(R8)													break;

		//C(R8G8_SNORM):
		//C(R8G8_USCALED):
		//C(R8G8_SSCALED):
		//C(R8G8_SINT):
		C(R8G8_UNORM):							F(R8G8)				/*P(lRGB)*/				T(UNORM)	break;
		C(R8G8_UINT):							F(R8G8)				/*P(lRGB)*/				T(UINT)		break;
		C(R8G8_SRGB):							F(R8G8)													break;

		//C(R8G8B8_SNORM):
		//C(R8G8B8_USCALED):
		//C(R8G8B8_SSCALED):
		//C(R8G8B8_SINT):
		C(R8G8B8_UNORM):						F(R8G8B8)			/*P(lRGB)*/				T(UNORM)	break;
		C(R8G8B8_UINT):							F(R8G8B8)			/*P(lRGB)*/				T(UINT)		break;
		C(R8G8B8_SRGB):							F(R8G8B8)												break;
		
		//C(R8G8B8A8_SNORM):
		//C(R8G8B8A8_USCALED):
		//C(R8G8B8A8_SSCALED):
		//C(R8G8B8A8_SINT):
		C(R8G8B8A8_UNORM):						F(R8G8B8A8)			/*P(lRGB)*/				T(UNORM)	break;
		C(R8G8B8A8_UINT):						F(R8G8B8A8)			/*P(lRGB)*/				T(UINT)		break;
		C(R8G8B8A8_SRGB):						F(R8G8B8A8)												break;

		//C(B8G8R8_SNORM):
		//C(B8G8R8_USCALED):
		//C(B8G8R8_SSCALED):
		//C(B8G8R8_SINT):
		C(B8G8R8_UNORM):						F(B8G8R8)			/*P(lRGB)*/				T(UNORM)	break;
		C(B8G8R8_UINT):							F(B8G8R8)			/*P(lRGB)*/				T(UINT)		break;
		C(B8G8R8_SRGB):							F(B8G8R8)												break;

		//C(B8G8R8A8_SNORM):
		//C(B8G8R8A8_USCALED):
		//C(B8G8R8A8_SSCALED):
		//C(B8G8R8A8_SINT):
		C(B8G8R8A8_UNORM):						F(B8G8R8A8)			/*P(lRGB)*/				T(UNORM)	break;
		C(B8G8R8A8_UINT):						F(B8G8R8A8)			/*P(lRGB)*/				T(UINT)		break;
		C(B8G8R8A8_SRGB):						F(B8G8R8A8)												break;

		C(B5G6R5_UNORM_PACK16):					F(G3B5R5G3)									T(UNORM)	break;
		C(B4G4R4A4_UNORM_PACK16):				F(G4B4A4R4)									T(UNORM)	break;
		C(B5G5R5A1_UNORM_PACK16):				F(G3B5A1R5G2)								T(UNORM)	break;

		C(R16_SFLOAT):							F(R16f)				P(HDRa)					T(SFLOAT)	break;
		C(R16G16_SFLOAT):						F(R16G16f)			P(HDRa)					T(SFLOAT)	break;
		C(R16G16B16A16_SFLOAT):					F(R16G16B16A16f)	P(HDRa)					T(SFLOAT)	break;
		C(R32_SFLOAT):							F(R32f)				P(HDRa)					T(SFLOAT)	break;
		C(R32G32_SFLOAT):						F(R32G32f)			P(HDRa)					T(SFLOAT)	break;
		C(R32G32B32A32_SFLOAT):					F(R32G32B32A32f)	P(HDRa)					T(SFLOAT)	break;
		C(B10G11R11_UFLOAT_PACK32):				F(B10G11R11uf)		P(HDRa)					T(UFLOAT)	break;		
		C(E5B9G9R9_UFLOAT_PACK32):				F(E5B9G9R9uf)		P(HDRa)					T(UFLOAT)	break;

		//
		// BC Formats.
		//
		C(BC1_RGB_UNORM_BLOCK):					F(BC1DXT1)			/*P(lRGB)*/				T(UNORM)	break;
		C(BC1_RGB_SRGB_BLOCK):					F(BC1DXT1)												break;
		C(BC1_RGBA_UNORM_BLOCK):				F(BC1DXT1A)			/*P(lRGB)*/				T(UNORM)	break;
		C(BC1_RGBA_SRGB_BLOCK):					F(BC1DXT1A)												break;
		C(BC2_UNORM_BLOCK):						F(BC2DXT2DXT3)		/*P(lRGB)*/				T(UNORM)	break;
		C(BC2_SRGB_BLOCK):						F(BC2DXT2DXT3)											break;
		C(BC3_UNORM_BLOCK):						F(BC3DXT4DXT5)		/*P(lRGB)*/				T(UNORM)	break;
		C(BC3_SRGB_BLOCK):						F(BC3DXT4DXT5)											break;

		// Signed not supported yet for BC4 and BC5.
		C(BC4_UNORM_BLOCK):						F(BC4ATI1U)			/*P(lRGB)*/				T(UNORM)	break;
		C(BC4_SNORM_BLOCK):						F(BC4ATI1S)			/*P(lRGB)*/				T(SNORM)	break;
		C(BC5_UNORM_BLOCK):						F(BC5ATI2U)			/*P(lRGB)*/				T(UNORM)	break;
		C(BC5_SNORM_BLOCK):						F(BC5ATI2S)			/*P(lRGB)*/				T(SNORM)	break;

		C(BC6H_UFLOAT_BLOCK):					F(BC6U)				P(HDRa)					T(UFLOAT)	break;
		C(BC6H_SFLOAT_BLOCK):					F(BC6S)				P(HDRa)					T(SFLOAT)	break;

		C(BC7_UNORM_BLOCK):						F(BC7)				/*P(lRGB)*/				T(UNORM)	break;
		C(BC7_SRGB_BLOCK):						F(BC7)													break;

		//
		// ETC2 and EAC.
		//
		C(ETC2_R8G8B8_UNORM_BLOCK):				F(ETC2RGB)			/*P(lRGB)*/				T(UNORM)	break;
		C(ETC2_R8G8B8_SRGB_BLOCK):				F(ETC2RGB)												break;
		C(ETC2_R8G8B8A8_UNORM_BLOCK):			F(ETC2RGBA)			/*P(lRGB)*/				T(UNORM)	break;
		C(ETC2_R8G8B8A8_SRGB_BLOCK):			F(ETC2RGBA)												break;
		C(ETC2_R8G8B8A1_UNORM_BLOCK):			F(ETC2RGBA1)		/*P(lRGB)*/				T(UNORM)	break;
		C(ETC2_R8G8B8A1_SRGB_BLOCK):			F(ETC2RGBA1)											break;

		C(EAC_R11_UNORM_BLOCK):					F(EACR11U)									T(UNORM)	break;
		C(EAC_R11_SNORM_BLOCK):					F(EACR11S)									T(SFLOAT)	break;
		C(EAC_R11G11_UNORM_BLOCK):				F(EACRG11U)									T(UNORM)	break;
		C(EAC_R11G11_SNORM_BLOCK):				F(EACRG11S)									T(SFLOAT)	break;

		//
		// ASTC
		//
		// From the Khronos description of ASTC:	
		// "Whether floats larger than 1.0 are allowed is not a per-image property; it's a per-block property. An
		// HDR-compressed ASTC image is simply one where blocks can return values larger than 1.0." and "So the format
		// does not specify if floating point values are greater than 1.0." and "There are only two properties of an
		// ASTC compressed image that are per-image (and therefore part of the format) rather than being per-block.
		// These properties are block size and sRGB colorspace conversion."
		//
		// It seems to me for an HDR ASTC KTX2 image there are two possibilities for VK_FORMAT:
		// 
		// 1) VK_FORMAT_ASTC_4x4_UNORM_BLOCK or VK_FORMAT_ASTC_4x4_SRGB_BLOCK, in which case blocks that return
		// component values > 1.0 are making a liar out of "UNORM" -- and
		// 2) VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT
		//
		// Which one is correct? AMD's compressonator, after converting an EXR to ASTC, it can't guarantee blocks won't
		// return values above 1.0 (i.e. an HDR image). It does not generate SFLOAT_BLOCK_EXT. I suspect compressionator
		// is incorrect.
		//
		// We chose HDR as the default profile because it can load LDR blocks. The other way around doesn't work with
		// with the tests images -- the LDR profile doesn't appear capable of loading HDR blocks (they become magenta).
		//
		C(ASTC_4x4_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_4x4_UNORM_BLOCK):				F(ASTC4X4)			P(HDRa)					T(UNORM)	break;
		C(ASTC_4x4_SRGB_BLOCK):					F(ASTC4X4)												break;

		C(ASTC_5x4_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_5x4_UNORM_BLOCK):				F(ASTC5X4)			P(HDRa)					T(UNORM)	break;
		C(ASTC_5x4_SRGB_BLOCK):					F(ASTC5X4)												break;

		C(ASTC_5x5_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_5x5_UNORM_BLOCK):				F(ASTC5X5)			P(HDRa)					T(UNORM)	break;
		C(ASTC_5x5_SRGB_BLOCK):					F(ASTC5X5)												break;

		C(ASTC_6x5_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_6x5_UNORM_BLOCK):				F(ASTC6X5)			P(HDRa)					T(UNORM)	break;
		C(ASTC_6x5_SRGB_BLOCK):					F(ASTC6X5)												break;

		C(ASTC_6x6_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_6x6_UNORM_BLOCK):				F(ASTC6X6)			P(HDRa)					T(UNORM)	break;
		C(ASTC_6x6_SRGB_BLOCK):					F(ASTC6X6)												break;

		C(ASTC_8x5_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_8x5_UNORM_BLOCK):				F(ASTC8X5)			P(HDRa)					T(UNORM)	break;
		C(ASTC_8x5_SRGB_BLOCK):					F(ASTC8X5)												break;

		C(ASTC_8x6_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_8x6_UNORM_BLOCK):				F(ASTC8X6)			P(HDRa)					T(UNORM)	break;
		C(ASTC_8x6_SRGB_BLOCK):					F(ASTC8X6)												break;

		C(ASTC_8x8_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_8x8_UNORM_BLOCK):				F(ASTC8X8)			P(HDRa)					T(UNORM)	break;
		C(ASTC_8x8_SRGB_BLOCK):					F(ASTC8X8)												break;

		C(ASTC_10x5_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_10x5_UNORM_BLOCK):				F(ASTC10X5)			P(HDRa)					T(UNORM)	break;
		C(ASTC_10x5_SRGB_BLOCK):				F(ASTC10X5)												break;

		C(ASTC_10x6_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_10x6_UNORM_BLOCK):				F(ASTC10X6)			P(HDRa)					T(UNORM)	break;
		C(ASTC_10x6_SRGB_BLOCK):				F(ASTC10X6)												break;

		C(ASTC_10x8_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_10x8_UNORM_BLOCK):				F(ASTC10X8)			P(HDRa)					T(UNORM)	break;
		C(ASTC_10x8_SRGB_BLOCK):				F(ASTC10X8)												break;

		C(ASTC_10x10_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_10x10_UNORM_BLOCK):				F(ASTC10X10)		P(HDRa)					T(UNORM)	break;
		C(ASTC_10x10_SRGB_BLOCK):				F(ASTC10X10)											break;

		C(ASTC_12x10_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_12x10_UNORM_BLOCK):				F(ASTC12X10)		P(HDRa)					T(UNORM)	break;
		C(ASTC_12x10_SRGB_BLOCK):				F(ASTC12X10)											break;

		C(ASTC_12x12_SFLOAT_BLOCK_EXT):			F(ASTC4X4)			P(HDRa)					T(SFLOAT)	break;
		C(ASTC_12x12_UNORM_BLOCK):				F(ASTC12X12)		P(HDRa)					T(UNORM)	break;
		C(ASTC_12x12_SRGB_BLOCK):				F(ASTC12X12)											break;
	}

	if (format == tPixelFormat::Invalid)
		P(Unspecified);

	#undef C
	#undef F
	#undef P
	#undef M
	#undef T
}


tImageKTX::tImageKTX()
{
	tStd::tMemset(Layers, 0, sizeof(Layers));
}


tImageKTX::tImageKTX(const tString& ktxFile, const LoadParams& loadParams) :
	Filename(ktxFile)
{
	tStd::tMemset(Layers, 0, sizeof(Layers));
	Load(ktxFile, loadParams);
}


tImageKTX::tImageKTX(const uint8* ktxFileInMemory, int numBytes, const LoadParams& loadParams)
{
	tStd::tMemset(Layers, 0, sizeof(Layers));
	Load(ktxFileInMemory, numBytes, loadParams);
}


void tImageKTX::Clear()
{
	for (int image = 0; image < NumImages; image++)
	{
		for (int layer = 0; layer < NumMipmapLayers; layer++)
		{
			delete Layers[layer][image];
			Layers[layer][image] = nullptr;
		}
	}

	States							= 0;	// Image will be invalid now since Valid state not set.
	PixelFormat						= tPixelFormat::Invalid;
	PixelFormatSrc					= tPixelFormat::Invalid;
	ColourProfile					= tColourProfile::Unspecified;
	ColourProfileSrc				= tColourProfile::Unspecified;
	AlphaMode						= tAlphaMode::Unspecified;
	ChannelType						= tChannelType::Unspecified;
	IsCubeMap						= false;
	RowReversalOperationPerformed	= false;
	NumImages						= 0;
	NumMipmapLayers					= 0;
}


bool tImageKTX::Set(tPixel* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Layers[0][0] = new tLayer(tPixelFormat::R8G8B8A8, width, height, (uint8*)pixels, steal);
	PixelFormat = tPixelFormat::R8G8B8A8;
	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	ColourProfile = tColourProfile::LDRsRGB_LDRlA;
	ColourProfileSrc = tColourProfile::LDRsRGB_LDRlA;
	AlphaMode = tAlphaMode::Normal;
	ChannelType = tChannelType::UNORM;
	NumImages = 1;
	NumMipmapLayers = 1;

	SetStateBit(StateBit::Valid);
	return true;
}


bool tImageKTX::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	tPixel* pixels = frame->GetPixels(steal);
	Set(pixels, frame->Width, frame->Height, steal);
	if (steal)
		delete frame;

	SetStateBit(StateBit::Valid);
	return true;
}


bool tImageKTX::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


tFrame* tImageKTX::GetFrame(bool steal)
{
	// Data must be decoded for this to work.
	if (!IsValid() || (PixelFormat != tPixelFormat::R8G8B8A8) || (Layers[0][0] == nullptr))
		return nullptr;

	tFrame* frame = new tFrame();
	frame->Width = Layers[0][0]->Width;
	frame->Height = Layers[0][0]->Height;
	frame->PixelFormatSrc = PixelFormatSrc;

	if (steal)
	{
		frame->Pixels = (tPixel*)Layers[0][0]->StealData();
		delete Layers[0][0];
		Layers[0][0] = nullptr;
	}
	else
	{
		frame->Pixels = new tPixel[frame->Width * frame->Height];
		tStd::tMemcpy(frame->Pixels, (tPixel*)Layers[0][0]->Data, frame->Width * frame->Height * sizeof(tPixel));
	}

	return frame;
}


bool tImageKTX::IsOpaque() const
{
	return tImage::tIsOpaqueFormat(PixelFormat);
}


bool tImageKTX::StealLayers(tList<tLayer>& layers)
{
	if (!IsValid() || IsCubemap() || (NumImages <= 0))
		return false;

	for (int mip = 0; mip < NumMipmapLayers; mip++)
	{
		layers.Append(Layers[mip][0]);
		Layers[mip][0] = nullptr;
	}

	Clear();
	return true;
}


bool tImageKTX::GetLayers(tList<tLayer>& layers) const
{
	if (!IsValid() || IsCubemap() || (NumImages <= 0))
		return false;

	for (int mip = 0; mip < NumMipmapLayers; mip++)
		layers.Append(Layers[mip][0]);

	return true;
}


int tImageKTX::StealCubemapLayers(tList<tLayer> layerLists[tSurfIndex_NumSurfaces], uint32 sideFlags)
{
	if (!IsValid() || !IsCubemap() || !sideFlags)
		return 0;

	int sideCount = 0;
	for (int side = 0; side < tSurfIndex_NumSurfaces; side++)
	{
		uint32 sideFlag = 1 << side;
		if (!(sideFlag & sideFlags))
			continue;

		tList<tLayer>& layers = layerLists[side];
		for (int mip = 0; mip < NumMipmapLayers; mip++)
		{
			layers.Append( Layers[mip][side] );
			Layers[mip][side] = nullptr;
		}
		sideCount++;
	}

	Clear();
	return sideCount;
}


int tImageKTX::GetCubemapLayers(tList<tLayer> layerLists[tSurfIndex_NumSurfaces], uint32 sideFlags) const
{
	if (!IsValid() || !IsCubemap() || !sideFlags)
		return 0;

	int sideCount = 0;
	for (int side = 0; side < tSurfIndex_NumSurfaces; side++)
	{
		uint32 sideFlag = 1 << side;
		if (!(sideFlag & sideFlags))
			continue;

		tList<tLayer>& layers = layerLists[side];
		for (int mip = 0; mip < NumMipmapLayers; mip++)
			layers.Append( Layers[mip][side] );

		sideCount++;
	}

	return sideCount;
}


bool tImageKTX::Load(const tString& ktxFile, const LoadParams& loadParams)
{
	Clear();
	Filename = ktxFile;
	tSystem::tFileType fileType = tSystem::tGetFileType(ktxFile);
	if ((fileType != tSystem::tFileType::KTX) && (fileType != tSystem::tFileType::KTX2))
	{
		SetStateBit(StateBit::Fatal_IncorrectFileType);
		return false;
	}

	if (!tSystem::tFileExists(ktxFile))
	{
		SetStateBit(StateBit::Fatal_FileDoesNotExist);
		return false;
	}

	int ktxSizeBytes = 0;
	uint8* ktxData = (uint8*)tSystem::tLoadFile(ktxFile, 0, &ktxSizeBytes);
	bool success = Load(ktxData, ktxSizeBytes, loadParams);
	delete[] ktxData;

	return success;
}


bool tImageKTX::Load(const uint8* ktxData, int ktxSizeBytes, const LoadParams& paramsIn)
{
	Clear();
	LoadParams params(paramsIn);

	ktx_error_code_e result = KTX_SUCCESS;
	ktxTexture* texture = nullptr;
	result = ktxTexture_CreateFromMemory(ktxData, ktxSizeBytes, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
	if (!texture || (result != KTX_SUCCESS))
	{
		SetStateBit(StateBit::Fatal_CouldNotParseFile);
		return false;
	}

	NumImages			= texture->numFaces;		// Number of faces. 1 or 6 for cubemaps.
	int numLayers		= texture->numLayers;		// Number of array layers. I believe this will be > 1 for 3D textures that are made of an array of layers.
	NumMipmapLayers		= texture->numLevels;		// Mipmap levels.
	int numDims			= texture->numDimensions;	// 1D, 2D, or 3D.
	int mainWidth		= texture->baseWidth;
	int mainHeight		= texture->baseHeight;
	bool isArray		= texture->isArray;
	bool isCompressed	= texture->isCompressed;

	if ((NumMipmapLayers <= 0) || (numDims != 2) || (mainWidth <= 0) || (mainHeight <= 0))
	{
		ktxTexture_Destroy(texture);
		SetStateBit(StateBit::Fatal_InvalidDimensions);
		return false;
	}

	if (NumMipmapLayers > MaxMipmapLayers)
	{
		ktxTexture_Destroy(texture);
		SetStateBit(StateBit::Fatal_MaxNumMipmapLevelsExceeded);
		return false;
	}

	IsCubeMap = (NumImages == 6);

	// We need to determine the pixel-format of the data. To do this we first need to know if we are dealing with
	// a ktx1 (OpenGL-style) or ktx2 (Vulkan-style) file.
	ktxTexture1* ktx1 = (texture->classId == ktxTexture1_c) ? (ktxTexture1*)texture : nullptr;
	ktxTexture2* ktx2 = (texture->classId == ktxTexture2_c) ? (ktxTexture2*)texture : nullptr;
	if (!ktx1 && !ktx2)
	{
		ktxTexture_Destroy(texture);
		SetStateBit(StateBit::Fatal_CorruptedFile);
		return false;
	}

	tSystem::tFileType fileType = tSystem::tGetFileType(Filename);
	if (ktx1)
	{
		tKTX::GetFormatInfo_FromGLFormat(PixelFormatSrc, ColourProfileSrc, AlphaMode, ChannelType, ktx1->glType, ktx1->glFormat, ktx1->glInternalformat);
		if (fileType == tSystem::tFileType::KTX2)
			SetStateBit(StateBit::Conditional_ExtVersionMismatch);
	}
	else if (ktx2)
	{
		tKTX::GetFormatInfo_FromVKFormat(PixelFormatSrc, ColourProfileSrc, AlphaMode, ChannelType, ktx2->vkFormat);
		if (fileType == tSystem::tFileType::KTX)
			SetStateBit(StateBit::Conditional_ExtVersionMismatch);
	}
	PixelFormat = PixelFormatSrc;
	ColourProfile = ColourProfileSrc;

	// From now on we should just be using the PixelFormat to decide what to do next.
	if (PixelFormat == tPixelFormat::Invalid)
	{
		ktxTexture_Destroy(texture);
		SetStateBit(StateBit::Fatal_PixelFormatNotSupported);
		return false;
	}

	if (tIsBCFormat(PixelFormat))
	{
		if ((params.Flags & LoadFlag_CondMultFourDim) && ((mainWidth%4) || (mainHeight%4)))
			SetStateBit(StateBit::Conditional_DimNotMultFourBC);
		if ((params.Flags & LoadFlag_CondPowerTwoDim) && (!tMath::tIsPower2(mainWidth) || !tMath::tIsPower2(mainHeight)))
			SetStateBit(StateBit::Conditional_DimNotMultFourBC);
	}

	bool reverseRowOrderRequested = params.Flags & LoadFlag_ReverseRowOrder;
	RowReversalOperationPerformed = false;

	// For images whose height is not a multiple of the block size it makes it tricky when deoompressing to do
	// the more efficient row reversal here, so we defer it. Packed formats have a block height of 1. Only BC
	// and astc have non-unity block dimensins.
	bool doRowReversalBeforeDecode = false;
	if (reverseRowOrderRequested)
	{
		bool canDo = true;
		for (int image = 0; image < NumImages; image++)
		{
			int height = mainHeight;
			for (int layer = 0; layer < NumMipmapLayers; layer++)
			{
				if (!CanReverseRowData(PixelFormat, height))
				{
					canDo = false;
					break;
				}
				height /= 2; if (height < 1) height = 1;
			}
			if (!canDo)
				break;
		}
		doRowReversalBeforeDecode = canDo;
	}

	for (int image = 0; image < NumImages; image++)
	{
		int width = mainWidth;
		int height = mainHeight;
		for (int layer = 0; layer < NumMipmapLayers; layer++)
		{
			size_t offset;
			result = ktxTexture_GetImageOffset(texture, layer, 0, image, &offset);
			if (result != KTX_SUCCESS)
			{
				ktxTexture_Destroy(texture);
				SetStateBit(StateBit::Fatal_InvalidDataOffset);
				return false;
			}

			uint8* currPixelData = ktxTexture_GetData(texture) + offset;
			int numBytes = 0;
			if (tImage::tIsBCFormat(PixelFormat) || tImage::tIsASTCFormat(PixelFormat) || tImage::tIsPackedFormat(PixelFormat))
			{
				// It's a block format (BC/DXTn or ASTC). Each block encodes a 4x4 up to 12x12 square of pixels. DXT2,3,4,5 and BC 6,7 use 128
				// bits per block.  DXT1 and DXT1A (BC1) use 64bits per block. ASTC always uses 128 bits per block but it's not always 4x4.
				// Packed formats are considered to have a block width and height of 1.
				int blockW = tGetBlockWidth(PixelFormat);
				int blockH = tGetBlockHeight(PixelFormat);
				int bytesPerBlock = tImage::tGetBytesPerBlock(PixelFormat);
				tAssert(bytesPerBlock > 0);
				int numBlocksW = tGetNumBlocks(blockW, width);
				int numBlocksH = tGetNumBlocks(blockH, height);
				int numBlocks = numBlocksW*numBlocksH;
				numBytes = numBlocks * bytesPerBlock;

				// Here's where we possibly modify the opaque DXT1 texture to be DXT1A if there are blocks with binary
				// transparency. We only bother checking the main layer. If it's opaque we assume all the others are too.
				if ((layer == 0) && (PixelFormat == tPixelFormat::BC1DXT1) && tImage::DoBC1BlocksHaveBinaryAlpha((tImage::BC1Block*)currPixelData, numBlocks))
					PixelFormat = PixelFormatSrc = tPixelFormat::BC1DXT1A;

				// KTX files store textures upside down. In the OpenGL RH coord system, the lower left of the texture
				// is the origin and consecutive rows go up. For this reason we need to read each row of blocks from
				// the top to the bottom row. We also need to flip the rows within the 4x4 block by flipping the lookup
				// tables. This should be fairly fast as there is no encoding or encoding going on. Width and height
				// will go down to 1x1, which will still use a 4x4 DXT pixel-block.
				if (doRowReversalBeforeDecode)
				{
					uint8* reversedPixelData = tImage::CreateReversedRowData(currPixelData, PixelFormat, numBlocksW, numBlocksH);
					tAssert(reversedPixelData);

					// We can simply get the layer to steal the memory (the last true arg).
					Layers[layer][image] = new tLayer(PixelFormat, width, height, reversedPixelData, true);
				}
				else
				{
					// Not reversing. Use the currPixelData.
					Layers[layer][image] = new tLayer(PixelFormat, width, height, (uint8*)currPixelData);
				}

				tAssert(Layers[layer][image]->GetDataSize() == numBytes);
			}
			else
			{
				// Unsupported pixel format.
				Clear();
				SetStateBit(StateBit::Fatal_PixelFormatNotSupported);
				return false;
			}

			currPixelData += numBytes;
			width  /= 2; tMath::tiClampMin(width, 1);
			height /= 2; tMath::tiClampMin(height, 1);
		}
	}

	if (doRowReversalBeforeDecode)
		RowReversalOperationPerformed = true;

	// Not asked to decode. We're basically done.
	if (!(params.Flags & LoadFlag_Decode))
	{
		if (reverseRowOrderRequested && !RowReversalOperationPerformed)
			SetStateBit(StateBit::Conditional_CouldNotFlipRows);

		SetStateBit(StateBit::Valid);
		tAssert(IsValid());
		return true;
	}

	// Decode to 32-bit RGBA.
	// Spread only applies to single-channel (R-only or L-only) formats.
	bool spread = params.Flags & LoadFlag_SpreadLuminance;

	// The gamma-compression load flags only apply when decoding. If the gamma mode is auto, we determine here
	// whether to apply sRGB compression. If the space is linear and a format that often encodes colours, we apply it.
	if (params.Flags & LoadFlag_AutoGamma)
	{
		// Clear all related flags.
		params.Flags &= ~(LoadFlag_AutoGamma | LoadFlag_SRGBCompression | LoadFlag_GammaCompression);
		if (tMath::tIsProfileLinearInRGB(ColourProfileSrc))
		{
			// Just cuz it's linear doesn't mean we want to gamma transform. Some formats should be kept linear.
			if
			(
				(PixelFormatSrc != tPixelFormat::A8)		&& (PixelFormatSrc != tPixelFormat::A8L8) &&
				(PixelFormatSrc != tPixelFormat::BC4ATI1U)	&& (PixelFormatSrc != tPixelFormat::BC4ATI1S) &&
				(PixelFormatSrc != tPixelFormat::BC5ATI2U)	&& (PixelFormatSrc != tPixelFormat::BC5ATI2S)
			)
			{
				params.Flags |= LoadFlag_SRGBCompression;
			}
		}
	}

	bool didRowReversalAfterDecode = false;
	for (int image = 0; image < NumImages; image++)
	{
		for (int layerNum = 0; layerNum < NumMipmapLayers; layerNum++)
		{
			tLayer* layer = Layers[layerNum][image];
			int w = layer->Width;
			int h = layer->Height;

			// At the end of decoding _either_ decoded4i _or_ decoded4f will be valid, not both.
			// The decoded4i format used for LDR images.
			// The decoded4f format used for HDR images.
			tColour4i* decoded4i = nullptr;
			tColour4f* decoded4f = nullptr;
			DecodeResult result = DecodePixelData
			(
				layer->PixelFormat, layer->Data, layer->GetDataSize(),
				w, h, decoded4i, decoded4f
			);

			if (result != DecodeResult::Success)
			{
				Clear();
				switch (result)
				{
					case DecodeResult::PackedDecodeError:	SetStateBit(StateBit::Fatal_PackedDecodeError);			break;
					case DecodeResult::BlockDecodeError:	SetStateBit(StateBit::Fatal_BCDecodeError);				break;
					case DecodeResult::ASTCDecodeError:		SetStateBit(StateBit::Fatal_ASTCDecodeError);			break;
					default:								SetStateBit(StateBit::Fatal_PixelFormatNotSupported);	break;
				}
				return false;
			}

			// Apply any decode flags.
			tAssert(decoded4i || decoded4f);
			bool flagTone = (params.Flags & tImageKTX::LoadFlag_ToneMapExposure) ? true : false;
			bool flagSRGB = (params.Flags & tImageKTX::LoadFlag_SRGBCompression) ? true : false;
			bool flagGama = (params.Flags & tImageKTX::LoadFlag_GammaCompression)? true : false;
			if (decoded4f && (flagTone || flagSRGB || flagGama))
			{
				for (int p = 0; p < w*h; p++)
				{
					tColour4f& colour = decoded4f[p];
					if (flagTone)
						colour.TonemapExposure(params.Exposure, tCompBit_RGB);
					if (flagSRGB)
						colour.LinearToSRGB(tCompBit_RGB);
					if (flagGama)
						colour.LinearToGamma(params.Gamma, tCompBit_RGB);
				}
			}
			if (decoded4i && (flagSRGB || flagGama))
			{
				for (int p = 0; p < w*h; p++)
				{
					tColour4f colour(decoded4i[p]);
					if (flagSRGB)
						colour.LinearToSRGB(tCompBit_RGB);
					if (flagGama)
						colour.LinearToGamma(params.Gamma, tCompBit_RGB);
					decoded4i[p].SetR(colour.R);
					decoded4i[p].SetG(colour.G);
					decoded4i[p].SetB(colour.B);
				}
			}

			// Update the layer with the 32-bit RGBA decoded data. If the data was HDR (float)
			// convert it to 32 bit. Start by getting ride of the existing layer pixel data.
			delete[] layer->Data;
			if (decoded4f)
			{
				tAssert(!decoded4i);
				decoded4i = new tColour4i[w*h];
				for (int p = 0; p < w*h; p++)
					decoded4i[p].Set(decoded4f[p]);
				delete[] decoded4f;
			}

			// Possibly spread the L/Red channel.
			if (spread && tIsLuminanceFormat(layer->PixelFormat))
			{
				for (int p = 0; p < w*h; p++)
				{
					decoded4i[p].G = decoded4i[p].R;
					decoded4i[p].B = decoded4i[p].R;
				}
			}

			layer->Data = (uint8*)decoded4i;
			layer->PixelFormat = tPixelFormat::R8G8B8A8;

			// We've got one more chance to reverse the rows here (if we still need to) because we were asked to decode.
			if (reverseRowOrderRequested && !RowReversalOperationPerformed && (layer->PixelFormat == tPixelFormat::R8G8B8A8))
			{
				// This shouldn't ever fail. Too easy to reverse RGBA 32-bit.
				uint8* reversedRowData = tImage::CreateReversedRowData(layer->Data, layer->PixelFormat, w, h);
				tAssert(reversedRowData);
				delete[] layer->Data;
				layer->Data = reversedRowData;
				didRowReversalAfterDecode = true;
			}

			if ((params.Flags & LoadFlag_SwizzleBGR2RGB) && (layer->PixelFormat == tPixelFormat::R8G8B8A8))
			{
				for (int xy = 0; xy < w*h; xy++)
				{
					tColour4i& col = ((tColour4i*)layer->Data)[xy];
					tStd::tSwap(col.R, col.B);
				}
			}
		}
	}

	if (reverseRowOrderRequested && !RowReversalOperationPerformed && didRowReversalAfterDecode)
		RowReversalOperationPerformed = true;

	if (params.Flags & LoadFlag_SRGBCompression)  ColourProfile = tColourProfile::LDRsRGB_LDRlA;
	if (params.Flags & LoadFlag_GammaCompression) ColourProfile = tColourProfile::LDRgRGB_LDRlA;

	// All images decoded. Can now set the object's pixel format. We do _not_ set the PixelFormatSrc here!
	PixelFormat = tPixelFormat::R8G8B8A8;

	if (reverseRowOrderRequested && !RowReversalOperationPerformed)
		SetStateBit(StateBit::Conditional_CouldNotFlipRows);

	ktxTexture_Destroy(texture);
	SetStateBit(StateBit::Valid);
	tAssert(IsValid());
	return true;
}


const char* tImageKTX::GetStateDesc(StateBit state)
{
	return StateDescriptions[int(state)];
}


const char* tImageKTX::StateDescriptions[] =
{
	"Valid",
	"Conditional Valid. Image rows could not be flipped.",
	"Conditional Valid. Image has dimension not multiple of four.",
	"Conditional Valid. Image has dimension not power of two.",
	"Conditional Valid. KTX extension doesn't match file version.",
	"Fatal Error. File does not exist.",
	"Fatal Error. Incorrect file type. Must be a KTX or KTX2 file.",
	"Fatal Error. LibKTX could not parse file.",
	"Fatal Error. KTX file corrupted.",
	"Fatal Error. Incorrect Dimensions.",
	"Fatal Error. KTX volume textures not supported.",
	"Fatal Error. Unsupported pixel format.",
	"Fatal Error. Invalid pixel data offset.",
	"Fatal Error. Maximum number of mipmap levels exceeded.",
	"Fatal Error. Unable to decode packed pixels.",
	"Fatal Error. Unable to decode BC pixels.",
	"Fatal Error. Unable to decode ASTC pixels."
};
tStaticAssert(tNumElements(tImageKTX::StateDescriptions) == int(tImageKTX::StateBit::NumStateBits));
tStaticAssert(int(tImageKTX::StateBit::NumStateBits) <= int(tImageKTX::StateBit::MaxStateBits));


}
