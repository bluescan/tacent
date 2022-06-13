// tImageJPG.cpp
//
// This class knows how to load and save a JPeg (.jpg and .jpeg) file. It does zero processing of image data. It knows
// the details of the jpg file format and loads the data into a tPixel array. These tPixels may be 'stolen' by the
// tPicture's constructor if a jpg file is specified. After the array is stolen the tImageJPG is invalid. This is
// purely for performance. The loading and saving uses libjpeg-turbo. See Licence_LibJpegTurbo.txt for more info.
//
// Copyright (c) 2020, 2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <System/tFile.h>
#include "Image/tImageJPG.h"
#if defined(PLATFORM_LINUX)
#include "TurboJpeg/Linux/turbojpeg.h"
#elif defined(PLATFORM_WINDOWS)
#include "TurboJpeg/Windows/turbojpeg.h"
#endif


using namespace tSystem;
namespace tImage
{


bool tImageJPG::Load(const tString& jpgFile, uint32 loadFlags)
{
	Clear();

	if (tSystem::tGetFileType(jpgFile) != tSystem::tFileType::JPG)
		return false;

	if (!tFileExists(jpgFile))
		return false;

	int numBytes = 0;
	uint8* jpgFileInMemory = tLoadFile(jpgFile, nullptr, &numBytes);
	bool success = Set(jpgFileInMemory, numBytes, loadFlags);
	delete[] jpgFileInMemory;

	return success;
}


bool tImageJPG::Set(const uint8* jpgFileInMemory, int numBytes, uint32 loadFlags)
{
	Clear();
	if ((numBytes <= 0) || !jpgFileInMemory)
		return false;

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
	Pixels = new tPixel[numPixels];

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
				if (loadFlags & tImageJPG::LoadFlag_Strict)
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

	SrcPixelFormat = tPixelFormat::R8G8B8;

	if ((loadFlags & LoadFlag_ExifOrient))
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

	return true;
}


bool tImageJPG::Set(tPixel* pixels, int width, int height, bool steal)
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

	SrcPixelFormat = tPixelFormat::R8G8B8A8;
	return true;
}


void tImageJPG::Rotate90(bool antiClockwise)
{
	tAssert((Width > 0) && (Height > 0) && Pixels);
	int newW = Height;
	int newH = Width;
	tPixel* newPixels = new tPixel[newW * newH];

	for (int y = 0; y < Height; y++)
		for (int x = 0; x < Width; x++)
			newPixels[ GetIndex(y, x, newW, newH) ] = Pixels[ GetIndex(antiClockwise ? x : Width-1-x, antiClockwise ? Height-1-y : y) ];

	Clear();
	Width = newW;
	Height = newH;
	Pixels = newPixels;
}


void tImageJPG::Flip(bool horizontal)
{
	tAssert((Width > 0) && (Height > 0) && Pixels);
	int newW = Width;
	int newH = Height;
	tPixel* newPixels = new tPixel[newW * newH];

	for (int y = 0; y < Height; y++)
		for (int x = 0; x < Width; x++)
			newPixels[ GetIndex(x, y) ] = Pixels[ GetIndex(horizontal ? Width-1-x : x, horizontal ? y : Height-1-y) ];

	Clear();
	Width = newW;
	Height = newH;
	Pixels = newPixels;
}


bool tImageJPG::PopulateMetaData(const uint8* jpgFileInMemory, int numBytes)
{
	tAssert(jpgFileInMemory && (numBytes > 0));
	MetaData.Set(jpgFileInMemory, numBytes);
	return MetaData,IsValid();
}


bool tImageJPG::Save(const tString& jpgFile, int quality) const
{
	if (!IsValid())
		return false;

	if (tSystem::tGetFileType(jpgFile) != tSystem::tFileType::JPG)
		return false;

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
    	&jpegBuf, &jpegSize, TJSAMP_444, quality, flags);

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


tPixel* tImageJPG::StealPixels()
{
	tPixel* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	return pixels;
}


}
