// tPicture.cpp
//
// This class represents a simple one-part image. It is a collection of raw uncompressed 32-bit tPixels. It can load
// various formats from disk such as jpg, tga, png, etc. It intentionally _cannot_ load a dds file. More on that later.
// This class can load many formats with a 'native implementation'. By native I mean formats for which there are
// specific and correct tImageAAA loaders. CxImage is used for the remainder (tiff/bmp mostly). Saving to different
// formats is supported natively where possible, and uses CxImage otherwise. Image manipulation (excluding compression)
// is supported in a tPicture, so there are crop, scale, etc functions in this class.
//
// Some image disk formats have more than one 'part' or image inside them. For example, tiff files can have more than
// layer, and gif/webp images may be animated and have more than one frame. A tPicture can only prepresent _one_ of 
// these parts.
//
// Copyright (c) 2006, 2016, 2017, 2019, 2020 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include "Foundation/tStandard.h"
#include "Image/tPicture.h"
#include "Math/tMatrix2.h"
#include <OpenEXR/loadImage.h>
#include <zlib.h>
#include <ximage.h>
#include <png.h>
#include <apngdis.h>
#ifdef PLATFORM_WINDOWS
#include "TurboJpeg/Windows/jconfig.h"
#include "WebP/Windows/include/demux.h"
#elif defined(PLATFORM_LINUX)
#include "TurboJpeg/Linux/jconfig.h"
#include "WebP/Linux/include/demux.h"
#endif
#include "Image/tResample.h"


using namespace tMath;
using namespace tImage;
using namespace tSystem;


const char* tImage::Version_TurboJPEG	= LIBJPEG_TURBO_VERSION;
const char* tImage::Version_OpenEXR		= OPENEXR_VERSION_STRING;
const char* tImage::Version_ZLIB		= ZLIB_VERSION;
const char* tImage::Version_LibPNG		= PNG_LIBPNG_VER_STRING;
const char* tImage::Version_ApngDis		= APNGDIS_VERSION_STRING;
int tImage::Version_WEBP_Major			= WEBP_DECODER_ABI_VERSION >> 8;
int tImage::Version_WEBP_Minor			= WEBP_DECODER_ABI_VERSION & 0xFF;


void tPicture::Set(int width, int height, const tPixel& colour)
{
	tAssert((width > 0) && (height > 0));

	// Reuse the existing buffer if possible.
	if (width*height != Width*Height)
	{
		delete[] Pixels;
		Pixels = new tPixel[width*height];
	}
	Width = width;
	Height = height;
	for (int pixel = 0; pixel < (Width*Height); pixel++)
		Pixels[pixel] = colour;

	SrcPixelFormat = tPixelFormat::R8G8B8A8;
}


void tPicture::Set(int width, int height, tPixel* pixelBuffer, bool copyPixels)
{
	tAssert((width > 0) && (height > 0) && pixelBuffer);

	// If we're copying the pixels we may be able to reuse the existing buffer if it's the right size. If we're not
	// copying and the buffer is being handed to us, we just need to free our current buffer.
	if (copyPixels)
	{
		if (width*height != Width*Height)
		{
			delete[] Pixels;
			Pixels = new tPixel[width*height];
		}
	}
	else
	{
		delete[] Pixels;
		Pixels = pixelBuffer;
	}
	Width = width;
	Height = height;

	if (copyPixels)
		tStd::tMemcpy(Pixels, pixelBuffer, Width*Height*sizeof(tPixel));

	SrcPixelFormat = tPixelFormat::R8G8B8A8;
}


bool tPicture::CanSave(tFileType fileType)
{
	switch (fileType)
	{
		case tFileType::TGA:
		case tFileType::BMP:		// CxImage.
		case tFileType::JPG:
		case tFileType::PNG:
			return true;
	}

	return false;
}


