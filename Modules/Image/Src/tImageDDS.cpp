// tImageDDS.cpp
//
// This class knows how to load Direct Draw Surface (.dds) files. Saving is not implemented yet.
// It does zero processing of image data. It knows the details of the dds file format and loads the data into tLayers.
// Currently it does not compress or decompress the image data if it is compressed (DXTn), it simply keeps it in the
// same format as the source file. The layers may be 'stolen' from a tImageDDS so that excessive memcpys are avoided.
// After they are stolen the tImageDDS is invalid.
//
// Copyright (c) 2006, 2017, 2019, 2020, 2022 Tristan Grimmer.
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
#define BCDEC_IMPLEMENTATION
#include "bcdec/bcdec.h"
#define FourCC(ch0, ch1, ch2, ch3) (uint(uint8(ch0)) | (uint(uint8(ch1)) << 8) | (uint(uint8(ch2)) << 16) | (uint(uint8(ch3)) << 24))
namespace tImage
{


tImageDDS::tImageDDS() :
	Filename(),
	Results(1 << int(ResultCode::Fatal_DefaultInitialized)),
	PixelFormat(tPixelFormat::Invalid),
	PixelFormatOrig(tPixelFormat::Invalid),
	IsCubeMap(false),
	RowReversalOperationPerformed(false),
	NumImages(0),
	NumMipmapLayers(0)
{
	tStd::tMemset(MipmapLayers, 0, sizeof(MipmapLayers));
}


tImageDDS::tImageDDS(const tString& ddsFile, uint32 loadFlags) :
	Filename(ddsFile),
	Results(1 << int(ResultCode::Success)),
	PixelFormat(tPixelFormat::Invalid),
	PixelFormatOrig(tPixelFormat::Invalid),
	IsCubeMap(false),
	RowReversalOperationPerformed(false),
	NumImages(0),
	NumMipmapLayers(0)
{
	tStd::tMemset(MipmapLayers, 0, sizeof(MipmapLayers));
	Load(ddsFile, loadFlags);
}


tImageDDS::tImageDDS(const uint8* ddsFileInMemory, int numBytes, uint32 loadFlags) :
	Filename(),
	Results(1 << int(ResultCode::Success)),
	PixelFormat(tPixelFormat::Invalid),
	PixelFormatOrig(tPixelFormat::Invalid),
	IsCubeMap(false),
	RowReversalOperationPerformed(false),
	NumImages(0),
	NumMipmapLayers(0)
{
	tStd::tMemset(MipmapLayers, 0, sizeof(MipmapLayers));
	Load(ddsFileInMemory, numBytes, loadFlags);
}


void tImageDDS::Clear()
{
	for (int image = 0; image < NumImages; image++)
	{
		for (int layer = 0; layer < NumMipmapLayers; layer++)
		{
			delete MipmapLayers[layer][image];
			MipmapLayers[layer][image] = nullptr;
		}
	}

	Results = (1 << int(ResultCode::Success));
	PixelFormat = tPixelFormat::Invalid;
	PixelFormatOrig = tPixelFormat::Invalid;
	ColourSpace = tColourSpace::Unknown;
	IsCubeMap = false;
	RowReversalOperationPerformed = false;
	NumImages = 0;
	NumMipmapLayers = 0;
}


bool tImageDDS::IsOpaque() const
{
	return !tImage::tFormatSupportsAlpha(PixelFormat);
}


bool tImageDDS::StealTextureLayers(tList<tLayer>& layers)
{
	if (!IsValid() || IsCubemap() || (NumImages <= 0))
		return false;

	for (int mip = 0; mip < NumMipmapLayers; mip++)
	{
		layers.Append(MipmapLayers[mip][0]);
		MipmapLayers[mip][0] = nullptr;
	}

	Clear();
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
			layers.Append( MipmapLayers[mip][side] );
			MipmapLayers[mip][side] = nullptr;
		}
		sideCount++;
	}

	Clear();
	return sideCount;
}


enum tDDSPixelFormatFlag
{
	// May be used in the DDSPixelFormat struct to indicate alphas present for RGB formats.
	tDDSPixelFormatFlag_Alpha	= 0x00000001,

	// A DDS file may contain this type of data (pixel format). eg. DXT1 is a fourCC format.
	tDDSPixelFormatFlag_FourCC	= 0x00000004,

	// A DDS file may contain this type of data (pixel format). eg. A8R8G8B8
	tDDSPixelFormatFlag_RGB		= 0x00000040
};


#pragma pack(push, 4)
struct tDDSPixelFormat
{
	// Must be 32.
	uint32 Size;

	// See tDDSPixelFormatFlag. Flags to indicate valid fields. Uncompressed formats will usually use
	// tDDSPixelFormatFlag_RGB to indicate an RGB format, while compressed formats will use tDDSPixelFormatFlag_FourCC
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


enum tDDSCapsBasic
{
	tDDSCapsBasic_Complex		= 0x00000008,
	tDDSCapsBasic_Texture		= 0x00001000,
	tDDSCapsBasic_Mipmap		= 0x00400000
};


enum tDDSCapsExtra
{
	tDDSCapsExtra_CubeMap		= 0x00000200,
	tDDSCapsExtra_CubeMapPosX	= 0x00000400,
	tDDSCapsExtra_CubeMapNegX	= 0x00000800,
	tDDSCapsExtra_CubeMapPosY	= 0x00001000,
	tDDSCapsExtra_CubeMapNegY	= 0x00002000,
	tDDSCapsExtra_CubeMapPosZ	= 0x00004000,
	tDDSCapsExtra_CubeMapNegZ	= 0x00008000,
	tDDSCapsExtra_Volume		= 0x00200000
};


#pragma pack(push, 4)
struct tDDSCapabilities
{
	// DDS files should always include tDDSCapsBasic_Texture. If the file contains mipmaps tDDSCapsBasic_Mipmap should
	// be set. For any dds file with more than one main surface, such as a mipmap, cubic environment map, or volume
	// texture, DDSCapsBasic_Complex should also be set.
	uint32 FlagsCapsBasic;

