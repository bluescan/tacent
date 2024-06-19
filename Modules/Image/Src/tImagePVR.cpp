// tImagePVR.cpp
//
// This class knows how to load PowerVR (.pvr) files. It knows the details of the pvr file format and loads the data
// into tLayers, optionally decompressing them. Saving is not implemented yet. The layers may be 'stolen' from a
// tImagePVR so that excessive memcpys are avoided. After they are stolen the tImagePVR is invalid. The tImagePVR
// class supports V1, V2, and V3 pvr files.
//
// Copyright (c) 2023, 2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tString.h>
#include "Image/tImagePVR.h"
#include "Image/tPixelUtil.h"
#include "Image/tPicture.h"
namespace tImage
{


namespace tPVR
{
	// There are 3 possible headers for V1, V2, and V3 PVR files. V1 and V2 are very similar with V2 having two more
	// 4-byte fields than the V1 header.
	#pragma pack(push, 1)

	struct HeaderV1V2
	{
		uint32 HeaderSize;			// 44 for V1. 52 for V2.
		uint32 Height;
		uint32 Width;
		uint32 MipMapCount;
		uint8  PixelFormat;
		uint8  Flags1;
		uint8  Flags2;
		uint8  Flags3;
		uint32 SurfaceSize;
		uint32 BitsPerPixel;
		uint32 RedMask;
		uint32 GreenMask;
		uint32 BlueMask;
		uint32 AlphaMask;
		uint32 FourCC;				// Only read for V2 headers.
		uint32 NumSurfaces;			// Only read for V2 headers. Set to 1 for V1 files.
	};
	tStaticAssert(sizeof(HeaderV1V2) == 52);

	struct HeaderV3
	{
		uint32 FourCCVersion;		// 'PVR3' for V3. LE = 0x03525650.
		uint32 Flags;
		uint64 PixelFormat;
		uint32 ColourSpace;			// 0 = Linear RGB. 1 = sRGB (I assume linear alpha for both).
		uint32 ChannelType;			// Matches PVR3CHANTYPE.
		uint32 Height;
		uint32 Width;
		uint32 Depth;
		uint32 NumSurfaces;
		uint32 NumFaces;
		uint32 NumMipmaps;
		uint32 MetaDataSize;
	};
	tStaticAssert(sizeof(HeaderV3) == 52);
	#pragma pack(pop)

	// These are the PVR legacy (V1 and V2 pvr files) format IDs from the specification document at:
	// https://powervr-graphics.github.io/WebGL_SDK/WebGL_SDK/Documentation/Specifications/PVR%20File%20Format.Specification.Legacy.pdf
	// The names match the names in the document.
	enum PVRLFMT : uint8
	{
		PVRLFMT_ARGB_4444			= 0x00,
		PVRLFMT_ARGB_1555			= 0x01,
		PVRLFMT_RGB_565				= 0x02,
		PVRLFMT_RGB_555				= 0x03,
		PVRLFMT_RGB_888				= 0x04,
		PVRLFMT_ARGB_8888			= 0x05,
		PVRLFMT_ARGB_8332			= 0x06,
		PVRLFMT_I_8					= 0x07,
		PVRLFMT_AI_88				= 0x08,
		PVRLFMT_1BPP				= 0x09,
		PVRLFMT_V_Y1_U_Y0			= 0x0A,
		PVRLFMT_Y1_V_Y0_U			= 0x0B,
		PVRLFMT_PVRTC2				= 0x0C,		// Better name would be PVRTCIBPP2 but want to match docs.
		PVRLFMT_PVRTC4				= 0x0D,		// Better name would be PVRTCIBPP4 but want to match docs.

		PVRLFMT_ARGB_4444_ALT		= 0x10,
		PVRLFMT_ARGB_1555_ALT		= 0x11,
		PVRLFMT_ARGB_8888_ALT		= 0x12,
		PVRLFMT_RGB_565_ALT			= 0x13,
		PVRLFMT_RGB_555_ALT			= 0x14,
		PVRLFMT_RGB_888_ALT			= 0x15,
		PVRLFMT_I_8_ALT				= 0x16,
		PVRLFMT_AI_88_ALT			= 0x17,
		PVRLFMT_PVRTC2_ALT			= 0x18,		// Better name would be PVRTCIBPP2 but want to match docs.
		PVRLFMT_PVRTC4_ALT			= 0x19,		// Better name would be PVRTCIBPP4 but want to match docs.

		PVRLFMT_BGRA_8888			= 0x1A,
		PVRLFMT_DXT1				= 0x20,
		PVRLFMT_DXT2				= 0x21,
		PVRLFMT_DXT3				= 0x22,
		PVRLFMT_DXT4				= 0x23,
		PVRLFMT_DXT5				= 0x24,
		PVRLFMT_RGB_332				= 0x25,
		PVRLFMT_AL_44				= 0x26,
		PVRLFMT_LVU_655				= 0x27,
		PVRLFMT_XLVU_8888			= 0x28,
		PVRLFMT_QWVU_8888			= 0x29,
		PVRLFMT_ABGR_2101010		= 0x2A,
		PVRLFMT_ARGB_2101010		= 0x2B,
		PVRLFMT_AWVU_2101010		= 0x2C,
		PVRLFMT_GR_1616				= 0x2D,
		PVRLFMT_VU_1616				= 0x2E,
		PVRLFMT_ABGR_16161616		= 0x2F,
		PVRLFMT_R_16F				= 0x30,
		PVRLFMT_GR_1616F			= 0x31,
		PVRLFMT_ABGR_16161616F		= 0x32,
		PVRLFMT_R_32F				= 0x33,
		PVRLFMT_GR_3232F			= 0x34,
		PVRLFMT_ABGR_32323232F		= 0x35,
		PVRLFMT_ETC					= 0x36,
		PVRLFMT_A_8					= 0x40,
		PVRLFMT_VU_88				= 0x41,
		PVRLFMT_L16					= 0x42,
		PVRLFMT_L8					= 0x43,
		PVRLFMT_AL_88				= 0x44,
		PVRLFMT_UYVY				= 0x45,
		PVRLFMT_YUY2				= 0x46
	};

	// These match the names of the channel type in the official PVR3 filespec found here:
	// https://imagination-technologies-cloudfront-assets.s3.eu-west-1.amazonaws.com/website-files/documents/PVR+File+Format.Specification.pdf
	enum PVR3CHANTYPE : uint32
	{
		PVR3CHANTYPE_UnsignedByteNormalised		= 0x00000000,
		PVR3CHANTYPE_SignedByteNormalised		= 0x00000001,
		PVR3CHANTYPE_UnsignedByte				= 0x00000002,
		PVR3CHANTYPE_SignedByte					= 0x00000003,
		PVR3CHANTYPE_UnsignedShortNormalised	= 0x00000004,
		PVR3CHANTYPE_SignedShortNormalised		= 0x00000005,
		PVR3CHANTYPE_UnsignedShort				= 0x00000006,
		PVR3CHANTYPE_SignedShort				= 0x00000007,
		PVR3CHANTYPE_UnsignedIntegerNormalised	= 0x00000008,
		PVR3CHANTYPE_SignedIntegerNormalised	= 0x00000009,
		PVR3CHANTYPE_UnsignedInteger			= 0x0000000A,
		PVR3CHANTYPE_SignedInteger				= 0x0000000B,
		PVR3CHANTYPE_Float						= 0x0000000C,
		PVR3CHANTYPE_UnsignedFloat				= 0x0000000D	// Inferred from files generated by PVRTexTool. Not found in spec doc.
	};

