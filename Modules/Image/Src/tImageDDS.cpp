// tImageDDS.cpp
//
// This class knows how to load Direct Draw Surface (.dds) files. Saving is not implemented yet.
// It does zero processing of image data. It knows the details of the dds file format and loads the data into tLayers.
// Currently it does not compress or decompress the image data if it is compressed (DXTn), it simply keeps it in the
// same format as the source file. The layers may be 'stolen' from a tImageDDS so that excessive memcpys are avoided.
// After they are stolen the tImageDDS is invalid.
//
// Copyright (c) 2006, 2017, 2019, 2020, 2022, 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tString.h>
#include <Foundation/tHalf.h>
#include <System/tMachine.h>
#include "Image/tImageDDS.h"
#include "Image/tPixelUtil.h"
#include "Image/tPicture.h"
#include "bcdec/bcdec.h"
#include "etcdec/etcdec.h"
#include "astcenc.h"
namespace tImage
{


// Helper functions, enums, and types for parsing DDS files.
namespace tDDS
{
	// Direct Draw pixel format flags. When the comment says legacy here, we really mean it. Not just legacy in that
	// there's no DX10 block, but legacy in that there won't be a DX10 block AND it's still a really old-style dds.
	enum DDFormatFlag : uint32
	{
		// Pixel has alpha. tDDS::Format's MaskAlpha will be valid. I believe ALPHAPIXELS is only ever used in
		// combination with RGB, YUV, or LUMINANCE. An alpha-only dds would use DDFormatFlag_A instead.
		DDFormatFlag_HASALPHA		= 0x00000001,

		// Legacy. Some dds files use this if all they contain is an alpha channel. tDDS::Format's RGBBitCount is the
		// number of bits used for the alpha channel and MaskAlpha will be valid.
		DDFormatFlag_A				= 0x00000002,

		// Use the Four-CC to determine format. That's another whole thing. tDDS::Format's FourCC will be valid. Modern
		// dds files have a Four-CC of 'DX10' and another whole header specifies the actual pixel format. Complex-much?
		DDFormatFlag_FOURCC			= 0x00000004,

		// Palette indexed. 8-bit. Never seen a dds with this.
		DDFormatFlag_PAL8			= 0x00000020,

		// Pixels contain RGB data. tDDS::Format's RGBBitCount is the number of bits used for RGB. Also go ahead and
		// read MaskRed, MaskGreen, and MaskBlue to determine where the data is and how many bits for each component.
		DDFormatFlag_RGB			= 0x00000040,

		// Legacy. Some dds files use this for YUV pixels. tDDS::Format's RGBBitCount will be the number of bits used
		// for YUV (not RGB). Similarly, MaskRed, MaskGreen, and MaskBlue will contain the masks for each YUV component.
		// Red for Y, green for U, and blue for V.
		DDFormatFlag_YUV			= 0x00000200,

		// Legacy. Some dds files use this for single channel luminance-only data. tDDS::Format's RGBBitCount will be
		// the number of bits used for luminance and MaskRed will contain the mask for the luminance component. If the
		// tDDPF_ALPHAPIXELS bit is also set, it would be a luminance-alpha (LA) dds.
		DDFormatFlag_L				= 0x00020000,

		// Now that the raw format flags are out of the way, here are some combinations representing what you can
		// expect to find in a real dds file. These are basically just convenience enumerants that contains some
		// bitwise-ORs of the above flags. You can check for full equality with these enums (no need to check bits).
		DDFormatFlags_FourCC		= DDFormatFlag_FOURCC,
		DDFormatFlags_RGB			= DDFormatFlag_RGB,
		DDFormatFlags_RGBA			= DDFormatFlag_RGB	| DDFormatFlag_HASALPHA,
		DDFormatFlags_L				= DDFormatFlag_L,
		DDFormatFlags_LA			= DDFormatFlag_L	| DDFormatFlag_HASALPHA,
		DDFormatFlags_A				= DDFormatFlag_A,
		DDFormatFlags_PAL8			= DDFormatFlag_PAL8
	};

	#pragma pack(push, 4)
	struct FormatData
	{
		// Must be 32.
		uint32 Size;

		// See tDDS::PixelFormatFlags. Flags to indicate valid fields. Uncompressed formats will usually use
		// tDDS::PixelFormatFlags_RGB to indicate an RGB format, while compressed formats will use tDDS::PixelFormatFlags_FourCC
		// with a four-character code.
		uint32 Flags;

		// "DXT1", "DXT3", and "DXT5" are examples. m_flags should have DDSPixelFormatFlag_FourCC.
		uint32 FourCC;

		// Valid if flags has DDSPixelFormatFlag_RGB. For RGB formats this is the total number of bits per pixel. This
		// value is usually 16, 24, or 32. For A8R8G8B8, this value would be 32.
		uint32 RGBBitCount;

		// For RGB formats these three fields contain the masks for the red, green, and blue channels. For A8R8G8B8 these
		// values would be 0x00FF0000, 0x0000FF00, and 0x000000FF respectively.
		uint32 MaskRed;
		uint32 MaskGreen;
		uint32 MaskBlue;

		// If the flags have DDSPixelFormatFlag_Alpha set, this is valid and contains tha alpha mask. Eg. For A8R8G8B8 this
		// value would be 0xFF000000.
		uint32 MaskAlpha;
	};
	#pragma pack(pop)

	enum CapsBasicFlag : uint32
	{
		CapsBasicFlag_Complex		= 0x00000008,
		CapsBasicFlag_Texture		= 0x00001000,
		CapsBasicFlag_Mipmap		= 0x00400000
	};

	enum CapsExtraFlag : uint32
	{
		CapsExtraFlag_CubeMap		= 0x00000200,
		CapsExtraFlag_CubeMapPosX	= 0x00000400,
		CapsExtraFlag_CubeMapNegX	= 0x00000800,
		CapsExtraFlag_CubeMapPosY	= 0x00001000,
		CapsExtraFlag_CubeMapNegY	= 0x00002000,
		CapsExtraFlag_CubeMapPosZ	= 0x00004000,
		CapsExtraFlag_CubeMapNegZ	= 0x00008000,
		CapsExtraFlag_Volume		= 0x00200000
	};

	#pragma pack(push, 4)
	struct Caps
	{
		// DDS files should always include CapsBasicFlag_Texture. If the file contains mipmaps CapsBasicFlag_Mipmap
		// should be set. For any dds file with more than one main surface, such as a mipmap, cubic environment map,
		// or volume texture, CapsBasicFlag_Complex should also be set.
		uint32 FlagsCapsBasic;

		// For cubic environment maps CapsExtraFlag_CubeMap should be included as well as one or more faces of the map
		// (CapsExtraFlag_CubeMapPosX, etc). For volume textures CapsExtraFlag_Volume should be set.
		uint32 FlagsCapsExtra;
		uint32 Unused[2];
	};
	#pragma pack(pop)

	enum HeaderFlag : uint32
	{
		HeaderFlag_Caps				= 0x00000001,	// Always included.
		HeaderFlag_Height			= 0x00000002,	// Always included. Height of largest image if mipmaps included.
		HeaderFlag_Width			= 0x00000004,	// Always included. Width of largest image if mipmaps included.
		HeaderFlag_Pitch			= 0x00000008,
		HeaderFlag_PixelFormat		= 0x00001000,	// Always included.
		HeaderFlag_MipmapCount		= 0x00020000,
		HeaderFlag_LinearSize		= 0x00080000,
		HeaderFlag_Depth			= 0x00800000
	};

	// Default packing is 8 bytes but the header is 128 bytes (mult of 4), so we make it all work here.
	#pragma pack(push, 4)
	struct Header
	{
		uint32 Size;								// Must be set to 124.
		uint32 Flags;								// See tDDSFlags.
		uint32 Height;								// Height of main image.
		uint32 Width;								// Width of main image.

		// For uncompressed formats, this is the number of bytes per scan line (32-bit aligned) for the main image. dwFlags
		// should include DDSD_PITCH in this case. For compressed formats, this is the total number of bytes for the main
		// image. m_flags should have tDDSFlag_LinearSize in this case.
		uint32 PitchLinearSize;
		uint32 Depth;								// For volume textures. tDDSFlag_Depth is set for this to be valid.
		uint32 MipmapCount;							// Valid if tDDSFlag_MipmapCount set. @todo Count includes main image?
		uint32 UnusedA[11];
		FormatData Format;							// 32 Bytes.
		Caps Capabilities;							// 16 Bytes.
		uint32 UnusedB;
	};
	#pragma pack(pop)

	// D3D formats that may appear in DDS files. Would normally not make them uint32s, but they are also used for FourCCs.
	enum D3DFMT : uint32
	{
		D3DFMT_UNKNOWN				=  0,

		D3DFMT_R8G8B8				= 20,
		D3DFMT_A8R8G8B8				= 21,
		D3DFMT_X8R8G8B8				= 22,
		D3DFMT_R5G6B5				= 23,
		D3DFMT_X1R5G5B5				= 24,
		D3DFMT_A1R5G5B5				= 25,
		D3DFMT_A4R4G4B4				= 26,
		D3DFMT_R3G3B2				= 27,
		D3DFMT_A8					= 28,
		D3DFMT_A8R3G3B2				= 29,
		D3DFMT_X4R4G4B4				= 30,
		D3DFMT_A2B10G10R10			= 31,
		D3DFMT_A8B8G8R8				= 32,
		D3DFMT_X8B8G8R8				= 33,
		D3DFMT_G16R16				= 34,
		D3DFMT_A2R10G10B10			= 35,
		D3DFMT_A16B16G16R16			= 36,

		D3DFMT_A8P8					= 40,
		D3DFMT_P8					= 41,

		D3DFMT_L8					= 50,
		D3DFMT_A8L8					= 51,
		D3DFMT_A4L4					= 52,

		D3DFMT_V8U8					= 60,
		D3DFMT_L6V5U5				= 61,
		D3DFMT_X8L8V8U8				= 62,
		D3DFMT_Q8W8V8U8				= 63,
		D3DFMT_V16U16				= 64,
		D3DFMT_A2W10V10U10			= 67,