	// For cubic environment maps tDDSCapsExtra_CubeMap should be included as well as one or more faces of the map
	// (tDDSCapsExtra_CubeMapPosX, etc). For volume textures tDDSCapsExtra_Volume should be included.
	uint32 FlagsCapsExtra;
	uint32 Unused[2];
};
#pragma pack(pop)


enum tDDSFlag
{
	tDDSFlag_Caps				= 0x00000001,	// Always included.
	tDDSFlag_Height				= 0x00000002,	// Always included. Height of largest image if mipmaps included.
	tDDSFlag_Width				= 0x00000004,	// Always included. Width of largest image if mipmaps included.
	tDDSFlag_Pitch				= 0x00000008,
	tDDSFlag_PixelFormat		= 0x00001000,	// Always included.
	tDDSFlag_MipmapCount		= 0x00020000,
	tDDSFlag_LinearSize			= 0x00080000,
	tDDSFlag_Depth				= 0x00800000
};


enum tD3DFORMAT
{
	tD3DFMT_UNKNOWN				=  0,

	tD3DFMT_R8G8B8				= 20,
	tD3DFMT_A8R8G8B8			= 21,
	tD3DFMT_X8R8G8B8			= 22,
	tD3DFMT_R5G6B5				= 23,
	tD3DFMT_X1R5G5B5			= 24,
	tD3DFMT_A1R5G5B5			= 25,
	tD3DFMT_A4R4G4B4			= 26,
	tD3DFMT_R3G3B2				= 27,
	tD3DFMT_A8					= 28,
	tD3DFMT_A8R3G3B2			= 29,
	tD3DFMT_X4R4G4B4			= 30,
	tD3DFMT_A2B10G10R10			= 31,
	tD3DFMT_A8B8G8R8			= 32,
	tD3DFMT_X8B8G8R8			= 33,
	tD3DFMT_G16R16				= 34,
	tD3DFMT_A2R10G10B10			= 35,
	tD3DFMT_A16B16G16R16		= 36,

	tD3DFMT_A8P8				= 40,
	tD3DFMT_P8					= 41,

	tD3DFMT_L8					= 50,
	tD3DFMT_A8L8				= 51,
	tD3DFMT_A4L4				= 52,

	tD3DFMT_V8U8				= 60,
	tD3DFMT_L6V5U5				= 61,
	tD3DFMT_X8L8V8U8			= 62,
	tD3DFMT_Q8W8V8U8			= 63,
	tD3DFMT_V16U16				= 64,
	tD3DFMT_A2W10V10U10			= 67,

	tD3DFMT_UYVY				= FourCC('U', 'Y', 'V', 'Y'),
	tD3DFMT_R8G8_B8G8			= FourCC('R', 'G', 'B', 'G'),
	tD3DFMT_YUY2				= FourCC('Y', 'U', 'Y', '2'),
	tD3DFMT_G8R8_G8B8			= FourCC('G', 'R', 'G', 'B'),
	tD3DFMT_DXT1				= FourCC('D', 'X', 'T', '1'),
	tD3DFMT_DXT2				= FourCC('D', 'X', 'T', '2'),
	tD3DFMT_DXT3				= FourCC('D', 'X', 'T', '3'),
	tD3DFMT_DXT4				= FourCC('D', 'X', 'T', '4'),
	tD3DFMT_DXT5				= FourCC('D', 'X', 'T', '5'),

	tD3DFMT_D16_LOCKABLE		= 70,
	tD3DFMT_D32					= 71,
	tD3DFMT_D15S1				= 73,
	tD3DFMT_D24S8				= 75,
	tD3DFMT_D24X8				= 77,
	tD3DFMT_D24X4S4				= 79,
	tD3DFMT_D16					= 80,

	tD3DFMT_D32F_LOCKABLE		= 82,
	tD3DFMT_D24FS8				= 83,

	tD3DFMT_D32_LOCKABLE		= 84,
	tD3DFMT_S8_LOCKABLE			= 85,

	tD3DFMT_L16					= 81,

	tD3DFMT_VERTEXDATA			= 100,
	tD3DFMT_INDEX16				= 101,
	tD3DFMT_INDEX32				= 102,

	tD3DFMT_Q16W16V16U16		= 110,

	tD3DFMT_MULTI2_ARGB8		= FourCC('M','E','T','1'),

	tD3DFMT_R16F				= 111,
	tD3DFMT_G16R16F				= 112,
	tD3DFMT_A16B16G16R16F		= 113,

	tD3DFMT_R32F				= 114,
	tD3DFMT_G32R32F				= 115,
	tD3DFMT_A32B32G32R32F		= 116,

	tD3DFMT_CxV8U8				= 117,

	tD3DFMT_FORCE_DWORD			= 0x7fffffff
};


// Default packing is 8 bytes but the header is 128 bytes (mult of 4), so we make it all work here.
#pragma pack(push, 4)
struct tDDSHeader
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
	tDDSPixelFormat PixelFormat;				// 32 Bytes.
	tDDSCapabilities Capabilities;				// 16 Bytes.
	uint32 UnusedB;
};


