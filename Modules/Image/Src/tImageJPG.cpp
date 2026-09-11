// tImageJPG.cpp
//
// This class knows how to load and save a JPeg (.jpg and .jpeg) file. It does zero processing of image data. It knows
// the details of the jpg file format and loads the data into a tPixel array. These tPixels may be 'stolen' by the
// tPicture's constructor if a jpg file is specified. After the array is stolen the tImageJPG is invalid. This is
// purely for performance. The loading and saving uses libjpeg-turbo. See Licence_LibJpegTurbo.txt for more info.
//
// Copyright (c) 2020, 2022-2024, 2026 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <System/tFile.h>
#include <Foundation/tStandard.h>
#include "Image/tImageJPG.h"
#include "Image/tPicture.h"
#include "turbojpeg.h"


using namespace tSystem;
namespace tImage
{


void tImageJPG::Clear()
{
	Width = 0;
	Height = 0;
	delete[] Pixels;
	Pixels = nullptr;
	if (MemImage) tjFree(MemImage);
	MemImage = nullptr;
	MemImageSize = 0;
	MetaData.Clear();

	tBaseImage::Clear();
}


bool tImageJPG::Load(const tString& jpgFile, const LoadParams& params)
{
	Clear();

	if (tSystem::tGetFileType(jpgFile) != tSystem::tFileType::JPG)
		return false;

	if (!tFileExists(jpgFile))
		return false;

	int numBytes = tGetFileSize(jpgFile);
	if (numBytes <= 0)
		return false;

	uint8* jpgFileInMemory = tjAlloc(numBytes);
	tLoadFile(jpgFile, jpgFileInMemory);
	bool success = Load(jpgFileInMemory, numBytes, params);
	delete[] jpgFileInMemory;

	return success;
}


bool tImageJPG::Load(const uint8* jpgFileInMemory, int numBytes, const LoadParams& params)
{
	Clear();
	if ((numBytes <= 0) || !jpgFileInMemory)
		return false;

	// If no decompress we simply set the MemImage members and we're done.
	if ((params.Flags & LoadFlag_NoDecompress))
	{
		MemImage = tjAlloc(numBytes);
		tStd::tMemcpy(MemImage, jpgFileInMemory, numBytes);
		MemImageSize = numBytes;
		return true;
	}

	PopulateMetaData(jpgFileInMemory, numBytes);

	tjhandle tjInstance = tjInitDecompress();
	if (!tjInstance)
		return false;

	int subSamp = 0;
	int colourSpace = 0;
	int headerResult = tjDecompressHeader3(tjInstance, jpgFileInMemory, numBytes, &Width, &Height, &subSamp, &colourSpace);
	if (headerResult < 0)
		return false;

	int numPixels = Width * Height;
	Pixels = new tPixel4b[numPixels];

    int jpgPixelFormat = TJPF_RGBA;
	int flags = 0;
	flags |= TJFLAG_BOTTOMUP;
	//flags |= TJFLAG_FASTUPSAMPLE;
	//flags |= TJFLAG_FASTDCT;
	flags |= TJFLAG_ACCURATEDCT;

	int decomResult = tjDecompress2
	(
		tjInstance, jpgFileInMemory, numBytes, (uint8*)Pixels,
		Width, 0, Height, jpgPixelFormat, flags
	);

	bool abortLoad = false;
	if (decomResult < 0)
	{
		int errorSeverity = tjGetErrorCode(tjInstance);
		switch (errorSeverity)
		{
			case TJERR_WARNING:
				if (params.Flags & tImageJPG::LoadFlag_Strict)
					abortLoad = true;
				break;

			case TJERR_FATAL:
				abortLoad = true;
				break;
		}
	}

	tjDestroy(tjInstance);
	if (abortLoad)
	{
		Clear();
		return false;
	}

	// The flips and rotates below do not clear the pixel format.
	if ((params.Flags & LoadFlag_ExifOrient))
	{
		const tMetaDatum& datum = MetaData[tMetaTag::Orientation];
		if (datum.IsSet())
		{
			switch (datum.Uint32)
			{
				case 0:		// Unspecified
				case 1:		// NoTransform
					break;

				case 2:		// Flip-Y.
					Flip(true);
					break;

				case 3:		// Flip-XY
					Flip(false);
					Flip(true);
					break;

				case 4:		// Flip-X
					Flip(false);
					break;

				case 5:		// Rot-CW90 Flip-Y
					Flip(true);
					Rotate90(true);
					break;
				
				case 6:		// Rot-ACW90
					Rotate90(false);
					break;

				case 7:		// Rot-ACW90 Flip-Y
					Flip(true);
					Rotate90(false);
					break;

				case 8:		// Rot-CW90
					Rotate90(true);
					break;
			}
		}
	}

	PixelFormatSrc = tPixelFormat::R8G8B8;
	PixelFormat = tPixelFormat::R8G8B8A8;

	// JPG file are assumed to be in sRGB.
	ColourProfileSrc = tColourProfile::sRGB;
	ColourProfile = tColourProfile::sRGB;

	return true;
}


bool tImageJPG::Set(tPixel4b* pixels, int width, int height, bool steal)
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


bool tImageJPG::Set(tFrame* frame, bool steal)
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