		D3DFMT_UYVY					= FourCC('U', 'Y', 'V', 'Y'),
		D3DFMT_R8G8_B8G8			= FourCC('R', 'G', 'B', 'G'),
		D3DFMT_YUY2					= FourCC('Y', 'U', 'Y', '2'),
		D3DFMT_G8R8_G8B8			= FourCC('G', 'R', 'G', 'B'),

		D3DFMT_DXT1					= FourCC('D', 'X', 'T', '1'),
		D3DFMT_DXT2					= FourCC('D', 'X', 'T', '2'),
		D3DFMT_DXT3					= FourCC('D', 'X', 'T', '3'),
		D3DFMT_DXT4					= FourCC('D', 'X', 'T', '4'),
		D3DFMT_DXT5					= FourCC('D', 'X', 'T', '5'),

		D3DFMT_BC4U					= FourCC('B', 'C', '4', 'U'),
		D3DFMT_BC4S					= FourCC('B', 'C', '4', 'S'),
		D3DFMT_BC5U					= FourCC('B', 'C', '5', 'U'),
		D3DFMT_BC5S					= FourCC('B', 'C', '5', 'S'),

		D3DFMT_ATI1					= FourCC('A', 'T', 'I', '1'),
		D3DFMT_AT1N					= FourCC('A', 'T', '1', 'N'),
		D3DFMT_ATI2					= FourCC('A', 'T', 'I', '2'),
		D3DFMT_AT2N					= FourCC('A', 'T', '2', 'N'),

		D3DFMT_ETC					= FourCC('E', 'T', 'C', ' '),	// ETC1 RGB.
		D3DFMT_ETC1					= FourCC('E', 'T', 'C', '1'),	// ETC1 RGB.
		D3DFMT_ETC2					= FourCC('E', 'T', 'C', '2'),	// This is the RGB   ETC2.
		D3DFMT_ETCA					= FourCC('E', 'T', 'C', 'A'),	// This is the RGBA  ETC2.
		D3DFMT_ETCP					= FourCC('E', 'T', 'C', 'P'),	// This is the RGBA1 ETC2.

		D3DFMT_ATC					= FourCC('A', 'T', 'C', ' '),
		D3DFMT_ATCA					= FourCC('A', 'T', 'C', 'A'),
		D3DFMT_ATCI					= FourCC('A', 'T', 'C', 'I'),
		D3DFMT_POWERVR_2BPP			= FourCC('P', 'T', 'C', '2'),
		D3DFMT_POWERVR_4BPP			= FourCC('P', 'T', 'C', '4'),

		D3DFMT_D16_LOCKABLE			= 70,
		D3DFMT_D32					= 71,
		D3DFMT_D15S1				= 73,
		D3DFMT_D24S8				= 75,
		D3DFMT_D24X8				= 77,
		D3DFMT_D24X4S4				= 79,
		D3DFMT_D16					= 80,

		D3DFMT_D32F_LOCKABLE		= 82,
		D3DFMT_D24FS8				= 83,

		D3DFMT_D32_LOCKABLE			= 84,
		D3DFMT_S8_LOCKABLE			= 85,

		D3DFMT_L16					= 81,

		D3DFMT_VERTEXDATA			= 100,
		D3DFMT_INDEX16				= 101,
		D3DFMT_INDEX32				= 102,

		D3DFMT_Q16W16V16U16			= 110,

		D3DFMT_MULTI2_ARGB8			= FourCC('M','E','T','1'),

		D3DFMT_R16F					= 111,
		D3DFMT_G16R16F				= 112,
		D3DFMT_A16B16G16R16F		= 113,

		D3DFMT_R32F					= 114,
		D3DFMT_G32R32F				= 115,
		D3DFMT_A32B32G32R32F		= 116,

		D3DFMT_CxV8U8				= 117,
		D3DFMT_DX10					= FourCC('D', 'X', '1', '0'),

		D3DFMT_FORCE_DWORD			= 0x7fffffff
	};


	// More modern DX formats. Made unsigned due to the force-uint (last member).
	enum DXGIFMT : uint32 
	{
		DXGIFMT_UNKNOWN,					// 0
		DXGIFMT_R32G32B32A32_TYPELESS,
		DXGIFMT_R32G32B32A32_FLOAT,
		DXGIFMT_R32G32B32A32_UINT,
		DXGIFMT_R32G32B32A32_SINT,
		DXGIFMT_R32G32B32_TYPELESS,
		DXGIFMT_R32G32B32_FLOAT,
		DXGIFMT_R32G32B32_UINT,
		DXGIFMT_R32G32B32_SINT,
		DXGIFMT_R16G16B16A16_TYPELESS,
		DXGIFMT_R16G16B16A16_FLOAT,			// 10
		DXGIFMT_R16G16B16A16_UNORM,
		DXGIFMT_R16G16B16A16_UINT,
		DXGIFMT_R16G16B16A16_SNORM,
		DXGIFMT_R16G16B16A16_SINT,
		DXGIFMT_R32G32_TYPELESS,
		DXGIFMT_R32G32_FLOAT,
		DXGIFMT_R32G32_UINT,
		DXGIFMT_R32G32_SINT,
		DXGIFMT_R32G8X24_TYPELESS,
		DXGIFMT_D32_FLOAT_S8X24_UINT,		// 20
		DXGIFMT_R32_FLOAT_X8X24_TYPELESS,
		DXGIFMT_X32_TYPELESS_G8X24_UINT,
		DXGIFMT_R10G10B10A2_TYPELESS,
		DXGIFMT_R10G10B10A2_UNORM,
		DXGIFMT_R10G10B10A2_UINT,
		DXGIFMT_R11G11B10_FLOAT,
		DXGIFMT_R8G8B8A8_TYPELESS,
		DXGIFMT_R8G8B8A8_UNORM,
		DXGIFMT_R8G8B8A8_UNORM_SRGB,
		DXGIFMT_R8G8B8A8_UINT,				// 30
		DXGIFMT_R8G8B8A8_SNORM,
		DXGIFMT_R8G8B8A8_SINT,
		DXGIFMT_R16G16_TYPELESS,
		DXGIFMT_R16G16_FLOAT,
		DXGIFMT_R16G16_UNORM,
		DXGIFMT_R16G16_UINT,
		DXGIFMT_R16G16_SNORM,
		DXGIFMT_R16G16_SINT,
		DXGIFMT_R32_TYPELESS,
		DXGIFMT_D32_FLOAT,					// 40
		DXGIFMT_R32_FLOAT,
		DXGIFMT_R32_UINT,
		DXGIFMT_R32_SINT,
		DXGIFMT_R24G8_TYPELESS,
		DXGIFMT_D24_UNORM_S8_UINT,
		DXGIFMT_R24_UNORM_X8_TYPELESS,
		DXGIFMT_X24_TYPELESS_G8_UINT,
		DXGIFMT_R8G8_TYPELESS,
		DXGIFMT_R8G8_UNORM,
		DXGIFMT_R8G8_UINT,					// 50
		DXGIFMT_R8G8_SNORM,
		DXGIFMT_R8G8_SINT,
		DXGIFMT_R16_TYPELESS,
		DXGIFMT_R16_FLOAT,
		DXGIFMT_D16_UNORM,
		DXGIFMT_R16_UNORM,
		DXGIFMT_R16_UINT,
		DXGIFMT_R16_SNORM,
		DXGIFMT_R16_SINT,
		DXGIFMT_R8_TYPELESS,				// 60
		DXGIFMT_R8_UNORM,
		DXGIFMT_R8_UINT,
		DXGIFMT_R8_SNORM,
		DXGIFMT_R8_SINT,
		DXGIFMT_A8_UNORM,
		DXGIFMT_R1_UNORM,
		DXGIFMT_R9G9B9E5_SHAREDEXP,
		DXGIFMT_R8G8_B8G8_UNORM,
		DXGIFMT_G8R8_G8B8_UNORM,
		DXGIFMT_BC1_TYPELESS,				// 70
		DXGIFMT_BC1_UNORM,
		DXGIFMT_BC1_UNORM_SRGB,
		DXGIFMT_BC2_TYPELESS,
		DXGIFMT_BC2_UNORM,
		DXGIFMT_BC2_UNORM_SRGB,
		DXGIFMT_BC3_TYPELESS,
		DXGIFMT_BC3_UNORM,
		DXGIFMT_BC3_UNORM_SRGB,
		DXGIFMT_BC4_TYPELESS,
		DXGIFMT_BC4_UNORM,					// 80
		DXGIFMT_BC4_SNORM,
		DXGIFMT_BC5_TYPELESS,
		DXGIFMT_BC5_UNORM,
		DXGIFMT_BC5_SNORM,
		DXGIFMT_B5G6R5_UNORM,
		DXGIFMT_B5G5R5A1_UNORM,
		DXGIFMT_B8G8R8A8_UNORM,
		DXGIFMT_B8G8R8X8_UNORM,
		DXGIFMT_R10G10B10_XR_BIAS_A2_UNORM,
		DXGIFMT_B8G8R8A8_TYPELESS,			// 90
		DXGIFMT_B8G8R8A8_UNORM_SRGB,
		DXGIFMT_B8G8R8X8_TYPELESS,
		DXGIFMT_B8G8R8X8_UNORM_SRGB,
		DXGIFMT_BC6H_TYPELESS,
		DXGIFMT_BC6H_UF16,
		DXGIFMT_BC6H_SF16,
		DXGIFMT_BC7_TYPELESS,
		DXGIFMT_BC7_UNORM,
		DXGIFMT_BC7_UNORM_SRGB,
		DXGIFMT_AYUV,						// 100
		DXGIFMT_Y410,
		DXGIFMT_Y416,
		DXGIFMT_NV12,
		DXGIFMT_P010,
		DXGIFMT_P016,
		DXGIFMT_420_OPAQUE,
		DXGIFMT_YUY2,
		DXGIFMT_Y210,
		DXGIFMT_Y216,
		DXGIFMT_NV11,						// 110
		DXGIFMT_AI44,
		DXGIFMT_IA44,
		DXGIFMT_P8,
		DXGIFMT_A8P8,
		DXGIFMT_B4G4R4A4_UNORM,				// 115

		DXGIFMT_P208						= 130,
		DXGIFMT_V208,
		DXGIFMT_V408,