bool tPicture::CanLoad(tFileType fileType)
{
	switch (fileType)
	{
		case tFileType::EXR:
		case tFileType::GIF:
		case tFileType::HDR:
		case tFileType::ICO:
		case tFileType::JPG:
		case tFileType::PNG:
		case tFileType::APNG:
		case tFileType::TGA:
		case tFileType::WEBP:
		case tFileType::XPM:
			return true;
	}

	// The rest (bmp/png/tiff) are handled by CxImage.
	if (GetCxFormat(fileType) != CXIMAGE_FORMAT_UNKNOWN)
		return true;

	return false;
}


bool tPicture::Save(const tString& imageFile, tPicture::tColourFormat colourFmt, int quality)
{
	if (!IsValid())
		return false;

	tFileType fileType = tGetFileType(imageFile);
	if (!CanSave(fileType))
		return false;

	// Native formats.
	switch (fileType)
	{
		case tFileType::TGA:
			return SaveTGA(imageFile, tImage::tImageTGA::tFormat(colourFmt), tImage::tImageTGA::tCompression::None);

		case tFileType::JPG:
			return SaveJPG(imageFile, quality);

		case tFileType::PNG:
			return SavePNG(imageFile);
	}

	// Remaining formats handled by CxImage.
	if (fileType != tFileType::BMP)
		return false;

	tPixel* reorderedPixelArray = new tPixel[Width*Height];
	for (int p = 0; p < Width*Height; p++)
	{
		tPixel pixel = Pixels[p];
		tStd::tSwap(pixel.R, pixel.B);
		reorderedPixelArray[p] = pixel;
	}

	CxImage image;
	image.CreateFromArray((uint8*)reorderedPixelArray, Width, Height, 32, Width*4, false);
	delete[] reorderedPixelArray;

	uint32 cxImgFormat = CXIMAGE_FORMAT_BMP;
	if (colourFmt == tPicture::tColourFormat::Colour)
		image.AlphaDelete();
	else if ((colourFmt == tPicture::tColourFormat::Auto) && IsOpaque())
		image.AlphaDelete();

	return image.Save(imageFile.ConstText(), cxImgFormat);
}


bool tPicture::SaveTGA(const tString& tgaFile, tImageTGA::tFormat format, tImageTGA::tCompression compression) const
{
	tFileType fileType = tGetFileType(tgaFile);
	if (!IsValid() || (fileType != tFileType::TGA))
		return false;

	tImageTGA targa(Pixels, Width, Height);
	tImageTGA::tFormat savedFormat = targa.Save(tgaFile, format, compression);
	if (savedFormat == tImageTGA::tFormat::Invalid)
		return false;

	return true;
}


bool tPicture::SaveJPG(const tString& jpgFile, int quality) const
{
	tFileType fileType = tGetFileType(jpgFile);
	if (!IsValid() || (fileType != tFileType::JPG))
		return false;

	tImageJPG jpeg(Pixels, Width, Height);
	bool success = jpeg.Save(jpgFile, quality);
	return success;
}


bool tPicture::SavePNG(const tString& pngFile) const
{
	tFileType fileType = tGetFileType(pngFile);
	if (!IsValid() || (fileType != tFileType::PNG))
		return false;

	tImagePNG png(Pixels, Width, Height);
	bool success = png.Save(pngFile);
	return success;
}


