// tImageASTC.cpp
//
// This class knows how to load and save ARM's Adaptive Scalable Texture Compression (.astc) files. The pixel data is
// stored in a tLayer. If decode was requested the layer will store raw pixel data. The layer may be 'stolen'. IF it
// is the tImageASTC is invalid afterwards. This is purely for performance.
//
// Copyright (c) 2022-2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <System/tFile.h>
#include <System/tMachine.h>
#include "Image/tImageASTC.h"
#include "Image/tPixelUtil.h"
#include "Image/tPicture.h"
#include "astcenc.h"
namespace tImage
{


namespace tASTC
{
	struct Header
	{
		uint8_t Magic[4];
		uint8_t BlockW;
		uint8_t BlockH;
		uint8_t BlockD;
		uint8_t DimX[3];
		uint8_t DimY[3];
		uint8_t DimZ[3];
	};
	tStaticAssert(sizeof(Header) == 16);

	tPixelFormat GetFormatFromBlockDimensions(int blockW, int blockH);
};


tPixelFormat tASTC::GetFormatFromBlockDimensions(int blockW, int blockH)
{
	int pixelsPerBlock = blockW * blockH;
	switch (pixelsPerBlock)
	{
		case 16:	return tPixelFormat::ASTC4X4;
		case 20:	return tPixelFormat::ASTC5X4;
		case 25:	return tPixelFormat::ASTC5X5;
		case 30:	return tPixelFormat::ASTC6X5;
		case 36:	return tPixelFormat::ASTC6X6;
		case 40:	return tPixelFormat::ASTC8X5;
		case 48:	return tPixelFormat::ASTC8X6;
		case 50:	return tPixelFormat::ASTC10X5;
		case 60:	return tPixelFormat::ASTC10X6;
		case 64:	return tPixelFormat::ASTC8X8;
		case 80:	return tPixelFormat::ASTC10X8;
		case 100:	return tPixelFormat::ASTC10X10;
		case 120:	return tPixelFormat::ASTC12X10;
		case 144:	return tPixelFormat::ASTC12X12;
	}
	return tPixelFormat::Invalid;
}


bool tImageASTC::Load(const tString& astcFile, const LoadParams& params)
{
	Clear();

	if (tSystem::tGetFileType(astcFile) != tSystem::tFileType::ASTC)
		return false;

	if (!tSystem::tFileExists(astcFile))
		return false;

	int numBytes = 0;
	uint8* astcFileInMemory = tSystem::tLoadFile(astcFile, nullptr, &numBytes);
	bool success = Load(astcFileInMemory, numBytes, params);
	delete[] astcFileInMemory;

	return success;
}


bool tImageASTC::Load(const uint8* astcInMemory, int numBytes, const LoadParams& paramsIn)
{
	Clear();

	// This will deal with zero-sized files properly as well. Basically we need
	// the header and at least one block of data. All ASTC blocks are 16 bytes.
	if (!astcInMemory || (numBytes < (sizeof(tASTC::Header)+16)))
		return false;

	LoadParams params(paramsIn);
	const uint8* astcCurr = astcInMemory;
	tASTC::Header& header = *((tASTC::Header*)astcInMemory);

	// Check the magic.
	if ((header.Magic[0] != 0x13) || (header.Magic[1] != 0xAB) || (header.Magic[2] != 0xA1) || (header.Magic[3] != 0x5C))
		return false;

	int blockW = header.BlockW;
	int blockH = header.BlockH;
	int blockD = header.BlockD;

	// We only support block depth of 1 (2D blocks) -- for now.
	if (blockD != 1)
		return false;

	int width  = header.DimX[0] + (header.DimX[1] << 8) + (header.DimX[2] << 16);
	int height = header.DimY[0] + (header.DimY[1] << 8) + (header.DimY[2] << 16);
	int depth  = header.DimZ[0] + (header.DimZ[1] << 8) + (header.DimZ[2] << 16);

	// We only support depth of 1 (2D images) -- for now.
	if ((width <= 0) || (height <= 0) || (depth > 1))
		return false;

	tPixelFormat format = tASTC::GetFormatFromBlockDimensions(blockW, blockH);
	if (!tIsASTCFormat(format))
		return false;
	PixelFormat = format;
	PixelFormatSrc = format;

	const uint8* astcData = astcInMemory + sizeof(tASTC::Header);
	int astcDataSize = numBytes - sizeof(tASTC::Header);
	tAssert(!Layer);

	// Update the colour profiles.
	ColourProfileSrc	= (params.Profile != tColourProfile::Unspecified) ? params.Profile : tColourProfile::sRGB;
	ColourProfile		= (params.Profile != tColourProfile::Unspecified) ? params.Profile : tColourProfile::sRGB;

	// If we were not asked to decode we just get the data over to the Layer and we're done.
	if (!(params.Flags & LoadFlag_Decode))
	{
		Layer = new tLayer(PixelFormat, width, height, (uint8*)astcData);
		return true;
	}

	// The gamma-compression load flags only apply when decoding. If the gamma mode is auto, we determine here
	// whether to apply sRGB compression. If the space is linear and a format that often encodes colours, we apply it.
	if (params.Flags & LoadFlag_AutoGamma)
	{
		// Clear all related flags.
		params.Flags &= ~(LoadFlag_AutoGamma | LoadFlag_SRGBCompression | LoadFlag_GammaCompression);
		if (tMath::tIsProfileLinearInRGB(params.Profile))
			params.Flags |= LoadFlag_SRGBCompression;
	}

	tColour4f* decoded4f = nullptr;
	tColour4b* decoded4b = nullptr;

	// ASTC data is always decoded into a float buffer.
	DecodeResult result = tImage::DecodePixelData_ASTC(format, astcData, astcDataSize, width, height, decoded4f);
	if (result != DecodeResult::Success)
		return false;

	// Apply any decode flags.
	tAssert(decoded4f);
	bool flagTone = (params.Flags & tImageASTC::LoadFlag_ToneMapExposure) ? true : false;
	bool flagSRGB = (params.Flags & tImageASTC::LoadFlag_SRGBCompression) ? true : false;
	bool flagGama = (params.Flags & tImageASTC::LoadFlag_GammaCompression)? true : false;
	if (flagTone || flagSRGB || flagGama)
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

	// We're decoded. Need to update the current colour profile.
	if (flagSRGB) ColourProfile = tColourProfile::sRGB;
	if (flagGama) ColourProfile = tColourProfile::gRGB;

	// Converts to RGBA32 into the decoded4b array. Cleans up the float buffer.
	tAssert(!decoded4b);
	decoded4b = new tColour4b[width*height];
	for (int p = 0; p < width*height; p++)
		decoded4b[p].Set(decoded4f[p]);
	delete[] decoded4f;

	// Give decoded4b to layer.
	tAssert(!Layer);
	Layer = new tLayer(tPixelFormat::R8G8B8A8, width, height, (uint8*)decoded4b, true);
	tAssert(Layer->OwnsData);

	// We've got one more chance to reverse the rows here (if we still need to) because we were asked to decode.
	if (params.Flags & tImageASTC::LoadFlag_ReverseRowOrder)
	{
		// This shouldn't ever fail. Too easy to reverse RGBA 32-bit.
		uint8* reversedRowData = tImage::CreateReversedRowData(Layer->Data, Layer->PixelFormat, width, height);
		tAssert(reversedRowData);
		delete[] Layer->Data;
		Layer->Data = reversedRowData;
	}

	// Update the current pixel format -- but not the source format.
	PixelFormat = tPixelFormat::R8G8B8A8;

	tAssert(IsValid());
	return true;
}


bool tImageASTC::Set(tPixel4b* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Layer = new tLayer(tPixelFormat::R8G8B8A8, width, height, (uint8*)pixels, steal);

	PixelFormatSrc		= tPixelFormat::R8G8B8A8;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume pixels must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

	return true;
}


bool tImageASTC::Set(tFrame* frame, bool steal)
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