		// These ASTC formats are extended DXGI formats not (yet?) officially supported by microsoft.
		// The NVTT exporter uses the UNORM versions of these (dx10 header only) when exporting ASTC dds files.
		DXGIFMT_EXT_ASTC_4X4_TYPELESS		= 133,
		DXGIFMT_EXT_ASTC_4X4_UNORM,
		DXGIFMT_EXT_ASTC_4X4_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_5X4_TYPELESS		= 137,
		DXGIFMT_EXT_ASTC_5X4_UNORM,
		DXGIFMT_EXT_ASTC_5X4_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_5X5_TYPELESS		= 141,
		DXGIFMT_EXT_ASTC_5X5_UNORM,
		DXGIFMT_EXT_ASTC_5X5_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_6X5_TYPELESS		= 145,
		DXGIFMT_EXT_ASTC_6X5_UNORM,
		DXGIFMT_EXT_ASTC_6X5_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_6X6_TYPELESS		= 149,
		DXGIFMT_EXT_ASTC_6X6_UNORM,
		DXGIFMT_EXT_ASTC_6X6_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_8X5_TYPELESS		= 153,
		DXGIFMT_EXT_ASTC_8X5_UNORM,
		DXGIFMT_EXT_ASTC_8X5_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_8X6_TYPELESS		= 157,
		DXGIFMT_EXT_ASTC_8X6_UNORM,
		DXGIFMT_EXT_ASTC_8X6_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_8X8_TYPELESS		= 161,
		DXGIFMT_EXT_ASTC_8X8_UNORM,
		DXGIFMT_EXT_ASTC_8X8_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_10X5_TYPELESS		= 165,
		DXGIFMT_EXT_ASTC_10X5_UNORM,
		DXGIFMT_EXT_ASTC_10X5_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_10X6_TYPELESS		= 169,
		DXGIFMT_EXT_ASTC_10X6_UNORM,
		DXGIFMT_EXT_ASTC_10X6_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_10X8_TYPELESS		= 173,
		DXGIFMT_EXT_ASTC_10X8_UNORM,
		DXGIFMT_EXT_ASTC_10X8_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_10X10_TYPELESS		= 177,
		DXGIFMT_EXT_ASTC_10X10_UNORM,
		DXGIFMT_EXT_ASTC_10X10_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_12X10_TYPELESS		= 181,
		DXGIFMT_EXT_ASTC_12X10_UNORM,
		DXGIFMT_EXT_ASTC_12X10_UNORM_SRGB,

		DXGIFMT_EXT_ASTC_12X12_TYPELESS		= 185,
		DXGIFMT_EXT_ASTC_12X12_UNORM,
		DXGIFMT_EXT_ASTC_12X12_UNORM_SRGB,

		DXGIFMT_FORCE_UINT					= 0xffffffff
	};

	enum D3D10_DIMENSION
	{
		D3D10_DIMENSION_UNKNOWN				= 0,
		D3D10_DIMENSION_BUFFER				= 1,
		D3D10_DIMENSION_TEXTURE1D			= 2,
		D3D10_DIMENSION_TEXTURE2D			= 3,
		D3D10_DIMENSION_TEXTURE3D			= 4
	};

	enum D3D11_MISCFLAG : uint32
	{
		D3D11_MISCFLAG_GENERATE_MIPS					= 0x00000001,
		D3D11_MISCFLAG_SHARED							= 0x00000002,
		D3D11_MISCFLAG_TEXTURECUBE						= 0x00000004,
		D3D11_MISCFLAG_DRAWINDIRECT_ARGS				= 0x00000010,
		D3D11_MISCFLAG_BUFFER_ALLOW_RAW_VIEWS			= 0x00000020,
		D3D11_MISCFLAG_BUFFER_STRUCTURED				= 0x00000040,
		D3D11_MISCFLAG_RESOURCE_CLAMP					= 0x00000080,
		D3D11_MISCFLAG_SHARED_KEYEDMUTEX				= 0x00000100,
		D3D11_MISCFLAG_GDI_COMPATIBLE					= 0x00000200,
		D3D11_MISCFLAG_SHARED_NTHANDLE					= 0x00000800,
		D3D11_MISCFLAG_RESTRICTED_CONTENT				= 0x00001000,
		D3D11_MISCFLAG_RESTRICT_SHARED_RESOURCE			= 0x00002000,
		D3D11_MISCFLAG_RESTRICT_SHARED_RESOURCE_DRIVER	= 0x00004000,
		D3D11_MISCFLAG_GUARDED							= 0x00008000,
		D3D11_MISCFLAG_TILE_POOL						= 0x00020000,
		D3D11_MISCFLAG_TILED							= 0x00040000,
		D3D11_MISCFLAG_HW_PROTECTED						= 0x00080000,

		// These are TBD. Don't use.
		D3D11_MISCFLAG_SHARED_DISPLAYABLE,
		D3D11_MISCFLAG_SHARED_EXCLUSIVE_WRITER
	};

	#pragma pack(push, 4)
	struct DX10Header
	{
		DXGIFMT DxgiFormat;
		D3D10_DIMENSION Dimension;
		uint32 MiscFlag;
		uint32 ArraySize;
		uint32 Reserved;
	};
	#pragma pack(pop)

	// These figure out the pixel format, the colour-space, and the alpha mode. tPixelFormat does not specify ancilllary
	// properties of the data -- it specified the encoding of the data. The extra information, like the colour-space it
	// was authored in, is stored in tColourSpace and tAlphaMode. In many cases this satellite information cannot be
	// determined, in which case colour-space and/or alpha-mode will be set to their 'unspecified' enumerant.
	void GetFormatInfo_FromDXGIFormat		(tPixelFormat&, tColourSpace&, tAlphaMode&, uint32 dxgiFormat);
	void GetFormatInfo_FromFourCC			(tPixelFormat&, tColourSpace&, tAlphaMode&, uint32 fourCC);
	void GetFormatInfo_FromComponentMasks	(tPixelFormat&, tColourSpace&, tAlphaMode&, const FormatData&);

