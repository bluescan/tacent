// tImageDDS.cpp
//
// This class knows how to load Direct Draw Surface (.dds) files. It knows the details of the dds file format and loads
// the data into tLayers, optionally decompressing them. Saving is not implemented yet. The layers may be 'stolen' from
// a tImageDDS so that excessive memcpys are avoided. After they are stolen the tImageDDS is invalid.
//
// Copyright (c) 2006, 2017, 2019, 2020, 2022-2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tString.h>
#include "Image/tImageDDS.h"
#include "Image/tPixelUtil.h"
#include "Image/tPicture.h"
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

	// These figure out the pixel-format, colour-profile, alpha-mode, and channel-type. tPixelFormat does not specify
	// ancilllary properties of the data -- it specified the encoding of the data. The extra information, like the
	// colour-profile it was authored in, is stored in tColourProfile, tAlphaMode, and tChannelType. In many cases this
	// satellite information cannot be determined, in which they will be set to their 'unspecified' enumerant.
	void GetFormatInfo_FromDXGIFormat		(tPixelFormat&, tColourProfile&, tAlphaMode&, tChannelType&, uint32 dxgiFormat);
	void GetFormatInfo_FromFourCC			(tPixelFormat&, tColourProfile&, tAlphaMode&, tChannelType&, uint32 fourCC);
	void GetFormatInfo_FromComponentMasks	(tPixelFormat&, tColourProfile&, tAlphaMode&, tChannelType&, const FormatData&);
}


