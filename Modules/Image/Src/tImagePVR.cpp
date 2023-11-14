// tImagePVR.cpp
//
// This class knows how to load PowerVR (.pvr) files. It knows the details of the pvr file format and loads the data
// into tLayers, optionally decompressing them. Saving is not implemented yet. The layers may be 'stolen' from a
// tImagePVR so that excessive memcpys are avoided. After they are stolen the tImagePVR is invalid. The tImagePVR
// class supports V1, V2, and V3 pvr files.
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
	tStaticAssert(sizeof(HeaderV1) == 44);

	struct HeaderV2
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
		uint32 FourCC;
		uint32 NumSurfaces;
	};
	tStaticAssert(sizeof(HeaderV2) == 52);

	struct HeaderV3
	{
		uint32 FourCCVersion;	// 'PVR3' for V3. LE = 0x03525650.
		uint32 Flags;
		uint64 PixelFormat;
		uint32 ColourSpace;		// 0 = Linear RGB. 1 = sRGB (I assume linear alpha for both).
		uint32 ChannelType;
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

	int DetermineVersionFromFirstFourBytes(const uint8 bytes[4]);

	// If the pixel format is returned unspecified the headerFmt is not supported or was invalid.
	// In this case the returned alpha-mode is meaningless and has no specified value.
	void DeterminePixelFormatFromV1V2Header(tPixelFormat&, tAlphaMode&, uint8 headerFmt);
	void DeterminePixelFormatFromV3Header(tPixelFormat&, tAlphaMode&, uint64 headerFmt);
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


void tPVR::DeterminePixelFormatFromV1V2Header(tPixelFormat& fmt, tAlphaMode& alpha, uint8 headerFmt)
{
	fmt = tPixelFormat::Invalid;
	alpha = tAlphaMode::Normal;
	switch (headerFmt)
	{
		//			Real in memory format.						   Naming in PVR1/2 spec document. [Naming in PVRTexToolUI]
		case 0x00:	fmt = tPixelFormat::G4B4A4R4;		break;	// ARGB 4444 (LE Naming).
		case 0x01:	fmt = tPixelFormat::G3B5A1R5G2;		break;	// ARGB 1555 (LE Naming).
		case 0x02:	fmt = tPixelFormat::G3B5R5G3;		break;	// RGB 565 (LE Naming). This is a slightly better name than B5G56R because at least it matches the memory order if you swap the two bytes.

		case 0x04:	fmt = tPixelFormat::R8G8B8;			break; 	// RGB 888.
		case 0x05:	fmt = tPixelFormat::B8G8R8A8;		break;	// ARGB 8888.
		case 0x07:	fmt = tPixelFormat::L8;				break;	// I 8.
		case 0x08:	fmt = tPixelFormat::A8L8;			break;	// AI 88.

		case 0x0C:	fmt = tPixelFormat::PVRBPP2;		break;	// PVRTC2.
		case 0x0D:	fmt = tPixelFormat::PVRBPP4;		break;	// PVRTC4.
		case 0x10:	fmt = tPixelFormat::G4B4A4R4;		break;	// ARGB 4444 (LE Naming).
		case 0x11:	fmt = tPixelFormat::G3B5A1R5G2;		break;	// ARGB 1555 (LE Naming).
		case 0x12:	fmt = tPixelFormat::R8G8B8A8;		break;	// ARGB 8888 [R8G8B8A8].
		case 0x13:	fmt = tPixelFormat::G3B5R5G3;		break;	// RGB 565.
		case 0x15:	fmt = tPixelFormat::R8G8B8;			break;	// RGB 888.
		case 0x16:  fmt = tPixelFormat::L8;				break;	// I 8.
		case 0x17:  fmt = tPixelFormat::A8L8;			break;	// AI 88.
		case 0x18:	fmt = tPixelFormat::PVRBPP2;		break;	// PVRTC2.
		case 0x19:  fmt = tPixelFormat::PVRBPP4;		break;	// PVRTC4.
//		case 0x1A:  fmt = tPixelFormat::R8G8B8A8;		break;	// BGRA 8888 [B8G8R8A8].
		case 0x1A:  fmt = tPixelFormat::B8G8R8A8;		break;	// BGRA 8888 [B8G8R8A8].
		case 0x20:	fmt = tPixelFormat::BC1DXT1;		break;	// DXT1.
		case 0x21:	fmt = tPixelFormat::BC2DXT2DXT3;	alpha = tAlphaMode::Premultiplied;	break;	// DXT2.
		case 0x22:	fmt = tPixelFormat::BC2DXT2DXT3;	break;	// DXT3.
		case 0x23:	fmt = tPixelFormat::BC3DXT4DXT5;	alpha = tAlphaMode::Premultiplied;	break;	// DXT4.
		case 0x24:	fmt = tPixelFormat::BC3DXT4DXT5;	break;	// DXT5.
		case 0x36:	fmt = tPixelFormat::ETC1;			break;	// ETC1.

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
		case 0x40:		// A 8.
		case 0x41:		// VU 88.
		case 0x42:		// L16.
		case 0x43:		// L8.
		case 0x44:		// AL 88.
		case 0x45:		// UYVY.
		case 0x46:		// YUY2.
		default:
			break;
	}
}


void tPVR::DeterminePixelFormatFromV3Header(tPixelFormat& fmt, tAlphaMode& alpha, uint64 headerFmt64)
{
	fmt = tPixelFormat::Invalid;
	alpha = tAlphaMode::Normal;

	// For V3 files if the MS 32 bits are 0, the format is determined by the LS 32 bits.
	// If the MS 32 do bits are non zero, the MS 32 bits contain the number of bits for
	// each channel and the present channels are specified by the LS 32 bits.
	uint32 fmtMS32 = headerFmt64 >> 32;
	uint32 fmtLS32 = headerFmt64 & 0x00000000FFFFFFFF;
	if (fmtMS32 == 0)
	{
		switch (fmtLS32)
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
			case 0x0000000C:	fmt = tPixelFormat::BC4ATI1;		break;	// BC4.
			case 0x0000000D:	fmt = tPixelFormat::BC5ATI2;		break;	// BC5.
			case 0x0000000E:	fmt = tPixelFormat::BC6U;			break;	// BC6. Not sure whether signed or unsigned. Assuming unsigned.
			case 0x0000000F:	fmt = tPixelFormat::BC7;			break;	// BC7.

			case 0x00000016:	fmt = tPixelFormat::ETC2RGB;		break;	// ETC2 RGB.
			case 0x00000017:	fmt = tPixelFormat::ETC2RGBA;		break;	// ETC2 RGBA.
			case 0x00000018:	fmt = tPixelFormat::ETC2RGBA1;		break;	// ETC2 RGB A1.
			case 0x00000019:	fmt = tPixelFormat::EACR11;			break;	// EAC R11.
			case 0x0000001A:	fmt = tPixelFormat::EACRG11;		break;	// EAC RG11.

			case 0x0000001B:	fmt = tPixelFormat::ASTC4X4;		break;	// ASTC_4x4.
			case 0x0000001C:	fmt = tPixelFormat::ASTC5X4;		break;	// ASTC_5x4.
			case 0x0000001D:	fmt = tPixelFormat::ASTC5X5;		break;	// ASTC_5x5.
			case 0x0000001E:	fmt = tPixelFormat::ASTC6X5;		break;	// ASTC_6x5.
			case 0x0000001F:	fmt = tPixelFormat::ASTC6X6;		break;	// ASTC_6x6.
			case 0x00000020:	fmt = tPixelFormat::ASTC8X5;		break;	// ASTC_8x5.
			case 0x00000021:	fmt = tPixelFormat::ASTC8X6;		break;	// ASTC_8x6.
			case 0x00000022:	fmt = tPixelFormat::ASTC8X8;		break;	// ASTC_8x8.
			case 0x00000023:	fmt = tPixelFormat::ASTC10X5;		break;	// ASTC_10x5.
			case 0x00000024:	fmt = tPixelFormat::ASTC10X6;		break;	// ASTC_10x6.
			case 0x00000025:	fmt = tPixelFormat::ASTC10X8;		break;	// ASTC_10x8.
			case 0x00000026:	fmt = tPixelFormat::ASTC10X10;		break;	// ASTC_10x10.
			case 0x00000027:	fmt = tPixelFormat::ASTC12X10;		break;	// ASTC_12x10.
			case 0x00000028:	fmt = tPixelFormat::ASTC12X12;		break;	// ASTC_12x12.

			case 0x00000010:	// UYVY.
			case 0x00000011:	// YUY2.
			case 0x00000012:	// BW1bpp.
			case 0x00000013:	// R9G9B9E5 Shared Exponent.
			case 0x00000014:	// RGBG8888.
			case 0x00000015:	// GRGB8888.
			case 0x00000029:	// ASTC_3x3x3.
			case 0x0000002A:	// ASTC_4x3x3.
			case 0x0000002B:	// ASTC_4x4x3.
			case 0x0000002C:	// ASTC_4x4x4.
			case 0x0000002D:	// ASTC_5x4x4.
			case 0x0000002E:	// ASTC_5x5x4.
			case 0x0000002F:	// ASTC_5x5x5.
			case 0x00000030:	// ASTC_6x5x5.
			case 0x00000031:	// ASTC_6x6x5.
			case 0x00000032:	// ASTC_6x6x6.
			default:
				break;
		}
	}
	else
	{
		// Both the cases and the hex values below are reversed because we are on a LE platform.
		switch (fmtLS32)
		{
			case 'bgra':	// LE PVR: argb
			{
				switch (fmtMS32)
				{
					case 0x05050501:	fmt = tPixelFormat::G3B5A1R5G2;	break;	// LE PVR: A1 R5 G5 B5.
					case 0x04040404:	fmt = tPixelFormat::G4B4A4R4;	break;	// LE PVR: A4 R4 G4 B4.
				}
				break;
			}

			case 'argb':	// LE PVR: bgra
			{
				switch (fmtMS32)
				{
					case 0x08080808:	fmt = tPixelFormat::B8G8R8A8;	break;	// LE PVR: A1 R5 G5 B5.
				}
				break;
			}

			case '\0bgr':	// LE PVR: rgb0
			{
				switch (fmtMS32)
				{
					case 0x00050605:	fmt = tPixelFormat::G3B5R5G3;	break;	// LE PVR: R5 G6 B5.
				}
				break;
			}
		}

		// WIP.
		tPrintf("PVR Header pixel format 64  : 0x%08|64X\n", headerFmt64);
		tPrintf("PVR Header pixel format 32MS: 0x%08|32X\n", fmtMS32);
		tPrintf("PVR Header pixel format 32LS: 0x%08|32X\n", fmtLS32);

		char c3 = ((fmtLS32 >> 24) & 0x000000FF) ? ((fmtLS32 >> 24) & 0x000000FF) : '0';
		char c2 = ((fmtLS32 >> 16) & 0x000000FF) ? ((fmtLS32 >> 16) & 0x000000FF) : '0';
		char c1 = ((fmtLS32 >>  8) & 0x000000FF) ? ((fmtLS32 >>  8) & 0x000000FF) : '0';
		char c0 = ((fmtLS32 >>  0) & 0x000000FF) ? ((fmtLS32 >>  0) & 0x000000FF) : '0';
		tPrintf("PVR Header pixel format LS32: %c %c %c %c\n", c3, c2, c1, c0);

		char b3 = (fmtMS32 >> 24) & 0x000000FF;
		char b2 = (fmtMS32 >> 16) & 0x000000FF;
		char b1 = (fmtMS32 >>  8) & 0x000000FF;
		char b0 = (fmtMS32 >>  0) & 0x000000FF;
		tPrintf("PVR Header pixel format MS32: %d %d %d %d\n", b3, b2, b1, b0);
	}
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
	// Clear all layers no matter what they're used for.
	for (int layer = 0; layer < NumLayers; layer++)
		delete Layers[layer];

	// Now delete the layers array.
	delete[] Layers;
	Layers = nullptr;
	NumLayers = 0;

	States							= 0;		// Image will be invalid now since Valid state not set.
	PVRVersion						= 0;
	PixelFormat						= tPixelFormat::Invalid;
	PixelFormatSrc					= tPixelFormat::Invalid;
	ColourProfile					= tColourProfile::Unspecified;
	ColourProfileSrc				= tColourProfile::Unspecified;
	AlphaMode						= tAlphaMode::Unspecified;
	RowReversalOperationPerformed	= false;

	NumSurfaces						= 0;		// For storing arrays of image data.
	NumFaces						= 0;		// For cubemaps.
	NumMipmaps						= 0;

	Depth							= 0;		// Number of slices.
	Width							= 0;
	Height							= 0;
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

	tPixel* pixels = frame->GetPixels(steal);
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

	tPixel* pixels = steal ? picture.StealPixels() : picture.GetPixels();
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

	tPrintf("PVR Version: %d\n", PVRVersion);

	uint32 fourCC = 'ENON';
	tColourProfile colourProfile = tColourProfile::Unspecified;
	int channelType = 0;		// 0 = UNORM Byte.
	int metaDataSize = 0;
	const uint8* metaData = nullptr;
	const uint8* textureData = nullptr;

	switch (PVRVersion)
	{
		case 1:
		{
			tPVR::HeaderV1* header = (tPVR::HeaderV1*)pvrData;
			tPVR::DeterminePixelFormatFromV1V2Header(PixelFormatSrc, AlphaMode, header->PixelFormat);

			NumSurfaces					= 1;		// Not supported by V1 files. Default to 1 surface.
			NumFaces					= 1;
			NumMipmaps					= header->MipMapCount;
			Depth						= 1;
			Width						= header->Width;
			Height						= header->Height;

			// Flags are LE order on disk.
			uint32 flags = (header->Flags1 << 24) | (header->Flags2 << 16) | (header->Flags1 << 8);
			
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

			int bytesPerSurface			= header->SurfaceSize;
			int bitsPerPixel			= header->BitsPerPixel;

			textureData					= pvrData + header->HeaderSize;
			break;
		}

		case 2:
		{
			tPVR::HeaderV2* header = (tPVR::HeaderV2*)pvrData;
			tPVR::DeterminePixelFormatFromV1V2Header(PixelFormatSrc, AlphaMode, header->PixelFormat);

			NumSurfaces					= header->NumSurfaces;
			NumFaces					= 1;
			NumMipmaps					= header->MipMapCount;
			Depth						= 1;
			Width						= header->Width;
			Height						= header->Height;

			// Flags are LE order on disk.
			uint32 flags = (header->Flags1 << 24) | (header->Flags2 << 16) | (header->Flags1 << 8);
			
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

			int bytesPerSurface = header->SurfaceSize;
			int bitsPerPixel = header->BitsPerPixel;
			fourCC = header->FourCC;

			textureData = pvrData + header->HeaderSize;
			break;
		}

		case 3:
		{
			tPVR::HeaderV3* header = (tPVR::HeaderV3*)pvrData;
			tPVR::DeterminePixelFormatFromV3Header(PixelFormatSrc, AlphaMode, header->PixelFormat);

			NumSurfaces					= header->NumSurfaces;
			NumFaces					= header->NumFaces;
			NumMipmaps					= header->NumMipmaps;
			Depth						= header->Depth;
			Width						= header->Width;
			Height						= header->Height;

			uint32 flags = header->Flags;
			bool premultipliedAlpha		= (flags & 0x00000002) ? true : false;

			if (header->ColourSpace == 0)
				colourProfile = tColourProfile::lRGB;
			else if (header->ColourSpace == 1)
				colourProfile = tColourProfile::sRGB;
			channelType = header->ChannelType;
			metaDataSize = header->MetaDataSize;

			metaData = pvrData + sizeof(tPVR::HeaderV3);
			textureData = metaData + metaDataSize;
			break;
		}

		default:
			SetStateBit(StateBit::Fatal_UnsupportedPVRFileVersion);
			return false;
	}

	#if 0
	tPrintf("PVR Pixel Format: %s\n", tGetPixelFormatName(PixelFormatSrc));
	tPrintf("PVR fourCC: %08X (%c %c %c %c)\n", fourCC, (fourCC>>0)&0xFF, (fourCC>>8)&0xFF, (fourCC>>16)&0xFF, (fourCC>>24)&0xFF);
	tPrintf("PVR colourProfile: %s\n", tGetColourProfileShortName(colourProfile));
	tPrintf("PVR channelType: %d\n", channelType);
	tPrintf("PVR metaDataSize: %d\n", metaDataSize);

	// We need NumSurfaces*NumFaces*NumMipmaps*Depth tLayers.
	tPrintf("PVR For tLayers:\n");
	tPrintf("PVR NumSurfaces: %d\n", NumSurfaces);
	tPrintf("PVR NumFaces: %d\n", NumFaces);
	tPrintf("PVR NumMipmaps: %d\n", NumMipmaps);
	tPrintf("PVR Depth: %d\n", Depth);
	tPrintf("PVR Width: %d\n", Width);
	tPrintf("PVR Height: %d\n", Height);
	#endif

	NumLayers = NumSurfaces * NumFaces * NumMipmaps * Depth;
	if (NumLayers <= 0)
	{
		SetStateBit(StateBit::Fatal_BadHeaderData);
		return false;
	}

	// tPrintf("PVR numLayers: %d\n", NumLayers);
	Layers = new tLayer*[NumLayers];
	for (int l = 0; l < NumLayers; l++)
		Layers[l] = nullptr;

	// These return 1 for packed formats. This allows us to treat BC, ASTC, Packed, etc all the same way.
	int blockW = tGetBlockWidth(PixelFormatSrc);
	int blockH = tGetBlockWidth(PixelFormatSrc);
	int bytesPerBlock = tImage::tGetBytesPerBlock(PixelFormatSrc);
	const uint8* srcPixelData = textureData;

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
						Layers[index] = new tLayer();

						// If we were asked to decode, do so.
						if (params.Flags & LoadFlag_Decode)
						{
							// At the end of decoding _either_ decoded4i _or_ decoded4f will be valid, not both.
							// The decoded4i format used for LDR images.
							// The decoded4f format used for HDR images.
							tColour4i* decoded4i = nullptr;
							tColour4f* decoded4f = nullptr;
							DecodeResult result = DecodePixelData
							(
								PixelFormatSrc, srcPixelData, numBytes,
								width, height, decoded4i, decoded4f
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

							tAssert(decoded4f || decoded4i);

							// Lets just start with LDR.
							delete[] decoded4f; decoded4f = nullptr;
							if (decoded4i)
								Layers[index]->Set(tPixelFormat::R8G8B8A8, width, height, (uint8*)decoded4i, true);
						}

						// Otherwise no decode. Just create the layers using the same pixel format that already exists.
						else
						{
							Layers[index]->Set(PixelFormatSrc, width, height, (uint8*)srcPixelData);
						}

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
	}

	// If we were asked to decode, set the current PixelFormat to the decoded format.
	// Otherwise set the current PixelFormat to be the same as the original PixelFormatSrc.
	PixelFormat = (params.Flags & LoadFlag_Decode) ? tPixelFormat::R8G8B8A8 : PixelFormatSrc;

	// We only try to reverse rows after possible decode. If no decode it may be impossible
	// to reverse rows depending on the pixel format (unless we decode and re-encode which is lossy).
	// Since the ability to reverse rows MAY be a function of the image height (when not decoding), we
	// only reverse rows if all layers may be reversed.
	if (params.Flags & LoadFlag_ReverseRowOrder)
	{
		bool canReverseAll = true;
		for (int l = 0; l < NumLayers; l++)
		{
			if (!CanReverseRowData(Layers[l]->PixelFormat, Layers[l]->Height))
			{
				canReverseAll = false;
				break;
			}
		}

		if (canReverseAll)
		{
			// This shouldn't ever fail -- we checked first.
			for (int l = 0; l < NumLayers; l++)
			{
				uint8* reversedRowData = CreateReversedRowData(Layers[l]->Data, Layers[l]->PixelFormat, Layers[l]->Width, Layers[l]->Height);
				tAssert(reversedRowData);
				delete[] Layers[l]->Data;
				Layers[l]->Data = reversedRowData;
			}
		}
		else
		{
			SetStateBit(StateBit::Conditional_CouldNotFlipRows);
		}
	}

	SetStateBit(StateBit::Valid);
	tAssert(IsValid());
	return true;
}