	void ProcessHDRFlags(tColour4f& colour, tcomps channels, const tImageDDS::LoadParams& params);
}


void tDDS::GetFormatInfo_FromDXGIFormat(tPixelFormat& format, tColourSpace& space, tAlphaMode& alpha, uint32 dxgiFormat)
{
	format = tPixelFormat::Invalid;
	alpha = tAlphaMode::Unspecified;

	// For colour space (the space of the data) we try to make an educated guess. In general only the asset author knows the
	// colour space. For most (non-HDR) pixel formats for colours, we assume the data is sRGB. If the pixel format has a specific
	// sRGB alternative, we _should_ assume if it's not the alternative, that the space is linear -- however many dds files in
	// the wild set them as UNORM rather than UNORM_SRGB. NVTT for example uses  use the UNORM (non sRGB) format for all ASTC
	// compressed textures, when it probably should have gone with the sRGB variant (i.e. They 'usually' encode colours).
	// Floating-point formats are assumed to be in linear-space (and are usually used for HDR images). In addition when the data
	// is probably not colour data (like ATI1/2) we assume it's in linear.
	space = tColourSpace::sRGB;

	switch (dxgiFormat)
	{
		//
		// BC Formats.
		//
		case tDDS::DXGIFMT_BC1_TYPELESS:
		case tDDS::DXGIFMT_BC1_UNORM:
		case tDDS::DXGIFMT_BC1_UNORM_SRGB:
			format = tPixelFormat::BC1DXT1;
			break;

		// DXGI formats do not specify premultiplied alpha mode like DXT2/3 so we leave it unspecified.
		case tDDS::DXGIFMT_BC2_TYPELESS:
		case tDDS::DXGIFMT_BC2_UNORM:
		case tDDS::DXGIFMT_BC2_UNORM_SRGB:
			format = tPixelFormat::BC2DXT2DXT3;
			break;

		// DXGI formats do not specify premultiplied alpha mode like DXT4/5 so we leave it unspecified. As for sRGB,
		// if it says UNORM_SRGB, sure, it may not contain sRGB data, but it's as good as you can get in terms of knowing.
		// I mean if the DirectX loader 'treats' it as being sRGB (in that it will convert it to linear), then we should
		// treat it as being sRGB data in general. Of course it could just be a recipe for apple pie, and if it is, it is
		// a recipe the authors wanted interpreted as sRGB data, otherwise they wouldn't have chosen the _SRGB pixel format.
		case tDDS::DXGIFMT_BC3_TYPELESS:
		case tDDS::DXGIFMT_BC3_UNORM:
		case tDDS::DXGIFMT_BC3_UNORM_SRGB:
			format = tPixelFormat::BC3DXT4DXT5;
			break;

		// case DXGIFMT_BC4_SNORM:
		case tDDS::DXGIFMT_BC4_TYPELESS:
		case tDDS::DXGIFMT_BC4_UNORM:
			space = tColourSpace::Linear;
			format = tPixelFormat::BC4ATI1;
			break;

		// case DXGIFMT_BC5_SNORM:
		case tDDS::DXGIFMT_BC5_TYPELESS:
		case tDDS::DXGIFMT_BC5_UNORM:
			space = tColourSpace::Linear;
			format = tPixelFormat::BC5ATI2;
			break;

		case tDDS::DXGIFMT_BC6H_TYPELESS:			// Interpret typeless as BC6H_S16... we gotta choose something.
		case tDDS::DXGIFMT_BC6H_SF16:
			space = tColourSpace::Linear;
			format = tPixelFormat::BC6S;
			break;

		case tDDS::DXGIFMT_BC6H_UF16:
			space = tColourSpace::Linear;
			format = tPixelFormat::BC6U;
			break;

		case tDDS::DXGIFMT_BC7_TYPELESS:			// Interpret typeless as BC7_UNORM... we gotta choose something.
		case tDDS::DXGIFMT_BC7_UNORM:
		case tDDS::DXGIFMT_BC7_UNORM_SRGB:
			format = tPixelFormat::BC7;
			break;

		//
		// Packed Formats.
		//
		case tDDS::DXGIFMT_A8_UNORM:
			space = tColourSpace::Linear;
			format = tPixelFormat::A8;
			break;

		case tDDS::DXGIFMT_R8_UNORM:
		case tDDS::DXGIFMT_R8_UINT:
		case tDDS::DXGIFMT_R8_TYPELESS:
		// DXGIFMT_R8_SNORM not implemented yet.
		// DXGIFMT_R8_SINT  not implemented yet.
		// It 'probably' makes more sense to consider this format as sRGB even though it's only one channel.
			format = tPixelFormat::R8;
			break;

		case tDDS::DXGIFMT_R8G8_UNORM:
		case tDDS::DXGIFMT_R8G8_UINT:
		case tDDS::DXGIFMT_R8G8_TYPELESS:
		// DXGIFMT_R8G8_SNORM not implemented yet.
		// DXGIFMT_R8G8_SINT  not implemented yet.
		// It 'probably' makes more sense to consider this format as sRGB even though it's only one channel.
			format = tPixelFormat::R8G8;
			break;

		case tDDS::DXGIFMT_R8G8B8A8_UNORM_SRGB:
		case tDDS::DXGIFMT_R8G8B8A8_UNORM:
		case tDDS::DXGIFMT_R8G8B8A8_UINT:
		case tDDS::DXGIFMT_R8G8B8A8_TYPELESS:
		// DXGIFMT_R8G8B8A8_SNORM not implemented yet.
		// DXGIFMT_R8G8B8A8_SINT  not implemented yet.
			format = tPixelFormat::R8G8B8A8;
			break;

		case tDDS::DXGIFMT_B8G8R8A8_UNORM_SRGB:
		case tDDS::DXGIFMT_B8G8R8A8_UNORM:
		case tDDS::DXGIFMT_B8G8R8A8_TYPELESS:
			format = tPixelFormat::B8G8R8A8;
			break;

		case tDDS::DXGIFMT_B5G6R5_UNORM:
			format = tPixelFormat::B5G6R5;
			break;

		case tDDS::DXGIFMT_B4G4R4A4_UNORM:
			format = tPixelFormat::B4G4R4A4;
			break;

		case tDDS::DXGIFMT_B5G5R5A1_UNORM:
			format = tPixelFormat::B5G5R5A1;
			break;

		case tDDS::DXGIFMT_R16_FLOAT:
			space = tColourSpace::Linear;
			format = tPixelFormat::R16F;
			break;

		case tDDS::DXGIFMT_R16G16_FLOAT:
			space = tColourSpace::Linear;
			format = tPixelFormat::R16G16F;
			break;

		case tDDS::DXGIFMT_R16G16B16A16_FLOAT:
			space = tColourSpace::Linear;
			format = tPixelFormat::R16G16B16A16F;
			break;

		case tDDS::DXGIFMT_R32_FLOAT:
			space = tColourSpace::Linear;
			format = tPixelFormat::R32F;
			break;

		case tDDS::DXGIFMT_R32G32_FLOAT:
			space = tColourSpace::Linear;
			format = tPixelFormat::R32G32F;
			break;

		case tDDS::DXGIFMT_R32G32B32A32_FLOAT:
			space = tColourSpace::Linear;
			format = tPixelFormat::R32G32B32A32F;
			break;

		//
		// ASTC Formats.
		//
		// For these we're assuming they are in sRGB-space even if the format is NOT the sRGB one.
		// This is because I know some popular exporters (NVTT) always use the UNORM (no sRGB) enumerant.
		case tDDS::DXGIFMT_EXT_ASTC_4X4_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_4X4_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_4X4_UNORM_SRGB:
			format = tPixelFormat::ASTC4X4;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_5X4_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_5X4_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_5X4_UNORM_SRGB:
			format = tPixelFormat::ASTC5X4;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_5X5_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_5X5_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_5X5_UNORM_SRGB:
			format = tPixelFormat::ASTC5X5;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_6X5_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_6X5_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_6X5_UNORM_SRGB:
			format = tPixelFormat::ASTC6X5;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_6X6_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_6X6_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_6X6_UNORM_SRGB:
			format = tPixelFormat::ASTC6X6;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_8X5_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_8X5_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_8X5_UNORM_SRGB:
			format = tPixelFormat::ASTC8X5;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_8X6_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_8X6_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_8X6_UNORM_SRGB:
			format = tPixelFormat::ASTC8X6;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_8X8_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_8X8_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_8X8_UNORM_SRGB:
			format = tPixelFormat::ASTC8X8;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_10X5_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_10X5_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_10X5_UNORM_SRGB:
			format = tPixelFormat::ASTC10X5;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_10X6_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_10X6_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_10X6_UNORM_SRGB:
			format = tPixelFormat::ASTC10X6;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_10X8_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_10X8_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_10X8_UNORM_SRGB:
			format = tPixelFormat::ASTC10X8;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_10X10_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_10X10_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_10X10_UNORM_SRGB:
			format = tPixelFormat::ASTC10X10;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_12X10_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_12X10_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_12X10_UNORM_SRGB:
			format = tPixelFormat::ASTC12X10;
			break;

		case tDDS::DXGIFMT_EXT_ASTC_12X12_TYPELESS:
		case tDDS::DXGIFMT_EXT_ASTC_12X12_UNORM:
		case tDDS::DXGIFMT_EXT_ASTC_12X12_UNORM_SRGB:
			format = tPixelFormat::ASTC12X12;
			break;

		default:
			space = tColourSpace::Unspecified;
			break;
	}
}


void tDDS::GetFormatInfo_FromFourCC(tPixelFormat& format, tColourSpace& space, tAlphaMode& alpha, uint32 fourCC)
{
	format = tPixelFormat::Invalid;
	space = tColourSpace::sRGB;
	alpha = tAlphaMode::Unspecified;

	switch (fourCC)
	{
		// Note that during inspecition of the individual layer data, the DXT1 pixel format might be modified
		// to DXT1BA (binary alpha).
		case tDDS::D3DFMT_DXT1:
			format = tPixelFormat::BC1DXT1;
			break;

		// DXT2 and DXT3 are the same format. Only how you interpret the data is different. In tacent we treat them
		// as the same pixel-format. How contents are interpreted (the data) is not part of the format. 
		case tDDS::D3DFMT_DXT2:
			alpha = tAlphaMode::Premultiplied;
			format = tPixelFormat::BC2DXT2DXT3;
			break;

		case tDDS::D3DFMT_DXT3:
			alpha = tAlphaMode::Normal;
			format = tPixelFormat::BC2DXT2DXT3;
			break;

		case tDDS::D3DFMT_DXT4:
			alpha = tAlphaMode::Premultiplied;
			format = tPixelFormat::BC3DXT4DXT5;
			break;

		case tDDS::D3DFMT_DXT5:
			alpha = tAlphaMode::Normal;
			format = tPixelFormat::BC3DXT4DXT5;
			break;

		case tDDS::D3DFMT_ATI1:
		case tDDS::D3DFMT_BC4U:
			space = tColourSpace::Linear;
			format = tPixelFormat::BC4ATI1;
			break;

		case tDDS::D3DFMT_ATI2:
		case tDDS::D3DFMT_BC5U:
			space = tColourSpace::Linear;
			format = tPixelFormat::BC5ATI2;
			break;

		case tDDS::D3DFMT_BC4S:				// We don't support signed BC4S.
		case tDDS::D3DFMT_BC5S:				// We don't support signed BC5S.
		case tDDS::D3DFMT_R8G8_B8G8:		// We don't support DXGIFMT_R8G8_B8G8_UNORM -- That's a lot of green precision.
		case tDDS::D3DFMT_G8R8_G8B8:		// We don't support DXGIFMT_G8R8_G8B8_UNORM -- That's a lot of green precision.
			break;

		case tDDS::D3DFMT_ETC:
		case tDDS::D3DFMT_ETC1:
			format = tPixelFormat::ETC1;
			tPrintf("Detected ETC1\n");
			break;

		case tDDS::D3DFMT_ETC2:
			format = tPixelFormat::ETC2RGB;
			tPrintf("Detected ETC2RGB\n");
			break;

		case tDDS::D3DFMT_ETCA:
			format = tPixelFormat::ETC2RGBA;
			tPrintf("Detected ETC2RGBA\n");
			break;

		case tDDS::D3DFMT_ETCP:
			format = tPixelFormat::ETC2RGBA1;
			tPrintf("Detected ETC2RGBA1\n");
			break;

		// Sometimes these D3D formats may be stored in the FourCC slot.
		case tDDS::D3DFMT_A16B16G16R16:	// We don't support DXGIFMT_R16G16B16A16_UNORM.
		case tDDS::D3DFMT_Q16W16V16U16:	// We don't support DXGIFMT_R16G16B16A16_SNORM.
			break;
		
		case tDDS::D3DFMT_A8:
			space = tColourSpace::Linear;
			format = tPixelFormat::A8;
			break;

		case tDDS::D3DFMT_L8:
			format = tPixelFormat::L8;
			break;

		case tDDS::D3DFMT_A8B8G8R8:
			// D3DFMT format name has incorrect component order. DXGI_FORMAT is correct.
			// See https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-legacy-formats
			format = tPixelFormat::R8G8B8A8;
			break;

		case tDDS::D3DFMT_R16F:
			space = tColourSpace::Linear;
			format = tPixelFormat::R16F;
			break;

		case tDDS::D3DFMT_G16R16F:
			// D3DFMT format name has incorrect component order. DXGI_FORMAT is correct.
			// See https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-legacy-formats
			space = tColourSpace::Linear;
			format = tPixelFormat::R16G16F;
			break;

		case tDDS::D3DFMT_A16B16G16R16F:
			// D3DFMT format name has incorrect component order. DXGI_FORMAT is correct.
			// See https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-legacy-formats
			space = tColourSpace::Linear;
			format = tPixelFormat::R16G16B16A16F;
			break;

		case tDDS::D3DFMT_R32F:
			space = tColourSpace::Linear;
			format = tPixelFormat::R32F;
			break;

		case tDDS::D3DFMT_G32R32F:
			// D3DFMT format name has incorrect component order. DXGI_FORMAT is correct.
			// See https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-legacy-formats
			space = tColourSpace::Linear;
			format = tPixelFormat::R32G32F;
			break;

		case tDDS::D3DFMT_A32B32G32R32F:
			// It's inconsistent calling the D3D format A32B32G32R32F. The floats in this case are clearly in RGBA
			// order, not ABGR. Anyway, I only have control over the tPixelFormat names. In fairness, it looks like
			// the format-name was fixed in the DX10 header format type names.
			// See https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-legacy-formats
			space = tColourSpace::Linear;
			format = tPixelFormat::R32G32B32A32F;
			break;

		default:
			space = tColourSpace::Unspecified;
			break;
	}
}


void tDDS::GetFormatInfo_FromComponentMasks(tPixelFormat& format, tColourSpace& space, tAlphaMode& alpha, const FormatData& fmtData)
{
	format = tPixelFormat::Invalid;
	space = tColourSpace::sRGB;
	alpha = tAlphaMode::Unspecified;

	uint32 bitCount	= fmtData.RGBBitCount;
	bool anyRGB		= (fmtData.Flags &  tDDS::DDFormatFlag_RGB);
	bool isRGB		= (fmtData.Flags == tDDS::DDFormatFlags_RGB);
	bool isRGBA		= (fmtData.Flags == tDDS::DDFormatFlags_RGBA);
	bool isA		= (fmtData.Flags == tDDS::DDFormatFlags_A);
	bool isL		= (fmtData.Flags == tDDS::DDFormatFlags_L);
	uint32 mskR		= fmtData.MaskRed;
	uint32 mskG		= fmtData.MaskGreen;
	uint32 mskB		= fmtData.MaskBlue;
	uint32 mskA		= fmtData.MaskAlpha;

	// Remember this is a little endian machine, so the masks are lying. Eg. 0x00FF0000 in memory is 00 00 FF 00 cuz it's encoded
	// as an int32 -- so the red comes after blue and green.
	switch (bitCount)
	{
		case 8:			// Supports A8, L8.
			if ((isA || anyRGB) && (mskA == 0xFF))
				format = tPixelFormat::A8;
			else if ((isL || anyRGB) && (mskR == 0xFF))
				format = tPixelFormat::L8;
			break;

		case 16:		// Supports B5G6R5, B4G4R4A4, and B5G5R5A1.
			if (isRGBA && (mskA == 0x8000) && (mskR == 0x7C00) && (mskG == 0x03E0) && (mskB == 0x001F))
				format = tPixelFormat::B5G5R5A1;
			else if (isRGBA && (mskA == 0xF000) && (mskR == 0x0F00) && (mskG == 0x00F0) && (mskB == 0x000F))
				format = tPixelFormat::B4G4R4A4;
			else if (isRGB && (mskR == 0xF800) && (mskG == 0x07E0) && (mskB == 0x001F))
				format = tPixelFormat::B5G6R5;
			break;

		case 24:		// Supports B8G8R8.
			if (isRGB && (mskR == 0xFF0000) && (mskG == 0x00FF00) && (mskB == 0x0000FF))
				format = tPixelFormat::B8G8R8;
			break;

		case 32:		// Supports B8G8R8A8. Alpha really is last (even though ABGR may seem more consistent).
			if (isRGBA && (mskA == 0xFF000000) && (mskR == 0x00FF0000) && (mskG == 0x0000FF00) && (mskB == 0x000000FF))
				format = tPixelFormat::B8G8R8A8;
			break;
	}

	switch (format)
	{
		case tPixelFormat::A8:
			space = tColourSpace::Linear;
			break;

		case tPixelFormat::Invalid:
			space = tColourSpace::Unspecified;
			break;
	}
}


void tDDS::ProcessHDRFlags(tColour4f& colour, tcomps channels, const tImageDDS::LoadParams& params)
{
	if (params.Flags & tImageDDS::LoadFlag_ToneMapExposure)
		colour.TonemapExposure(params.Exposure, channels);
	if (params.Flags & tImageDDS::LoadFlag_SRGBCompression)
		colour.LinearToSRGB(channels);
	if (params.Flags & tImageDDS::LoadFlag_GammaCompression)
		colour.LinearToGamma(params.Gamma, channels);
}


tImageDDS::tImageDDS()
{
	tStd::tMemset(Layers, 0, sizeof(Layers));
}


tImageDDS::tImageDDS(const tString& ddsFile, const LoadParams& loadParams) :
	Filename(ddsFile)
{
	tStd::tMemset(Layers, 0, sizeof(Layers));
	Load(ddsFile, loadParams);
}


tImageDDS::tImageDDS(const uint8* ddsFileInMemory, int numBytes, const LoadParams& loadParams)
{
	tStd::tMemset(Layers, 0, sizeof(Layers));
	Load(ddsFileInMemory, numBytes, loadParams);
}


void tImageDDS::Clear()
{
	for (int image = 0; image < NumImages; image++)
	{
		for (int layer = 0; layer < NumMipmapLayers; layer++)
		{
			delete Layers[layer][image];
			Layers[layer][image] = nullptr;
		}
	}

	Results							= 0;							// This means no results.
	PixelFormat						= tPixelFormat::Invalid;
	PixelFormatSrc					= tPixelFormat::Invalid;
	ColourSpace						= tColourSpace::Unspecified;
	ColourSpaceSrc					= tColourSpace::Unspecified;
	AlphaMode						= tAlphaMode::Unspecified;
	IsCubeMap						= false;
	IsModernDX10					= false;
	RowReversalOperationPerformed	= false;
	NumImages						= 0;
	NumMipmapLayers					= 0;
}


bool tImageDDS::Set(tPixel* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Layers[0][0]					= new tLayer(tPixelFormat::R8G8B8A8, width, height, (uint8*)pixels, steal);
	PixelFormat						= tPixelFormat::R8G8B8A8;
	PixelFormatSrc					= tPixelFormat::R8G8B8A8;
	ColourSpace						= tColourSpace::sRGB;
	ColourSpaceSrc					= tColourSpace::sRGB;
	AlphaMode						= tAlphaMode::Normal;
	IsCubeMap						= false;
	IsModernDX10					= false;
	RowReversalOperationPerformed	= false;
	NumImages						= 1;
	NumMipmapLayers					= 1;

	return true;
}


bool tImageDDS::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	tPixel* pixels = frame->GetPixels(steal);
	Set(pixels, frame->Width, frame->Height, steal);
	if (steal)
		delete frame;

