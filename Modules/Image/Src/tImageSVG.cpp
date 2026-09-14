// tImageSVG.cpp
//
// This class knows how to load Scalable Vector Graphics (.svg) files into a tPixel4b array. The heavy lifting is done
// by the LunaSVG library (see Contrib/LunaSVG): the SVG document is parsed, rasterized onto a canvas, and the
// resulting bitmap is converted to straight 8-bit RGBA. The rows are then reversed to Tacent's bottom-up (OpenGL)
// order. See tImageSVG.h for details on the LoadParams options (target size and background colour).
//
// Copyright (c) 2026 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <System/tFile.h>
#include <Math/tColour.h>
#include <Foundation/tStandard.h>
#include "Image/tImageSVG.h"
#include "Image/tPicture.h"
#include "Image/tFrame.h"
#include <algorithm>
#include <cmath>
#include <lunasvg.h>
namespace tImage
{


bool tImageSVG::Load(const tString& svgFile, const LoadParams& params)
{
	Clear();

	if (tSystem::tGetFileType(svgFile) != tSystem::tFileType::SVG)
		return false;

	if (!tSystem::tFileExists(svgFile))
		return false;

	int numBytes = 0;
	uint8* svgFileInMemory = tSystem::tLoadFile(svgFile, nullptr, &numBytes);
	bool success = Load(svgFileInMemory, numBytes, params);
	delete[] svgFileInMemory;

	return success;
}


bool tImageSVG::Load(const uint8* svgFileInMemory, int numBytes, const LoadParams& params)
{
	Clear();
	if ((numBytes <= 0) || !svgFileInMemory)
		return false;

012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
	// Parse the SVG document from memory. loadFromData returns a unique_ptr that owns the document, or nullptr if the
	// data is not a valid SVG.
	auto document = lunasvg::Document::loadFromData(reinterpret_cast<const char*>(svgFileInMemory), (size_t)numBytes);
	if (!document)
		return false;

	// Determine the rasterization size. The SVG's intrinsic (native) size is the basis for everything; LoadParams'
	// Dimension and DimensionMode say how (or whether) to scale it. The aspect ratio is always preserved.
	float sw = document->width();
	float sh = document->height();

	// Work out the uniform scale factor to apply to the intrinsic size, based on the requested mode.
	float scale = 1.0f;
	if ((sw > 0.0f) && (sh > 0.0f) && (params.Dimension > 0.0f))
	{
		switch (params.Mode)
		{
			case DimensionMode_Width:
				scale = params.Dimension / sw;	// Target width: derive the height from the intrinsic aspect ratio.
				break;
			case DimensionMode_Height:
				scale = params.Dimension / sh;	// Target height: derive the width from the intrinsic aspect ratio.
				break;

			// DimensionMode_Auto (and the default): keep the SVG's intrinsic (native) size -- Dimension is ignored in this mode.
			case DimensionMode_Auto:
			default:
				scale = 1.0f;
				break;
		}
	}

	// Clamp to at least one pixel so a degenerate SVG still produces a valid (if tiny) image.
	int width	= (int)std::max(1.0f, (float)std::lround(sw * scale));
	int height	= (int)std::max(1.0f, (float)std::lround(sh * scale));

	// Rasterize the document at the target size with a transparent background. LunaSVG stores the result top-down as
	// premultiplied ARGB; the convertToRGBA() call below turns it into straight (un-premultiplied) RGBA to match
	// Tacent's byte order and colour encoding.
	lunasvg::Bitmap bitmap = document->renderToBitmap(width, height, 0x00000000);
	if (bitmap.isNull() || !bitmap.data())
		return false;

	bitmap.convertToRGBA();

	// LunaSVG produces its rows top-down. Tacent stores pixels bottom-up (OpenGL row order), so we reverse the rows
	// as we copy. If a non-transparent background colour was requested, flatten the (straight) RGBA onto it.
	bool flatten	= (params.BackgroundColor.A != 0);
	uint8 bgR		= params.BackgroundColor.R;
	uint8 bgG		= params.BackgroundColor.G;
	uint8 bgB		= params.BackgroundColor.B;

	Pixels = new tPixel4b[width * height];
	const uint8* src = bitmap.data();
	int bytesPerRow = bitmap.stride();
	for (int y = 0; y < height; y++)
	{
		const uint8* srcRow	= src + y * bytesPerRow;
		tPixel4b* destRow	= Pixels + (height - 1 - y) * width;
		for (int x = 0; x < width; x++)
		{
			uint8 R = srcRow[x * 4 + 0];
			uint8 G = srcRow[x * 4 + 1];
			uint8 B = srcRow[x * 4 + 2];
			uint8 A = srcRow[x * 4 + 3];

			if (flatten)
			{
				float a		= A * (1.0f / 255.0f);
				float invA	= 1.0f - a;
				destRow[x].R = (uint8)std::lround(R * a + bgR * invA);
				destRow[x].G = (uint8)std::lround(G * a + bgG * invA);
				destRow[x].B = (uint8)std::lround(B * a + bgB * invA);
				destRow[x].A = 255;
			}
			else
			{
				destRow[x].R = R;
				destRow[x].G = G;
				destRow[x].B = B;
				destRow[x].A = A;
			}
		}
	}

	Width				= width;
	Height				= height;
	PixelFormatSrc		= tPixelFormat::R8G8B8A8;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;
	ColourProfile		= tColourProfile::sRGB;

	return true;
}


bool tImageSVG::Set(tPixel4b* pixels, int width, int height, bool steal)
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
	ColourProfileSrc	= tColourProfile::sRGB;
	ColourProfile		= tColourProfile::sRGB;

	return true;
}


bool tImageSVG::Set(tFrame* frame, bool steal)
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


bool tImageSVG::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	PixelFormatSrc		= picture.PixelFormatSrc;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	// We don't know colour profile of tPicture.

	// This is worth some explanation. If steal is true the picture becomes invalid and the 'set' call will steal the
	// stolen pixels. If steal is false GetPixels is called and the 'set' call will memcpy them out... which makes
	// sure the picture is still valid after and no-one is sharing the pixel buffer.
	tPixel4b* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	bool success = Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
	tAssert(success);
	return true;
}


tFrame* tImageSVG::GetFrame(bool steal)
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


tPixel4b* tImageSVG::StealPixels()
{
	tPixel4b* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;

	return pixels;
}


}