	return true;
}


bool tImageASTC::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	PixelFormatSrc = picture.PixelFormatSrc;
	PixelFormat = tPixelFormat::R8G8B8A8;
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


tFrame* tImageASTC::GetFrame(bool steal)
{
	// Data must be decoded for this to work.
	if (!IsValid() || (PixelFormat != tPixelFormat::R8G8B8A8))
		return nullptr;

	tFrame* frame = new tFrame();
	frame->Width = Layer->Width;
	frame->Height = Layer->Height;
	frame->PixelFormatSrc = PixelFormatSrc;

	if (steal)
	{
		frame->Pixels = (tPixel4b*)Layer->StealData();
		delete Layer;
		Layer = nullptr;
	}
	else
	{
		frame->Pixels = new tPixel4b[frame->Width * frame->Height];
		tStd::tMemcpy(frame->Pixels, (tPixel4b*)Layer->Data, frame->Width * frame->Height * sizeof(tPixel4b));
	}

	return frame;
}


bool tImageASTC::IsOpaque() const
{
	if (!IsValid())
		return false;
	
	if (Layer->PixelFormat == tPixelFormat::R8G8B8A8)
	{
		tPixel4b* pixels = (tPixel4b*)Layer->Data;
		for (int p = 0; p < (Layer->Width * Layer->Height); p++)
		{
			if (pixels[p].A < 255)
				return false;
		}
	}

	return false;
}


}