bool tPicture::Load(const tString& imageFile, int partNum, LoadParams params)
{
	tMath::tiClampMin(partNum, 0);

	Clear();
	if (!tFileExists(imageFile))
		return false;

	tFileType fileType = tGetFileType(imageFile);
	if (!CanLoad(fileType))
		return false;

	// Native handlers.
	// @todo It's looking like we should make an abstract tImageBase class at this point to get rid of all this duplication.
	switch (fileType)
	{
		case tFileType::EXR:
		{
			tImageEXR exr;
			bool ok = exr.Load
			(
				imageFile,
				partNum,
				params.GammaValue,
				params.EXR_Exposure,
				params.EXR_Defog,
				params.EXR_KneeLow,
				params.EXR_KneeHigh
			);
			if (!ok || !exr.IsValid())
				return false;
			Width = exr.GetWidth();
			Height = exr.GetHeight();
			Pixels = exr.StealPixels();
			SrcPixelFormat = exr.SrcPixelFormat;
			return true;
		}

		case tFileType::GIF:
		{
			tImageGIF gif;
			bool ok = gif.Load(imageFile);
			if (!ok || !gif.IsValid())
				return false;

			if (partNum >= gif.GetNumFrames())
				return false;

			Width = gif.GetWidth();
			Height = gif.GetHeight();
		
			tImageGIF::Frame* stolenFrame = gif.StealFrame(partNum);
			Pixels = stolenFrame->Pixels;

			// This is safe as the frame does not own/delete the pixels.
			delete stolenFrame;

			SrcPixelFormat = gif.SrcPixelFormat;
			return true;
		}

		case tFileType::HDR:
		{
			// HDRs can only have one part.
			if (partNum != 0)
				return false;
			tImageHDR hdr;
			hdr.Load
			(
				imageFile,
				params.GammaValue,
				params.HDR_Exposure
			);
			if (!hdr.IsValid())
				return false;
			Width = hdr.GetWidth();
			Height = hdr.GetHeight();
			Pixels = hdr.StealPixels();
			SrcPixelFormat = hdr.SrcPixelFormat;
			return true;
		}

		case tFileType::ICO:
		{
			tImageICO ico;
			bool ok = ico.Load(imageFile);
			if (!ok || !ico.IsValid())
				return false;

			if (partNum >= ico.GetNumParts())
				return false;

			tImageICO::Part* stolenPart = ico.StealPart(partNum);
			Pixels = stolenPart->Pixels;

			Width = stolenPart->Width;
			Height = stolenPart->Height;		
			SrcPixelFormat = stolenPart->SrcPixelFormat;

			// This is safe as the part does not own/delete the pixels.
			delete stolenPart;
			return true;
		}

		case tFileType::JPG:
		{
			// JPGs can only have one part.
			if (partNum != 0)
				return false;
			tImageJPG jpeg(imageFile);
			if (!jpeg.IsValid())
				return false;
			Width = jpeg.GetWidth();
			Height = jpeg.GetHeight();
			Pixels = jpeg.StealPixels();
			SrcPixelFormat = jpeg.SrcPixelFormat;
			return true;
		}

		case tFileType::PNG:
		{
			// PNGs can only have one part.
			if (partNum != 0)
				return false;
			tImagePNG png(imageFile);
			if (!png.IsValid())
				return false;
			Width = png.GetWidth();
			Height = png.GetHeight();
			Pixels = png.StealPixels();
			SrcPixelFormat = png.SrcPixelFormat;
			return true;
		}

		case tFileType::APNG:
		{
			tImageAPNG apng;
			bool ok = apng.Load(imageFile);
			if (!ok || !apng.IsValid())
				return false;

			if (partNum >= apng.GetNumFrames())
				return false;

			tImageAPNG::Frame* stolenFrame = apng.StealFrame(partNum);
			Width = stolenFrame->Width;
			Height = stolenFrame->Height;
			SrcPixelFormat = stolenFrame->SrcPixelFormat;
		
			Pixels = stolenFrame->Pixels;

			// This is safe as the frame does not own/delete the pixels.
			delete stolenFrame;
			return true;
		}

		case tFileType::TGA:
		{
			// TGAs can only have one part.
			if (partNum != 0)
				return false;
			tImageTGA targa(imageFile);
			if (!targa.IsValid())
				return false;
			Width = targa.GetWidth();
			Height = targa.GetHeight();
			Pixels = targa.StealPixels();
			SrcPixelFormat = targa.SrcPixelFormat;
			return true;
		}

		case tFileType::WEBP:
		{
			tImageWEBP webp;
			bool ok = webp.Load(imageFile);
			if (!ok || !webp.IsValid())
				return false;

			if (partNum >= webp.GetNumFrames())
				return false;

			tImageWEBP::Frame* stolenFrame = webp.StealFrame(partNum);
			Width = stolenFrame->Width;
			Height = stolenFrame->Height;
			SrcPixelFormat = stolenFrame->SrcPixelFormat;
		
			Pixels = stolenFrame->Pixels;

			// This is safe as the frame does not own/delete the pixels.
			delete stolenFrame;
			return true;
		}

		case tFileType::XPM:
		{
			// XPMs can only have one part.
			if (partNum != 0)
				return false;
			tImageXPM xpm(imageFile);
			if (!xpm.IsValid())
				return false;
			Width = xpm.GetWidth();
			Height = xpm.GetHeight();
			Pixels = xpm.StealPixels();
			SrcPixelFormat = xpm.SrcPixelFormat;
			return true;
		}
	}

	// For bmp and tiff we use the CxImage library for loading.
	// @todo Support multi-part tiffs.
	ENUM_CXIMAGE_FORMATS cxFormat = ENUM_CXIMAGE_FORMATS(GetCxFormat(fileType));
	if (cxFormat == CXIMAGE_FORMAT_UNKNOWN)
		return false;

	CxImage image;
	if (partNum != 0)
		return false;

	bool cxok = image.Load(imageFile.ConstText(), cxFormat);
	if (!cxok)
		return false;

	int width = image.GetWidth();
	int height = image.GetHeight();
	if (!image.IsValid() || (width <= 0) || (height <= 0))
		return false;

	Width = width;
	Height = height;
	Pixels = new tPixel[Width*Height];

	// CxImage alpha oddness. If we request the alpha using GetPixelColor and there is no alpha channel, it returns 0
	// for the alpha, which is incorrect as alpha is normally interpreted as opacity, not transparency. It should be
	// returning full opacity. That's why we need imageHasValidAlphas -- so we can check if the channel exists at all.
	bool imageHasValidAlphas = image.AlphaIsValid();
	SrcPixelFormat = imageHasValidAlphas ? tPixelFormat::R8G8B8A8 : tPixelFormat::R8G8B8;

	int index = 0;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			RGBQUAD rgba = image.GetPixelColor(x, y);
			tColouri colour;
			colour.R = rgba.rgbRed;
			colour.G = rgba.rgbGreen;
			colour.B = rgba.rgbBlue;

			if (imageHasValidAlphas)
				colour.A = rgba.rgbReserved;
			else
				colour.A = 255;

			Pixels[index++] = colour;
		}
	}
	Filename = imageFile;
	return true;
}


