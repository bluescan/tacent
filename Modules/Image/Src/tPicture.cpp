// tPicture.cpp
//
// This class represents a simple one-frame image. It is a collection of raw uncompressed 32-bit tPixels. It can load
// various formats from disk such as jpg, tga, png, etc. It intentionally _cannot_ load a dds file. More on that later.
// Image manipulation (excluding compression) is supported in a tPicture, so there are crop, scale, rotate, etc
// functions in this class.
//
// Some image disk formats have more than one 'frame' or image inside them. For example, tiff files can have more than
// layer, and gif/webp/apng images may be animated and have more than one frame. A tPicture can only prepresent _one_
// of these frames.
//
// Copyright (c) 2006, 2016, 2017, 2020-2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <LibKTX/include/version.h>
#define KTXSTRINGIFY(x) #x
#define KTXTOSTRING(x) KTXSTRINGIFY(x)
#define LIBKTX_VERSION_STRING KTXTOSTRING(LIBKTX_VERSION);

// It says cli, but it's only the version number. Nothing to do with a CLI.
#include <astcenccli_version.h>
const char* ASTCENCODER_VERSION_STRING		= VERSION_STRING;
#undef VERSION_STRING

#include "Image/tPicture.h"
#include "Math/tMatrix2.h"
#include "Math/tLinearAlgebra.h"
#include <OpenEXR/loadImage.h>
#include <zlib.h>
#include <png.h>
#include <apngdis.h>
#include <apngasm.h>
#include <bcdec.h>
#include <tiffvers.h>
#include <jconfig.h>						// JpegTurbo
#include <demux.h>							// WebP
#include <tinyxml2.h>
#include <TinyEXIF.h>
#include "Image/tResample.h"


using namespace tMath;
using namespace tImage;
using namespace tSystem;


const char* tImage::Version_LibJpegTurbo	= LIBJPEG_TURBO_VERSION;
const char* tImage::Version_ASTCEncoder		= ASTCENCODER_VERSION_STRING;
const char* tImage::Version_OpenEXR			= OPENEXR_VERSION_STRING;
const char* tImage::Version_ZLIB			= ZLIB_VERSION;
const char* tImage::Version_LibPNG			= PNG_LIBPNG_VER_STRING;
const char* tImage::Version_ApngDis			= APNGDIS_VERSION_STRING;
const char* tImage::Version_ApngAsm			= APNGASM_VERSION_STRING;
const char* tImage::Version_LibTIFF			= TIFFLIB_STANDARD_VERSION_STR;
const char* tImage::Version_LibKTX			= LIBKTX_VERSION_STRING;
int tImage::Version_WEBP_Major				= WEBP_DECODER_ABI_VERSION >> 8;
int tImage::Version_WEBP_Minor				= WEBP_DECODER_ABI_VERSION & 0xFF;
int tImage::Version_BCDec_Major				= BCDEC_VERSION_MAJOR;
int tImage::Version_BCDec_Minor				= BCDEC_VERSION_MINOR;
int tImage::Version_TinyXML2_Major			= TINYXML2_MAJOR_VERSION;
int tImage::Version_TinyXML2_Minor			= TINYXML2_MINOR_VERSION;
int tImage::Version_TinyXML2_Patch			= TINYXML2_PATCH_VERSION;
int tImage::Version_TinyEXIF_Major			= TINYEXIF_MAJOR_VERSION;
int tImage::Version_TinyEXIF_Minor			= TINYEXIF_MINOR_VERSION;
int tImage::Version_TinyEXIF_Patch			= TINYEXIF_PATCH_VERSION;


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
	PixelFormatSrc = tPixelFormat::R8G8B8A8;
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