int tImagePVR::LayerIdx(int surf, int face, int mip, int depth)
{
	int index = depth + mip*(Depth) + face*(NumMipmaps*Depth) + surf*(NumFaces*NumMipmaps*Depth);
	tAssert(index < NumLayers);
	return index;
}


const char* tImagePVR::GetStateDesc(StateBit state)
{
	return StateDescriptions[int(state)];
}


tFrame* tImagePVR::GetFrame(bool steal)
{
	////////////WIP
	return nullptr;
}


bool tImagePVR::StealLayers(tList<tLayer>& layers)
{
	if (!IsValid())// || IsCubemap() || (NumImages <= 0))
		return false;

	for (int layer = 0; layer < NumLayers; layer++)
	{
		layers.Append(Layers[layer]);
		Layers[layer] = nullptr;
	}

	Clear();
	return true;
}


const char* tImagePVR::StateDescriptions[] =
{
	"Valid",
	"Conditional Valid. Image rows could not be flipped.",
	"Conditional Valid. Pixel format specification ill-formed.",
	"Conditional Valid. Image has dimension not multiple of four.",
	"Conditional Valid. Image has dimension not power of two.",
	"Conditional Valid. V1 V2 Mipmap flag doesn't match mipmap count.",
	"Fatal Error. File does not exist.",
	"Fatal Error. Incorrect file type. Must be a PVR file.",
	"Fatal Error. Filesize incorrect.",
	"Fatal Error. Magic FourCC Incorrect.",
	"Fatal Error. Incorrect PVR header size.",
	"Fatal Error. Bad PVR header data.",
	"Fatal Error. Unsupported PVR file version.",
	"Fatal Error. Incorrect Dimensions.",
	"Fatal Error. Pixel format header size incorrect.",
	"Fatal Error. Pixel format specification incorrect.",
	"Fatal Error. Unsupported pixel format.",
	"Fatal Error. V1 V2 Mipmap flag doesn't match mipmap count.",
	"Fatal Error. V1 V2 Cubemap flag doesn't match map count.",
	"Fatal Error. V1 V2 Twiddled data not supported.",
	"Fatal Error. Unable to decode packed pixels.",
	"Fatal Error. Unable to decode BC pixels.",
	"Fatal Error. Unable to decode PVR pixels.",
	"Fatal Error. Unable to decode ASTC pixels."
};
tStaticAssert(tNumElements(tImagePVR::StateDescriptions) == int(tImagePVR::StateBit::NumStateBits));
tStaticAssert(int(tImagePVR::StateBit::NumStateBits) <= int(tImagePVR::StateBit::MaxStateBits));


}