void tPicture::Save(tChunkWriter& chunk) const
{
	chunk.Begin(tChunkID::Image_Picture);
	{
		chunk.Begin(tChunkID::Image_PictureProperties);
		{
			chunk.Write(Width);
			chunk.Write(Height);
		}
		chunk.End();

		chunk.Begin(tChunkID::Image_PicturePixels);
		{
			chunk.Write(Pixels, GetNumPixels());
		}
		chunk.End();
	}
	chunk.End();
}


void tPicture::Load(const tChunk& chunk)
{
	Clear();
	if (chunk.ID() != tChunkID::Image_Picture)
		return;

	for (tChunk ch = chunk.First(); ch.IsValid(); ch = ch.Next())
	{
		switch (ch.ID())
		{
			case tChunkID::Image_PictureProperties:
			{
				ch.GetItem(Width);
				ch.GetItem(Height);
				break;
			}

			case tChunkID::Image_PicturePixels:
			{
				tAssert(!Pixels && (GetNumPixels() > 0));
				Pixels = new tPixel[GetNumPixels()];
				ch.GetItems(Pixels, GetNumPixels());
				break;
			}
		}
	}
	SrcPixelFormat = tPixelFormat::R8G8B8A8;
}


void tPicture::Crop(int newW, int newH, Anchor anchor, const tColouri& fill)
{
	int originx = 0;
	int originy = 0;

	switch (anchor)
	{
		case Anchor::LeftTop:		originx = 0;				originy = Height-newH;		break;
		case Anchor::MiddleTop:		originx = Width/2 - newW/2;	originy = Height-newH;		break;
		case Anchor::RightTop:		originx = Width - newW;		originy = Height-newH;		break;

		case Anchor::LeftMiddle:	originx = 0;				originy = Height/2-newH/2;	break;
		case Anchor::MiddleMiddle:	originx = Width/2 - newW/2;	originy = Height/2-newH/2;	break;
		case Anchor::RightMiddle:	originx = Width - newW;		originy = Height/2-newH/2;	break;

		case Anchor::LeftBottom:	originx = 0;				originy = 0;				break;
		case Anchor::MiddleBottom:	originx = Width/2 - newW/2;	originy = 0;				break;
		case Anchor::RightBottom:	originx = Width - newW;		originy = 0;				break;
	}

	Crop(newW, newH, originx, originy, fill);
}