void tDDS::GetFormatInfo_FromDXGIFormat(tPixelFormat& format, tColourProfile& profile, tAlphaMode& alphaMode, tChannelType& chanType, uint32 dxgiFormat)
{
	// For colour profile (the space of the data) we try to make an educated guess. In general only the asset author knows the
	// colour space/profile. For most (non-HDR) pixel formats for colours, we assume the data is sRGB. If the pixel format has
	// a specific sRGB alternative, we _should_ assume that the space is the alternative (usually linear) -- however many dds
	// files in the wild set them as UNORM rather than UNORM_SRGB. NVTT for example uses the UNORM (non sRGB) format for all ASTC
	// compressed textures, when it probably should have gone with the sRGB variant (i.e. They 'usually' encode colours).
	// Floating-point formats are assumed to be in linear-space (and are usually used for HDR images). In addition when the data
	// is probably not colour data (like ATI1/2) we assume it's linear.
	//
	// To keep the loading code as clean as possible, we'll respect the dds file's encoded format even if some files are set
	// incorrectly. If the format is typeless, we'll assume sRGB, but set the chanType to Unspecified.
	format		= tPixelFormat::Invalid;
	profile		= tColourProfile::sRGB;
	alphaMode	= tAlphaMode::None;
	chanType	= tChannelType::NONE;

	#define C(c) case DXGIFMT_##c
	#define F(f) format = tPixelFormat::f;
	#define P(p) profile = tColourProfile::p;
	#define M(m) alphaMode = tAlphaMode::m;
	#define T(t) chanType = tChannelType::t;

	// DXGI formats do not specify premultiplied alpha mode like DXT4/5 so we leave it unspecified. As for sRGB,
	// if it says UNORM_SRGB, sure, it may not contain sRGB data, but it's as good as you can get in terms of knowing.
	// I mean if the DirectX loader 'treats' it as being sRGB (in that it will convert it to linear), then we should
	// treat it as being sRGB data in general. Of course it could just be a recipe for apple pie, and if it is, it is
	// a recipe the authors wanted interpreted as sRGB data, otherwise they wouldn't have chosen the _SRGB pixel format.
	// Additionally real files in the wild are mostly still sRGB even with the non _SRGB DXGIFMT. For these cases the
	// switch below still chooses sRGB, but to make it clear, we explicity tag it with a commented out lRGB. That is,
	// when there is a non-sRGB variant and an sRGB variant, it _should_ be that the non-sRGB is linear, but real files
	// are still sRGB, so we explicitly put a commented out lRGB tag. eg. DXGIFMT_BC1_UNORM.
	switch (dxgiFormat)
	{
		//
		// BC Formats.
		//
		C(BC1_TYPELESS):				F(BC1DXT1)			P(sRGB)		M(None)		T(NONE)		break;
		C(BC1_UNORM):					F(BC1DXT1)			/*P(lRGB)*/				T(UNORM)	break;
		C(BC1_UNORM_SRGB):				F(BC1DXT1)									T(UNORM)	break;

		// DXGI formats do not specify premultiplied alpha mode like DXT2/3 so we leave it unspecified.
		C(BC2_TYPELESS):				F(BC2DXT2DXT3)											break;
		C(BC2_UNORM):					F(BC2DXT2DXT3)		/*P(lRGB)*/				T(UNORM)	break;
		C(BC2_UNORM_SRGB):				F(BC2DXT2DXT3)								T(UNORM)	break;

		C(BC3_TYPELESS):				F(BC3DXT4DXT5)											break;
		C(BC3_UNORM):					F(BC3DXT4DXT5)		/*P(lRGB)*/				T(UNORM)	break;
		C(BC3_UNORM_SRGB):				F(BC3DXT4DXT5)								T(UNORM)	break;

		// We don't decode signed properly yet.
		C(BC4_TYPELESS):				F(BC4ATI1U)			P(lRGB)								break;
		C(BC4_UNORM):					F(BC4ATI1U)			P(lRGB)					T(UNORM)	break;
		C(BC4_SNORM):					F(BC4ATI1S)			P(lRGB)					T(SNORM)	break;

		// We don't decode signed properly yet.
		C(BC5_TYPELESS):				F(BC5ATI2U)			P(lRGB) 							break;
		C(BC5_UNORM):					F(BC5ATI2U)			P(lRGB)					T(UNORM)	break;
		C(BC5_SNORM):					F(BC5ATI2S)			P(lRGB) 				T(SNORM)	break;

		// Alpha not used by BC6. Interpret typeless as BC6H_U16... we gotta choose something.
		C(BC6H_TYPELESS):				F(BC6U)				P(HDRa)								break;
		C(BC6H_UF16):					F(BC6U)				P(HDRa)					T(UFLOAT)	break;
		C(BC6H_SF16):					F(BC6S)				P(HDRa)					T(SFLOAT)	break;

		// Interpret typeless as sRGB. UNORM without the SRGB must be linear.
		C(BC7_TYPELESS):				F(BC7)													break;
		C(BC7_UNORM):					F(BC7)				/*P(lRGB)*/				T(UNORM)	break;
		C(BC7_UNORM_SRGB):				F(BC7)										T(UNORM)	break;

		//
		// Packed Formats.
		//
		C(A8_UNORM):					F(A8)				P(lRGB)								break;

		// We don't decode signed properly yet. We treat single R channel as if it's in sRGB.
		C(R8_TYPELESS):					F(R8)													break;
		C(R8_UNORM):					F(R8)										T(UNORM)	break;
		C(R8_UINT):						F(R8)										T(UINT)		break;
		//C(R8_SNORM):					F(R8)										T(SNORM)	break;
		//C(R8_SINT):					F(R8)										T(SINT)		break;

		// We don't decode signed properly yet.
		C(R8G8_TYPELESS):				F(R8G8)													break;
		C(R8G8_UNORM):					F(R8G8)										T(UNORM)	break;
		C(R8G8_UINT):					F(R8G8)										T(UINT)		break;
		//C(R8G8_SNORM):				F(R8G8)										T(SNORM)	break;
		//C(R8G8_SINT):					F(R8G8)										T(SINT)		break;

		// UINT is stored same as UNORM. Only diff is that UNORM ends up as a 'float' from 0.0 to 1.0.
		// We don't decode signed properly yet. Since there is UNORM and UNORM_SRGB, need to assume
		// the UNORM one is linear (otherwise why have sRGB variant).
		// Apparently real files in the wild are mostly still sRGB even with the non _SRGB DXGIFMT.
		C(R8G8B8A8_TYPELESS):			F(R8G8B8A8)												break;
		C(R8G8B8A8_UNORM):				F(R8G8B8A8)			/*P(lRGB)*/				T(UNORM)	break;
		C(R8G8B8A8_UINT):				F(R8G8B8A8)									T(UINT)		break;
		C(R8G8B8A8_UNORM_SRGB):			F(R8G8B8A8)									T(UNORM)	break;
		//C(R8G8B8A8_SNORM):			F(R8G8B8A8)									T(SNORM)	break;
		//C(R8G8B8A8_SINT):				F(R8G8B8A8)									T(SINT)		break;

		C(B8G8R8A8_TYPELESS):			F(B8G8R8A8)												break;
		C(B8G8R8A8_UNORM):				F(B8G8R8A8)			/*P(lRGB)*/				T(UNORM)	break;
		C(B8G8R8A8_UNORM_SRGB):			F(B8G8R8A8)									T(UNORM)	break;

		// Formats without explicit sRGB variants are considered sRGB.
		C(B5G6R5_UNORM):				F(G3B5R5G3)									T(UNORM)	break;
		C(B4G4R4A4_UNORM):				F(G4B4A4R4)									T(UNORM)	break;
		C(B5G5R5A1_UNORM):				F(G3B5A1R5G2)								T(UNORM)	break;

		C(R16_FLOAT):					F(R16f)				P(HDRa)					T(SFLOAT)	break;
		C(R16G16_FLOAT):				F(R16G16f)			P(HDRa)					T(SFLOAT)	break;
		C(R16G16B16A16_FLOAT):			F(R16G16B16A16f)	P(HDRa)					T(SFLOAT)	break;

		C(R32_FLOAT):					F(R32f)				P(HDRa)					T(SFLOAT)	break;
		C(R32G32_FLOAT):				F(R32G32f)			P(HDRa)					T(SFLOAT)	break;
		C(R32G32B32_FLOAT):				F(R32G32B32f)		P(HDRa)					T(SFLOAT)	break;
		C(R32G32B32A32_FLOAT):			F(R32G32B32A32f)	P(HDRa)					T(SFLOAT)	break;

		C(R11G11B10_FLOAT):				F(B10G11R11uf)		P(HDRa)					T(UFLOAT)	break;
		C(R9G9B9E5_SHAREDEXP):			F(E5B9G9R9uf)		P(HDRa)					T(UFLOAT)	break;

		//
		// ASTC Formats.
		//
		// We chose HDR as the default profile because it can load LDR blocks. The other way around doesn't work with
		// with the tests images -- the LDR profile doesn't appear capable of loading HDR blocks (they become magenta).
		// Apparently real files in the wild are mostly still sRGB even with the non _SRGB DXGIFMT.
		//
		C(EXT_ASTC_4X4_TYPELESS):		F(ASTC4X4)			/*P(HDRa)*/							break;
		C(EXT_ASTC_4X4_UNORM):			F(ASTC4X4)			/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_4X4_UNORM_SRGB):		F(ASTC4X4)									T(UNORM)	break;

		C(EXT_ASTC_5X4_TYPELESS):		F(ASTC5X4)			/*P(HDRa)*/							break;
		C(EXT_ASTC_5X4_UNORM):			F(ASTC5X4)			/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_5X4_UNORM_SRGB):		F(ASTC5X4)									T(UNORM)	break;

		C(EXT_ASTC_5X5_TYPELESS):		F(ASTC5X5)			/*P(HDRa)*/							break;
		C(EXT_ASTC_5X5_UNORM):			F(ASTC5X5)			/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_5X5_UNORM_SRGB):		F(ASTC5X5)									T(UNORM)	break;

		C(EXT_ASTC_6X5_TYPELESS):		F(ASTC6X5)			/*P(HDRa)*/							break;
		C(EXT_ASTC_6X5_UNORM):			F(ASTC6X5)			/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_6X5_UNORM_SRGB):		F(ASTC6X5)									T(UNORM)	break;

		C(EXT_ASTC_6X6_TYPELESS):		F(ASTC6X6)			/*P(HDRa)*/							break;
		C(EXT_ASTC_6X6_UNORM):			F(ASTC6X6)			/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_6X6_UNORM_SRGB):		F(ASTC6X6)									T(UNORM)	break;

		C(EXT_ASTC_8X5_TYPELESS):		F(ASTC8X5)			/*P(HDRa)*/							break;
		C(EXT_ASTC_8X5_UNORM):			F(ASTC8X5)			/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_8X5_UNORM_SRGB):		F(ASTC8X5)									T(UNORM)	break;

		C(EXT_ASTC_8X6_TYPELESS):		F(ASTC8X6)			/*P(HDRa)*/							break;
		C(EXT_ASTC_8X6_UNORM):			F(ASTC8X6)			/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_8X6_UNORM_SRGB):		F(ASTC8X6)									T(UNORM)	break;

		C(EXT_ASTC_8X8_TYPELESS):		F(ASTC8X8)			/*P(HDRa)*/							break;
		C(EXT_ASTC_8X8_UNORM):			F(ASTC8X8)			/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_8X8_UNORM_SRGB):		F(ASTC8X8)									T(UNORM)	break;

		C(EXT_ASTC_10X5_TYPELESS):		F(ASTC10X5)			/*P(HDRa)*/							break;
		C(EXT_ASTC_10X5_UNORM):			F(ASTC10X5)			/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_10X5_UNORM_SRGB):	F(ASTC10X5)									T(UNORM)	break;

		C(EXT_ASTC_10X6_TYPELESS):		F(ASTC10X6)			/*P(HDRa)*/							break;
		C(EXT_ASTC_10X6_UNORM):			F(ASTC10X6)			/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_10X6_UNORM_SRGB):	F(ASTC10X6)									T(UNORM)	break;

		C(EXT_ASTC_10X8_TYPELESS):		F(ASTC10X8)			/*P(HDRa)*/							break;
		C(EXT_ASTC_10X8_UNORM):			F(ASTC10X8)			/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_10X8_UNORM_SRGB):	F(ASTC10X8)									T(UNORM)	break;

		C(EXT_ASTC_10X10_TYPELESS):		F(ASTC10X10)		/*P(HDRa)*/							break;
		C(EXT_ASTC_10X10_UNORM):		F(ASTC10X10)		/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_10X10_UNORM_SRGB):	F(ASTC10X10)								T(UNORM)	break;

		C(EXT_ASTC_12X10_TYPELESS):		F(ASTC12X10)		/*P(HDRa)*/							break;
		C(EXT_ASTC_12X10_UNORM):		F(ASTC12X10)		/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_12X10_UNORM_SRGB):	F(ASTC12X10)								T(UNORM)	break;

		C(EXT_ASTC_12X12_TYPELESS):		F(ASTC12X12)		/*P(HDRa)*/							break;
		C(EXT_ASTC_12X12_UNORM):		F(ASTC12X12)		/*P(HDRa)*/				T(UNORM)	break;
		C(EXT_ASTC_12X12_UNORM_SRGB):	F(ASTC12X12)								T(UNORM)	break;

		default:													P(None)								break;
	}
	#undef C
	#undef F
	#undef P
	#undef M
	#undef T
}