void tPicture::RotateCenter(float angle, const tPixel& fill, tResampleFilterr upFilter, tResampleFilterr downFilter)
{
	if (!IsValid())
		return;

	tMatrix2 rotMat;
	rotMat.MakeRotateZ(angle);

	// Matrix is orthonormal so inverse is transpose.
	tMatrix2 invRot(rotMat);
	invRot.Transpose();

	// UpFilter		DownFilter		Description
	// None			NA				No up/down scaling. Preserves colours. Nearest Neighbour. Fast. Good for pixel art.
	// Valid		Valid			Up/down scaling. Smooth. Good results with up=bilinear, down=box.
	// Valid		None			Up/down scaling. Use alternate (sharper) downscaling scheme (pad + 2 X ScaleHalf).
	if (upFilter == tResampleFilterr::None)
		RotateCenterNearest(rotMat, invRot, fill);
	else
		RotateCenterResampled(rotMat, invRot, fill, upFilter, downFilter);
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


void tPicture::RotateCenterResampled
(
	const tMatrix2& rotMat, const tMatrix2& invRot, const tPixel& fill,
	tResampleFilterr upFilter, tResampleFilterr downFilter
)
{
	tAssert(upFilter != tResampleFilterr::None);
	if (upFilter == tResampleFilterr::Nearest)
	{
		Resample(Width*2, Height*2, upFilter);
		Resample(Width*2, Height*2, upFilter);
	}
	else
	{
		Resample(Width*4, Height*4, upFilter);
	}
	
	RotateCenterNearest(rotMat, invRot, fill);

	// After this call we are not guaranteed that the width and height are multiples of 4. If the downFilder is None
	// we need to use the ScaleHalf procedure, in which case a padding/crop call mey need to be done in order to get
	// the dimensions as a multiple of 4.
	if (downFilter == tResampleFilterr::None)
	{
		int newW = (Width % 4)  ? Width  + (4 - (Width  % 4)) : Width;
		int newH = (Height % 4) ? Height + (4 - (Height % 4)) : Height;
		if ((newW != Width) || (newH != Height))
			Crop(newW, newH, Anchor::MiddleMiddle, fill);
	
		bool scaleHalfSuccess;
		scaleHalfSuccess = ScaleHalf();		tAssert(scaleHalfSuccess);
		scaleHalfSuccess = ScaleHalf();		tAssert(scaleHalfSuccess);
	}
	else
	{
		Resample(Width/4, Height/4, downFilter);
	}
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


bool tPicture::Crop(const tColouri& colour, uint32 channels)
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

	int newWidth = Width - numLeftCols - numRightCols;
	int newHeight = Height - numBottomRows - numTopRows;
	if ((newWidth <= 0) || (newHeight <= 0))
		return false;

	Crop(newWidth, newHeight, numLeftCols, numBottomRows);
	return true;
}


bool tPicture::AdjustmentBegin()
{
	if (!IsValid() || OriginalPixels)
		return false;

	OriginalPixels = new tPixel[Width*Height];

	// We need to compute min and max component values so the extents of the brigtness parameter
	// exactly match all black at 0 and full white at 1. We do this as we copy the pixel values.
	BrightnessRGBMin = 256;
	BrightnessRGBMax = -1;

	tStd::tMemset(HistogramR, 0, sizeof(HistogramR));	MaxRCount = 0.0f;
	tStd::tMemset(HistogramG, 0, sizeof(HistogramG));	MaxGCount = 0.0f;
	tStd::tMemset(HistogramB, 0, sizeof(HistogramB));	MaxBCount = 0.0f;
	tStd::tMemset(HistogramA, 0, sizeof(HistogramA));	MaxACount = 0.0f;
	tStd::tMemset(HistogramI, 0, sizeof(HistogramI));	MaxICount = 0.0f;
	for (int p = 0; p < Width*Height; p++)
	{
		tColour4i& colour = Pixels[p];

		// Min/max. All RGB components considered.
		int minRGB = tMath::tMin(colour.R, colour.G, colour.B);
		int maxRGB = tMath::tMax(colour.R, colour.G, colour.B);
		if (minRGB < BrightnessRGBMin)
			BrightnessRGBMin = minRGB;
		if (maxRGB > BrightnessRGBMax)
			BrightnessRGBMax = maxRGB;

		// Histograms.
		float alpha = colour.GetA();
		HistogramR[colour.R] += alpha;
		HistogramG[colour.G] += alpha;
		HistogramB[colour.B] += alpha;
		HistogramA[colour.A] += 1.0f;
		HistogramI[colour.Intensity()] += alpha;

		OriginalPixels[p] = colour;
	}
	tiClamp(BrightnessRGBMin, 0, 255);
	tiClamp(BrightnessRGBMax, 0, 255);

	// Find max counts for the histograms so we can normalize if needed.
	for (int g = 0; g < NumGroups; g++)
	{
		if (HistogramR[g] > MaxRCount)		MaxRCount = HistogramR[g];
		if (HistogramG[g] > MaxGCount)		MaxGCount = HistogramG[g];
		if (HistogramB[g] > MaxBCount)		MaxBCount = HistogramB[g];
		if (HistogramA[g] > MaxACount)		MaxACount = HistogramA[g];
		if (HistogramI[g] > MaxICount)		MaxICount = HistogramI[g];
	}

	return true;
}


bool tPicture::AdjustBrightness(float brightness, tcomps comps)
{
	if (!IsValid() || !OriginalPixels)
		return false;

	// We want to guarantee all black at brightness level 0 (and no higher) and
	// all white at brightness 1 (and no lower). As an example, say the min RGB
	// for the entire image is 2 and the max is 240 -- we need 0 (black) to offset
	// by -240 and 1 to offset by +(255-2) = +253.
	int zeroOffset = -BrightnessRGBMax;
	int fullOffset = 255 - BrightnessRGBMin;
	float offsetFlt = tMath::tLinearInterp(brightness, 0.0f, 1.0f, float(zeroOffset), float(fullOffset));
	int offset = int(offsetFlt);
	for (int p = 0; p < Width*Height; p++)
	{
		tColour4i& srcColour = OriginalPixels[p];
		tColour4i& adjColour = Pixels[p];
		if (comps & tComp_R)	adjColour.R = tClamp(int(srcColour.R) + offset, 0, 255);
		if (comps & tComp_G)	adjColour.G = tClamp(int(srcColour.G) + offset, 0, 255);
		if (comps & tComp_B)	adjColour.B = tClamp(int(srcColour.B) + offset, 0, 255);
		if (comps & tComp_A)	adjColour.A = tClamp(int(srcColour.A) + offset, 0, 255);
	}

	return true;
}


bool tPicture::AdjustGetDefaultBrightness(float& brightness)
{
	if (!IsValid() || !OriginalPixels)
		return false;

	int zeroOffset = -BrightnessRGBMax;
	int fullOffset = 255 - BrightnessRGBMin;
	brightness = tMath::tLinearInterp(0.0f, float(zeroOffset), float(fullOffset), 0.0f, 1.0f);
	return true;	
}


bool tPicture::AdjustContrast(float contrastNorm, tcomps comps)
{
	if (!IsValid() || !OriginalPixels)
		return false;

	float contrast = tMath::tLinearInterp(contrastNorm, 0.0f, 1.0f, -255.0f, 255.0f);

	// The 259 is correct. Not a typo.
	float factor = (259.0f * (contrast + 255.0f)) / (255.0f * (259.0f - contrast));
	for (int p = 0; p < Width*Height; p++)
	{
		tColour4i& srcColour = OriginalPixels[p];
		tColour4i& adjColour = Pixels[p];
		if (comps & tComp_R)	adjColour.R = tClamp(int(factor * (float(srcColour.R) - 128.0f) + 128.0f), 0, 255);
		if (comps & tComp_G)	adjColour.G = tClamp(int(factor * (float(srcColour.G) - 128.0f) + 128.0f), 0, 255);
		if (comps & tComp_B)	adjColour.B = tClamp(int(factor * (float(srcColour.B) - 128.0f) + 128.0f), 0, 255);
		if (comps & tComp_A)	adjColour.A = tClamp(int(factor * (float(srcColour.A) - 128.0f) + 128.0f), 0, 255);
	}

	return true;
}


bool tPicture::AdjustGetDefaultContrast(float& contrast)
{
	if (!IsValid() || !OriginalPixels)
		return false;

	contrast = 0.5f;
	return true;
}


bool tPicture::AdjustLevels(float blackPoint, float midPoint, float whitePoint, float blackOut, float whiteOut, bool powerMidGamma, tcomps comps)
{
	if (!IsValid() || !OriginalPixels)
		return false;

	// We do all the calculations in floating point, and only convert back to denorm and clamp at the end.
	// First step is to ensure well-formed input values.
	tiSaturate(blackPoint);		tiSaturate(midPoint);	tiSaturate(whitePoint);		tiSaturate(blackOut);	tiSaturate(whiteOut);
	tiClampMin(midPoint, blackPoint);
	tiClampMin(whitePoint, midPoint);
	tiClampMin(whiteOut, blackOut);

	// Midtone gamma.
	float gamma = 1.0f;
	if (powerMidGamma)
	{
		// The first attempt at this was to use a quadratic bezier to interpolate the gamma points.The accepted answer at
		// https://stackoverflow.com/questions/6711707/draw-a-quadratic-b%C3%A9zier-curve-through-three-given-points
		// is, in fact, incorrect. There are not an infinite number of solutions, and there's only one CV that will
		// interpolate the middle point. The equation for this is given a bit later and it is not in general at t=1/2
		// and so is useless. This isn't surprising if you think about scaling, skewing, and translating a parabola.
		//
		// Instead we use a continuous pow function in base 10. The base is chosen as our max gamma value that we want
		// at the white point and it will be when input is 1.0. The min gamma we want is 0.1 so that will be at 10^-1.
		midPoint = tLinearInterp(midPoint, blackPoint, whitePoint, -1.0f, 1.0f);
		gamma = tMath::tPow(10.0f, midPoint);
	}
	else
	{
		// We want the midPoint to have the full range from 0..1 for >0 blackPoints and <1 whitePoints. This is needed because
		// we simplified the interface to have mid-point between black and white.
		midPoint = tLinearInterp(midPoint, blackPoint, whitePoint, 0.0f, 1.0f);
		if (midPoint < 0.5f)
			gamma = tMin(1.0f + (9.0f*(1.0f - 2.0f*midPoint)), 9.99f);
		else if (gamma > 0.5f)
			// 1 - ((MidtoneNormal*2) - 1)
			// 1 - MidtoneNormal*2 + 1
			// 2 - MidtoneNormal*2
			// 2*(1-MidtoneNormal)
			gamma = tMax(2.0f*(1.0f - midPoint), 0.01f);
		gamma = 1.0f/gamma;
	}

	// Apply for every pixel.
	for (int p = 0; p < Width*Height; p++)
	{
		tColour4i& srcColour = OriginalPixels[p];
		tColour4i& dstColour = Pixels[p];

		for (int e = 0; e < 4; e++)
		{
			if ((1 << e) & comps)
			{
				float src = float(srcColour.E[e])/255.0f;

				// Black/white levels.
				float adj = (src - blackPoint) / (whitePoint - blackPoint);

				// Midtones.
				adj = tPow(adj, gamma);

				// Output black/white levels.
				adj = blackOut + adj*(whiteOut - blackOut);
				dstColour.E[e] = tClamp(int(adj*255.0f), 0, 255);
			}
		}
	}
	return true;
}


bool tPicture::AdjustGetDefaultLevels(float& blackPoint, float& midPoint, float& whitePoint, float& outBlack, float& outWhite)
{
	if (!IsValid() || !OriginalPixels)
		return false;

	blackPoint	= 0.0f;
	midPoint	= 0.5f;
	whitePoint	= 1.0f;
	outBlack	= 0.0f,
	outWhite	= 1.0f;
	return true;
}


bool tPicture::AdjustRestoreOriginal()
{
	if (!IsValid() || !OriginalPixels)
		return false;

	tStd::tMemcpy(Pixels, OriginalPixels, Width*Height*sizeof(tPixel));
	return true;
}


bool tPicture::AdjustmentEnd()
{
	if (!IsValid() || !OriginalPixels)
		return false;

	delete[] OriginalPixels;
	OriginalPixels = nullptr;
	return true;
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


bool tPicture::Resample(int width, int height, tResampleFilterr filter, tResampleEdgeMode edgeMode)
{
	if (!IsValid() || (width <= 0) || (height <= 0))
		return false;

	if ((width == Width) && (height == Height))
		return true;

	tPixel* newPixels = new tPixel[width*height];
	bool success = tImage::Resample(Pixels, Width, Height, newPixels, width, height, filter, edgeMode);
	if (!success)
	{
		delete[] newPixels;
		return false;
	}

	delete[] Pixels;
	Pixels = newPixels;
	Width = width;
	Height = height;

	return true;
}


int tPicture::GenerateLayers(tList<tLayer>& layers, tResampleFilterr filter, tResampleEdgeMode edgeMode, bool chain)
{
	if (!IsValid())
		return 0;

	int numAppended = 0;

	// We always append a fullsize layer.
	layers.Append(new tLayer(tPixelFormat::R8G8B8A8, Width, Height, (uint8*)GetPixelPointer()));
	numAppended++;

	if (filter == tResampleFilterr::None)
		return numAppended;

	int srcW = Width;
	int srcH = Height;
	uint8* srcPixels = (uint8*)GetPixelPointer();

	// We base the next mip level on previous -- mostly because it's faster than resampling from the full
	// image each time. It's unclear to me which would generate better results.
	while ((srcW > 1) || (srcH > 1))
	{
		int dstW = srcW >> 1; tiClampMin(dstW, 1);
		int dstH = srcH >> 1; tiClampMin(dstH, 1);
		uint8* dstPixels = new uint8[dstW*dstH*sizeof(tPixel)];

		bool success = false;
		if (chain)
			success = tImage::Resample((tPixel*)srcPixels, srcW, srcH, (tPixel*)dstPixels, dstW, dstH, filter, edgeMode);
		else
			success = tImage::Resample(GetPixelPointer(), Width, Height, (tPixel*)dstPixels, dstW, dstH, filter, edgeMode);
		if (!success)
			break;

		layers.Append(new tLayer(tPixelFormat::R8G8B8A8, dstW, dstH, dstPixels, true));
		numAppended++;

		// Het ready for next loop.
		srcH = dstH;
		srcW = dstW;
		srcPixels = dstPixels;
	}

	return numAppended;
}