void tPicture::Crop(int newW, int newH, int originX, int originY, const tColouri& fill)
{
	if ((newW <= 0) || (newH <= 0))
	{
		Clear();
		return;
	}

	if ((newW == Width) && (newH == Height) && (originX == 0) && (originY == 0))
		return;

	tPixel* newPixels = new tPixel[newW * newH];

	// Set the new pixel colours.
	for (int y = 0; y < newH; y++)
	{
		for (int x = 0; x < newW; x++)
		{
			// If we're in range of the old picture we just copy the colour. If the old image is invalid no problem, as
			// we'll fall through to the else and the pixel will be set to black.
			if (tMath::tInIntervalIE(originX + x, 0, Width) && tMath::tInIntervalIE(originY + y, 0, Height))
				newPixels[y * newW + x] = GetPixel(originX + x, originY + y);
			else
				newPixels[y * newW + x] = fill;
		}
	}

	Clear();
	Width = newW;
	Height = newH;
	Pixels = newPixels;
}


void tPicture::Crop(const tColouri& colour, uint32 channels)
{
	// Count bottom rows to crop.
	int numBottomRows = 0;
	for (int y = 0; y < Height; y++)
	{
		bool allMatch = true;
		for (int x = 0; x < Width; x++)
		{
			if (!colour.Equal(Pixels[ GetIndex(x, y) ], channels))
			{
				allMatch = false;
				break;
			}
		}
		if (allMatch)
			numBottomRows++;
		else
			break;
	}

	// Count top rows to crop.
	int numTopRows = 0;
	for (int y = Height-1; y >= 0; y--)
	{
		bool allMatch = true;
		for (int x = 0; x < Width; x++)
		{
			if (!colour.Equal(Pixels[ GetIndex(x, y) ], channels))
			{
				allMatch = false;
				break;
			}
		}
		if (allMatch)
			numTopRows++;
		else
			break;
	}

	// Count left columns to crop.
	int numLeftCols = 0;
	for (int x = 0; x < Width; x++)
	{
		bool allMatch = true;
		for (int y = 0; y < Height; y++)
		{
			if (!colour.Equal(Pixels[ GetIndex(x, y) ], channels))
			{
				allMatch = false;
				break;
			}
		}
		if (allMatch)
			numLeftCols++;
		else
			break;
	}

	// Count right columns to crop.
	int numRightCols = 0;
	for (int x = Width-1; x >= 0; x--)
	{
		bool allMatch = true;
		for (int y = 0; y < Height; y++)
		{
			if (!colour.Equal(Pixels[ GetIndex(x, y) ], channels))
			{
				allMatch = false;
				break;
			}
		}
		if (allMatch)
			numRightCols++;
		else
			break;
	}

	Crop(Width - numLeftCols - numRightCols, Height - numBottomRows - numTopRows, numLeftCols, numBottomRows);
}


void tPicture::Rotate90(bool antiClockwise)
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


void tPicture::RotateCenter(float angle, const tPixel& fill, tResampleFilter filter)
{
	if (!IsValid())
		return;

	tMatrix2 rotMat;
	rotMat.MakeRotateZ(angle);

	// Matrix is orthonormal so inverse is transpose.
	tMatrix2 invRot(rotMat);
	invRot.Transpose();

	if (filter != tResampleFilter::Invalid)
		RotateCenterResampled(rotMat, invRot, fill, filter);
	else
		RotateCenterNearest(rotMat, invRot, fill);
}