enum tDXGI_FORMAT 
{
	tDXGI_FORMAT_UNKNOWN = 0,
	tDXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
	tDXGI_FORMAT_R32G32B32A32_FLOAT = 2,
	tDXGI_FORMAT_R32G32B32A32_UINT = 3,
	tDXGI_FORMAT_R32G32B32A32_SINT = 4,
	tDXGI_FORMAT_R32G32B32_TYPELESS = 5,
	tDXGI_FORMAT_R32G32B32_FLOAT = 6,
	tDXGI_FORMAT_R32G32B32_UINT = 7,
	tDXGI_FORMAT_R32G32B32_SINT = 8,
	tDXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
	tDXGI_FORMAT_R16G16B16A16_FLOAT = 10,
	tDXGI_FORMAT_R16G16B16A16_UNORM = 11,
	tDXGI_FORMAT_R16G16B16A16_UINT = 12,
	tDXGI_FORMAT_R16G16B16A16_SNORM = 13,
	tDXGI_FORMAT_R16G16B16A16_SINT = 14,
	tDXGI_FORMAT_R32G32_TYPELESS = 15,
	tDXGI_FORMAT_R32G32_FLOAT = 16,
	tDXGI_FORMAT_R32G32_UINT = 17,
	tDXGI_FORMAT_R32G32_SINT = 18,
	tDXGI_FORMAT_R32G8X24_TYPELESS = 19,
	tDXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
	tDXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
	tDXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
	tDXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
	tDXGI_FORMAT_R10G10B10A2_UNORM = 24,
	tDXGI_FORMAT_R10G10B10A2_UINT = 25,
	tDXGI_FORMAT_R11G11B10_FLOAT = 26,
	tDXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
	tDXGI_FORMAT_R8G8B8A8_UNORM = 28,
	tDXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
	tDXGI_FORMAT_R8G8B8A8_UINT = 30,
	tDXGI_FORMAT_R8G8B8A8_SNORM = 31,
	tDXGI_FORMAT_R8G8B8A8_SINT = 32,
	tDXGI_FORMAT_R16G16_TYPELESS = 33,
	tDXGI_FORMAT_R16G16_FLOAT = 34,
	tDXGI_FORMAT_R16G16_UNORM = 35,
	tDXGI_FORMAT_R16G16_UINT = 36,
	tDXGI_FORMAT_R16G16_SNORM = 37,
	tDXGI_FORMAT_R16G16_SINT = 38,
	tDXGI_FORMAT_R32_TYPELESS = 39,
	tDXGI_FORMAT_D32_FLOAT = 40,
	tDXGI_FORMAT_R32_FLOAT = 41,
	tDXGI_FORMAT_R32_UINT = 42,
	tDXGI_FORMAT_R32_SINT = 43,
	tDXGI_FORMAT_R24G8_TYPELESS = 44,
	tDXGI_FORMAT_D24_UNORM_S8_UINT = 45,
	tDXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
	tDXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
	tDXGI_FORMAT_R8G8_TYPELESS = 48,
	tDXGI_FORMAT_R8G8_UNORM = 49,
	tDXGI_FORMAT_R8G8_UINT = 50,
	tDXGI_FORMAT_R8G8_SNORM = 51,
	tDXGI_FORMAT_R8G8_SINT = 52,
	tDXGI_FORMAT_R16_TYPELESS = 53,
	tDXGI_FORMAT_R16_FLOAT = 54,
	tDXGI_FORMAT_D16_UNORM = 55,
	tDXGI_FORMAT_R16_UNORM = 56,
	tDXGI_FORMAT_R16_UINT = 57,
	tDXGI_FORMAT_R16_SNORM = 58,
	tDXGI_FORMAT_R16_SINT = 59,
	tDXGI_FORMAT_R8_TYPELESS = 60,
	tDXGI_FORMAT_R8_UNORM = 61,
	tDXGI_FORMAT_R8_UINT = 62,
	tDXGI_FORMAT_R8_SNORM = 63,
	tDXGI_FORMAT_R8_SINT = 64,
	tDXGI_FORMAT_A8_UNORM = 65,
	tDXGI_FORMAT_R1_UNORM = 66,
	tDXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
	tDXGI_FORMAT_R8G8_B8G8_UNORM = 68,
	tDXGI_FORMAT_G8R8_G8B8_UNORM = 69,
	tDXGI_FORMAT_BC1_TYPELESS = 70,
	tDXGI_FORMAT_BC1_UNORM = 71,
	tDXGI_FORMAT_BC1_UNORM_SRGB = 72,
	tDXGI_FORMAT_BC2_TYPELESS = 73,
	tDXGI_FORMAT_BC2_UNORM = 74,
	tDXGI_FORMAT_BC2_UNORM_SRGB = 75,
	tDXGI_FORMAT_BC3_TYPELESS = 76,
	tDXGI_FORMAT_BC3_UNORM = 77,
	tDXGI_FORMAT_BC3_UNORM_SRGB = 78,
	tDXGI_FORMAT_BC4_TYPELESS = 79,
	tDXGI_FORMAT_BC4_UNORM = 80,
	tDXGI_FORMAT_BC4_SNORM = 81,
	tDXGI_FORMAT_BC5_TYPELESS = 82,
	tDXGI_FORMAT_BC5_UNORM = 83,
	tDXGI_FORMAT_BC5_SNORM = 84,
	tDXGI_FORMAT_B5G6R5_UNORM = 85,
	tDXGI_FORMAT_B5G5R5A1_UNORM = 86,
	tDXGI_FORMAT_B8G8R8A8_UNORM = 87,
	tDXGI_FORMAT_B8G8R8X8_UNORM = 88,
	tDXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
	tDXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
	tDXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
	tDXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
	tDXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
	tDXGI_FORMAT_BC6H_TYPELESS = 94,
	tDXGI_FORMAT_BC6H_UF16 = 95,
	tDXGI_FORMAT_BC6H_SF16 = 96,
	tDXGI_FORMAT_BC7_TYPELESS = 97,
	tDXGI_FORMAT_BC7_UNORM = 98,
	tDXGI_FORMAT_BC7_UNORM_SRGB = 99,
	tDXGI_FORMAT_AYUV = 100,
	tDXGI_FORMAT_Y410 = 101,
	tDXGI_FORMAT_Y416 = 102,
	tDXGI_FORMAT_NV12 = 103,
	tDXGI_FORMAT_P010 = 104,
	tDXGI_FORMAT_P016 = 105,
	tDXGI_FORMAT_420_OPAQUE = 106,
	tDXGI_FORMAT_YUY2 = 107,
	tDXGI_FORMAT_Y210 = 108,
	tDXGI_FORMAT_Y216 = 109,
	tDXGI_FORMAT_NV11 = 110,
	tDXGI_FORMAT_AI44 = 111,
	tDXGI_FORMAT_IA44 = 112,
	tDXGI_FORMAT_P8 = 113,
	tDXGI_FORMAT_A8P8 = 114,
	tDXGI_FORMAT_B4G4R4A4_UNORM = 115,
	tDXGI_FORMAT_P208 = 130,
	tDXGI_FORMAT_V208 = 131,
	tDXGI_FORMAT_V408 = 132,
	tDXGI_FORMAT_FORCE_UINT = 0xffffffff
};


enum tD3D10_RESOURCE_DIMENSION 
{
	tD3D10_RESOURCE_DIMENSION_UNKNOWN = 0,
	tD3D10_RESOURCE_DIMENSION_BUFFER = 1,
	tD3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
	tD3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
	tD3D10_RESOURCE_DIMENSION_TEXTURE3D = 4
};