void tDDS::GetFormatInfo_FromFourCC(tPixelFormat& format, tColourProfile& profile, tAlphaMode& alphaMode, tChannelType& chanType, uint32 fourCC)
{
	format		= tPixelFormat::Invalid;
	profile		= tColourProfile::sRGB;
	alphaMode	= tAlphaMode::None;
	chanType	= tChannelType::NONE;

	#define C(c) case D3DFMT_##c
	#define F(f) format = tPixelFormat::f;
	#define P(p) profile = tColourProfile::p;
	#define M(m) alphaMode = tAlphaMode::m;
	#define T(t) chanType = tChannelType::t;
	switch (fourCC)
	{
		// Note that during inspecition of the individual layer data, the DXT1 pixel format might be modified
		// to DXT1BA (binary alpha).
		C(DXT1):						F(BC1DXT1)			P(sRGB)		M(None)		T(NONE)		break;

		// DXT2 and DXT3 are the same format. Only how you interpret the data is different. In tacent we treat them
		// as the same pixel-format. How contents are interpreted (the data) is not part of the format. 
		C(DXT2):						F(BC2DXT2DXT3)					M(Mult)					break;
		C(DXT3):						F(BC2DXT2DXT3)					M(Norm)					break;
		C(DXT4):						F(BC3DXT4DXT5)					M(Mult)					break;
		C(DXT5):						F(BC3DXT4DXT5)					M(Norm)					break;

		C(ATI1):						F(BC4ATI1U)			P(lRGB)					T(UNORM)	break;
		C(BC4U):						F(BC4ATI1U)			P(lRGB)					T(UNORM)	break;
		C(BC4S):						F(BC4ATI1S)			P(lRGB)					T(SNORM)	break;

		C(ATI2):						F(BC5ATI2U)			P(lRGB)					T(UNORM)	break;
		C(BC5U):						F(BC5ATI2U)			P(lRGB)					T(UNORM)	break;
		C(BC5S):						F(BC5ATI2S)			P(lRGB)					T(SNORM)	break;

		// We don't yet support signed BC4S or BC5S.
		//C(BC4S):																				break;
		//C(BC5S):																				break;

		// We don't yet support D3DFMT_R8G8_B8G8 or D3DFMT_G8R8_G8B8 -- That's a lot of green precision.
		//C(R8G8_B8G8):																			break;
		//C(G8R8_G8B8):																			break;

		C(ETC):							F(ETC1)													break;
		C(ETC1):						F(ETC1)													break;
		C(ETC2):						F(ETC2RGB)												break;
		C(ETCA):						F(ETC2RGBA)												break;
		C(ETCP):						F(ETC2RGBA1)											break;

		// Sometimes these D3D formats may be stored in the FourCC slot.
		// We don't yet support D3DFMT_A16B16G16R16 or D3DFMT_Q16W16V16U16.
		//C(A16B16G16R16):																		break;
		//C(Q16W16V16U16):																		break;

		C(A8):							F(A8)				P(lRGB)								break;
		C(L8):							F(L8)													break;

		// It's inconsistent calling the D3D format ABGR. The components are clearly in RGBA order, not ABGR.
		// Anyway, I only have control over the tPixelFormat names. In fairness, it looks like the format-name
		// was fixed in the DX10 header format type names. See
		// https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-legacy-formats
		C(A8B8G8R8):					F(R8G8B8A8)												break;

		C(R16F):						F(R16f)				P(HDRa)								break;
		C(G16R16F):						F(R16G16f)			P(HDRa)								break;
		C(A16B16G16R16F):				F(R16G16B16A16f)	P(HDRa)								break;

		C(R32F):						F(R32f)				P(HDRa)								break;
		C(G32R32F):						F(R32G32f)			P(HDRa)								break;
		C(A32B32G32R32F):				F(R32G32B32A32f)	P(HDRa)								break;

		default:											P(None)								break;
	}
	#undef C
	#undef F
	#undef P
	#undef M
	#undef T
}


