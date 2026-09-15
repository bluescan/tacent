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

	// Extract XMP metadata from the SVG document. Failing to parse it is not a load failure -- the image may still
	// rasterize perfectly (most SVGs carry no metadata at all).
	PopulateMetaData(svgFileInMemory, numBytes);

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
	if ((sw > 0.0f) && (sh > 0.0f) && (params.Dimension > 0))
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


static bool FindXMLElement(const uint8* base, int windowNumBytes, const char* openTag, const char* closeTag, const uint8** outStart, const uint8** outEnd)
{
	// Locate an XML element (its "<open" start through the end of its "</close>") within a window of [windowNumBytes]
	// bytes starting at 'base'. Returns true and sets [outStart, outEnd) to the element's byte extent on success; false
	// if either the open or the close tag is not present in the window.
	if ((!base) || (windowNumBytes <= 0) || (!openTag) || (!closeTag))
		return false;

	const int openTagNumBytes = (int)tStd::tStrlen(openTag);
	const int closeTagNumBytes = (int)tStd::tStrlen(closeTag);

	const uint8* open = (const uint8*)tStd::tMemsrch(base, windowNumBytes, openTag, openTagNumBytes);
	if (!open)
		return false;

	const int afterOpenNumBytes = windowNumBytes - (int)(open - base) - openTagNumBytes;
	const uint8* close = (const uint8*)tStd::tMemsrch(open + openTagNumBytes, afterOpenNumBytes, closeTag, closeTagNumBytes);
	if (!close)
		return false;

	*outStart = open;
	*outEnd = close + closeTagNumBytes;
	return true;
}


bool tImageSVG::PopulateMetaData(const uint8* svgFileInMemory, int numBytes)
{
	if ((!svgFileInMemory) || (numBytes <= 0))
		return false;

	tList<tMetaData::tMetaSegment> exifSegments, xmpSegments;

	// SVG files don't carry EXIF, so only the XMP list can be populated. Per the SVG spec the XMP payload lives in the
	// <metadata> element as an <x:xmpmeta> element, wrapped in xpacket begin/end processing-instruction comments. We
	// therefore locate it structurally: first the <metadata> section (the container -- the SVG analogue of a PNG's
	// "xMP " chunk that holds its payload), then the <x:xmpmeta> element within it, and hand that fragment to tMetaData,
	// which parses it via TinyEXIF -- the same consumer already used for the PNG/WEBP chunks.
	//
	// We deliberately do not parse the whole document as an XML DOM: the only XML parser linked into this module
	// (tinyxml2, via TinyEXIF) rejects the xpacket processing instructions, so a full-document parse fails on real-world
	// files. The extracted <x:xmpmeta>...</x:xmpmeta> fragment is self-contained, PI-free XML -- exactly the shape
	// TinyEXIF::parseFromXMPSegmentXML expects -- and TinyEXIF validates it as RDF, so a coincidental tag match outside
	// the <metadata> section cannot produce metadata.
	static const char metadataOpen[] = "<metadata";
	static const char metadataClose[] = "</metadata>";
	static const char xmpOpen[] = "<x:xmpmeta";
	static const char xmpClose[] = "</x:xmpmeta>";

	// 1. Locate the <metadata> ... </metadata> section. Its absence simply means "no XMP" (most SVGs carry none).
	const uint8* metadataStart = nullptr;
	const uint8* metadataEnd = nullptr;
	if (!FindXMLElement(svgFileInMemory, numBytes, metadataOpen, metadataClose, &metadataStart, &metadataEnd))
		return false;

	// 2. Within it, locate the <x:xmpmeta> ... </x:xmpmeta> element.
	const uint8* xmpStart = nullptr;
	const uint8* xmpEnd = nullptr;
	if (!FindXMLElement(metadataStart, (int)(metadataEnd - metadataStart), xmpOpen, xmpClose, &xmpStart, &xmpEnd))
		return false;

	// 3. Hand the fragment to tMetaData.
	xmpSegments.Append(new tMetaData::tMetaSegment(xmpStart, (int)(xmpEnd - xmpStart)));

	return MetaData.AddSegments(exifSegments, xmpSegments);
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
