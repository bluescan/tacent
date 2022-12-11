// tImageQOI.cpp
//
// This class knows how to load and save Quite OK Images (.qoi) files into tPixel arrays. These tPixels may be 'stolen'
// by the tPicture's constructor if a targa file is specified. After the array is stolen the tImageQOI is invalid. This
// is purely for performance.
//
// Copyright (c) 2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <System/tFile.h>
#include "Image/tImageQOI.h"
#include "Image/tPicture.h"
#define QOI_NO_STDIO
#define QOI_IMPLEMENTATION
#include <QOI/qoi.h>
namespace tImage
{


bool tImageQOI::Load(const tString& qoiFile)
{
	Clear();

	if (tSystem::tGetFileType(qoiFile) != tSystem::tFileType::QOI)
		return false;

	if (!tSystem::tFileExists(qoiFile))
		return false;

	int numBytes = 0;
	uint8* qoiFileInMemory = tSystem::tLoadFile(qoiFile, nullptr, &numBytes);
	bool success = Load(qoiFileInMemory, numBytes);
	delete[] qoiFileInMemory;

	return success;
}


bool tImageQOI::Load(const uint8* qoiFileInMemory, int numBytes)
{
	Clear();
	if ((numBytes <= 0) || !qoiFileInMemory)
		return false;

	// Decode a QOI image from memory. The function either returns NULL on failure (invalid parameters or malloc failed)
	// or a pointer to the decoded pixels. On success, the qoi_desc struct is filled with the description from the file
	// header. The returned pixel data should be free()d after use.
	qoi_desc results;
	void* pixelData = qoi_decode(qoiFileInMemory, numBytes, &results, 4);
	if (!pixelData)
		return false;

	Width			= results.width;	
	Height			= results.height;
	ColourSpace		= (results.colorspace == QOI_LINEAR) ? tSpace::Linear : tSpace::sRGB;
	PixelFormatSrc	= (results.channels == 3) ? tPixelFormat::R8G8B8 : tPixelFormat::R8G8B8A8;

	tAssert((Width > 0) && (Height > 0));
	Pixels = new tPixel[Width*Height];
	tStd::tMemcpy(Pixels, pixelData, Width*Height*sizeof(tPixel));
	free(pixelData);

	return true;
}


bool tImageQOI::Set(tPixel* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Width = width;
	Height = height;

	if (steal)
	{
		Pixels = pixels;
	}
	else
	{
		Pixels = new tPixel[Width*Height];
		tStd::tMemcpy(Pixels, pixels, Width*Height*sizeof(tPixel));
	}

	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	ColourSpace = tSpace::sRGB;
	return true;
}


bool tImageQOI::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	Set(frame->GetPixels(steal), frame->Width, frame->Height, steal);
	if (steal)
		delete frame;

	return true;
}


bool tImageQOI::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


tFrame* tImageQOI::GetFrame(bool steal)
{
	if (!IsValid())
		return nullptr;

	tFrame* frame = new tFrame();
	frame->PixelFormatSrc = PixelFormatSrc;

	if (steal)
	{
		frame->StealFrom(Pixels, Width, Height);
		Pixels = nullptr;
	}
	else
	{
		frame->Set(Pixels, Width, Height);
	}

	return frame;
}


tImageQOI::tFormat tImageQOI::Save(const tString& qoiFile, tFormat format) const
{
	if (!IsValid() || (format == tFormat::Invalid))
		return tFormat::Invalid;

	if (tSystem::tGetFileType(qoiFile) != tSystem::tFileType::QOI)
		return tFormat::Invalid;

	if (format == tFormat::Auto)
	{
		if (IsOpaque())
			format = tFormat::Bit24;
		else
			format = tFormat::Bit32;
	}

	tFileHandle file = tSystem::tOpenFile(qoiFile.Chr(), "wb");
	if (!file)
		return tFormat::Invalid;

	qoi_desc qoiDesc;
	qoiDesc.channels	= (format == tFormat::Bit24) ? 3 : 4;
	qoiDesc.colorspace	= (ColourSpace == tSpace::Linear) ? QOI_LINEAR : QOI_SRGB;
	qoiDesc.height		= Height;
	qoiDesc.width		= Width;

	// If we're saving in 24bit we need to convert our source data to 24bit.
	uint8* pixels		= (uint8*)Pixels;
	bool deletePixels	= false;
	if (format == tFormat::Bit24)
	{
		int numPixels = Width*Height;
		pixels = new uint8[numPixels*3];
		for (int p = 0; p < numPixels; p++)
		{
			pixels[p*3 + 0] = Pixels[p].R;
			pixels[p*3 + 1] = Pixels[p].G;
			pixels[p*3 + 2] = Pixels[p].B;
		}
		deletePixels = true;
	}

	// Encode raw RGB or RGBA pixels into a QOI image in memory. The function either returns NULL on failure (invalid
	// parameters or malloc failed) or a pointer to the encoded data on success. On success the out_len is set to the
	// size in bytes of the encoded data. The returned qoi data should be free()d after use.
	int outLength = 0;
	void* memImage = qoi_encode(pixels, &qoiDesc, &outLength);
	if (deletePixels)
		delete[] pixels;

	if (!memImage)
		return tFormat::Invalid;
		
	tAssert(outLength);
	int numWritten = tSystem::tWriteFile(file, memImage, outLength);
	tSystem::tCloseFile(file);
	free(memImage);
	
	if (numWritten != outLength)
		return tFormat::Invalid;

	return format;
}


bool tImageQOI::IsOpaque() const
{
	for (int p = 0; p < (Width*Height); p++)
	{
		if (Pixels[p].A < 255)
			return false;
	}

	return true;
}


tPixel* tImageQOI::StealPixels()
{
	tPixel* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	
	return pixels;
}


}