	// We don't know the colour space of the pixels.
	return true;
}


bool tImageJPG::Set(tPicture& picture, bool steal)
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


tFrame* tImageJPG::GetFrame(bool steal)
{
	if (!Pixels)
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


void tImageJPG::Rotate90(bool antiClockwise)
{
	tAssert((Width > 0) && (Height > 0) && Pixels);
	int newW = Height;
	int newH = Width;
	tPixel4b* newPixels = new tPixel4b[newW * newH];

	for (int y = 0; y < Height; y++)
		for (int x = 0; x < Width; x++)
			newPixels[ GetIndex(y, x, newW, newH) ] = Pixels[ GetIndex(antiClockwise ? x : Width-1-x, antiClockwise ? Height-1-y : y) ];

	ClearPixelData();
	Width = newW;
	Height = newH;
	Pixels = newPixels;
}


void tImageJPG::Flip(bool horizontal)
{
	tAssert((Width > 0) && (Height > 0) && Pixels);
	int newW = Width;
	int newH = Height;
	tPixel4b* newPixels = new tPixel4b[newW * newH];

	for (int y = 0; y < Height; y++)
		for (int x = 0; x < Width; x++)
			newPixels[ GetIndex(x, y) ] = Pixels[ GetIndex(horizontal ? Width-1-x : x, horizontal ? y : Height-1-y) ];

	ClearPixelData();
	Width = newW;
	Height = newH;
	Pixels = newPixels;
}


bool tImageJPG::CanDoPerfectLosslessTransform(Transform trans) const
{
	if (!MemImage || (MemImageSize <= 0))
		return false;

	tjhandle handle = tjInitTransform();
	if (!handle)
		return false;

	int width = 0;
	int height = 0;
	int subsamp = 0;
	tjDecompressHeader2(handle, MemImage, MemImageSize, &width, &height, &subsamp);
	tjDestroy(handle);

	tMath::tiClamp(subsamp, 0, TJ_NUMSAMP-1);
	int mcuWidth	= tjMCUWidth[subsamp];
	int mcuHeight	= tjMCUHeight[subsamp];
	bool widthOK	= (width % mcuWidth) == 0;
	bool heightOK	= (height % mcuHeight) == 0;

	// Regardless of the transform, if both width and height are multiples of mcu size, we're ok.
	if (widthOK && heightOK)
		return true;

	switch (trans)
	{
		case Transform::Rotate90ACW:
		case Transform::FlipH:
			if (!widthOK)
				return false;
			break;

		case Transform::Rotate90CW:
		case Transform::FlipV:
			if (!heightOK)
				return false;
			break;
	}

	return true;
}


bool tImageJPG::LosslessTransform(Transform trans, bool allowImperfect)
{
	if (!MemImage || (MemImageSize <= 0))
		return false;

	tjhandle handle = tjInitTransform();
	if (!handle)
		return false;

	TJXOP oper = TJXOP_NONE;
	switch (trans)
	{
		case Transform::Rotate90ACW:	oper = TJXOP_ROT270;	break;
		case Transform::Rotate90CW:		oper = TJXOP_ROT90;		break;
		case Transform::FlipH:			oper = TJXOP_HFLIP;		break;
		case Transform::FlipV:			oper = TJXOP_VFLIP;		break;
	}
	int options = allowImperfect ? TJXOPT_TRIM : TJXOPT_PERFECT;

	uint8* dstBufs[1];
	dstBufs[0] = nullptr;
	ulong dstSizes[1];
	dstSizes[0] = 0;
	tjtransform transforms[1];
	transforms[0].r.x = 0;
	transforms[0].r.y = 0;
	transforms[0].r.w = 0;
	transforms[0].r.h = 0;
	transforms[0].op = oper;
	transforms[0].options = options;
	transforms[0].data = nullptr;
	transforms[0].customFilter = nullptr;
	int flags = 0;

	int errorCode = tjTransform
	(
		handle, MemImage, MemImageSize, 1,
		dstBufs, dstSizes, transforms, flags
	);
	if ((errorCode != 0) || (dstBufs[0] == nullptr) || (dstSizes[0] <= 0))
	{
		tjDestroy(handle);
		return false;
	}

	// Success. Simply hand the buffer over to the MemImage.
	if (MemImage)
		tjFree(MemImage);
	MemImage = dstBufs[0];
	MemImageSize = dstSizes[0];

	tjDestroy(handle);
	return true;
}


bool tImageJPG::PopulateMetaData(const uint8* jpgFileInMemory, int numBytes)
{
	tAssert(jpgFileInMemory && (numBytes > 0));

	// Walk the JPEG segment table and pull out the APP1 segments. EXIF lives in an APP1 whose payload starts with
	// "Exif\0\0"; XMP lives in an APP1 whose payload starts with the Adobe namespace "http://ns.adobe.com/xap/1.0/\0".
	// Strip each APP1's framing and hand the bare EXIF TIFF / raw XMP XML to tMetaData.
	bool found = false;
	int offs = 0;

	// Skip the SOI marker (0xFFD8) if present.
	if ((numBytes > 2) && (jpgFileInMemory[0] == 0xFF) && (jpgFileInMemory[1] == 0xD8))
		offs = 2;

	while (offs + 4 <= numBytes)
	{
		if (jpgFileInMemory[offs] != 0xFF)
			break; // Not a valid marker; stop scanning.

		const uint8 marker = jpgFileInMemory[offs + 1];
		// Standalone markers with no payload.
		if ((marker == 0x01) || (marker == 0xD8) || ((marker >= 0xD0) && (marker <= 0xD9)))
		{
			offs += 2;
			continue;
		}
		// Start-of-scan (compressed data follows) / end-of-image: stop scanning.
		if ((marker == 0xD9) || (marker == 0xDA))
			break;
		if (marker == 0xFF)
			break; // Fill byte; can't reliably continue.

		const int segLen = (jpgFileInMemory[offs + 2] << 8) | jpgFileInMemory[offs + 3];
		const int segEnd = offs + 2 + segLen; // segLen includes the 2 length bytes.
		if ((segLen < 2) || (segEnd > numBytes))
			break; // Truncated/malformed segment; stop scanning.

		if (marker == 0xE1) // APP1
		{
			const uint8* payload = jpgFileInMemory + offs + 4;
			const int payloadLen = segLen - 2;

			// EXIF: payload is "Exif\0\0" followed by the bare TIFF.
			if ((payloadLen >= 6) && (tStd::tMemcmp(payload, "Exif\0\0", 6) == 0))
			{
				found |= MetaData.AddEXIF(payload + 6, payloadLen - 6);
			}
			// XMP: payload is the 29-byte Adobe namespace prefix followed by the raw XML.
			else if ((payloadLen >= 29) && (tStd::tMemcmp(payload, "http://ns.adobe.com/xap/1.0/\0", 29) == 0))
			{
				found |= MetaData.AddXMP(payload + 29, payloadLen - 29);
			}
		}

		offs = segEnd;
	}

	return found;
}


bool tImageJPG::Save(const tString& jpgFile, int quality) const
{
	SaveParams params;
	params.Quality = quality;
	return Save(jpgFile, params);
}


bool tImageJPG::Save(const tString& jpgFile, const SaveParams& params) const
{
	if (!IsValid())
		return false;

	if (tSystem::tGetFileType(jpgFile) != tSystem::tFileType::JPG)
		return false;

	// If we're a simple memory image, just save the bytes to disk and we're done.
	if (MemImage && (MemImageSize > 0))
	{
		tFileHandle fileHandle = tOpenFile(jpgFile, "wb");
		if (!fileHandle)
			return false;
		int numWritten = tWriteFile(fileHandle, MemImage, MemImageSize);
		tCloseFile(fileHandle);
		return (numWritten == MemImageSize);
	}

	tjhandle tjInstance = tjInitCompress();
	if (!tjInstance)
		return false;

	uint8* jpegBuf = nullptr;
	ulong jpegSize = 0;

	int flags = 0;
	flags |= TJFLAG_BOTTOMUP;
	//flags |= TJFLAG_FASTUPSAMPLE;
	//flags |= TJFLAG_FASTDCT;
	flags |= TJFLAG_ACCURATEDCT;

    int compResult = tjCompress2(tjInstance, (uint8*)Pixels, Width, 0, Height, TJPF_RGBA,
    	&jpegBuf, &jpegSize, TJSAMP_444, params.Quality, flags);

	tjDestroy(tjInstance);
	if (compResult < 0)
	{
		tjFree(jpegBuf);
		return false;
	}

	tFileHandle fileHandle = tOpenFile(jpgFile.Chars(), "wb");
	if (!fileHandle)
	{
		tjFree(jpegBuf);
		return false;
	}
	bool success = tWriteFile(fileHandle, jpegBuf, jpegSize);
	tCloseFile(fileHandle);
	tjFree(jpegBuf);

	return success;
}


tPixel4b* tImageJPG::StealPixels()
{
	tPixel4b* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	return pixels;
}


}
