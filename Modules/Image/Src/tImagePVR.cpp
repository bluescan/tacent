// tImagePVR.cpp
//
// This class knows how to load PowerVR (.pvr) files. It knows the details of the pvr file format and loads
// the data into tLayers, optionally decompressing them. Saving is not implemented yet. The layers may be 'stolen' from
// a tImagePVR so that excessive memcpys are avoided. After they are stolen the tImagePVR is invalid.
//
// Copyright (c) 2023 Tristan Grimmer.
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
	// There are 3 possible headers for V1, V2, and V3 PVR files. V1 and V2 are very similar, but just for clarity we
	// explicitly define the 3 different headers as their own struct.
	#pragma pack(push, 1)
	struct HeaderV1
	{
		uint32 HeaderSize;
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
	};

	struct HeaderV2
	{
		uint32 HeaderSize;		// 44 For V1, 52 for V2.
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
		uint32 FourCC;
		uint32 NumSurfaces;
	};

	struct HeaderV3
	{
		uint32 FourCCVersion;	// 'PVR3' for V3. LE = 0x03525650.
		uint32 Flags;
		uint64 PixelFormat;
		uint32 ColourSpace;
		uint32 ChannelType;
		uint32 Height;
		uint32 Width;
		uint32 Depth;
		uint32 NumSurfaces;
		uint32 NumFaces;
		uint32 NumMipmaps;
		uint32 MetaDataSize;
	};
	#pragma pack(pop)

	bool DeterminePixelFormatFromV1V2Header(tPixelFormat&, tAlphaMode&, uint8 headerFmt);
	bool DeterminePixelFormatFromV3Header(tPixelFormat&, tAlphaMode&, uint64 headerFmt);
}


tImagePVR::tImagePVR()
{
}


tImagePVR::tImagePVR(const tString& pvrFile, const LoadParams& loadParams) :
	Filename(pvrFile)
{
	Load(pvrFile, loadParams);
}


tImagePVR::tImagePVR(const uint8* pvrFileInMemory, int numBytes, const LoadParams& loadParams)
{
	Load(pvrFileInMemory, numBytes, loadParams);
}


void tImagePVR::Clear()
{
	Results							= 0;							// This means no results.
	PixelFormat						= tPixelFormat::Invalid;
	PixelFormatSrc					= tPixelFormat::Invalid;
	ColourProfile					= tColourProfile::Unspecified;
	ColourProfileSrc				= tColourProfile::Unspecified;
	AlphaMode						= tAlphaMode::Unspecified;
	RowReversalOperationPerformed	= false;
}


bool tImagePVR::Set(tPixel* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	PixelFormat						= tPixelFormat::R8G8B8A8;
	PixelFormatSrc					= tPixelFormat::R8G8B8A8;
	ColourProfile					= tColourProfile::LDRsRGB_LDRlA;
	ColourProfileSrc				= tColourProfile::LDRsRGB_LDRlA;
	AlphaMode						= tAlphaMode::Normal;
	RowReversalOperationPerformed	= false;

	return true;
}


bool tImagePVR::Set(tFrame* frame, bool steal)
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


bool tImagePVR::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


tFrame* tImagePVR::GetFrame(bool steal)
{
	return nullptr;
}


bool tImagePVR::Load(const tString& pvrFile, const LoadParams& loadParams)
{
	Clear();
	Filename = pvrFile;
	if (tSystem::tGetFileType(pvrFile) != tSystem::tFileType::PVR)
	{
		Results |= 1 << int(ResultCode::Fatal_IncorrectFileType);
		return false;
	}

	if (!tSystem::tFileExists(pvrFile))
	{
		Results |= 1 << int(ResultCode::Fatal_FileDoesNotExist);
		return false;
	}

	int pvrSizeBytes = 0;
	uint8* pvrData = (uint8*)tSystem::tLoadFile(pvrFile, 0, &pvrSizeBytes);
	bool success = Load(pvrData, pvrSizeBytes, loadParams);
	delete[] pvrData;

	return success;
}


int tImagePVR::DetermineVersionFromFirstFourBytes(const uint8 bytes[4])
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