	// These match the names of the LS 32-bits of the 64-bit pixel format as specified in the PVR3 file-spec found here:
	// https://imagination-technologies-cloudfront-assets.s3.eu-west-1.amazonaws.com/website-files/documents/PVR+File+Format.Specification.pdf
	enum PVR3FMT : uint32
	{
		PVR3FMT_PVRTC_2BPP_RGB					= 0x00000000,
		PVR3FMT_PVRTC_2BPP_RGBA					= 0x00000001,
		PVR3FMT_PVRTC_4BPP_RGB					= 0x00000002,
		PVR3FMT_PVRTC_4BPP_RGBA					= 0x00000003,
		PVR3FMT_PVRTC_II_2BPP					= 0x00000004,
		PVR3FMT_PVRTC_II_4BPP					= 0x00000005,
		PVR3FMT_ETC1							= 0x00000006,
		PVR3FMT_DXT1_BC1						= 0x00000007,
		PVR3FMT_DXT2							= 0x00000008,
		PVR3FMT_DXT3_BC2						= 0x00000009,
		PVR3FMT_DXT4							= 0x0000000A,
		PVR3FMT_DXT5_BC3						= 0x0000000B,
		PVR3FMT_BC4								= 0x0000000C,
		PVR3FMT_BC5								= 0x0000000D,
		PVR3FMT_BC6								= 0x0000000E,
		PVR3FMT_BC7								= 0x0000000F,
		PVR3FMT_UYVY							= 0x00000010,
		PVR3FMT_YUY2							= 0x00000011,
		PVR3FMT_BW1BPP							= 0x00000012,
		PVR3FMT_R9G9B9E5_Shared_Exponent		= 0x00000013,
		PVR3FMT_RGBG8888						= 0x00000014,
		PVR3FMT_GRGB8888						= 0x00000015,
		PVR3FMT_ETC2_RGB						= 0x00000016,
		PVR3FMT_ETC2_RGBA						= 0x00000017,
		PVR3FMT_ETC2_RGB_A1						= 0x00000018,
		PVR3FMT_EAC_R11							= 0x00000019,
		PVR3FMT_EAC_RG11						= 0x0000001A,
		PVR3FMT_ASTC_4X4						= 0x0000001B,
		PVR3FMT_ASTC_5X4						= 0x0000001C,
		PVR3FMT_ASTC_5X5						= 0x0000001D,
		PVR3FMT_ASTC_6X5						= 0x0000001E,
		PVR3FMT_ASTC_6X6						= 0x0000001F,
		PVR3FMT_ASTC_8X5						= 0x00000020,
		PVR3FMT_ASTC_8X6						= 0x00000021,
		PVR3FMT_ASTC_8X8						= 0x00000022,
		PVR3FMT_ASTC_10X5						= 0x00000023,
		PVR3FMT_ASTC_10X6						= 0x00000024,
		PVR3FMT_ASTC_10X8						= 0x00000025,
		PVR3FMT_ASTC_10X10						= 0x00000026,
		PVR3FMT_ASTC_12X10						= 0x00000027,
		PVR3FMT_ASTC_12X12						= 0x00000028,
		PVR3FMT_ASTC_3X3X3						= 0x00000029,
		PVR3FMT_ASTC_4X3X3						= 0x0000002A,
		PVR3FMT_ASTC_4X4X3						= 0x0000002B,
		PVR3FMT_ASTC_4X4X4						= 0x0000002C,
		PVR3FMT_ASTC_5X4X4						= 0x0000002D,
		PVR3FMT_ASTC_5X5X4						= 0x0000002E,
		PVR3FMT_ASTC_5X5X5						= 0x0000002F,
		PVR3FMT_ASTC_6X5X5						= 0x00000030,
		PVR3FMT_ASTC_6X6X5						= 0x00000031,
		PVR3FMT_ASTC_6X6X6						= 0x00000032,

		PVR3FMT_RGBM							= 0x00000035,
		PVR3FMT_RGBD							= 0x00000036,
	};

	enum PVR3KEY : uint32
	{
		PVR3KEY_ATLAS							= 0x00000000,
		PVR3KEY_NORMALMAP						= 0x00000001,
		PVR3KEY_CUBEMAP							= 0x00000002,
		PVR3KEY_ORIENTATION						= 0x00000003,
		PVR3KEY_BORDER							= 0x00000004,
		PVR3KEY_PADDING							= 0x00000005,
		PVR3KEY_UNKNOWN							= 0x00000006
	};

	int DetermineVersionFromFirstFourBytes(const uint8 bytes[4]);

	// Determine the pixel-format and, if possible, the alpha-mode and channel-type. There is no possibility of
	// determining the colour-profile for V1V2 file headers but we pass it in anyways so it behaves similarly to the V3
	// GetFormatInfo. If the pixel format is returned unspecified the headerFmt is not supported or was invalid. In
	// this case the returned colour-profile, alpha-mode, and channel-type will be unspecified.
	void GetFormatInfo_FromV1V2Header(tPixelFormat&, tColourProfile&, tAlphaMode&, tChannelType&, const HeaderV1V2&);

	// For V3 file headers the channel-type, alpha-mode, and colour-space can always be determined. In addition some
	// V3 pixel-formats imply a particular colour space and alpha-mode. In cases where these do not match the required
	// type, mode, or space of the pixel-format, the pixel-format's required setting is chosen.
	void GetFormatInfo_FromV3Header(tPixelFormat&, tColourProfile&, tAlphaMode&, tChannelType&, const HeaderV3&);