void tDDS::GetFormatInfo_FromComponentMasks(tPixelFormat& format, tColourProfile& profile, tAlphaMode& alpha, tChannelType& chanType, const FormatData& fmtData)
{
	format		= tPixelFormat::Invalid;
	profile		= tColourProfile::sRGB;
	alpha		= tAlphaMode::Unspecified;
	chanType	= tChannelType::Unspecified;

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

		case 16:		// Supports G3B5R5G3, G4B4A4R4, and G3B5A1R5G2.
			if (isRGBA && (mskB == 0x001F) && (mskG == 0x03E0) && (mskR == 0x7C00) && (mskA == 0x8000))
				format = tPixelFormat::G3B5A1R5G2;
			else if (isRGBA && (mskB == 0x000F) && (mskG == 0x00F0) && (mskR == 0x0F00) && (mskA == 0xF000))
				format = tPixelFormat::G4B4A4R4;
			else if (isRGB && (mskB == 0x001F) && (mskG == 0x07E0) && (mskR == 0xF800))
				format = tPixelFormat::G3B5R5G3;
			break;

		case 24:		// Supports B8G8R8 and R8G8B8.
			if (isRGB && (mskB == 0x0000FF) && (mskG == 0x00FF00) && (mskR == 0xFF0000))
				format = tPixelFormat::B8G8R8;
			else if (isRGB && (mskR == 0x0000FF) && (mskG == 0x00FF00) && (mskB == 0xFF0000))
				format = tPixelFormat::R8G8B8;
			break;

		case 32:		// Supports B8G8R8A8 and R8G8B8A8.
			if (isRGBA && (mskB == 0x000000FF) && (mskG == 0x0000FF00) && (mskR == 0x00FF0000) && (mskA == 0xFF000000))
				format = tPixelFormat::B8G8R8A8;
			else if (isRGBA && (mskR == 0x000000FF) && (mskG == 0x0000FF00) && (mskB == 0x00FF0000) && (mskA == 0xFF000000))
				format = tPixelFormat::R8G8B8A8;
			break;
	}

	switch (format)
	{
		case tPixelFormat::A8:
			profile = tColourProfile::lRGB;
			break;

		case tPixelFormat::Invalid:
			profile = tColourProfile::None;
			break;
	}
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
	tBaseImage::Clear();
	for (int image = 0; image < NumImages; image++)
	{
		for (int layer = 0; layer < NumMipmapLayers; layer++)
		{
			delete Layers[layer][image];
			Layers[layer][image] = nullptr;
		}
	}

	States							= 0;	// Image will be invalid now since Valid state not set.
	AlphaMode						= tAlphaMode::Unspecified;
	ChannelType						= tChannelType::Unspecified;
	IsCubeMap						= false;
	IsModernDX10					= false;
	RowReversalOperationPerformed	= false;
	NumImages						= 0;
	NumMipmapLayers					= 0;
}