bool tPVR::DeterminePixelFormatFromV1V2Header(tPixelFormat& fmt, tAlphaMode& alpha, uint8 headerFmt)
{
	fmt = tPixelFormat::Invalid;
	alpha = tAlphaMode::Normal;
	switch (headerFmt)
	{
		case 0x00:	fmt = tPixelFormat::B4G4R4A4;		break;	// ARGB 4444.
		case 0x01:	fmt = tPixelFormat::B5G5R5A1;		break;	// ARGB 1555.
		case 0x02:	fmt = tPixelFormat::B5G6R5;			break;	// RGB 565.

		case 0x04:	fmt = tPixelFormat::R8G8B8;			break; 	// RGB 888.
		case 0x05:	fmt = tPixelFormat::B8G8R8A8;		break;	// ARGB 8888.
		case 0x07:	fmt = tPixelFormat::L8;				break;	// I 8.
		case 0x08:	fmt = tPixelFormat::A8L8;			break;	// AI 88.

		case 0x0C:	fmt = tPixelFormat::PVRBPP2;		break;	// PVRTC2.
		case 0x0D:	fmt = tPixelFormat::PVRBPP4;		break;	// PVRTC4.
		case 0x10:	fmt = tPixelFormat::B4G4R4A4;		break;	// ARGB 4444.
		case 0x11:	fmt = tPixelFormat::B5G5R5A1;		break;	// ARGB 1555.
		case 0x12:	fmt = tPixelFormat::B8G8R8A8;		break;	// ARGB 8888.
		case 0x13:	fmt = tPixelFormat::B5G6R5;			break;	// RGB 565.
		case 0x15:	fmt = tPixelFormat::R8G8B8;			break;	// RGB 888.
		case 0x16:  fmt = tPixelFormat::L8;				break;	// I 8.
		case 0x17:  fmt = tPixelFormat::A8L8;			break;	// AI 88.
		case 0x18:	fmt = tPixelFormat::PVRBPP2;		break;	// PVRTC2.
		case 0x19:  fmt = tPixelFormat::PVRBPP4;		break;	// PVRTC4.
		case 0x1A:  fmt = tPixelFormat::R8G8B8A8;		break;	// BGRA 8888.
		case 0x20:	fmt = tPixelFormat::BC1DXT1;		break;	// DXT1.
		case 0x21:	fmt = tPixelFormat::BC2DXT2DXT3;	alpha = tAlphaMode::Premultiplied;	break;	// DXT2.
		case 0x22:	fmt = tPixelFormat::BC2DXT2DXT3;	break;	// DXT3.
		case 0x23:	fmt = tPixelFormat::BC3DXT4DXT5;	alpha = tAlphaMode::Premultiplied;	break;	// DXT4.
		case 0x24:	fmt = tPixelFormat::BC3DXT4DXT5;	break;	// DXT5.

		case 0x03:		// RGB 555.
		case 0x06:		// ARGB 8332.
		case 0x09:		// 1BPP.
		case 0x0A:		// (V,Y1,U,Y0).
		case 0x0B:		// (Y1,V,Y0,U).
		case 0x14:		// RGB 555.
		case 0x25:		// RGB 332.
		case 0x26:		// AL 44.
		case 0x27:		// LVU 655.
		case 0x28:		// XLVU 8888.
		case 0x29:		// QWVU 8888.
		case 0x2A:		// ABGR 2101010	02 10 10 10 (32 total).
		case 0x2B:		// ARGB 2101010	02 10 10 10 (32 total).
		case 0x2C:		// AWVU 2101010	02 10 10 10 (32 total).
		case 0x2D:		// GR 1616.
		case 0x2E:		// VU 1616.
		case 0x2F:		// ABGR 16161616.
		case 0x30:		// R 16F.
		case 0x31:		// GR 1616F.
		case 0x32:		// ABGR 16161616F.
		case 0x33:		// R 32F.
		case 0x34:		// GR 3232F.
		case 0x35:		// ABGR 32323232F.
		case 0x36:		// ETC.
		case 0x40:		// A 8.
		case 0x41:		// VU 88.
		case 0x42:		// L16.
		case 0x43:		// L8.
		case 0x44:		// AL 88.
		case 0x45:		// UYVY.
		case 0x46:		// YUY2.
		default:
			alpha = tAlphaMode::Unspecified;
			return false;
	}

	return true;
}