	void Flip(tLayer*, bool horizontal);
	inline int GetIndex(int x, int y, int w, int h)																		{ tAssert((x >= 0) && (y >= 0) && (x < w) && (y < h)); return y * w + x; }
}


int tPVR::DetermineVersionFromFirstFourBytes(const uint8 bytes[4])
{
	if (!bytes)
		return 0;

	uint32 value = *((uint32*)bytes);
	if (value == 44)
		return 1;
	
	if (value == 52)
		return 2;
	
	if (value == 0x03525650)
		return 3;

	return 0;
}


void tPVR::GetFormatInfo_FromV1V2Header(tPixelFormat& format, tColourProfile& profile, tAlphaMode& alphaMode, tChannelType& chanType, const HeaderV1V2& header)
{
	format		= tPixelFormat::Invalid;
	profile		= tColourProfile::sRGB;
	alphaMode	= tAlphaMode::Unspecified;
	chanType	= tChannelType::Unspecified;

	#define C(c) case PVRLFMT_##c
	#define F(f) format = tPixelFormat::f;
	#define P(p) profile = tColourProfile::p;
	#define M(m) alphaMode = tAlphaMode::m;
	#define T(t) chanType = tChannelType::t;
	switch (header.PixelFormat)
	{
		// Commented out PVRLFMT formats are not implemented yet.
		//						Format				Profile		AlphaMode	ChanType	Break
		C(ARGB_4444):			F(G4B4A4R4)												break;
		C(ARGB_1555):			F(G3B5A1R5G2)											break;
		C(RGB_565):				F(G3B5R5G3)												break;
		//C(RGB_555):
		C(RGB_888):				F(R8G8B8)												break;
		C(ARGB_8888):			F(B8G8R8A8)												break;
		//C(ARGB_8332):
		C(I_8):					F(L8)													break;
		C(AI_88):				F(A8L8)													break;
		//C(1BPP):
		//C(V_Y1_U_Y0):
		//C(Y1_V_Y0_U):
		C(PVRTC2):				F(PVRBPP2)												break;
		C(PVRTC4):				F(PVRBPP4)												break;

		C(ARGB_4444_ALT):		F(G4B4A4R4)												break;
		C(ARGB_1555_ALT):		F(G3B5A1R5G2)											break;
		C(ARGB_8888_ALT):		F(R8G8B8A8)												break;
		C(RGB_565_ALT):			F(G3B5R5G3)												break;
		//C(RGB_555_ALT):
		C(RGB_888_ALT):			F(R8G8B8)												break;
		C(I_8_ALT):				F(L8)													break;
		C(AI_88_ALT):			F(A8L8)													break;
		C(PVRTC2_ALT):			F(PVRBPP2)												break;
		C(PVRTC4_ALT):			F(PVRBPP4)												break;

		C(BGRA_8888):			F(B8G8R8A8)												break;
		C(DXT1):				F(BC1DXT1)												break;
		C(DXT2):				F(BC2DXT2DXT3)					M(Mult)					break;
		C(DXT3):				F(BC2DXT2DXT3)											break;
		C(DXT4):				F(BC3DXT4DXT5)					M(Mult)					break;
		C(DXT5):				F(BC3DXT4DXT5)											break;
		//C(RGB_332):
		//C(AL_44):
		//C(LVU_655):
		//C(XLVU_8888):
		//C(QWVU_8888):
		//C(ABGR_2101010):
		//C(ARGB_2101010):
		//C(AWVU_2101010):
		//C(GR_1616):
		//C(VU_1616):
		//C(ABGR_16161616):
		C(R_16F):				F(R16f)				P(lRGB)					T(SFLOAT)	break;
		C(GR_1616F):			F(R16G16f)			P(lRGB)					T(SFLOAT)	break;
		C(ABGR_16161616F):		F(R16G16B16A16f)	P(lRGB)					T(SFLOAT)	break;
		C(R_32F):				F(R32f)				P(lRGB)					T(SFLOAT)	break;
		C(GR_3232F):			F(R32G32f)			P(lRGB)					T(SFLOAT)	break;
		C(ABGR_32323232F):		F(R32G32B32A32f)	P(lRGB)					T(SFLOAT)	break;

		// V2 ETC1 files generated from PVRTexTool are always in linear space. There is no sRGB option.
		C(ETC):					F(ETC1)				P(lRGB)								break;
		C(A_8):					F(A8)													break;
		//C(VU_88):
		//C(L16):
		C(L8):					F(L8)				P(lRGB)					T(UINT)		break;
		//C(AL_88):
		//C(UYVY):
		//C(YUY2):
		default:									P(None)								break;
	}
	#undef C
	#undef F
	#undef P
	#undef M
	#undef T
}


void tPVR::GetFormatInfo_FromV3Header(tPixelFormat& format, tColourProfile& profile, tAlphaMode& alphaMode, tChannelType& chanType, const HeaderV3& header)
{
	format		= tPixelFormat::Invalid;
	profile		= (header.ColourSpace == 0) ? tColourProfile::lRGB : tColourProfile::sRGB;
	alphaMode	= (header.Flags & 0x00000002) ? tAlphaMode::Premultiplied : tAlphaMode::Normal;
	chanType	= tChannelType::Unspecified;

	#define C(c) case PVR3CHANTYPE_##c
	#define T(t) chanType = tChannelType::t;
	switch (header.ChannelType)
	{
		C(UnsignedByteNormalised):		T(UNORM)		break;
		C(SignedByteNormalised):		T(SNORM)		break;
		C(UnsignedByte):				T(UINT)			break;
		C(SignedByte):					T(SINT)			break;

		C(UnsignedShortNormalised):		T(UNORM)		break;
		C(SignedShortNormalised):		T(SNORM)		break;
		C(UnsignedShort):				T(UINT)			break;
		C(SignedShort):					T(SINT)			break;

		C(UnsignedIntegerNormalised):	T(UNORM)		break;
		C(SignedIntegerNormalised):		T(SNORM)		break;
		C(UnsignedInteger):				T(UINT)			break;
		C(SignedInteger):				T(SINT)			break;

		C(Float):						T(SFLOAT)		break;
		C(UnsignedFloat):				T(UFLOAT)		break;
	}
	#undef C
	#undef T

	// For V3 files if the MS 32 bits are 0, the format is determined by the LS 32 bits.
	// If the MS 32 do bits are non zero, the MS 32 bits contain the number of bits for
	// each channel and the present channels are specified by the LS 32 bits.
	uint32 fmtMS32 = header.PixelFormat >> 32;
	uint32 fmtLS32 = header.PixelFormat & 0x00000000FFFFFFFF;
	if (fmtMS32 == 0)
	{
		#define C(c) case PVR3FMT_##c
		#define F(f) format = tPixelFormat::f;
		#define P(p) profile = tColourProfile::p;
		#define M(m) alphaMode = tAlphaMode::m;
		#define T(t) chanType = tChannelType::t;
		switch (fmtLS32)
		{
			// Commented out PVRLFMT formats are not implemented yet. Some formats require specific profiles and/or
			// alpha-modes. When these are required they override the settings from the header.
			// PVR stores alpha on a per-block basis, not the entire image. Images without alpha just happen
			// to have all opaque blocks. In either case, the pixel format is the same -- PVRBPP2 or PVRBPP4.
			//								Format				Profile		AlphaMode	ChanType	Break
			C(PVRTC_2BPP_RGB):				F(PVRBPP2)												break;
			C(PVRTC_2BPP_RGBA):				F(PVRBPP2)												break;
			C(PVRTC_4BPP_RGB):				F(PVRBPP4)												break;
			C(PVRTC_4BPP_RGBA):				F(PVRBPP4)												break;

			#ifdef PIXEL_FORMAT_INCLUDE_NOT_IMPLEMENTED
			C(PVRTC_II_2BPP):				F(PVR2BPP2)												break;
			C(PVRTC_II_4BPP):				F(PVR2BPP4)												break;
			#endif
			C(ETC1):						F(ETC1)													break;

			C(DXT1_BC1):					F(BC1DXT1)												break;
			C(DXT2):						F(BC2DXT2DXT3)					M(Mult)					break;
			C(DXT3_BC2):					F(BC2DXT2DXT3)											break;
			C(DXT4):						F(BC3DXT4DXT5)					M(Mult)					break;
			C(DXT5_BC3):					F(BC3DXT4DXT5)											break;
			C(BC4):	P(lRGB) if (chanType == tChannelType::SNORM)	F(BC4ATI1S) else	F(BC4ATI1U)	break;
			C(BC5):	P(lRGB) if (chanType == tChannelType::SNORM)	F(BC5ATI2S) else	F(BC5ATI2U)	break;
			C(BC6):							F(BC6U)				P(HDRa)					T(UFLOAT)	break;	// Not sure whether signed or unsigned. Assuming unsigned.
			C(BC7):							F(BC7)													break;
			//C(UYVY):
			//C(YUY2):
			//C(BW1BPP):
			C(R9G9B9E5_Shared_Exponent):	F(E5B9G9R9uf)		P(HDRa)					T(UFLOAT)	break;
			//C(RGBG8888):
			//C(GRGB8888):
			C(ETC2_RGB):					F(ETC2RGB)												break;
			C(ETC2_RGBA):					F(ETC2RGBA)												break;
			C(ETC2_RGB_A1):					F(ETC2RGBA1)											break;
			C(EAC_R11):  if (chanType == tChannelType::SNORM)		F(EACR11S)  else	F(EACR11U)	break;
			C(EAC_RG11): if (chanType == tChannelType::SNORM)		F(EACRG11S) else	F(EACRG11U)	break;
			C(ASTC_4X4):					F(ASTC4X4)												break;
			C(ASTC_5X4):					F(ASTC5X4)												break;
			C(ASTC_5X5):					F(ASTC5X5)												break;
			C(ASTC_6X5):					F(ASTC6X5)												break;
			C(ASTC_6X6):					F(ASTC6X6)												break;
			C(ASTC_8X5):					F(ASTC8X5)												break;
			C(ASTC_8X6):					F(ASTC8X6)												break;
			C(ASTC_8X8):					F(ASTC8X8)												break;
			C(ASTC_10X5):					F(ASTC10X5)												break;
			C(ASTC_10X6):					F(ASTC10X6)												break;
			C(ASTC_10X8):					F(ASTC10X8)												break;
			C(ASTC_10X10):					F(ASTC10X10)											break;
			C(ASTC_12X10):					F(ASTC12X10)											break;
			C(ASTC_12X12):					F(ASTC12X12)											break;
			//C(ASTC_3X3X3):
			//C(ASTC_4X3X3):
			//C(ASTC_4X4X3):
			//C(ASTC_4X4X4):
			//C(ASTC_5X4X4):
			//C(ASTC_5X5X4):
			//C(ASTC_5X5X5):
			//C(ASTC_6X5X5):
			//C(ASTC_6X6X5):
			//C(ASTC_6X6X6):
			C(RGBM):						F(R8G8B8M8)			P(HDRa)								break;
			C(RGBD):						F(R8G8B8D8)			P(HDRa)								break;
			default:											P(None)		M(None)	 	T(NONE)		break;
		}
		#undef C
		#undef F
		#undef P
		#undef M
		#undef T
	}
	else
	{
		// The FourCC and the tSwapEndian32 calls below deal with endianness. The values
		// of the literals in the fourCC match the values of the masks in fmtMS32 member.
		#define F(f) format = tPixelFormat::f;
		#define P(p) profile = tColourProfile::p;
		switch (fmtLS32)
		{
			case tImage::FourCC('r', '\0', '\0', '\0'):
			{
				if (chanType == tChannelType::SFLOAT)
				{
					switch (fmtMS32)
					{
						case tSwapEndian32(0x10000000):	F(R16f)	P(HDRa)	break;
						case tSwapEndian32(0x20000000):	F(R32f)	P(HDRa)	break;
					}
				}
				else
				{
					switch (fmtMS32)
					{
						case tSwapEndian32(0x10000000):	F(R16)	P(lRGB)	break;
						case tSwapEndian32(0x20000000):	F(R32)	P(lRGB)	break;
					}
				}
				break;
			}

			case tImage::FourCC('r', 'g', '\0', '\0'):
			{
				if (chanType == tChannelType::SFLOAT)
				{
					switch (fmtMS32)
					{
						case tSwapEndian32(0x10100000):	F(R16G16f) P(HDRa)	break;
						case tSwapEndian32(0x20200000):	F(R32G32f) P(HDRa)	break;
					}
				}
				else
				{
					switch (fmtMS32)
					{
						case tSwapEndian32(0x10100000):	F(R16G16)	P(lRGB)	break;
						case tSwapEndian32(0x20200000):	F(R32G32)	P(lRGB)	break;
					}
				}
				break;
			}

			case tImage::FourCC('r', 'g', 'b', '\0'):
			{
				if (chanType == tChannelType::SFLOAT)
				{
					switch (fmtMS32)
					{
						case tSwapEndian32(0x10101000):	F(R16G16B16f)	P(HDRa)	break;
						case tSwapEndian32(0x20202000):	F(R32G32B32f)	P(HDRa)	break;
					}
				}
				else
				{
					switch (fmtMS32)
					{
						case tSwapEndian32(0x05060500):	F(G3B5R5G3)				break;	// LE PVR: R5 G6 B5.
						case tSwapEndian32(0x10101000):	F(R16G16B16)	P(lRGB)	break;
						case tSwapEndian32(0x20202000):	F(R32G32B32)	P(lRGB)	break;
					}
				}
				break;
			}

			case tImage::FourCC('b', 'g', 'r', '\0'):
			{
				if (chanType == tChannelType::UFLOAT)
				{
					switch (fmtMS32)
					{
						case tSwapEndian32(0x0a0b0b00):	F(B10G11R11uf)		break;	// PVR: B10 G11 R11 UFLOAT.
					}
					break;
				}
				break;
			}

			case tImage::FourCC('r', 'g', 'b', 'a'):
			{
				if (chanType == tChannelType::SFLOAT)
				{
					switch (fmtMS32)
					{
						case tSwapEndian32(0x10101010):	F(R16G16B16A16f)	break;
						case tSwapEndian32(0x20202020):	F(R32G32B32A32f)	break;
					}
				}
				else
				{
					switch (fmtMS32)
					{
						case tSwapEndian32(0x08080808):	F(R8G8B8A8)			break;
						case tSwapEndian32(0x04040404):	F(B4A4R4G4)			break;
						case tSwapEndian32(0x05050501):	F(G2B5A1R5G3)		break;
						case tSwapEndian32(0x10101010):	F(R16G16B16A16)		P(lRGB)	break;
						case tSwapEndian32(0x20202020):	F(R32G32B32A32)		P(lRGB)	break;
					}
				}
				break;
			}

			case tImage::FourCC('a', 'r', 'g', 'b'):
			{
				switch (fmtMS32)
				{
					case tSwapEndian32(0x01050505):	F(G3B5A1R5G2)		break;	// LE PVR: A1 R5 G5 B5.
					case tSwapEndian32(0x04040404):	F(G4B4A4R4)			break;	// LE PVR: A4 R4 G4 B4.
				}
				break;
			}

			case tImage::FourCC('b', 'g', 'r', 'a'):
			{
				switch (fmtMS32)
				{
					case tSwapEndian32(0x08080808):	F(B8G8R8A8)			break;	// LE PVR: A1 R5 G5 B5.
				}
				break;
			}
		}
		#undef F
		#undef P
	}

	// PVR V3 files do not distinguish between lRGB and HDRa profiles -- it only supports lRGB. While they
	// both are linear, Tacent follows the ASTC convention of distinguishing between them by supporting > 1.0
	// for HDR profiles. Here, if the type is float and the profile is linear, we switch to HDR.
	if ((profile == tColourProfile::lRGB) && ((chanType == tChannelType::UFLOAT) || (chanType == tChannelType::SFLOAT)))
		profile = tColourProfile::HDRa;
}


void tPVR::Flip(tLayer* layer, bool horizontal)
{
	if (!layer->IsValid() || (layer->PixelFormat != tPixelFormat::R8G8B8A8))
		return;

	int w = layer->Width;
	int h = layer->Height;
	tPixel4b* srcPixels = (tPixel4b*)layer->Data;
	tPixel4b* newPixels = new tPixel4b[w*h];

	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
			newPixels[ GetIndex(x, y, w, h) ] = srcPixels[ GetIndex(horizontal ? w-1-x : x, horizontal ? y : h-1-y, w, h) ];

	// We modify the existing data just in case layer doesn't own it.
	tStd::tMemcpy(layer->Data, newPixels, w*h*sizeof(tPixel4b));
	delete[] newPixels;
}


void tImagePVR::Clear()
{
	// Clear all layers no matter what they're used for.
	for (int layer = 0; layer < NumLayers; layer++)
		delete Layers[layer];

	// Now delete the layers array.
	delete[] Layers;
	Layers = nullptr;
	NumLayers = 0;

	States							= 0;		// Image will be invalid now since Valid state not set.
	PVRVersion						= 0;
	PixelFormatSrc					= tPixelFormat::Invalid;
	PixelFormat						= tPixelFormat::Invalid;
	ColourProfileSrc				= tColourProfile::Unspecified;
	ColourProfile					= tColourProfile::Unspecified;
	AlphaMode						= tAlphaMode::Unspecified;
	ChannelType						= tChannelType::Unspecified;
	RowReversalOperationPerformed	= false;

	NumSurfaces						= 0;		// For storing arrays of image data.
	NumFaces						= 0;		// For cubemaps.
	NumMipmaps						= 0;

	Depth							= 0;		// Number of slices.
	Width							= 0;
	Height							= 0;

	MetaData_Orientation_Flip_X		= false;
	MetaData_Orientation_Flip_Y		= false;
}


bool tImagePVR::Set(tPixel4b* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	PixelFormatSrc					= tPixelFormat::R8G8B8A8;
	PixelFormat						= tPixelFormat::R8G8B8A8;
	ColourProfileSrc				= tColourProfile::LDRsRGB_LDRlA;
	ColourProfile					= tColourProfile::LDRsRGB_LDRlA;
	AlphaMode						= tAlphaMode::Normal;
	ChannelType						= tChannelType::UNORM;
	RowReversalOperationPerformed	= false;

	NumSurfaces						= 1;
	NumFaces						= 1;
	NumMipmaps						= 1;
	Depth							= 1;
	Width							= width;
	Height							= height;

	NumLayers						= 1;
	Layers = new tLayer*[1];

	// Order is surface, face, mipmap, slice.
	Layers[0]						= new tLayer(tPixelFormat::R8G8B8A8, Width, Height, (uint8*)pixels, steal);

	SetStateBit(StateBit::Valid);
	return true;
}


bool tImagePVR::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	tPixel4b* pixels = frame->GetPixels(steal);
	Set(pixels, frame->Width, frame->Height, steal);
	if (steal)
		delete frame;