void tPicture::RotateCenterNearest(const tMatrix2& rotMat, const tMatrix2& invRot, const tPixel& fill)
{
	int srcW = Width;
	int srcH = Height;

	// Rotate all corners to get new size. Memfill it with fill colour. Map from old to new.
	float srcHalfW = float(Width)/2.0f;
	float srcHalfH = float(Height)/2.0f;
	tPixel* srcPixels = Pixels;

	tVector2 tl(-srcHalfW,  srcHalfH);
	tVector2 tr( srcHalfW,  srcHalfH);
	tVector2 bl(-srcHalfW, -srcHalfH);
	tVector2 br( srcHalfW, -srcHalfH);
	tl = rotMat*tl;	tr = rotMat*tr;	bl = rotMat*bl;	br = rotMat*br;
	float epsilon = 0.0002f;
	int minx = int(tFloor(tRound(tMin(tl.x, tr.x, bl.x, br.x), epsilon)));
	int miny = int(tFloor(tRound(tMin(tl.y, tr.y, bl.y, br.y), epsilon)));
	int maxx = int(tCeiling(tRound(tMax(tl.x, tr.x, bl.x, br.x), epsilon)));
	int maxy = int(tCeiling(tRound(tMax(tl.y, tr.y, bl.y, br.y), epsilon)));
	Width = maxx - minx;
	Height = maxy - miny;

	Pixels = new tPixel[Width*Height];
	float halfW = float(Width)/2.0f;
	float halfH = float(Height)/2.0f;

	// We now need to loop through every pixel in the new image and do a weighted sample of
	// the pixels it maps to in the original image. Actually weighted is not implemented yet
	// so do nearest.
	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			// Lets start with nearest pixel. We can get fancier after.
			// dstPos is the middle of the pixel we are writing to. srcPos is the original we are coming from.
			// The origin' of a pixel is the lower-left corner. The 0.5s get us to the center (and back).
			tVector2 dstPos(float(x)+0.5f - halfW, float(y)+0.5f - halfH);
			tVector2 srcPos = invRot*dstPos;
			srcPos += tVector2(srcHalfW, srcHalfH);
			srcPos -= tVector2(0.5f, 0.5f);

			tPixel srcCol = tPixel::black;

			int srcX = int(tRound(srcPos.x));
			int srcY = int(tRound(srcPos.y));
			bool useFill = (srcX < 0) || (srcX >= srcW) || (srcY < 0) || (srcY >= srcH);
			srcCol = useFill ? fill : srcPixels[ GetIndex(srcX, srcY, srcW, srcH) ];
			Pixels[ GetIndex(x, y) ] = srcCol;
		}
	}

	delete[] srcPixels;
}


void tPicture::RotateCenterResampled(const tMatrix2& rotMat, const tMatrix2& invRot, const tPixel& fill, tResampleFilter filter)
{
	tAssert(filter != tResampleFilter::Invalid);
	if (filter == tResampleFilter::Nearest)
	{
		Resample(Width*2, Height*2, filter);
		Resample(Width*2, Height*2, filter);
	}
	else
	{
		Resample(Width*4, Height*4, filter);
	}
	
	RotateCenterNearest(rotMat, invRot, fill);
	ScaleHalf();
	ScaleHalf();
}


void tPicture::Flip(bool horizontal)
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