	return true;
}


bool tImageDDS::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


tFrame* tImageDDS::GetFrame(bool steal)
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


bool tImageDDS::IsOpaque() const
{
	return tImage::tIsOpaqueFormat(PixelFormat);
}


bool tImageDDS::StealLayers(tList<tLayer>& layers)
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


bool tImageDDS::GetLayers(tList<tLayer>& layers) const
{
	if (!IsValid() || IsCubemap() || (NumImages <= 0))
		return false;

	for (int mip = 0; mip < NumMipmapLayers; mip++)
		layers.Append(Layers[mip][0]);

	return true;
}


int tImageDDS::StealCubemapLayers(tList<tLayer> layerLists[tSurfIndex_NumSurfaces], uint32 sideFlags)
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


int tImageDDS::GetCubemapLayers(tList<tLayer> layerLists[tSurfIndex_NumSurfaces], uint32 sideFlags) const
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


bool tImageDDS::Load(const tString& ddsFile, const LoadParams& loadParams)
{
	Clear();
	Filename = ddsFile;
	if (tSystem::tGetFileType(ddsFile) != tSystem::tFileType::DDS)
	{
		Results |= 1 << int(ResultCode::Fatal_IncorrectFileType);
		return false;
	}

	if (!tSystem::tFileExists(ddsFile))
	{
		Results |= 1 << int(ResultCode::Fatal_FileDoesNotExist);
		return false;
	}

	int ddsSizeBytes = 0;
	uint8* ddsData = (uint8*)tSystem::tLoadFile(ddsFile, 0, &ddsSizeBytes);
	bool success = Load(ddsData, ddsSizeBytes, loadParams);
	delete[] ddsData;

	return success;
}