bool tPVR::DeterminePixelFormatFromV3Header(tPixelFormat& fmt, tAlphaMode& alpha, uint64 headerFmt64)
{
	fmt = tPixelFormat::Invalid;
	alpha = tAlphaMode::Normal;

	// For V3 files the MS 32 bits are always 0.
	uint32 headerFmt = headerFmt64 & 0x00000000FFFFFFFF;

	switch (headerFmt)
	{
		// PVR stores alpha on a per-block basis, not the entire image. Images without alpha just happen
		// to have all opaque blocks. In either case, the pixel format is the same -- PVRBPP2 or PVRBPP4.
		case 0x00000000:	fmt = tPixelFormat::PVRBPP2;		break;	// PVRTC 2bpp RGB.
		case 0x00000001:	fmt = tPixelFormat::PVRBPP2;		break;	// PVRTC 2bpp RGBA.
		case 0x00000002:	fmt = tPixelFormat::PVRBPP4;		break;	// PVRTC 4bpp RGB.
		case 0x00000003:	fmt = tPixelFormat::PVRBPP4;		break;	// PVRTC 4bpp RGBA.
		case 0x00000004:	fmt = tPixelFormat::PVR2BPP2;		break;	// PVRTC-II 2bpp.
		case 0x00000005:	fmt = tPixelFormat::PVR2BPP4;		break;	// PVRTC-II 4bpp.
		case 0x00000006:	fmt = tPixelFormat::ETC1;			break;	// ETC1.

		case 0x00000007:	fmt = tPixelFormat::BC1DXT1;		break;	// DXT1. BC1.
		case 0x00000008:	fmt = tPixelFormat::BC2DXT2DXT3;	alpha = tAlphaMode::Premultiplied;	break;	// DXT2.
		case 0x00000009:	fmt = tPixelFormat::BC2DXT2DXT3;	break;	// DXT3. BC2.
		case 0x0000000A:	fmt = tPixelFormat::BC3DXT4DXT5;	alpha = tAlphaMode::Premultiplied;	break;	// DXT4.
		case 0x0000000B:	fmt = tPixelFormat::BC3DXT4DXT5;	break;	// DXT5. BC3.

	}
#if 0
BC4 12
BC5 13
BC6 14
BC7 15
UYVY 16
YUY2 17
BW1bpp 18
R9G9B9E5 Shared Exponent 19
RGBG8888 20
GRGB8888 21
ETC2 RGB 22
ETC2 RGBA 23
ETC2 RGB A1 24
EAC R11 25
EAC RG11 26
ASTC_4x4 27
ASTC_5x4 28
ASTC_5x5 29
ASTC_6x5 30
ASTC_6x6 31
ASTC_8x5 32
ASTC_8x6 33
ASTC_8x8 34
ASTC_10x5 35
ASTC_10x6 36
ASTC_10x8 37
ASTC_10x10 38
ASTC_12x10 39
ASTC_12x12 40
ASTC_3x3x3 41
ASTC_4x3x3 42
ASTC_4x4x3 43
ASTC_4x4x4 44
ASTC_5x4x4 45
ASTC_5x5x4 46
ASTC_5x5x5 47
ASTC_6x5x5 48
ASTC_6x6x5 49
ASTC_6x6x6 50
#endif
	return true;
}