	return IsValid();
}


bool tImagePVR::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel4b* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


bool tImagePVR::Load(const tString& pvrFile, const LoadParams& loadParams)
{
	Clear();
	Filename = pvrFile;
	if (tSystem::tGetFileType(pvrFile) != tSystem::tFileType::PVR)
	{
		SetStateBit(StateBit::Fatal_IncorrectFileType);
		return false;
	}

	if (!tSystem::tFileExists(pvrFile))
	{
		SetStateBit(StateBit::Fatal_FileDoesNotExist);
		return false;
	}

	int pvrSizeBytes = 0;
	uint8* pvrData = (uint8*)tSystem::tLoadFile(pvrFile, 0, &pvrSizeBytes);
	bool success = Load(pvrData, pvrSizeBytes, loadParams);
	delete[] pvrData;

	return success;
}


bool tImagePVR::Load(const uint8* pvrData, int pvrDataSize, const LoadParams& paramsIn)
{
	Clear();
	LoadParams params(paramsIn);

	PVRVersion = tPVR::DetermineVersionFromFirstFourBytes(pvrData);
	if (PVRVersion == 0)
	{
		SetStateBit(StateBit::Fatal_UnsupportedPVRFileVersion);
		return false;
	}

	ColourProfile		= tColourProfile::Unspecified;
	ColourProfileSrc	= tColourProfile::Unspecified;
	AlphaMode			= tAlphaMode::Unspecified;
	ChannelType			= tChannelType::Unspecified;

	int metaDataSize = 0;
	const uint8* metaData = nullptr;
	const uint8* textureData = nullptr;

	switch (PVRVersion)
	{
		case 1:
		case 2:
		{
			tPVR::HeaderV1V2* header = (tPVR::HeaderV1V2*)pvrData;
			tPVR::GetFormatInfo_FromV1V2Header(PixelFormatSrc, ColourProfileSrc, AlphaMode, ChannelType,  *header);
			if (PixelFormatSrc == tPixelFormat::Invalid)
			{
				SetStateBit(StateBit::Fatal_PixelFormatNotSupported);
				return false;
			}
			PixelFormat					= PixelFormatSrc;
			ColourProfile				= ColourProfileSrc;
			NumSurfaces					= (PVRVersion == 2) ? header->NumSurfaces : 1;		// NumSurfaces not supported by V1 files.
			NumFaces					= 1;
			NumMipmaps					= header->MipMapCount;
			Depth						= 1;
			Width						= header->Width;
			Height						= header->Height;
			if
			(
				((PixelFormat == tPixelFormat::PVRBPP2) || (PixelFormat == tPixelFormat::PVRBPP4)) &&
				((Width < 4) || (Height < 4) || !tMath::tIsPower2(Width) || !tMath::tIsPower2(Height))
			)
			{
				if (params.Flags & LoadFlag_StrictLoading)
				{
					SetStateBit(StateBit::Fatal_V1V2InvalidDimensionsPVRTC1);
					return false;
				}
				SetStateBit(StateBit::Conditional_V1V2InvalidDimensionsPVRTC1);
			}

			// Flags are LE order on disk.
			uint32 flags				= (header->Flags1 << 24) | (header->Flags2 << 16) | (header->Flags1 << 8);
			bool hasMipmaps				= (flags & 0x00000100) ? true : false;
			bool dataTwiddled			= (flags & 0x00000200) ? true : false;
			bool containsNormalData		= (flags & 0x00000400) ? true : false;
			bool hasABorder				= (flags & 0x00000800) ? true : false;
			bool isACubemap				= (flags & 0x00001000) ? true : false;		// Every 6 surfaces is one cubemap.
			bool mipmapsDebugColour		= (flags & 0x00002000) ? true : false;
			bool isAVolumeTexture		= (flags & 0x00004000) ? true : false;		// NumSurfaces is the number of slices (depth).
			bool alphaPresentInPVRTC	= (flags & 0x00008000) ? true : false;

			// This is a bit odd, but if a PVR V1 V2 does not have mipmaps it does not set
			// the number of mipmaps to 1. It would be cleaner if it did, so we do it here.
			if (!hasMipmaps && (NumMipmaps == 0))
				NumMipmaps = 1;

			if ((!hasMipmaps && (NumMipmaps > 1)) || (hasMipmaps && (NumMipmaps <= 1)))
			{
				if (params.Flags & LoadFlag_StrictLoading)
				{
					SetStateBit(StateBit::Fatal_V1V2MipmapFlagInconsistent);
					return false;
				}
				SetStateBit(StateBit::Conditional_V1V2MipmapFlagInconsistent);
			}			

			if (dataTwiddled)
			{
				SetStateBit(StateBit::Fatal_V1V2TwiddlingUnsupported);
				return false;
			}

			if (isACubemap && (NumSurfaces != 6))
			{
				SetStateBit(StateBit::Fatal_V1V2CubemapFlagInconsistent);
				return false;
			}

			if (isACubemap)
			{
				NumFaces = NumSurfaces;
				NumSurfaces = 1;
			}
			else if (isAVolumeTexture)
			{
				Depth = NumSurfaces;
				NumSurfaces = 1;
			}

			int bytesPerSurface			= header->SurfaceSize;
			int bitsPerPixel			= header->BitsPerPixel;

			// Only check the FourCC magic for V2 files.
			if ((PVRVersion == 2) && (header->FourCC != tImage::FourCC('P', 'V', 'R', '!')))		// LE 0x21525650.
			{
				if (params.Flags & LoadFlag_StrictLoading)
				{
					SetStateBit(StateBit::Fatal_V2IncorrectFourCC);
					return false;
				}
				else
				{
					SetStateBit(StateBit::Conditional_V2IncorrectFourCC);
				}
			}

			textureData					= pvrData + header->HeaderSize;
			break;
		}

		case 3:
		{
			tPVR::HeaderV3* header = (tPVR::HeaderV3*)pvrData;
			tPVR::GetFormatInfo_FromV3Header(PixelFormatSrc, ColourProfileSrc, AlphaMode, ChannelType, *header);
			if (PixelFormatSrc == tPixelFormat::Invalid)
			{
				SetStateBit(StateBit::Fatal_PixelFormatNotSupported);
				return false;
			}
			PixelFormat					= PixelFormatSrc;
			ColourProfile				= ColourProfileSrc;
			NumSurfaces					= header->NumSurfaces;
			NumFaces					= header->NumFaces;
			NumMipmaps					= header->NumMipmaps;
			Depth						= header->Depth;
			Width						= header->Width;
			Height						= header->Height;
			metaDataSize				= header->MetaDataSize;
			metaData					= pvrData + sizeof(tPVR::HeaderV3);
			textureData					= metaData + metaDataSize;
			break;
		}

		default:
			SetStateBit(StateBit::Fatal_UnsupportedPVRFileVersion);
			return false;
	}

	ParseMetaData(metaData, metaDataSize);

	NumLayers = NumSurfaces * NumFaces * NumMipmaps * Depth;
	if (NumLayers <= 0)
	{
		SetStateBit(StateBit::Fatal_BadHeaderData);
		return false;
	}

	Layers = new tLayer*[NumLayers];
	for (int l = 0; l < NumLayers; l++)
		Layers[l] = nullptr;

	// These return 1 for packed formats. This allows us to treat BC, ASTC, Packed, etc all the same way.
	int blockW = tGetBlockWidth(PixelFormatSrc);
	int blockH = tGetBlockHeight(PixelFormatSrc);
	int bytesPerBlock = tImage::tGetBytesPerBlock(PixelFormatSrc);
	const uint8* srcPixelData = textureData;

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

	// The ordering is different depending on V1V2 or V3. We have already checked for unsupported versions.
	// Start with a V1V2. NumFaces and Depth have already been adjusted.
	if ((PVRVersion == 1) || (PVRVersion == 2))
	{
		for (int surf = 0; surf < NumSurfaces; surf++)
		{
			for (int face = 0; face < NumFaces; face++)
			{
				int width = Width;
				int height = Height;
				for (int mip = 0; mip < NumMipmaps; mip++)
				{
					int numBlocksW = tGetNumBlocks(blockW, width);
					int numBlocksH = tGetNumBlocks(blockH, height);
					int numBlocks = numBlocksW*numBlocksH;
					int numBytes = numBlocks * bytesPerBlock;
					for (int slice = 0; slice < Depth; slice++)
					{
						int index = LayerIdx(surf, face, mip, slice);
						tAssert(Layers[index] == nullptr);
						tLayer* newLayer = CreateNewLayer(params, srcPixelData, numBytes, width, height);
						if (!newLayer)
						{
							Clear();
							return false;
						}
						Layers[index] = newLayer;
						srcPixelData += numBytes;
					}
					width  /= 2; tMath::tiClampMin(width, 1);
					height /= 2; tMath::tiClampMin(height, 1);
				}
			}
		}
	}
	else
	{
		tAssert(PVRVersion == 3);
		int width = Width;
		int height = Height;
		for (int mip = 0; mip < NumMipmaps; mip++)
		{
			int numBlocksW = tGetNumBlocks(blockW, width);
			int numBlocksH = tGetNumBlocks(blockH, height);
			int numBlocks = numBlocksW*numBlocksH;
			int numBytes = numBlocks * bytesPerBlock;
			for (int surf = 0; surf < NumSurfaces; surf++)
			{
				for (int face = 0; face < NumFaces; face++)
				{
					for (int slice = 0; slice < Depth; slice++)
					{
						int index = LayerIdx(surf, face, mip, slice);
						tAssert(Layers[index] == nullptr);
						tLayer* newLayer = CreateNewLayer(params, srcPixelData, numBytes, width, height);
						if (!newLayer)
						{
							Clear();
							return false;
						}
						Layers[index] = newLayer;
						srcPixelData += numBytes;
					}
				}
			}
			width  /= 2; tMath::tiClampMin(width, 1);
			height /= 2; tMath::tiClampMin(height, 1);
		}
	}

	// The flips and rotates below do not clear the pixel format.
	if ((params.Flags & LoadFlag_MetaDataOrient) && (MetaData_Orientation_Flip_X || MetaData_Orientation_Flip_Y))
	{
		// The order of the loops doesn't matter here. We just need to hit every layer.
		for (int surf = 0; surf < NumSurfaces; surf++)
		{
			for (int face = 0; face < NumFaces; face++)
			{
				for (int mip = 0; mip < NumMipmaps; mip++)
				{
					for (int slice = 0; slice < Depth; slice++)
					{
						int index = LayerIdx(surf, face, mip, slice);
						tLayer* layer = Layers[index];
						tAssert(layer != nullptr);
						if (MetaData_Orientation_Flip_X)
							tPVR::Flip(layer, true);
						if (MetaData_Orientation_Flip_Y)
							tPVR::Flip(layer, false);
					}
				}
			}
		}
	}

	// If we were asked to decode, set the current PixelFormat to the decoded format.
	// Otherwise set the current PixelFormat to be the same as the original PixelFormatSrc.
	PixelFormat = (params.Flags & LoadFlag_Decode) ? tPixelFormat::R8G8B8A8 : PixelFormatSrc;

	if (params.Flags & LoadFlag_SRGBCompression)  ColourProfile = tColourProfile::sRGB;
	if (params.Flags & LoadFlag_GammaCompression) ColourProfile = tColourProfile::gRGB;

	SetStateBit(StateBit::Valid);
	tAssert(IsValid());
	return true;
}