bool tImageDDS::Load(const uint8* ddsData, int ddsSizeBytes, const LoadParams& paramsIn)
{
	Clear();
	LoadParams params(paramsIn);

	// This will deal with zero-sized files properly as well.
	if (ddsSizeBytes < int(sizeof(tDDS::Header)+4))
	{
		Results |= 1 << int(ResultCode::Fatal_IncorrectFileSize);
		return false;
	}

	const uint8* ddsCurr = ddsData;
	uint32& magic = *((uint32*)ddsCurr); ddsCurr += sizeof(uint32);
	if (magic != FourCC('D','D','S',' '))
	{
		Results |= 1 << int(ResultCode::Fatal_IncorrectMagicNumber);
		return false;
	}

	tDDS::Header& header = *((tDDS::Header*)ddsCurr);  ddsCurr += sizeof(header);
	tAssert(sizeof(tDDS::Header) == 124);
	const uint8* currPixelData = ddsCurr;
	if (header.Size != 124)
	{
		Results |= 1 << int(ResultCode::Fatal_IncorrectHeaderSize);
		return false;
	}

	uint32 flags = header.Flags;
	int mainWidth = header.Width;						// Main image.
	int mainHeight = header.Height;						// Main image.
	if ((mainWidth <= 0) || (mainHeight <= 0))
	{
		Results |= 1 << int(ResultCode::Fatal_InvalidDimensions);
		return false;
	}

	// A strictly correct dds will have linear-size xor pitch set (one, not both).
	// It seems ATI tools like GenCubeMap don't set the correct bits, but we still load
	// with a conditional success if strict flag unset.
	int pitch = 0;										// Num bytes per line on main image (uncompressed images only).
	int linearSize = 0;									// Num bytes total main image (compressed images only).
	if (flags & tDDS::HeaderFlag_Pitch)
		pitch = header.PitchLinearSize;
	if (flags & tDDS::HeaderFlag_LinearSize)
		linearSize = header.PitchLinearSize;

	if ((!linearSize && !pitch) || (linearSize && pitch))
	{
		if (params.Flags & LoadFlag_StrictLoading)
		{
			Results |= 1 << int(ResultCode::Fatal_PitchXORLinearSize);
			return false;
		}
		else
		{
			Results |= 1 << int(ResultCode::Conditional_PitchXORLinearSize);
		}
	}

	// Volume textures are not supported.
	if (flags & tDDS::HeaderFlag_Depth)
	{
		Results |= 1 << int(ResultCode::Fatal_VolumeTexturesNotSupported);
		return false;
	}

	// Determine the expected number of layers by looking at the mipmap count if it is supplied. We assume a single layer
	// if it is not specified.
	NumMipmapLayers = 1;
	bool hasMipmaps = (header.Capabilities.FlagsCapsBasic & tDDS::CapsBasicFlag_Mipmap) ? true : false;
	if ((flags & tDDS::HeaderFlag_MipmapCount) && hasMipmaps)
		NumMipmapLayers = header.MipmapCount;

	if (NumMipmapLayers > MaxMipmapLayers)
	{
		Results |= 1 << int(ResultCode::Fatal_MaxNumMipmapLevelsExceeded);
		return false;
	}

	tDDS::FormatData& format = header.Format;
	if (format.Size != 32)
	{
		Results |= 1 << int(ResultCode::Fatal_IncorrectPixelFormatHeaderSize);
		return false;
	}

	// Determine if we support the pixel format and which one it is.
	bool isRGB			= (format.Flags == tDDS::DDFormatFlags_RGB);
	bool isRGBA			= (format.Flags == tDDS::DDFormatFlags_RGBA);
	bool isA			= (format.Flags == tDDS::DDFormatFlags_A);
	bool isL			= (format.Flags == tDDS::DDFormatFlags_L);
	bool isFourCCFormat	= (format.Flags == tDDS::DDFormatFlags_FourCC);
	if (!isRGB && !isRGBA && !isA && !isL && !isFourCCFormat)
	{
		if (params.Flags & LoadFlag_StrictLoading)
		{
			Results |= 1 << int(ResultCode::Fatal_IncorrectPixelFormatSpec);
			return false;
		}
		else
		{
			// If the flags completely fail to specify a format, we try to use the FourCC.
			Results |= 1 << int(ResultCode::Conditional_IncorrectPixelFormatSpec);
			isFourCCFormat = true;
		}
	}

	IsModernDX10 = isFourCCFormat && (format.FourCC == FourCC('D', 'X', '1', '0'));

	// Determine if this is a cubemap dds with 6 images. No need to check which images are present since they are
	// required to be all there by the dds standard. All tools these days seem to write them all. If there are
	// complaints when using legacy files we can fix this. We use FlagsCapsExtra in all cases where we are not using
	// the DX10 extension header.
	IsCubeMap = false;
	NumImages = 1;
	if (!IsModernDX10 && (header.Capabilities.FlagsCapsExtra & tDDS::CapsExtraFlag_CubeMap))
	{
		IsCubeMap = true;
		NumImages = 6;
	}

	if (IsModernDX10)
	{
		tDDS::DX10Header& headerDX10 = *((tDDS::DX10Header*)ddsCurr);  ddsCurr += sizeof(tDDS::DX10Header);
		currPixelData = ddsCurr;
		if (headerDX10.ArraySize == 0)
		{
			Results |= 1 << int(ResultCode::Fatal_DX10HeaderSizeIncorrect);
			return false;
		}

		// We only handle 2D textures for now.
		if (headerDX10.Dimension != tDDS::D3D10_DIMENSION_TEXTURE2D)
		{
			Results |= 1 << int(ResultCode::Fatal_DX10DimensionNotSupported);
			return false;
		}

		if (headerDX10.MiscFlag & tDDS::D3D11_MISCFLAG_TEXTURECUBE)
		{
			IsCubeMap = true;
			NumImages = 6;
		}

		// If we found a dx10 chunk. It must be used to determine the pixel format and possibly any known colour-space info.
		tDDS::GetFormatInfo_FromDXGIFormat(PixelFormat, ColourSpace, AlphaMode, headerDX10.DxgiFormat);
	}
	else if (isFourCCFormat)
	{
		tDDS::GetFormatInfo_FromFourCC(PixelFormat, ColourSpace, AlphaMode, format.FourCC);
	}
	// It must be a simple uncompressed format.
	else
	{
		tDDS::GetFormatInfo_FromComponentMasks(PixelFormat, ColourSpace, AlphaMode, format);
	}
	PixelFormatSrc = PixelFormat;
	ColourSpaceSrc = ColourSpace;

	// From now on we should just be using the PixelFormat to decide what to do next.
	if (PixelFormat == tPixelFormat::Invalid)
	{
		Results |= 1 << int(ResultCode::Fatal_PixelFormatNotSupported);
		return false;
	}

	if (tIsBCFormat(PixelFormat))
	{
		if ((params.Flags & LoadFlag_CondMultFourDim) && ((mainWidth%4) || (mainHeight%4)))
			Results |= 1 << int(ResultCode::Conditional_DimNotMultFourBC);
		if ((params.Flags & LoadFlag_CondPowerTwoDim) && (!tMath::tIsPower2(mainWidth) || !tMath::tIsPower2(mainHeight)))
			Results |= 1 << int(ResultCode::Conditional_DimNotMultFourBC);
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

				// DDS files store textures upside down. In the OpenGL RH coord system, the lower left of the texture
				// is the origin and consecutive rows go up. For this reason we need to read each row of blocks from
				// the top to the bottom row. We also need to flip the rows within the 4x4 block by flipping the lookup
				// tables. This should be fairly fast as there is no encoding or decoding going on. Width and height
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
				// Upsupported pixel format.
				Clear();
				Results |= 1 << int(ResultCode::Fatal_PixelFormatNotSupported);
				return false;
			}

			currPixelData += numBytes;
			width  /= 2; tMath::tiClampMin(width, 1);
			height /= 2; tMath::tiClampMin(height, 1);
		}
	}

	if (doRowReversalBeforeDecode)
		RowReversalOperationPerformed = true;

	// Decode to 32-bit RGBA if requested. If we're already in the correct R8G8B8A8 format, no need to do anything.
	// Note, gamma-correct load flag only applies when decoding HDR/floating-point formats, so never any need to do
	// it on R8G8B8A8. Likewise for spread-flag, never applies to R8G8B8A8 (only R-only or L-only formats)..
	if ((params.Flags & LoadFlag_Decode) && (PixelFormat != tPixelFormat::R8G8B8A8))
	{
		// Spread only applies to single-channel (R-only or L-only) formats.
		bool spread = params.Flags & LoadFlag_SpreadLuminance;

		// The gamma-compression load flags only apply when decoding. If the gamma mode is auto, we determine here
		// whether to apply sRGB compression. If the space is linear and a format that often encodes colours, we apply it.
		if (params.Flags & LoadFlag_AutoGamma)
		{
			// Clear all related flags.
			params.Flags &= ~(LoadFlag_AutoGamma | LoadFlag_SRGBCompression | LoadFlag_GammaCompression);
			if (ColourSpace == tColourSpace::Linear)
			{
				// Just cuz it's linear doesn't mean we want to gamma transform. Some formats should be kept linear.
				if
				(
					(PixelFormat != tPixelFormat::A8) && (PixelFormat != tPixelFormat::A8L8) &&
					(PixelFormat != tPixelFormat::BC4ATI1) && (PixelFormat != tPixelFormat::BC5ATI2)
				)
				{
					params.Flags |= LoadFlag_SRGBCompression;
				}
			}
		}

		bool didRowReversalAfterDecode = false;
		bool processedHDRFlags = false;
		for (int image = 0; image < NumImages; image++)
		{
			for (int layerNum = 0; layerNum < NumMipmapLayers; layerNum++)
			{
				tLayer* layer = Layers[layerNum][image];
				int w = layer->Width;
				int h = layer->Height;
				uint8* src = layer->Data;

				if (tImage::tIsPackedFormat(PixelFormat))
				{
					tPixel* uncompData = new tPixel[w*h];
					switch (layer->PixelFormat)
					{
						case tPixelFormat::A8:
							// Convert to 32-bit RGBA with alpha in A and 0s for RGB.
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4i col(0u, 0u, 0u, src[ij]);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::L8:
						case tPixelFormat::R8:
						{
							// Convert to 32-bit RGBA with red or luminance in R and 255 for A. If SpreadLuminance flag set,
							// also set luminance or red in the GB channels, if not then GB get 0s.
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4i col(src[ij], spread ? src[ij] : 0u, spread ? src[ij] : 0u, 255u);
								uncompData[ij].Set(col);
							}
							break;
						}

						case tPixelFormat::R8G8:
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4i col(src[ij*2+0], src[ij*2+1], 0u, 255u);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::R8G8B8:
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4i col(src[ij*3+0], src[ij*3+1], src[ij*3+2], 255u);
								uncompData[ij].Set(col);
							}
							break;
						
						case tPixelFormat::R8G8B8A8:
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4i col(src[ij*4+0], src[ij*4+1], src[ij*4+2], src[ij*4+3]);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::B8G8R8:
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4i col(src[ij*3+2], src[ij*3+1], src[ij*3+0], 255u);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::B8G8R8A8:
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4i col(src[ij*4+2], src[ij*4+1], src[ij*4+0], src[ij*4+3]);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::B5G6R5:
							for (int ij = 0; ij < w*h; ij++)
							{
								// On an LE machine casting to a uint16 effectively swaps the bytes when doing bit ops.
								// This means red will be in the most significant bits -- that's why it looks backwards.
								uint16 u = *((uint16*)(src+ij*2));

								uint8 r = (u         ) >> 11;		// 1111 1000 0000 0000 >> 11.
								uint8 g = (u & 0x07E0) >> 5;		// 0000 0111 1110 0000 >> 5.
								uint8 b = (u & 0x001F)     ;		// 0000 0000 0001 1111 >> 0.

								// Normalize to range.
								// Careful here, you can't just do bit ops to get the components into position.
								// For example, a full red (11111) has to go to 255 (1.0f), and a zero red (00000) to 0(0.0f).
								// That is, the normalize has to divide by the range. At first I just masked and shifted the bits
								// to the right spot in an 8-bit type, but you don't know what to put in the LSBits. Putting 0s
								// would be bad (an 4 bit alpha of 1111 would go to 11110000... suddenly image not fully opaque)
								// and putting all 1s would add red (or alpha or whatever) when there was none. Half way won't
								// work either. You need the endpoints to work.
								float rf = (float(r) / 31.0f);		// Max is 2^5 - 1.
								float gf = (float(g) / 63.0f);		// Max is 2^6 - 1.
								float bf = (float(b) / 31.0f);		// Max is 2^5 - 1.
								tColour4i col(rf, gf, bf, 1.0f);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::B4G4R4A4:
							for (int ij = 0; ij < w*h; ij++)
							{
								uint16 u = *((uint16*)(src+ij*2));
								uint8 a = (u         ) >> 12;		// 1111 0000 0000 0000 >> 12.
								uint8 r = (u & 0x0F00) >> 8;		// 0000 1111 0000 0000 >> 8.
								uint8 g = (u & 0x00F0) >> 4;		// 0000 0000 1111 0000 >> 4.
								uint8 b = (u & 0x000F)     ;		// 0000 0000 0000 1111 >> 0.

								// Normalize to range.
								float af = float(a) / 15.0f;		// Max is 2^4 - 1.
								float rf = float(r) / 15.0f;
								float gf = float(g) / 15.0f;
								float bf = float(b) / 15.0f;

								tColour4i col(rf, gf, bf, af);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::B5G5R5A1:
							for (int ij = 0; ij < w*h; ij++)
							{
								uint16 u = *((uint16*)(src+ij*2));
								bool  a = (u & 0x8000);				// 1000 0000 0000 0000.
								uint8 r = (u & 0x7C00) >> 10;		// 0111 1100 0000 0000 >> 10.
								uint8 g = (u & 0x03E0) >> 5;		// 0000 0011 1110 0000 >> 5.
								uint8 b = (u & 0x001F)     ;		// 0000 0000 0001 1111 >> 0.

								// Normalize to range.
								float rf = float(r) / 31.0f;		// Max is 2^5 - 1.
								float gf = float(g) / 31.0f;
								float bf = float(b) / 31.0f;

								tColour4i col(rf, gf, bf, a ? 1.0f : 0.0f);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::R16F:
						{
							// This HDR format has 1 red half-float channel.
							tHalf* hdata = (tHalf*)src;
							for (int ij = 0; ij < w*h; ij++)
							{
								float r = hdata[ij*1 + 0];
								tColour4f col(r, spread ? r : 0.0f, spread ? r : 0.0f, 1.0f);
								tDDS::ProcessHDRFlags(col, spread ? tComp_RGB : tComp_R, params);
								uncompData[ij].Set(col);
							}
							processedHDRFlags = true;
							break;
						}

						case tPixelFormat::R16G16F:
						{
							// This HDR format has 2 half-float channels. Red and green.
							tHalf* hdata = (tHalf*)src;
							for (int ij = 0; ij < w*h; ij++)
							{
								float r = hdata[ij*2 + 0];
								float g = hdata[ij*2 + 1];
								tColour4f col(r, g, 0.0f, 1.0f);
								tDDS::ProcessHDRFlags(col, tComp_RG, params);
								uncompData[ij].Set(col);
							}
							processedHDRFlags = true;
							break;
						}

						case tPixelFormat::R16G16B16A16F:
						{
							// This HDR format has 4 half-float channels. RGBA.
							tHalf* hdata = (tHalf*)src;
							for (int ij = 0; ij < w*h; ij++)
							{
								float r = hdata[ij*4 + 0];
								float g = hdata[ij*4 + 1];
								float b = hdata[ij*4 + 2];
								float a = hdata[ij*4 + 3];
								tColour4f col(r, g, b, a);
								tDDS::ProcessHDRFlags(col, tComp_RGB, params);
								uncompData[ij].Set(col);
							}
							processedHDRFlags = true;
							break;
						}

						case tPixelFormat::R32F:
						{
							// This HDR format has 1 red float channel.
							float* fdata = (float*)src;
							for (int ij = 0; ij < w*h; ij++)
							{
								float r = fdata[ij*1 + 0];
								tColour4f col(r, spread ? r : 0.0f, spread ? r : 0.0f, 1.0f);
								tDDS::ProcessHDRFlags(col, spread ? tComp_RGB : tComp_R, params);
								uncompData[ij].Set(col);
							}
							processedHDRFlags = true;
							break;
						}

						case tPixelFormat::R32G32F:
						{
							// This HDR format has 2 float channels. Red and green.
							float* fdata = (float*)src;
							for (int ij = 0; ij < w*h; ij++)
							{
								float r = fdata[ij*2 + 0];
								float g = fdata[ij*2 + 1];
								tColour4f col(r, g, 0.0f, 1.0f);
								tDDS::ProcessHDRFlags(col, tComp_RG, params);
								uncompData[ij].Set(col);
							}
							processedHDRFlags = true;
							break;
						}

						case tPixelFormat::R32G32B32A32F:
						{
							// This HDR format has 4 RGBA floats.
							float* fdata = (float*)src;
							for (int ij = 0; ij < w*h; ij++)
							{
								float r = fdata[ij*4 + 0];
								float g = fdata[ij*4 + 1];
								float b = fdata[ij*4 + 2];
								float a = fdata[ij*4 + 3];
								tColour4f col(r, g, b, a);
								tDDS::ProcessHDRFlags(col, tComp_RGB, params);
								uncompData[ij].Set(col);
							}
							processedHDRFlags = true;
							break;
						}

						default:
							delete[] uncompData;
							Clear();
							Results |= 1 << int(ResultCode::Fatal_PackedDecodeError);
							return false;
					}

					// Decode worked. We are now in RGBA 32-bit. Other params like width and height are already correct.
					delete[] layer->Data;
					layer->Data = (uint8*)uncompData;

					// We are now in in RGBA. In that order in memory.
					layer->PixelFormat = tPixelFormat::R8G8B8A8;
				}
				else if (tImage::tIsBCFormat(PixelFormat))
				{
					// We need extra room because the decompressor (bcdec) does not take an input for
					// the width and height, only the pitch (bytes per row). This means a texture that is 5
					// high will actually have row 6, 7, 8 written to.
					int wfull = 4 * tGetNumBlocks(4, w);
					int hfull = 4 * tGetNumBlocks(4, h);
					tPixel* uncompData = new tPixel[wfull*hfull];
					switch (layer->PixelFormat)
					{
						case tPixelFormat::BC1DXT1:
						case tPixelFormat::BC1DXT1A:
						{
							for (int y = 0; y < hfull; y += 4)
								for (int x = 0; x < wfull; x += 4)
								{
									uint8* dst = (uint8*)uncompData + (y*wfull + x) * 4;

									// At first didn't understand the pitch (3rd) argument. It's cuz the block needs to be written into
									// multiple rows of the destination and we need to know how far to increment to the next row of 4.
									bcdec_bc1(src, dst, wfull * 4);
									src += BCDEC_BC1_BLOCK_SIZE;
								}

							break;
						}

						case tPixelFormat::BC2DXT2DXT3:
						{
							for (int y = 0; y < hfull; y += 4)
								for (int x = 0; x < wfull; x += 4)
								{
									uint8* dst = (uint8*)uncompData + (y*wfull + x) * 4;
									bcdec_bc2(src, dst, wfull * 4);
									src += BCDEC_BC2_BLOCK_SIZE;
								}
							break;
						}

						case tPixelFormat::BC3DXT4DXT5:
						{
							for (int y = 0; y < hfull; y += 4)
								for (int x = 0; x < wfull; x += 4)
								{
									uint8* dst = (uint8*)uncompData + (y*wfull + x) * 4;
									bcdec_bc3(src, dst, wfull * 4);
									src += BCDEC_BC3_BLOCK_SIZE;
								}
							break;
						}

						case tPixelFormat::BC4ATI1:
						{
							// This HDR format decompresses to R uint8s.
							uint8* rdata = new uint8[wfull*hfull];

							for (int y = 0; y < hfull; y += 4)
								for (int x = 0; x < wfull; x += 4)
								{
									uint8* dst = (rdata + (y*wfull + x) * 1);
									bcdec_bc4(src, dst, wfull * 1);
									src += BCDEC_BC4_BLOCK_SIZE;
								}

							// Now convert to 32-bit RGBA.
							for (int xy = 0; xy < wfull*hfull; xy++)
							{
								uint8 v = rdata[xy];
								tColour4i col(v, spread ? v : 0u, spread ? v : 0u, 255u);
								uncompData[xy].Set(col);
							}
							delete[] rdata;
							break;
						}

						case tPixelFormat::BC5ATI2:
						{
							struct RG { uint8 R; uint8 G; };
							// This HDR format decompresses to RG uint8s.
							RG* rgData = new RG[wfull*hfull];

							for (int y = 0; y < hfull; y += 4)
								for (int x = 0; x < wfull; x += 4)
								{
									uint8* dst = (uint8*)rgData + (y*wfull + x) * 2;
									bcdec_bc5(src, dst, wfull * 2);
									src += BCDEC_BC5_BLOCK_SIZE;
								}

							// Now convert to 32-bit RGBA with 0,255 for B,A.
							for (int xy = 0; xy < wfull*hfull; xy++)
							{
								tColour4i col(rgData[xy].R, rgData[xy].G, 0u, 255u);
								uncompData[xy].Set(col);
							}
							delete[] rgData;
							break;
						}

						case tPixelFormat::BC6S:
						case tPixelFormat::BC6U:
						{
							// This HDR format decompresses to RGB floats.
							tColour3f* rgbData = new tColour3f[wfull*hfull];

							for (int y = 0; y < hfull; y += 4)
								for (int x = 0; x < wfull; x += 4)
								{
									uint8* dst = (uint8*)((float*)rgbData + (y*wfull + x) * 3);
									bool signedData = layer->PixelFormat == tPixelFormat::BC6S;
									bcdec_bc6h_float(src, dst, wfull * 3, signedData);
									src += BCDEC_BC6H_BLOCK_SIZE;
								}

							// Now convert to 32-bit RGBA with 255 alpha.
							for (int xy = 0; xy < wfull*hfull; xy++)
							{
								tColour4f col(rgbData[xy], 1.0f);
								tDDS::ProcessHDRFlags(col, tComp_RGB, params);
								uncompData[xy].Set(col);
							}
							processedHDRFlags = true;
							delete[] rgbData;
							break;
						}

						case tPixelFormat::BC7:
						{
							for (int y = 0; y < hfull; y += 4)
								for (int x = 0; x < wfull; x += 4)
								{
									uint8* dst = (uint8*)uncompData + (y*wfull + x) * 4;
									bcdec_bc7(src, dst, wfull * 4);
									src += BCDEC_BC7_BLOCK_SIZE;
								}
							break;
						}

						case tPixelFormat::ETC1:
						case tPixelFormat::ETC2RGB:				// Same decoder. Backwards compatible.
						{
							for (int y = 0; y < hfull; y += 4)
								for (int x = 0; x < wfull; x += 4)
								{
									uint8* dst = (uint8*)uncompData + (y*wfull + x) * 4;

									// At first didn't understand the pitch (3rd) argument. It's cuz the block needs to be written into
									// multiple rows of the destination and we need to know how far to increment to the next row of 4.
									etcdec_etc_rgb(src, dst, wfull * 4);
									src += ETCDEC_ETC_RGB_BLOCK_SIZE;
								}
							break;
						}

						case tPixelFormat::ETC2RGBA:
						{
							for (int y = 0; y < hfull; y += 4)
								for (int x = 0; x < wfull; x += 4)
								{
									uint8* dst = (uint8*)uncompData + (y*wfull + x) * 4;
									etcdec_eac_rgba(src, dst, wfull * 4);
									src += ETCDEC_EAC_RGBA_BLOCK_SIZE;
								}
							break;
						}

						case tPixelFormat::ETC2RGBA1:
						{
							for (int y = 0; y < hfull; y += 4)
								for (int x = 0; x < wfull; x += 4)
								{
									uint8* dst = (uint8*)uncompData + (y*wfull + x) * 4;
									etcdec_etc_rgb_a1(src, dst, wfull * 4);
									src += ETCDEC_ETC_RGB_A1_BLOCK_SIZE;
								}
							break;
						}

						default:
							delete[] uncompData;
							Clear();
							Results |= 1 << int(ResultCode::Fatal_BCDecodeError);
							return false;
					}

					// Decode worked. We are now in RGBA 32-bit. Other params like width and height are already correct.
					// This isn't the most efficient because we don't have a stride in a tLayer, but correctness first.
					// Basically the uncompData may be too big if we needed extra room for w and h to do the decompression.
					// This happens when the image dimensions where not multiples of the block size. We deal with that here.
					// This is only inefficient if the dimensions were not a mult of 4, otherwise we can use the buffer directly.
					delete[] layer->Data;
					if ((wfull == w) && (hfull == h))
					{
						layer->Data = (uint8*)uncompData;
					}
					else
					{
						layer->Data = new uint8[w*h*sizeof(tPixel)];
						uint8* s = (uint8*)uncompData;
						uint8* d = layer->Data;
						for (int r = 0; r < h; r++)
						{
							tStd::tMemcpy(d, s, w*sizeof(tPixel));
							s += wfull * sizeof(tPixel);
							d += w     * sizeof(tPixel);
						}
						delete[] uncompData;
					}
					layer->PixelFormat = tPixelFormat::R8G8B8A8;
				}
				else if (tImage::tIsASTCFormat(PixelFormat))
				{
					int blockW = 0;
					int blockH = 0;
					int blockD = 1;

					// We use HDR profile if we detect a linear colour-space. Otherwise it's the LDR or LDR_SRGB profile.
					astcenc_profile profile = ASTCENC_PRF_LDR;
					if (ColourSpaceSrc == tColourSpace::Linear)
						profile = ASTCENC_PRF_HDR_RGB_LDR_A;
					else if (ColourSpaceSrc == tColourSpace::sRGB)
						profile = ASTCENC_PRF_LDR_SRGB;

					switch (PixelFormat)
					{
						case tPixelFormat::ASTC4X4:		blockW = 4;		blockH = 4;		break;
						case tPixelFormat::ASTC5X4:		blockW = 5;		blockH = 4;		break;
						case tPixelFormat::ASTC5X5:		blockW = 5;		blockH = 5;		break;
						case tPixelFormat::ASTC6X5:		blockW = 6;		blockH = 5;		break;
						case tPixelFormat::ASTC6X6:		blockW = 6;		blockH = 6;		break;
						case tPixelFormat::ASTC8X5:		blockW = 8;		blockH = 5;		break;
						case tPixelFormat::ASTC8X6:		blockW = 8;		blockH = 6;		break;
						case tPixelFormat::ASTC8X8:		blockW = 8;		blockH = 8;		break;
						case tPixelFormat::ASTC10X5:	blockW = 10;	blockH = 5;		break;
						case tPixelFormat::ASTC10X6:	blockW = 10;	blockH = 6;		break;
						case tPixelFormat::ASTC10X8:	blockW = 10;	blockH = 8;		break;
						case tPixelFormat::ASTC10X10:	blockW = 10;	blockH = 10;	break;
						case tPixelFormat::ASTC12X10:	blockW = 12;	blockH = 10;	break;
						case tPixelFormat::ASTC12X12:	blockW = 12;	blockH = 12;	break;
						default:														break;
					}

					if (!blockW || !blockH)
					{
						// astcenc_get_error_string(status) can be called for details.
						Clear();
						Results |= 1 << int(ResultCode::Fatal_ASTCDecodeError);
						return false;
					}

					float quality = ASTCENC_PRE_MEDIUM;			// Only need for compression.
					astcenc_error result = ASTCENC_SUCCESS;

					astcenc_config config;
					astcenc_config_init(profile, blockW, blockH, blockD, quality, ASTCENC_FLG_DECOMPRESS_ONLY, &config);
					if (result != ASTCENC_SUCCESS)
					{
						// astcenc_get_error_string(status) can be called for details.
						Clear();
						Results |= 1 << int(ResultCode::Fatal_ASTCDecodeError);
						return false;
					}

					astcenc_context* context = nullptr;
					int numThreads = tMath::tMax(tSystem::tGetNumCores(), 2);
					result = astcenc_context_alloc(&config, numThreads, &context);
					if (result != ASTCENC_SUCCESS)
					{
						Clear();
						Results |= 1 << int(ResultCode::Fatal_ASTCDecodeError);
						return false;
					}

					tColour4f* uncompData = new tColour4f[w*h];
					astcenc_image image;
					image.dim_x = w;
					image.dim_y = h;
					image.dim_z = 1;
					image.data_type = ASTCENC_TYPE_F32;

					tColour4f* slices = uncompData;
					image.data = reinterpret_cast<void**>(&slices);
					astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

					result = astcenc_decompress_image(context, src, layer->GetDataSize(), &image, &swizzle, 0);
					if (result != ASTCENC_SUCCESS)
					{
						astcenc_context_free(context);
						delete[] uncompData;
						Clear();
						Results |= 1 << int(ResultCode::Fatal_ASTCDecodeError);
						return false;
					}

					// Convert to 32-bit RGBA.
					tPixel* pixelData = new tPixel[w*h];
					for (int p = 0; p < w*h; p++)
					{
						tColour4f col(uncompData[p]);
						tDDS::ProcessHDRFlags(col, tComp_RGB, params);
						pixelData[p].Set(col);
					}
					processedHDRFlags = true;

					// Decode worked. We are now in RGBA 32-bit. Other params like width and height are already correct.
					tAssert(layer->OwnsData);
					delete[] layer->Data;
					layer->Data = (uint8*)pixelData;
					layer->PixelFormat = tPixelFormat::R8G8B8A8;

					astcenc_context_free(context);
					delete[] uncompData;
				}

				else // Unsupported PixelFormat
				{
					Clear();
					Results |= 1 << int(ResultCode::Fatal_PixelFormatNotSupported);
					return false;
				}

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

		if (processedHDRFlags)
		{
			if (params.Flags & LoadFlag_SRGBCompression)  ColourSpace = tColourSpace::sRGB;
			if (params.Flags & LoadFlag_GammaCompression) ColourSpace = tColourSpace::Gamma;
		}

		// All images decoded. Can now set the object's pixel format. We do _not_ set the PixelFormatSrc here!
		PixelFormat = tPixelFormat::R8G8B8A8;
	}

	if (reverseRowOrderRequested && !RowReversalOperationPerformed)
		Results |= 1 << int(ResultCode::Conditional_CouldNotFlipRows);

	tAssert(IsValid());

	Results |= 1 << int(ResultCode::Success);
	return true;
}


const char* tImageDDS::GetResultDesc(ResultCode code)
{
	return ResultDescriptions[int(code)];
}


const char* tImageDDS::ResultDescriptions[] =
{
	"Success",
	"Conditional Success. Image rows could not be flipped.",
	"Conditional Success. One of Pitch or LinearSize should be specified. Using dimensions instead.",
	"Conditional Success. Pixel format specification ill-formed. Assuming FourCC.",
	"Conditional Success. Image has dimension not multiple of four.",
	"Conditional Success. Image has dimension not power of two.",
	"Fatal Error. File does not exist.",
	"Fatal Error. Incorrect file type. Must be a DDS file.",
	"Fatal Error. Filesize incorrect.",
	"Fatal Error. Magic FourCC Incorrect.",
	"Fatal Error. Incorrect DDS header size.",
	"Fatal Error. Incorrect Dimensions.",
	"Fatal Error. DDS volume textures not supported.",
	"Fatal Error. Pixel format header size incorrect.",
	"Fatal Error. One of Pitch or LinearSize must be specified when strict-loading set.",
	"Fatal Error. Pixel format specification incorrect.",
	"Fatal Error. Unsupported pixel format.",
	"Fatal Error. Maximum number of mipmap levels exceeded.",
	"Fatal Error. DX10 header size incorrect.",
	"Fatal Error. DX10 resource dimension not supported. 2D support only.",
	"Fatal Error. Unable to decode packed pixels.",
	"Fatal Error. Unable to decode BC pixels.",
	"Fatal Error. Unable to decode ASTC pixels."
};
tStaticAssert(tNumElements(tImageDDS::ResultDescriptions) == int(tImageDDS::ResultCode::NumCodes));
tStaticAssert(int(tImageDDS::ResultCode::NumCodes) <= int(tImageDDS::ResultCode::MaxCodes));


}
