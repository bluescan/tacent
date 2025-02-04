// tImageQOI.cpp
//
// This class knows how to load and save Quite OK Images (.qoi) files into tPixel arrays. These tPixels may be 'stolen'
// by the tPicture's constructor if a targa file is specified. After the array is stolen the tImageQOI is invalid. This
// is purely for performance.
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
	void* reversedPixels = qoi_decode(qoiFileInMemory, numBytes, &results, 4);
	if (!reversedPixels)
		return false;

	Width				= results.width;	
	Height				= results.height;
	PixelFormatSrc		= (results.channels == 3) ? tPixelFormat::R8G8B8 : tPixelFormat::R8G8B8A8;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= (results.colorspace == QOI_LINEAR) ? tColourProfile::lRGB : tColourProfile::sRGB;
	ColourProfile		= ColourProfileSrc;
	tAssert((Width > 0) && (Height > 0));

	// Reverse rows.
	Pixels = new tPixel4b[Width*Height];
	int bytesPerRow = Width*4;
	for (int y = Height-1; y >= 0; y--)
		tStd::tMemcpy((uint8*)Pixels + ((Height-1)-y)*bytesPerRow, (uint8*)reversedPixels + y*bytesPerRow, bytesPerRow);
	free(reversedPixels);

	return true;
}


bool tImageQOI::Set(tPixel4b* pixels, int width, int height, bool steal)
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
		Pixels = new tPixel4b[Width*Height];
		tStd::tMemcpy(Pixels, pixels, Width*Height*sizeof(tPixel4b));
	}

	PixelFormatSrc		= tPixelFormat::R8G8B8A8;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume pixels must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

	return true;
}


bool tImageQOI::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	PixelFormatSrc		= frame->PixelFormatSrc;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume frame must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

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


tImageQOI::tFormat tImageQOI::Save(const tString& qoiFile, tFormat format, tColourProfile profile) const
{
	SaveParams params;
	params.Format = format;
	params.ColourProfile = profile;
	return Save(qoiFile, params);
}


tImageQOI::tFormat tImageQOI::Save(const tString& qoiFile, const SaveParams& params) const
{
	tFormat format = params.Format;
	tColourProfile profile = params.ColourProfile;
	if (!IsValid() || (format == tFormat::Invalid))
		return tFormat::Invalid;

	if (tSystem::tGetFileType(qoiFile) != tSystem::tFileType::QOI)
		return tFormat::Invalid;

	if (format == tFormat::Auto)
	{
		if (IsOpaque())
			format = tFormat::BPP24;
		else
			format = tFormat::BPP32;
	}
	if (profile == tColourProfile::Auto)
		profile = ColourProfileSrc;

	tFileHandle file = tSystem::tOpenFile(qoiFile.Chr(), "wb");
	if (!file)
		return tFormat::Invalid;

	qoi_desc qoiDesc;
	qoiDesc.channels	= (format == tFormat::BPP24) ? 3 : 4;

	// This also catches space being set to invalid. Basically if it's not linear, it's sRGB.
	qoiDesc.colorspace	= (profile == tColourProfile::lRGB) ? QOI_LINEAR : QOI_SRGB;
	qoiDesc.height		= Height;
	qoiDesc.width		= Width;

	// No matter the format, we need to reverse the rows before saving.
	tPixel4b* reversedRows = new tPixel4b[Width*Height];
	int bytesPerRow = Width*4;
	for (int y = Height-1; y >= 0; y--)
		tStd::tMemcpy((uint8*)reversedRows + ((Height-1)-y)*bytesPerRow, (uint8*)Pixels + y*bytesPerRow, bytesPerRow);

	// If we're saving in 24bit we need to convert our source data to 24bit.
	uint8* pixels		= (uint8*)reversedRows;
	bool deletePixels	= false;
	if (format == tFormat::BPP24)
	{
		int numPixels = Width*Height;
		pixels = new uint8[numPixels*3];
		for (int p = 0; p < numPixels; p++)
		{
			pixels[p*3 + 0] = reversedRows[p].R;
			pixels[p*3 + 1] = reversedRows[p].G;
			pixels[p*3 + 2] = reversedRows[p].B;
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
	delete[] reversedRows;

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


tPixel4b* tImageQOI::StealPixels()
{
	tPixel4b* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	
	return pixels;
}


}