bool tPicture::ScaleHalf()
{
	if (!IsValid())
		return false;

	// A 1x1 image is defined as already being rescaled.
	if ((Width == 1) && (Height == 1))
		return true;

	// We only allow non-divisible-by-2 dimensions if that dimension is exactly 1.
	if ( ((Width & 1) && (Width != 1)) || ((Height & 1) && (Height != 1)) )
		return false;

	int newWidth = Width >> 1;
	int newHeight = Height >> 1;
	if (newWidth == 0)
		newWidth = 1;
	if (newHeight == 0)
		newHeight = 1;

	int numNewPixels = newWidth*newHeight;
	tPixel* newPixels = new tPixel[numNewPixels];

	// Deal with case where src height is 1 and src width is divisible by 2 OR where src width is 1 and src height is
	// divisible by 2. Image is either a row or column vector in this case.
	if ((Height == 1) || (Width == 1))
	{
		for (int p = 0; p < numNewPixels; p++)
		{
			int p2 = 2*p;

			int p0r = Pixels[p2].R;
			int p1r = Pixels[p2 + 1].R;
			newPixels[p].R = tMath::tClamp((p0r + p1r)>>1, 0, 255);

			int p0g = Pixels[p2].G;
			int p1g = Pixels[p2 + 1].G;
			newPixels[p].G = tMath::tClamp((p0g + p1g)>>1, 0, 255);

			int p0b = Pixels[p2].B;
			int p1b = Pixels[p2 + 1].B;
			newPixels[p].B = tMath::tClamp((p0b + p1b)>>1, 0, 255);

			int p0a = Pixels[p2].A;
			int p1a = Pixels[p2 + 1].A;
			newPixels[p].A = tMath::tClamp((p0a + p1a)>>1, 0, 255);
		}
	}

	// Handle the case where both width and height are both divisible by 2.
	else
	{
		for (int x = 0; x < newWidth; x++)
		{
			int x2 = 2*x;
			for (int y = 0; y < newHeight; y++)
			{
				int y2 = 2*y;

				// @todo Use SSE/SIMD here?
				int p0r = Pixels[y2*Width + x2].R;
				int p1r = Pixels[y2*Width + x2 + 1].R;
				int p2r = Pixels[(y2+1)*Width + x2].R;
				int p3r = Pixels[(y2+1)*Width + x2 + 1].R;
				newPixels[y*newWidth + x].R = tMath::tClamp((p0r + p1r + p2r + p3r)>>2, 0, 255);

				int p0g = Pixels[y2*Width + x2].G;
				int p1g = Pixels[y2*Width + x2 + 1].G;
				int p2g = Pixels[(y2+1)*Width + x2].G;
				int p3g = Pixels[(y2+1)*Width + x2 + 1].G;
				newPixels[y*newWidth + x].G = tMath::tClamp((p0g + p1g + p2g + p3g)>>2, 0, 255);

				int p0b = Pixels[y2*Width + x2].B;
				int p1b = Pixels[y2*Width + x2 + 1].B;
				int p2b = Pixels[(y2+1)*Width + x2].B;
				int p3b = Pixels[(y2+1)*Width + x2 + 1].B;
				newPixels[y*newWidth + x].B = tMath::tClamp((p0b + p1b + p2b + p3b)>>2, 0, 255);

				int p0a = Pixels[y2*Width + x2].A;
				int p1a = Pixels[y2*Width + x2 + 1].A;
				int p2a = Pixels[(y2+1)*Width + x2].A;
				int p3a = Pixels[(y2+1)*Width + x2 + 1].A;
				newPixels[y*newWidth + x].A = tMath::tClamp((p0a + p1a + p2a + p3a)>>2, 0, 255);
			}
		}
	}

	Clear();
	Pixels = newPixels;
	Width = newWidth;
	Height = newHeight;
	return true;
}


bool tPicture::Resample(int width, int height, tResampleFilter filter, tResampleEdgeMode edgeMode)
{
	if (!IsValid() || (width <= 0) || (height <= 0))
		return false;

	if ((width == Width) && (height == Height))
		return true;

	tPixel* newPixels = new tPixel[width*height];
	bool success = tImage::Resample(Pixels, Width, Height, newPixels, width, height, filter, edgeMode);
	if (!success)
	{
		delete newPixels;
		return false;
	}

	delete[] Pixels;
	Pixels = newPixels;
	Width = width;
	Height = height;

	return true;
}


int tPicture::GetCxFormat(tFileType fileType)
{
	switch (fileType)
	{
		case tFileType::BMP:
			return CXIMAGE_FORMAT_BMP;

		case tFileType::TIFF:
			return CXIMAGE_FORMAT_TIF;
	}

	return CXIMAGE_FORMAT_UNKNOWN;
}