enum tD3D11_RESOURCE_MISC_FLAG
{
	tD3D11_RESOURCE_MISC_GENERATE_MIPS = 0x1L,
	tD3D11_RESOURCE_MISC_SHARED = 0x2L,
	tD3D11_RESOURCE_MISC_TEXTURECUBE = 0x4L,
	tD3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS = 0x10L,
	tD3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS = 0x20L,
	tD3D11_RESOURCE_MISC_BUFFER_STRUCTURED = 0x40L,
	tD3D11_RESOURCE_MISC_RESOURCE_CLAMP = 0x80L,
	tD3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX = 0x100L,
	tD3D11_RESOURCE_MISC_GDI_COMPATIBLE = 0x200L,
	tD3D11_RESOURCE_MISC_SHARED_NTHANDLE = 0x800L,
	tD3D11_RESOURCE_MISC_RESTRICTED_CONTENT = 0x1000L,
	tD3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE = 0x2000L,
	tD3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER = 0x4000L,
	tD3D11_RESOURCE_MISC_GUARDED = 0x8000L,
	tD3D11_RESOURCE_MISC_TILE_POOL = 0x20000L,
	tD3D11_RESOURCE_MISC_TILED = 0x40000L,
	tD3D11_RESOURCE_MISC_HW_PROTECTED = 0x80000L,
	tD3D11_RESOURCE_MISC_SHARED_DISPLAYABLE,
	tD3D11_RESOURCE_MISC_SHARED_EXCLUSIVE_WRITER
};


struct tDDSHeaderDX10Ext
{
	tDXGI_FORMAT DxgiFormat;
	tD3D10_RESOURCE_DIMENSION ResourceDimension;
	uint32 MiscFlag;
    uint32 ArraySize;
    uint32 Eeserved;
};
#pragma pack(pop)


// These DXT blocks are needed so that the tImageDDS class can re-order the rows by messing with each block's lookup
// table and alpha tables. This is because DDS files have the rows of their textures upside down (texture origin in
// OpenGL is lower left, while in DirectX it is upper left). See: http://en.wikipedia.org/wiki/S3_Texture_Compression
#pragma pack(push, 1)


// This block is used for both DXT1 and DXT1 with binary alpha. It's also used as the colour information block in the
// DXT 2, 3, 4 and 5 formats. Size is 64 bits.
struct tDXT1Block
{
	uint16 Colour0;								// R5G6B5
	uint16 Colour1;								// R5G6B5
	uint8 LookupTableRows[4];
};


// This one is the same for DXT2 and 3, although we don't support 2 (premultiplied alpha). Size is 128 bits.
struct tDXT3Block
{
	uint16 AlphaTableRows[4];					// Each alpha is 4 bits.
	tDXT1Block ColourBlock;
};


// This one is the same for DXT4 and 5, although we don't support 4 (premultiplied alpha). Size is 128 bits.
struct tDXT5Block
{
	uint8 Alpha0;
	uint8 Alpha1;
	uint8 AlphaTable[6];						// Each of the 4x4 pixel entries is 3 bits.
	tDXT1Block ColourBlock;

	// These accessors are needed because of the unusual alignment of the 3bit alpha indexes. They each return or set a
	// value in [0, 2^12) which represents a single row. The row variable should be in [0, 3]
	uint16 GetAlphaRow(int row)
	{
		tAssert(row < 4);
		switch (row)
		{
			case 1:
				return (AlphaTable[2] << 4) | (0x0F & (AlphaTable[1] >> 4));

			case 0:
				return ((AlphaTable[1] & 0x0F) << 8) | AlphaTable[0];

			case 3:
				return (AlphaTable[5] << 4) | (0x0F & (AlphaTable[4] >> 4));

			case 2:
				return ((AlphaTable[4] & 0x0F) << 8) | AlphaTable[3];
		}
		return 0;
	}

	void SetAlphaRow(int row, uint16 val)
	{
		tAssert(row < 4);
		tAssert(val < 4096);
		switch (row)
		{
			case 1:
				AlphaTable[2] = val >> 4;
				AlphaTable[1] = (AlphaTable[1] & 0x0F) | ((val & 0x000F) << 4);
				break;

			case 0:
				AlphaTable[1] = (AlphaTable[1] & 0xF0) | (val >> 8);
				AlphaTable[0] = val & 0x00FF;
				break;

			case 3:
				AlphaTable[5] = val >> 4;
				AlphaTable[4] = (AlphaTable[4] & 0x0F) | ((val & 0x000F) << 4);
				break;

			case 2:
				AlphaTable[4] = (AlphaTable[4] & 0xF0) | (val >> 8);
				AlphaTable[3] = val & 0x00FF;
				break;
		}
	}
};
#pragma pack(pop)


bool tImageDDS::Load(const tString& ddsFile, uint32 loadFlags)
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
	bool success = Load(ddsData, ddsSizeBytes, loadFlags);
	delete[] ddsData;

	return success;
}