bool tImageDDS::Set(tPixel4b* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Layers[0][0]					= new tLayer(tPixelFormat::R8G8B8A8, width, height, (uint8*)pixels, steal);
	AlphaMode						= tAlphaMode::Normal;
	ChannelType						= tChannelType::UNORM;
	IsCubeMap						= false;
	IsModernDX10					= false;
	RowReversalOperationPerformed	= false;
	NumImages						= 1;
	NumMipmapLayers					= 1;

	PixelFormatSrc		= tPixelFormat::R8G8B8A8;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume pixels must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

	SetStateBit(StateBit::Valid);
	return true;
}


bool tImageDDS::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	PixelFormatSrc		= frame->PixelFormatSrc;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume frame must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

	tPixel4b* pixels = frame->GetPixels(steal);
	Set(pixels, frame->Width, frame->Height, steal);
	if (steal)
		delete frame;

	SetStateBit(StateBit::Valid);
	return true;
}


bool tImageDDS::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	PixelFormatSrc		= picture.PixelFormatSrc;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	// We don't know colour profile of tPicture.

	// This is worth some explanation. If steal is true the picture becomes invalid and the
	// 'set' call will steal the stolen pixels. If steal is false GetPixels is called and the
	// 'set' call will memcpy them out... which makes sure the picture is still valid after and
	// no-one is sharing the pixel buffer. We don't check the success of 'set' because it must
	// succeed if picture was valid.
	tPixel4b* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	bool success = Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
	tAssert(success);
	return true;
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
		frame->Pixels = (tPixel4b*)Layers[0][0]->StealData();
		delete Layers[0][0];
		Layers[0][0] = nullptr;
	}
	else
	{
		frame->Pixels = new tPixel4b[frame->Width * frame->Height];
		tStd::tMemcpy(frame->Pixels, (tPixel4b*)Layers[0][0]->Data, frame->Width * frame->Height * sizeof(tPixel4b));
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


int tImageDDS::GetLayers(tList<tLayer>& layers) const
{
	if (!IsValid() || IsCubemap() || (NumImages <= 0))
		return 0;

	for (int mip = 0; mip < NumMipmapLayers; mip++)
		layers.Append(Layers[mip][0]);

	return NumMipmapLayers;
}


int tImageDDS::StealCubemapLayers(tList<tLayer> layerLists[tFaceIndex_NumFaces], uint32 faceFlags)
{
	if (!IsValid() || !IsCubemap() || !faceFlags)
		return 0;

	int faceCount = 0;
	for (int face = 0; face < tFaceIndex_NumFaces; face++)
	{
		uint32 faceFlag = 1 << face;
		if (!(faceFlag & faceFlags))
			continue;

		tList<tLayer>& layers = layerLists[face];
		for (int mip = 0; mip < NumMipmapLayers; mip++)
		{
			layers.Append( Layers[mip][face] );
			Layers[mip][face] = nullptr;
		}
		faceCount++;
	}

	Clear();
	return faceCount;
}


int tImageDDS::GetCubemapLayers(tList<tLayer> layerLists[tFaceIndex_NumFaces], uint32 faceFlags) const
{
	if (!IsValid() || !IsCubemap() || !faceFlags)
		return 0;

	int sideCount = 0;
	for (int face = 0; face < tFaceIndex_NumFaces; face++)
	{
		uint32 faceFlag = 1 << face;
		if (!(faceFlag & faceFlags))
			continue;

		tList<tLayer>& layers = layerLists[face];
		for (int mip = 0; mip < NumMipmapLayers; mip++)
			layers.Append( Layers[mip][face] );

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
		SetStateBit(StateBit::Fatal_IncorrectFileType);
		return false;
	}

	if (!tSystem::tFileExists(ddsFile))
	{
		SetStateBit(StateBit::Fatal_FileDoesNotExist);
		return false;
	}

	int ddsSizeBytes = 0;
	uint8* ddsData = (uint8*)tSystem::tLoadFile(ddsFile, 0, &ddsSizeBytes);
	bool success = Load(ddsData, ddsSizeBytes, loadParams);
	delete[] ddsData;

	return success;
}


bool tImageDDS::Load(const uint8* ddsData, int ddsDataSize, const LoadParams& paramsIn)
{
	Clear();
	LoadParams params(paramsIn);

	// This will deal with zero-sized files properly as well.
	if (ddsDataSize < int(sizeof(tDDS::Header)+sizeof(uint32)))
	{
		SetStateBit(StateBit::Fatal_IncorrectFileSize);
		return false;
	}

	uint32& magic = *((uint32*)ddsData);
	ddsData += sizeof(uint32); ddsDataSize -= sizeof(uint32);
	if (magic != FourCC('D','D','S',' '))
	{
		SetStateBit(StateBit::Fatal_IncorrectMagicNumber);
		return false;
	}

	tDDS::Header& header = *((tDDS::Header*)ddsData);
	ddsData += sizeof(header); ddsDataSize -= sizeof(header);
	tStaticAssert(sizeof(tDDS::Header) == 124);
	if (header.Size != 124)
	{
		SetStateBit(StateBit::Fatal_IncorrectHeaderSize);
		return false;
	}

	uint32 flags = header.Flags;
	int mainWidth = header.Width;						// Main image.
	int mainHeight = header.Height;						// Main image.
	if ((mainWidth <= 0) || (mainHeight <= 0))
	{
		SetStateBit(StateBit::Fatal_InvalidDimensions);
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
			SetStateBit(StateBit::Fatal_PitchXORLinearSize);
			return false;
		}
		else
		{
			SetStateBit(StateBit::Conditional_PitchXORLinearSize);
		}
	}

	// Volume textures are not supported.
	if (flags & tDDS::HeaderFlag_Depth)
	{
		SetStateBit(StateBit::Fatal_VolumeTexturesNotSupported);
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
		SetStateBit(StateBit::Fatal_MaxNumMipmapLevelsExceeded);
		return false;
	}

	tDDS::FormatData& format = header.Format;
	if (format.Size != 32)
	{
		SetStateBit(StateBit::Fatal_IncorrectPixelFormatHeaderSize);
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
			SetStateBit(StateBit::Fatal_IncorrectPixelFormatSpec);
			return false;
		}
		else
		{
			// If the flags completely fail to specify a format, we try to use the FourCC.
			SetStateBit(StateBit::Conditional_IncorrectPixelFormatSpec);
			isFourCCFormat = true;
		}
	}

	IsModernDX10 = isFourCCFormat && (format.FourCC == tDDS::D3DFMT_DX10);

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
		if (ddsDataSize < int(sizeof(tDDS::DX10Header)))
		{
			SetStateBit(StateBit::Fatal_IncorrectFileSize);
			return false;
		}

		tDDS::DX10Header& headerDX10 = *((tDDS::DX10Header*)ddsData);
		ddsData += sizeof(tDDS::DX10Header); ddsDataSize -= sizeof(tDDS::DX10Header);
		if (headerDX10.ArraySize == 0)
		{
			SetStateBit(StateBit::Fatal_DX10HeaderSizeIncorrect);
			return false;
		}

		// We only handle 2D textures for now.
		if (headerDX10.Dimension != tDDS::D3D10_DIMENSION_TEXTURE2D)
		{
			SetStateBit(StateBit::Fatal_DX10DimensionNotSupported);
			return false;
		}

		if (headerDX10.MiscFlag & tDDS::D3D11_MISCFLAG_TEXTURECUBE)
		{
			IsCubeMap = true;
			NumImages = 6;
		}

		// If we found a dx10 chunk. It must be used to determine the pixel format and possibly any known colour-profile info.
		tDDS::GetFormatInfo_FromDXGIFormat(PixelFormat, ColourProfile, AlphaMode, ChannelType, headerDX10.DxgiFormat);
	}
	else if (isFourCCFormat)
	{
		tDDS::GetFormatInfo_FromFourCC(PixelFormat, ColourProfile, AlphaMode, ChannelType, format.FourCC);
	}
	// It must be a simple uncompressed format.
	else
	{
		tDDS::GetFormatInfo_FromComponentMasks(PixelFormat, ColourProfile, AlphaMode, ChannelType, format);
	}
	PixelFormatSrc = PixelFormat;
	ColourProfileSrc = ColourProfile;

	// From now on we should just be using the PixelFormat to decide what to do next.
	if (PixelFormat == tPixelFormat::Invalid)
	{
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

				// Check that the amount of data left is enough to read in numBytes.
				if (numBytes > ddsDataSize)
				{
					Clear();
					SetStateBit(StateBit::Fatal_IncorrectFileSize);
					return false;
				}

				// Here's where we possibly modify the opaque DXT1 texture to be DXT1A if there are blocks with binary
				// transparency. We only bother checking the main layer. If it's opaque we assume all the others are too.
				if ((layer == 0) && (PixelFormat == tPixelFormat::BC1DXT1) && tImage::DoBC1BlocksHaveBinaryAlpha((tImage::BC1Block*)ddsData, numBlocks))
					PixelFormat = PixelFormatSrc = tPixelFormat::BC1DXT1A;

				// DDS files store textures upside down. In the OpenGL RH coord system, the lower left of the texture
				// is the origin and consecutive rows go up. For this reason we need to read each row of blocks from
				// the top to the bottom row. We also need to flip the rows within the 4x4 block by flipping the lookup
				// tables. This should be fairly fast as there is no encoding or decoding going on. Width and height
				// will go down to 1x1, which will still use a 4x4 DXT pixel-block.
				if (doRowReversalBeforeDecode)
				{
					uint8* reversedPixelData = tImage::CreateReversedRowData(ddsData, PixelFormat, numBlocksW, numBlocksH);
					tAssert(reversedPixelData);

					// We can simply get the layer to steal the memory (the last true arg).
					Layers[layer][image] = new tLayer(PixelFormat, width, height, reversedPixelData, true);
				}
				else
				{
					// Not reversing. Use the current ddsData. Note that steal is false here so the data
					// is both copied and owned by the new tLayer.
					Layers[layer][image] = new tLayer(PixelFormat, width, height, (uint8*)ddsData);
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

			ddsData += numBytes; ddsDataSize -= numBytes;
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

	// Decode to 32-bit RGBA.
	bool didRowReversalAfterDecode = false;
	for (int image = 0; image < NumImages; image++)
	{
		for (int layerNum = 0; layerNum < NumMipmapLayers; layerNum++)
		{
			tLayer* layer = Layers[layerNum][image];
			int w = layer->Width;
			int h = layer->Height;

			// At the end of decoding _either_ decoded4b _or_ decoded4f will be valid, not both.
			// The decoded4b format used for LDR images.
			// The decoded4f format used for HDR images.
			tColour4b* decoded4b = nullptr;
			tColour4f* decoded4f = nullptr;
			DecodeResult result = DecodePixelData
			(
				layer->PixelFormat, layer->Data, layer->GetDataSize(),
				w, h, decoded4b, decoded4f
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
			tAssert(decoded4f || decoded4b);
			bool flagTone = (params.Flags & tImageDDS::LoadFlag_ToneMapExposure) ? true : false;
			bool flagSRGB = (params.Flags & tImageDDS::LoadFlag_SRGBCompression) ? true : false;
			bool flagGama = (params.Flags & tImageDDS::LoadFlag_GammaCompression)? true : false;
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
			if (decoded4b && (flagSRGB || flagGama))
			{
				for (int p = 0; p < w*h; p++)
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
				decoded4b = new tColour4b[w*h];
				for (int p = 0; p < w*h; p++)
					decoded4b[p].Set(decoded4f[p]);
				delete[] decoded4f;
			}

			// Possibly spread the L/Red channel.
			if (spread && tIsLuminanceFormat(layer->PixelFormat))
			{
				for (int p = 0; p < w*h; p++)
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
					tColour4b& col = ((tColour4b*)layer->Data)[xy];
					tStd::tSwap(col.R, col.B);
				}
			}
		}
	}

	if (reverseRowOrderRequested && !RowReversalOperationPerformed && didRowReversalAfterDecode)
		RowReversalOperationPerformed = true;

	// Maybe update the current colour profile.
	if (params.Flags & LoadFlag_SRGBCompression)  ColourProfile = tColourProfile::sRGB;
	if (params.Flags & LoadFlag_GammaCompression) ColourProfile = tColourProfile::gRGB;

	// All images decoded. Can now set the object's pixel format. We do _not_ set the PixelFormatSrc here!
	PixelFormat = tPixelFormat::R8G8B8A8;

	if (reverseRowOrderRequested && !RowReversalOperationPerformed)
		SetStateBit(StateBit::Conditional_CouldNotFlipRows);

	SetStateBit(StateBit::Valid);
	tAssert(IsValid());
	return true;
}


const char* tImageDDS::GetStateDesc(StateBit state)
{
	return StateDescriptions[int(state)];
}


const char* tImageDDS::StateDescriptions[] =
{
	"Valid",
	"Conditional Valid. Image rows could not be flipped.",
	"Conditional Valid. One of Pitch or LinearSize should be specified. Using dimensions instead.",
	"Conditional Valid. Pixel format specification ill-formed. Assuming FourCC.",
	"Conditional Valid. Image has dimension not multiple of four.",
	"Conditional Valid. Image has dimension not power of two.",
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
tStaticAssert(tNumElements(tImageDDS::StateDescriptions) == int(tImageDDS::StateBit::NumStateBits));
tStaticAssert(int(tImageDDS::StateBit::NumStateBits) <= int(tImageDDS::StateBit::MaxStateBits));


}