int tImagePVR::LayerIdx(int surf, int face, int mip, int depth) const
{
	int index = depth + mip*(Depth) + face*(NumMipmaps*Depth) + surf*(NumFaces*NumMipmaps*Depth);
	tAssert(index < NumLayers);
	return index;
}


tLayer* tImagePVR::CreateNewLayer(const LoadParams& params, const uint8* srcPixelData, int numBytes, int width, int height)
{
	tLayer* layer = new tLayer();

	bool reverseRowOrderRequested = params.Flags & LoadFlag_ReverseRowOrder;
	RowReversalOperationPerformed = false;

	// For images whose height is not a multiple of the block size it makes it tricky when deoompressing to do
	// the more efficient row reversal here, so we defer it. Packed formats have a block height of 1. Only BC
	// and astc have non-unity block dimensins.
	bool doRowReversalBeforeDecode = false;
	if (reverseRowOrderRequested)
	{
		bool canDo = true;
		if (!CanReverseRowData(PixelFormatSrc, height))
			canDo = false;
		doRowReversalBeforeDecode = canDo;
	}

	if (doRowReversalBeforeDecode)
	{
		int blockW = tGetBlockWidth(PixelFormatSrc);
		int blockH = tGetBlockHeight(PixelFormatSrc);
		int numBlocksW = tGetNumBlocks(blockW, width);
		int numBlocksH = tGetNumBlocks(blockH, height);

		uint8* reversedPixelData = tImage::CreateReversedRowData(srcPixelData, PixelFormat, numBlocksW, numBlocksH);
		tAssert(reversedPixelData);

		// We can simply get the layer to steal the memory (the last true arg).
		layer->Set(PixelFormatSrc, width, height, reversedPixelData, true);
	}
	else
	{
		// Not reversing. Use the current srcPixelData. Note that steal is false here so the data
		// is both copied and owned by the new tLayer. The srcPixelData gets deleted later.
		layer->Set(PixelFormatSrc, width, height, (uint8*)srcPixelData, false);
	}
	if (doRowReversalBeforeDecode)
		RowReversalOperationPerformed = true;

	// Not asked to decode. We're basically done.
	if (!(params.Flags & LoadFlag_Decode))
	{
		if (reverseRowOrderRequested && !RowReversalOperationPerformed)
			SetStateBit(StateBit::Conditional_CouldNotFlipRows);

		return layer;
	}

	// We were asked to decode if we made it here.
	// Spread only applies to single-channel (R-only or L-only) formats.
	bool spread = params.Flags & LoadFlag_SpreadLuminance;

	// Decode to 32-bit RGBA.
	bool didRowReversalAfterDecode = false;

	// At the end of decoding _either_ decoded4b _or_ decoded4f will be valid, not both.
	// The decoded4b format used for LDR images.
	// The decoded4f format used for HDR images.
	tColour4b* decoded4b = nullptr;
	tColour4f* decoded4f = nullptr;
	tAssert(layer->GetDataSize() == numBytes);
	DecodeResult result = DecodePixelData
	(
		layer->PixelFormat, layer->Data, numBytes,
		width, height, decoded4b, decoded4f, tColourProfile::Auto, params.MaxRange
	);

	if (result != DecodeResult::Success)
	{
		switch (result)
		{
			case DecodeResult::PackedDecodeError:	SetStateBit(StateBit::Fatal_PackedDecodeError);			break;
			case DecodeResult::BlockDecodeError:	SetStateBit(StateBit::Fatal_BCDecodeError);				break;
			case DecodeResult::ASTCDecodeError:		SetStateBit(StateBit::Fatal_ASTCDecodeError);			break;
			case DecodeResult::PVRDecodeError:		SetStateBit(StateBit::Fatal_PVRDecodeError);			break;
			default:								SetStateBit(StateBit::Fatal_PixelFormatNotSupported);	break;
		}
		delete layer;
		return nullptr;
	}

	// Apply any decode flags.
	tAssert(decoded4f || decoded4b);
	bool flagTone = (params.Flags & tImagePVR::LoadFlag_ToneMapExposure) ? true : false;
	bool flagSRGB = (params.Flags & tImagePVR::LoadFlag_SRGBCompression) ? true : false;
	bool flagGama = (params.Flags & tImagePVR::LoadFlag_GammaCompression)? true : false;
	if (decoded4f && (flagTone || flagSRGB || flagGama))
	{
		for (int p = 0; p < width*height; p++)
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
	if (decoded4b && (flagSRGB || flagGama))
	{
		for (int p = 0; p < width*height; p++)
		{
			tColour4f colour(decoded4b[p]);
			if (flagSRGB)
				colour.LinearToSRGB(tCompBit_RGB);
			if (flagGama)
				colour.LinearToGamma(params.Gamma, tCompBit_RGB);
			decoded4b[p].SetR(colour.R);
			decoded4b[p].SetG(colour.G);
			decoded4b[p].SetB(colour.B);
		}
	}

	// Update the layer with the 32-bit RGBA decoded data. If the data was HDR (float)
	// convert it to 32 bit. Start by getting rid of the existing layer pixel data.
	delete[] layer->Data;
	if (decoded4f)
	{
		tAssert(!decoded4b);
		decoded4b = new tColour4b[width*height];
		for (int p = 0; p < width*height; p++)
			decoded4b[p].Set(decoded4f[p]);
		delete[] decoded4f;
	}

	// Possibly spread the L/Red channel.
	if (spread && tIsLuminanceFormat(layer->PixelFormat))
	{
		for (int p = 0; p < width*height; p++)
		{
			decoded4b[p].G = decoded4b[p].R;
			decoded4b[p].B = decoded4b[p].R;
		}
	}

	layer->Data = (uint8*)decoded4b;
	layer->PixelFormat = tPixelFormat::R8G8B8A8;

	// We've got one more chance to reverse the rows here (if we still need to) because we were asked to decode.
	if (reverseRowOrderRequested && !RowReversalOperationPerformed && (layer->PixelFormat == tPixelFormat::R8G8B8A8))
	{
		// This shouldn't ever fail. Too easy to reverse RGBA 32-bit.
		uint8* reversedRowData = tImage::CreateReversedRowData(layer->Data, layer->PixelFormat, width, height);
		tAssert(reversedRowData);
		delete[] layer->Data;
		layer->Data = reversedRowData;
		didRowReversalAfterDecode = true;
	}

	if (reverseRowOrderRequested && !RowReversalOperationPerformed && didRowReversalAfterDecode)
		RowReversalOperationPerformed = true;

	if (reverseRowOrderRequested && !RowReversalOperationPerformed)
		SetStateBit(StateBit::Conditional_CouldNotFlipRows);

	return layer;
}


bool tImagePVR::ParseMetaData(const uint8* metaData, int metaDataSize)
{
	if (!metaData || (metaDataSize <= 0))
		return false;

	while (metaDataSize >= 12)
	{
		uint32 fourCC	= *((uint32*)metaData);		metaData += 4;	metaDataSize -= 4;
		uint32 key		= *((uint32*)metaData);		metaData += 4;	metaDataSize -= 4;
		uint32 dataSize	= *((uint32*)metaData);		metaData += 4;	metaDataSize -= 4;

		switch (fourCC)
		{
			case tImage::FourCC('P', 'V', 'R', 3):
			{
				// Most of the built-in meta-data does not affect display of image. Indeed nearly all
				// of the built-in meta-data is not even supported by the official PVRTexTool as of 2023_12_30.
				switch (key)
				{
					case tPVR::PVR3KEY_ATLAS:		break;
					case tPVR::PVR3KEY_NORMALMAP:	break;
					case tPVR::PVR3KEY_CUBEMAP:		break;

					case tPVR::PVR3KEY_ORIENTATION:
						// Three bytes, one for each axis in the order X, Y, Z.						
						// X == 0: Increases right.  X != 0: Increases left.
						// Y == 0: Increases down.   Y != 0: Increases up.
						// X == 0: Increases inward. Z != 0: Increases outward.
						if (dataSize != 3)
							break;
						MetaData_Orientation_Flip_X = metaData[0] ? true : false;
						MetaData_Orientation_Flip_Y = metaData[1] ? true : false;
						break;

					case tPVR::PVR3KEY_BORDER:		break;
					case tPVR::PVR3KEY_PADDING:		break;
					case tPVR::PVR3KEY_UNKNOWN:		break;
				}
				break;
			}
		}

		metaData += dataSize;	metaDataSize -= dataSize;
	}

	return true;
}


const char* tImagePVR::GetStateDesc(StateBit state)
{
	return StateDescriptions[int(state)];
}


tFrame* tImagePVR::GetFrame(bool steal)
{
	// Data must be decoded for this to work.
	tLayer* layer = Layers ? Layers[ LayerIdx(0) ] : nullptr;
	if (!IsValid() || (PixelFormat != tPixelFormat::R8G8B8A8) || (layer == nullptr))
		return nullptr;

	tFrame* frame = new tFrame();
	frame->Width = layer->Width;
	frame->Height = layer->Height;
	frame->PixelFormatSrc = PixelFormatSrc;

	if (steal)
	{
		frame->Pixels = (tPixel4b*)layer->StealData();
		delete layer;
		Layers[ LayerIdx(0) ] = nullptr;
	}
	else
	{
		frame->Pixels = new tPixel4b[frame->Width * frame->Height];
		tStd::tMemcpy(frame->Pixels, (tPixel4b*)layer->Data, frame->Width * frame->Height * sizeof(tPixel4b));
	}

	return frame;
}


bool tImagePVR::StealLayers(tList<tLayer>& layers)
{
	if (!IsValid() || IsCubemap())
		return false;

	for (int layer = 0; layer < NumLayers; layer++)
	{
		layers.Append(Layers[layer]);
		Layers[layer] = nullptr;
	}

	Clear();
	return true;
}


bool tImagePVR::GetLayers(tList<tLayer>& layers) const
{
	if (!IsValid() || IsCubemap())
		return false;

	for (int layer = 0; layer < NumLayers; layer++)
		layers.Append(Layers[layer]);

	return true;
}


int tImagePVR::StealCubemapLayers(tList<tLayer> layerLists[tFaceIndex_NumFaces], uint32 faceFlags)
{
	if (!IsValid() || !IsCubemap() || !faceFlags)
		return 0;

	int faceCount = 0;
	for (int face = 0; face < tFaceIndex_NumFaces; face++)
	{
		uint32 faceFlag = 1 << face;
		if (!(faceFlag & faceFlags))
			continue;

		tList<tLayer>& dstLayers = layerLists[face];
		for (int mip = 0; mip < NumMipmaps; mip++)
		{
			int index = LayerIdx(0, face, mip, 0);
			tLayer* layer = Layers[index];
			dstLayers.Append(layer);
			Layers[index] = nullptr;
		}
		faceCount++;
	}

	Clear();
	return faceCount;
}


int tImagePVR::GetCubemapLayers(tList<tLayer> layerLists[tFaceIndex_NumFaces], uint32 faceFlags) const
{
	if (!IsValid() || !IsCubemap() || !faceFlags)
		return 0;

	int faceCount = 0;
	for (int face = 0; face < tFaceIndex_NumFaces; face++)
	{
		uint32 faceFlag = 1 << face;
		if (!(faceFlag & faceFlags))
			continue;

		tList<tLayer>& dstLayers = layerLists[face];
		for (int mip = 0; mip < NumMipmaps; mip++)
		{
			int index = LayerIdx(0, face, mip, 0);
			tLayer* layer = Layers[index];
			dstLayers.Append(layer);
		}

		faceCount++;
	}

	return faceCount;
}


const char* tImagePVR::StateDescriptions[] =
{
	"Valid",
	"Conditional Valid. Image rows could not be flipped.",
	"Conditional Valid. Pixel format specification ill-formed.",
	"Conditional Valid. V2 Magic FourCC Incorrect.",
	"Conditional Valid. V1 V2 PVRTC1 non-POT dimension or less than 4.",
	"Conditional Valid. V1 V2 Mipmap flag doesn't match mipmap count.",
	"Fatal Error. File does not exist.",
	"Fatal Error. Incorrect file type. Must be a PVR file.",
	"Fatal Error. Filesize incorrect.",
	"Fatal Error. V2 Magic FourCC Incorrect.",
	"Fatal Error. Incorrect PVR header size.",
	"Fatal Error. Bad PVR header data.",
	"Fatal Error. Unsupported PVR file version.",
	"Fatal Error. V1 V2 PVRTC1 non-POT dimension or less than 4.",
	"Fatal Error. Pixel format header size incorrect.",
	"Fatal Error. Pixel format specification incorrect.",
	"Fatal Error. Unsupported pixel format.",
	"Fatal Error. V1 V2 Mipmap flag doesn't match mipmap count.",
	"Fatal Error. V1 V2 Cubemap flag doesn't match map count.",
	"Fatal Error. V1 V2 Twiddled data not supported.",
	"Fatal Error. Unable to decode packed pixels.",
	"Fatal Error. Unable to decode BC pixels.",
	"Fatal Error. Unable to decode ASTC pixels.",
	"Fatal Error. Unable to decode PVR pixels."
};
tStaticAssert(tNumElements(tImagePVR::StateDescriptions) == int(tImagePVR::StateBit::NumStateBits));
tStaticAssert(int(tImagePVR::StateBit::NumStateBits) <= int(tImagePVR::StateBit::MaxStateBits));


}