bool tImageDDS::Load(const uint8* ddsData, int ddsSizeBytes, uint32 loadFlags)
{
	Clear();

	// This will deal with zero-sized files properly as well.
	if (ddsSizeBytes < int(sizeof(tDDSHeader)+4))
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

	tDDSHeader& header = *((tDDSHeader*)ddsCurr);  ddsCurr += sizeof(header);
	tAssert(sizeof(tDDSHeader) == 124);
	const uint8* currPixelData = ddsCurr;
	if (header.Size != 124)
	{
		Results |= 1 << int(ResultCode::Fatal_IncorrectHeaderSize);
		return false;
	}

	uint32 flags = header.Flags;
	int mainWidth = header.Width;						// Main image.
	int mainHeight = header.Height;						// Main image.

	// A strictly correct dds will have linear-size xor pitch set (one, not both).
	// It seems ATI tools like GenCubeMap don't set the correct bits, but we still load
	// with a conditional success.
	int pitch = 0;										// Num bytes per line on main image (uncompressed images only).
	int linearSize = 0;									// Num bytes total main image (compressed images only).
	if (flags & tDDSFlag_Pitch)
		pitch = header.PitchLinearSize;
	if (flags & tDDSFlag_LinearSize)
		linearSize = header.PitchLinearSize;

	if ((!linearSize && !pitch) || (linearSize && pitch))
		Results |= 1 << int(ResultCode::Conditional_PitchXORLinearSize);

	// Volume textures are not supported.
	if (flags & tDDSFlag_Depth)
	{
		Results |= 1 << int(ResultCode::Fatal_VolumeTexturesNotSupported);
		return false;
	}

	// Determine the expected number of layers by looking at the mipmap count if it is supplied. We assume a single layer
	// if it is not specified.
	NumMipmapLayers = 1;
	bool hasMipmaps = (header.Capabilities.FlagsCapsBasic & tDDSCapsBasic_Mipmap) ? true : false;
	if ((flags & tDDSFlag_MipmapCount) && hasMipmaps)
		NumMipmapLayers = header.MipmapCount;

	if (NumMipmapLayers > MaxMipmapLayers)
	{
		Results |= 1 << int(ResultCode::Fatal_MaxNumMipmapLevelsExceeded);
		return false;
	}

	tDDSPixelFormat& format = header.PixelFormat;
	if (format.Size != 32)
	{
		Results |= 1 << int(ResultCode::Fatal_IncorrectPixelFormatHeaderSize);
		return false;
	}

	// Has alpha should be true if the pixel format is uncompressed (RGB) and there is an alpha channel.
	// Determine if we support the pixel format and which one it is.
	bool rgbHasAlpha = (format.Flags & tDDSPixelFormatFlag_Alpha) ? true : false;
	bool rgbFormat = (format.Flags & tDDSPixelFormatFlag_RGB) ? true : false;
	bool fourCCFormat = (format.Flags & tDDSPixelFormatFlag_FourCC) ? true : false;
	if ((!rgbFormat && !fourCCFormat) || (rgbFormat && fourCCFormat))
	{
		Results |= 1 << int(ResultCode::Fatal_IncorrectPixelFormatSpec);
		return false;
	}

	bool useDX10Ext = fourCCFormat && (format.FourCC == FourCC('D', 'X', '1', '0'));

	// Determine if this is a cubemap dds with 6 images. No need to check which images are present since they are
	// required to be all there by the dds standard. All tools these days seem to write them all. If there are complaints
	// when using legacy files we can fix this. We use FlagsCapsExtra in all cases where we are not
	// using the DX10 extension header.
	IsCubeMap = false;
	NumImages = 1;
	if (!useDX10Ext && (header.Capabilities.FlagsCapsExtra & tDDSCapsExtra_CubeMap))
	{
		IsCubeMap = true;
		NumImages = 6;
	}

	if (useDX10Ext)
	{
		tDDSHeaderDX10Ext& headerDX10 = *((tDDSHeaderDX10Ext*)ddsCurr);  ddsCurr += sizeof(tDDSHeaderDX10Ext);
		currPixelData = ddsCurr;
		if (headerDX10.ArraySize == 0)
		{
			Results |= 1 << int(ResultCode::Fatal_DX10HeaderSizeIncorrect);
			return false;
		}

		// We only handle 2D textures for now.
		if (headerDX10.ResourceDimension != tD3D10_RESOURCE_DIMENSION_TEXTURE2D)
		{
			Results |= 1 << int(ResultCode::Fatal_DX10DimensionNotSupported);
			return false;
		}

		if (headerDX10.MiscFlag & tD3D11_RESOURCE_MISC_TEXTURECUBE)
		{
			IsCubeMap = true;
			NumImages = 6;
		}

		// If we found a dx10 chunk. It must be used to determine the pixel format.
		// 
		switch (headerDX10.DxgiFormat)
		{
			case tDXGI_FORMAT_BC1_UNORM_SRGB:
				ColourSpace = tColourSpace::sRGB;
			case tDXGI_FORMAT_BC1_TYPELESS:
			case tDXGI_FORMAT_BC1_UNORM:
				PixelFormat = PixelFormatOrig = tPixelFormat::BC1_DXT1;
				break;

			case tDXGI_FORMAT_BC2_UNORM_SRGB:
				ColourSpace = tColourSpace::sRGB;
			case tDXGI_FORMAT_BC2_TYPELESS:
			case tDXGI_FORMAT_BC2_UNORM:
				PixelFormat = PixelFormatOrig = tPixelFormat::BC2_DXT3;
				break;

			case tDXGI_FORMAT_BC3_UNORM_SRGB:
				ColourSpace = tColourSpace::sRGB;
			case tDXGI_FORMAT_BC3_TYPELESS:
			case tDXGI_FORMAT_BC3_UNORM:
				PixelFormat = PixelFormatOrig = tPixelFormat::BC3_DXT5;
				break;

			case tDXGI_FORMAT_BC7_UNORM_SRGB:
				ColourSpace = tColourSpace::sRGB;
			case tDXGI_FORMAT_BC7_TYPELESS:			// Interpret typeless as BC7_UNORM... we gotta choose something.
			case tDXGI_FORMAT_BC7_UNORM:
				PixelFormat = PixelFormatOrig = tPixelFormat::BC7;
				break;

			// case tDXGI_FORMAT_BC6H_UF16:
			case tDXGI_FORMAT_BC6H_TYPELESS:		// Interpret typeless as BC6H_S16... we gotta choose something.
			case tDXGI_FORMAT_BC6H_SF16:
				PixelFormat = PixelFormatOrig = tPixelFormat::BC6H_S16;
				ColourSpace = tColourSpace::Linear;
				break;
		}
	}
	else if (fourCCFormat)
	{
		switch (format.FourCC)
		{
			case FourCC('D','X','T','1'):
				// Note that during inspecition of the individual layer data, the DXT1 pixel format might be modified
				// to DXT1BA (binary alpha).
				PixelFormat = PixelFormatOrig = tPixelFormat::BC1_DXT1;
				break;

			case FourCC('D','X','T','3'):
				PixelFormat = PixelFormatOrig = tPixelFormat::BC2_DXT3;
				break;

			case FourCC('D','X','T','5'):
				PixelFormat = PixelFormatOrig = tPixelFormat::BC3_DXT5;
				break;

			case tD3DFMT_R32F:
				// Unsupported.
				// PixelFormat = PixelFormatOrig = tPixelFormat::R32F;
				break;

			case tD3DFMT_G32R32F:
				// Unsupported.
				// PixelFormat = PixelFormatOrig = tPixelFormat::G32R32F;
				break;

			case tD3DFMT_A32B32G32R32F:
				// Unsupported.
				// PixelFormat = PixelFormatOrig = tPixelFormat::A32B32G32R32F;
				break;
		}
	}

	// It must be an RGB format.
	else
	{
		// Remember this is a little endian machine, so the masks are lying. Eg. 0xFF0000 in memory is 00 00 FF, so the red is last.
		bool   hasA = rgbHasAlpha;
		uint32 mskA = format.MaskAlpha; uint32 mskR = format.MaskRed; uint32 mskG = format.MaskGreen; uint32 mskB = format.MaskBlue;
		switch (format.RGBBitCount)
		{
			case 16:		// Supports G3B5A1R5G2, G4B4A4R4, and G3B5R5G3.
				if (hasA && (mskA == 0x8000) && (mskR == 0x7C00) && (mskG == 0x03E0) && (mskB == 0x001F))
					PixelFormat = PixelFormatOrig = tPixelFormat::G3B5A1R5G2;
				else if (hasA && (mskA == 0xF000) && (mskR == 0x0F00) && (mskG == 0x00F0) && (mskB == 0x000F))
					PixelFormat = PixelFormatOrig = tPixelFormat::G4B4A4R4;
				else if (!hasA && (mskR == 0xF800) && (mskG == 0x07E0) && (mskB == 0x001F))
					PixelFormat = PixelFormatOrig = tPixelFormat::G3B5R5G3;
				break;

			case 24:		// Supports B8G8R8.
				if (!hasA && (mskR == 0xFF0000) && (mskG == 0x00FF00) && (mskB == 0x0000FF))
					PixelFormat = PixelFormatOrig = tPixelFormat::B8G8R8;
				break;

			case 32:		// Supports B8G8R8A8. This is a little endian machine so the masks are lying. 0xFF000000 in memory is 00 00 00 FF with alpha last.
				if (hasA && (mskA == 0xFF000000) && (mskR == 0x00FF0000) && (mskG == 0x0000FF00) && (mskB == 0x000000FF))
					PixelFormat = PixelFormatOrig = tPixelFormat::B8G8R8A8;
				break;
		}
	}

	// @todo We do not yet support these formats.
	if (PixelFormat == tPixelFormat::Invalid)
	{
		Results |= 1 << int(ResultCode::Fatal_UnsupportedPixelFormat);
		return false;
	}

	// From now on we should just be using the PixelFormat to decide what to do next.
	tAssert(PixelFormat != tPixelFormat::Invalid);
	if (!rgbFormat && ((mainWidth%4) || (mainHeight%4)))
	{
		Results |= 1 << int(ResultCode::Fatal_BCDimensionsNotDivisibleByFour);
		return false;
	}

	bool reverseRowOrderRequested = loadFlags & LoadFlag_ReverseRowOrder;
	RowReversalOperationPerformed = false;

	for (int image = 0; image < NumImages; image++)
	{
		int width = mainWidth;
		int height = mainHeight;
		for (int layer = 0; layer < NumMipmapLayers; layer++)
		{
			int numBytes = 0;
			if (tImage::tIsNormalFormat(PixelFormat))
			{
				numBytes = width*height*tImage::tGetBitsPerPixel(PixelFormat)/8;

				// Deal with reversing row order for RGB formats.
				if (reverseRowOrderRequested)
				{
					uint8* reversedPixelData = CreateReversedRowData_Normal(currPixelData, PixelFormat, width, height);
					if (reversedPixelData)
					{
						// We can simply get the layer to steal the memory (the last true arg).
						MipmapLayers[layer][image] = new tLayer(PixelFormat, width, height, reversedPixelData, true);
						RowReversalOperationPerformed = true;
					}
					else
					{
						// Row reversal failed. May be a conditional success if we don't convert to RGBA 32-bit later.
						MipmapLayers[layer][image] = new tLayer(PixelFormat, width, height, (uint8*)currPixelData);
					}
				}
				else
				{
					MipmapLayers[layer][image] = new tLayer(PixelFormat, width, height, (uint8*)currPixelData);
				}
				tAssert(MipmapLayers[layer][image]->GetDataSize() == numBytes);
			}

			else if (tImage::tIsBlockCompressedFormat(PixelFormat))
			{
				// It's a BC/DXTn format. Each block encodes a 4x4 square of pixels. DXT2,3,4,5 and BC 6,7 use 128
				// bits per block.  DXT1 and DXT1BA (BC1) use 64bits per block.
				int bcBlockSize = tImage::tGetBytesPer4x4PixelBlock(PixelFormat);
				int numBlocksW = tMath::tMax(1, (width + 3) / 4);
				int numBlocksH = tMath::tMax(1, (height + 3) / 4);
				int numBlocks = numBlocksW*numBlocksH;
				numBytes = numBlocks * bcBlockSize;

				// Here's where we possibly modify the opaque DXT1 texture to be DXT1BA if there are blocks with binary
				// transparency. We only bother checking the main layer. If it's opaque we assume all the others are too.
				if ((layer == 0) && (PixelFormat == tPixelFormat::BC1_DXT1) && DoDXT1BlocksHaveBinaryAlpha((tDXT1Block*)currPixelData, numBlocks))
					PixelFormat = PixelFormatOrig = tPixelFormat::BC1_DXT1BA;

				// DDS files store textures upside down. In the OpenGL RH coord system, the lower left of the texture
				// is the origin and consecutive rows go up. For this reason we need to read each row of blocks from
				// the top to the bottom row. We also need to flip the rows within the 4x4 block by flipping the lookup
				// tables. This should be fairly fast as there is no encoding or encoding going on. Width and height
				// will go down to 1x1, which will still use a 4x4 DXT pixel-block.
				if (reverseRowOrderRequested)
				{
					uint8* reversedPixelData = CreateReversedRowData_BC(currPixelData, PixelFormat, numBlocksW, numBlocksH);
					if (reversedPixelData)
					{
						// We can simply get the layer to steal the memory (the last true arg).
						MipmapLayers[layer][image] = new tLayer(PixelFormat, width, height, reversedPixelData, true);
						RowReversalOperationPerformed = true;
					}
					else
					{
						MipmapLayers[layer][image] = new tLayer(PixelFormat, width, height, (uint8*)currPixelData);
					}
				}
				else
				{
					// If reverseRowOrder is false we want the data to go straight in so we use the currPixelData directly.
					MipmapLayers[layer][image] = new tLayer(PixelFormat, width, height, (uint8*)currPixelData);
				}
				tAssert(MipmapLayers[layer][image]->GetDataSize() == numBytes);
			}
			else
			{
				// Upsupported pixel format.
				Clear();
				Results |= 1 << int(ResultCode::Fatal_UnsupportedPixelFormat);
				return false;
			}

			currPixelData += numBytes;
			width /= 2;
			if (width < 1)
				width = 1;

			height /= 2;
			if (height < 1)
				height = 1;
		}
	}

	// Decode to 32-bit RGBA if requested.
	if (loadFlags & LoadFlag_Decode)
	{
		for (int image = 0; image < NumImages; image++)
		{
			for (int layerNum = 0; layerNum < NumMipmapLayers; layerNum++)
			{
				tLayer* layer = MipmapLayers[layerNum][image];
				int w = layer->Width;
				int h = layer->Height;
				uint8* src = layer->Data;

				if (tImage::tIsNormalFormat(PixelFormat))
				{
					// @todo Convert/decode normal formats. Easier than the BC ones.
				}
				else if (tImage::tIsBlockCompressedFormat(PixelFormat))
				{
					// We need extra room because the decompressor (bcdec) does not take an input for
					// the width and height, only the pitch (bytes per row). This means a texture that is 5
					// high will actually have row 6, 7, 8 written to.
					int wextra = w + ((w%4) ? 4-(w%4) : 0);
					int hextra = h + ((h%4) ? 4-(h%4) : 0);
					tPixel* uncompData = new tPixel[wextra*hextra];
					switch (layer->PixelFormat)
					{
						case tPixelFormat::BC1_DXT1:
						case tPixelFormat::BC1_DXT1BA:
						{
							for (int i = 0; i < h; i += 4)
								for (int j = 0; j < w; j += 4)
								{
									uint8* dst = (uint8*)uncompData + (i * w + j) * 4;

									// At first didn't understand the pitch (3rd) argument. It's cuz the block needs to be
									// written into multiple rows of the destination... and we need to know how far to get to the
									// next row for each pixel.
									bcdec_bc1(src, dst, w * 4);
									src += BCDEC_BC1_BLOCK_SIZE;
								}
							
							break;
						}

						case tPixelFormat::BC7:
						{
							for (int i = 0; i < h; i += 4)
								for (int j = 0; j < w; j += 4)
								{
									uint8* dst = (uint8*)uncompData + (i * w + j) * 4;
									bcdec_bc7(src, dst, w * 4);
									src += BCDEC_BC7_BLOCK_SIZE;
								}
							break;
						}

						case tPixelFormat::BC6H_S16:
						{
							// This HDR format decompresses to RGB floats.
							tColour3f* rgbData = new tColour3f[wextra*hextra];

							for (int i = 0; i < h; i += 4)
								for (int j = 0; j < w; j += 4)
								{
									uint8* dst = (uint8*)((float*)rgbData + (i * w + j) * 3);
									bcdec_bc6h_float(src, dst, w * 3, true);
									src += BCDEC_BC6H_BLOCK_SIZE;
								}

							// Now convert to 32-bit RGBA with 255 alpha.
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4f col(rgbData[ij], 1.0f);
								if (loadFlags & LoadFlag_GammaCorrectHDR)
									col.ToGammaSpaceApprox();
								uncompData[ij].Set(col);
							}
							delete[] rgbData;
							break;
						}

						default:
							delete[] uncompData;
							Clear();
							Results |= 1 << int(ResultCode::Fatal_BlockDecodeError);
							return false;
					}

					// Decode worked. We are now in RGBA 32-bit. Other params like width and height are already correct.
					delete[] layer->Data;
					layer->Data = (uint8*)uncompData;
					layer->PixelFormat = tPixelFormat::B8G8R8A8;
				}
				else // Unsupported PixelFormat
				{
					// ASTC Would fall in this category. It's neither a BC format or a normal RGB format.
				}

				// We've got one more chance to reverse the rows here (if we still need to) because we were asked to decode.
				if (reverseRowOrderRequested && !RowReversalOperationPerformed && (layer->PixelFormat == tPixelFormat::B8G8R8A8))
				{
					// This shouldn't ever fail. Too easy to reverse RGBA 32-bit.
					uint8* reversedRowData = CreateReversedRowData_Normal(layer->Data, layer->PixelFormat, w, h);
					tAssert(reversedRowData);
					delete[] layer->Data;
					layer->Data = reversedRowData;
					RowReversalOperationPerformed = true;
				}
			}
		}

		// All images decoded. Can now set the object's pixel format. We do _not_ set the PixelFormatOrig here!
		PixelFormat = tPixelFormat::B8G8R8A8;
	}

	tAssert(IsValid());
	return true;
}