bool tImagePVR::Load(const uint8* pvrData, int pvrDataSize, const LoadParams& paramsIn)
{
	Clear();
	PVRVersion = DetermineVersionFromFirstFourBytes(pvrData);
	if (PVRVersion == 0)
		return false;

	tPrintf("PVR Version: %d\n", PVRVersion);

	int height = 0;
	int width = 0;
	int mipmapCount = 0;
	uint8 flagsV12A = 0;
	uint8 flagsV12B = 0;
	uint8 flagsV12C = 0;
	uint32 flagsV3 = 0;
	int bytesPerSurface = 0;
	int bitsPerPixel = 0;
	uint32 redMask = 0;
	uint32 grnMask = 0;
	uint32 bluMask = 0;
	uint32 alpMask = 0;
	uint32 fourCC = 0;
	int numSurfaces = 1;

	switch (PVRVersion)
	{
		case 1:
		{
			tPVR::HeaderV1* header = (tPVR::HeaderV1*)pvrData;
			tPrintf("PVR Header pixel format: %d\n", header->PixelFormat);
			tPVR::DeterminePixelFormatFromV1V2Header(PixelFormatSrc, AlphaMode, header->PixelFormat);
			tPrintf("PVR Pixel Format: %s\n", tGetPixelFormatName(PixelFormatSrc));

			height = header->Height;
			width = header->Width;
			mipmapCount = header->MipMapCount;
			flagsV12A = header->Flags1;
			flagsV12B = header->Flags2;
			flagsV12C = header->Flags3;
			bytesPerSurface = header->SurfaceSize;
			bitsPerPixel = header->BitsPerPixel;
			redMask = header->RedMask;
			grnMask = header->GreenMask;
			bluMask = header->BlueMask;
			alpMask = header->AlphaMask;
			break;
		}

		case 2:
		{
			tPVR::HeaderV2* header = (tPVR::HeaderV2*)pvrData;
			tPrintf("PVR Header pixel format: %d\n", header->PixelFormat);
			tPVR::DeterminePixelFormatFromV1V2Header(PixelFormatSrc, AlphaMode, header->PixelFormat);
			tPrintf("PVR Pixel Format: %s\n", tGetPixelFormatName(PixelFormatSrc));

			height = header->Height;
			width = header->Width;
			mipmapCount = header->MipMapCount;
			flagsV12A = header->Flags1;
			flagsV12B = header->Flags2;
			flagsV12C = header->Flags3;
			bytesPerSurface = header->SurfaceSize;
			bitsPerPixel = header->BitsPerPixel;
			redMask = header->RedMask;
			grnMask = header->GreenMask;
			bluMask = header->BlueMask;
			alpMask = header->AlphaMask;
			fourCC = header->FourCC;
			numSurfaces = header->NumSurfaces;
			break;
		}

		case 3:
		{
			tPVR::HeaderV3* header = (tPVR::HeaderV3*)pvrData;
			tPrintf("PVR Header pixel format: %d\n", header->PixelFormat);
			tPVR::DeterminePixelFormatFromV3Header(PixelFormatSrc, AlphaMode, header->PixelFormat);
			tPrintf("PVR Pixel Format: %s\n", tGetPixelFormatName(PixelFormatSrc));

			flagsV3 = header->Flags;
			break;
		}

		default:
			Results |= uint32(ResultCode::Fatal_UnsupportedPVRFileVersion);
			return false;
	}

	tPrintf("PVR height: %d\n", height);
	tPrintf("PVR width: %d\n", width);
	tPrintf("PVR mipmapCount: %d\n", mipmapCount);
	tPrintf("PVR flagsV12A: %08!1b\n", flagsV12A);
	tPrintf("PVR flagsV12B: %08!1b\n", flagsV12B);
	tPrintf("PVR flagsV12C: %08!1b\n", flagsV12C);
	tPrintf("PVR flagsV3: %08!4b\n", flagsV3);	
	tPrintf("PVR bytesPerSurface: %d\n", bytesPerSurface);
	tPrintf("PVR bitsPerPixel: %d\n", bitsPerPixel);
	tPrintf("PVR redMask: %08!4b\n", redMask);
	tPrintf("PVR grnMask: %08!4b\n", grnMask);
	tPrintf("PVR bluMask: %08!4b\n", bluMask);
	tPrintf("PVR alpMask: %08!4b\n", alpMask);
	tPrintf("PVR fourCC: %08X (%c %c %c %c)\n", fourCC, (fourCC>>0)&0xFF, (fourCC>>8)&0xFF, (fourCC>>16)&0xFF, (fourCC>>24)&0xFF);
	tPrintf("PVR numSurfaces: %d\n", numSurfaces);

	return false;
}


const char* tImagePVR::GetResultDesc(ResultCode code)
{
	return ResultDescriptions[int(code)];
}


const char* tImagePVR::ResultDescriptions[] =
{
	"Success",
	"Conditional Success. Image rows could not be flipped.",
	"Conditional Success. Pixel format specification ill-formed.",
	"Conditional Success. Image has dimension not multiple of four.",
	"Conditional Success. Image has dimension not power of two.",
	"Fatal Error. File does not exist.",
	"Fatal Error. Incorrect file type. Must be a PVR file.",
	"Fatal Error. Filesize incorrect.",
	"Fatal Error. Magic FourCC Incorrect.",
	"Fatal Error. Incorrect PVR header size.",
	"Fatal Error. Unsupported PVR file version.",
	"Fatal Error. Incorrect Dimensions.",
	"Fatal Error. Pixel format header size incorrect.",
	"Fatal Error. Pixel format specification incorrect.",
	"Fatal Error. Unsupported pixel format.",
	"Fatal Error. Maximum number of mipmap levels exceeded.",
	"Fatal Error. Unable to decode packed pixels.",
	"Fatal Error. Unable to decode BC pixels.",
	"Fatal Error. Unable to decode PVR pixels.",
	"Fatal Error. Unable to decode ASTC pixels."
};
tStaticAssert(tNumElements(tImagePVR::ResultDescriptions) == int(tImagePVR::ResultCode::NumCodes));
tStaticAssert(int(tImagePVR::ResultCode::NumCodes) <= int(tImagePVR::ResultCode::MaxCodes));


}