uint8* tImageDDS::CreateReversedRowData_Normal(const uint8* pixelData, tPixelFormat pixelDataFormat, int width, int height)
{
	// We only support pixel formats that contain a whole number of bytes per pixel. That will cover
	// all reasonable RGB and RGBA formats.
	int bitsPerPixel = tImage::tGetBitsPerPixel(pixelDataFormat);
	if (bitsPerPixel % 8)
		return nullptr;
	int bytesPerPixel = bitsPerPixel/8;
	int numBytes = width*height*bytesPerPixel;

	uint8* reversedPixelData = new uint8[numBytes];
	uint8* dstData = reversedPixelData;
	for (int row = height-1; row >= 0; row--)
	{
		for (int col = 0; col < width; col++)
		{
			const uint8* srcData = pixelData + row*bytesPerPixel*width + col*bytesPerPixel;
			for (int byte = 0; byte < bytesPerPixel; byte++, dstData++, srcData++)
				*dstData = *srcData;
		}
	}
	return reversedPixelData;
}


uint8* tImageDDS::CreateReversedRowData_BC(const uint8* pixelData, tPixelFormat pixelDataFormat, int numBlocksW, int numBlocksH)
{
	if ((pixelDataFormat == tPixelFormat::BC7) || (pixelDataFormat == tPixelFormat::BC6H_S16))
		return nullptr;

	int bcBlockSize = tImage::tGetBytesPer4x4PixelBlock(pixelDataFormat);
	int numBlocks = numBlocksW*numBlocksH;
	int numBytes = numBlocks * bcBlockSize;

	uint8* reversedPixelData = new uint8[numBytes];
	uint8* dstData = reversedPixelData;
	for (int row = numBlocksH-1; row >= 0; row--)
	{
		for (int col = 0; col < numBlocksW; col++)
		{
			const uint8* srcData = pixelData + row*bcBlockSize*numBlocksW + col*bcBlockSize;
			for (int byte = 0; byte < bcBlockSize; byte++, dstData++, srcData++)
				*dstData = *srcData;
		}
	}

	// Now we flip the inter-block rows by messing with the block's lookup-table.  We need to handle all
	// three types of blocks: 1) DXT1, DXT1BA  2) DXT2, DXT3  3) DXT4, DXT5
	switch (pixelDataFormat)
	{
		case tPixelFormat::BC1_DXT1BA:
		case tPixelFormat::BC1_DXT1:
		{
			tDXT1Block* block = (tDXT1Block*)reversedPixelData;
			for (int b = 0; b < numBlocks; b++, block++)
			{
				// Reorder each row's colour indexes.
				tStd::tSwap(block->LookupTableRows[0], block->LookupTableRows[3]);
				tStd::tSwap(block->LookupTableRows[1], block->LookupTableRows[2]);
			}
			break;
		}

		case tPixelFormat::BC2_DXT3:
		{
			tDXT3Block* block = (tDXT3Block*)reversedPixelData;
			for (int b = 0; b < numBlocks; b++, block++)
			{
				// Reorder the explicit alphas AND the colour indexes.
				tStd::tSwap(block->AlphaTableRows[0], block->AlphaTableRows[3]);
				tStd::tSwap(block->AlphaTableRows[1], block->AlphaTableRows[2]);
				tStd::tSwap(block->ColourBlock.LookupTableRows[0], block->ColourBlock.LookupTableRows[3]);
				tStd::tSwap(block->ColourBlock.LookupTableRows[1], block->ColourBlock.LookupTableRows[2]);
			}
			break;
		}

		case tPixelFormat::BC3_DXT5:
		{
			tDXT5Block* block = (tDXT5Block*)reversedPixelData;
			for (int b = 0; b < numBlocks; b++, block++)
			{
				// Reorder the alpha indexes AND the colour indexes.
				uint16 orig0 = block->GetAlphaRow(0);
				block->SetAlphaRow(0, block->GetAlphaRow(3));
				block->SetAlphaRow(3, orig0);

				uint16 orig1 = block->GetAlphaRow(1);
				block->SetAlphaRow(1, block->GetAlphaRow(2));
				block->SetAlphaRow(2, orig1);

				tStd::tSwap(block->ColourBlock.LookupTableRows[0], block->ColourBlock.LookupTableRows[3]);
				tStd::tSwap(block->ColourBlock.LookupTableRows[1], block->ColourBlock.LookupTableRows[2]);
			}
			break;
		}

		default:
			// We should not get here. Should have early returned already.
			delete[] reversedPixelData;
			return nullptr;
	}

	return reversedPixelData;
}


bool tImageDDS::DoDXT1BlocksHaveBinaryAlpha(tDXT1Block* block, int numBlocks)
{
	// The only way to check if the DXT1 format has alpha is by checking each block individually. If the block uses
	// alpha, the min and max colours are ordered in a particular order.
	for (int b = 0; b < numBlocks; b++)
	{
		if (block->Colour0 <= block->Colour1)
		{
			// OK, well, that's annoying. It seems that at least the nVidia DXT compressor can generate an opaque DXT1
			// block with the colours in the order for a transparent one. This forces us to check all the indexes to
			// see if the alpha index (11 in binary) is used -- if not then it's still an opaque block.
			for (int row = 0; row < 4; row++)
			{
				uint8 bits = block->LookupTableRows[row];
				if
				(
					((bits & 0x03) == 0x03) ||
					((bits & 0x0C) == 0x0C) ||
					((bits & 0x30) == 0x30) ||
					((bits & 0xC0) == 0xC0)
				)
					return true;
			}
		}

		block++;
	}

	return false;
}


const char* tImageDDS::GetResultDesc(ResultCode code)
{
	return ResultDescriptions[int(code)];
}


const char* tImageDDS::ResultDescriptions[] =
{
	"Success",
	"Conditional Success. Image rows could not be flipped.",
	"Conditional Success. Only one of Pitch or LinearSize should be specified. Using dimensions instead.",
	"Fatal Error. Load not called. Invalid object.",
	"Fatal Error. File does not exist.",
	"Fatal Error. Incorrect file type. Must be a DDS file.",
	"Fatal Error. Filesize incorrect.",
	"Fatal Error. Magic FourCC Incorrect.",
	"Fatal Error. Incorrect DDS header size.",
	"Fatal Error. DDS volume textures not supported.",
	"Fatal Error. Pixel format header size incorrect.",
	"Fatal Error. Pixel format specification incorrect.",
	"Fatal Error. Unsupported pixel format.",
	"Fatal Error. Incorrect BC data size.",
	"Fatal Error. BC texture dimensions not divisible by 4.",
	"Fatal Error. Maximum number of mipmap levels exceeded.",
	"Fatal Error. Unable to decode BC pixels.",
	"Fatal Error. DX10 header size incorrect.",
	"Fatal Error. DX10 resource dimension not supported. 2D support only."
};
tStaticAssert(tNumElements(tImageDDS::ResultDescriptions) == int(tImageDDS::ResultCode::NumCodes));
tStaticAssert(int(tImageDDS::ResultCode::NumCodes) <= int(tImageDDS::ResultCode::MaxCodes));


}
