// tImageSVG.h
//
// This class knows how to load Scalable Vector Graphics (.svg) files into a tPixel4b array. The SVG is rasterized
// using LunaSVG (see Contrib/LunaSVG) and the resulting bitmap is stored as straight (un-premultiplied) 8-bit RGBA
// pixels in Tacent's bottom-up (OpenGL) row order.
//
// Because SVG is a vector format it can be rasterized at any resolution. By default the SVG's intrinsic (native)
// size is used, but a target size can be requested through LoadParams. The size is given as a single Dimension value
// together with a DimensionMode that says whether it is a target width, a target height, or a general size to fit
// within. The SVG is never stretched -- its aspect ratio is always preserved. Optionally the image can be flattened
// onto a solid background colour (any transparent area takes that colour and the result becomes fully opaque).
//
// This class is load-only -- it cannot save back to SVG.
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

#pragma once
#include <Foundation/tString.h>
#include <Math/tColour.h>
#include <Image/tPixelFormat.h>
#include <Image/tMetaData.h>
#include <Image/tBaseImage.h>
namespace tImage
{


class tImageSVG : public tBaseImage
{
public:
	// How the rasterization size is requested: Auto keeps the intrinsic size; Width / Height use LoadParams::Dimension as the target dimension.
	enum DimensionMode
	{
		DimensionMode_Auto,			// Use the SVG's intrinsic (native) size. LoadParams::Dimension is ignored in this mode.
		DimensionMode_Width,		// LoadParams::Dimension is the target width in pixels; the height is derived from the aspect ratio. If Dimension is 0 or negative, fall back to Auto (the intrinsic size).
		DimensionMode_Height,		// LoadParams::Dimension is the target height in pixels; the width is derived from the aspect ratio. If Dimension is 0 or negative, fall back to Auto (the intrinsic size).
	};

	// Parameters that control how the SVG is rasterized when loading.
	struct LoadParams
	{
		LoadParams()																										{ Reset(); }
		LoadParams(const LoadParams& src)																					: Mode(src.Mode), Dimension(src.Dimension), BackgroundColor(src.BackgroundColor) { }
		LoadParams& operator=(const LoadParams& src)																		{ Mode = src.Mode; Dimension = src.Dimension; BackgroundColor = src.BackgroundColor; return *this; }
		void Reset()																										{ Mode = DimensionMode_Auto; Dimension = 512; BackgroundColor = tColour4b::transparent; }

		// How Dimension should be interpreted. See the DimensionMode enum for details.
		DimensionMode Mode;

		// The target rasterization size in pixels: the target width in DimensionMode_Width, or the target height in
		// DimensionMode_Height. Ignored in DimensionMode_Auto; a value of 0 or negative also uses the intrinsic size. The
		// aspect ratio is always preserved. Defaults to 512.
		int Dimension;

		// The colour that shows through wherever the SVG is transparent. If its alpha is 0 (the default) no background is
		// used and the result keeps its per-pixel alpha. If its alpha is non-zero the image is flattened onto this
		// background colour and every resulting pixel is fully opaque.
		tColour4b BackgroundColor;
	};

	// Creates an invalid tImageSVG. You must call Load manually.
	tImageSVG()																												{ }
	tImageSVG(const tString& svgFile, const LoadParams& params = LoadParams())												{ Load(svgFile, params); }

	// The data is copied out of svgFileInMemory. Go ahead and delete[] it after the call if you want.
	tImageSVG(const uint8* svgFileInMemory, int numBytes, const LoadParams& params = LoadParams())							{ Load(svgFileInMemory, numBytes, params); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageSVG(tPixel4b* pixels, int width, int height, bool steal = false)													{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageSVG(tFrame* frame, bool steal = true)																				{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImageSVG(tPicture& picture, bool steal = true)																			{ Set(picture, steal); }

	virtual ~tImageSVG()																									{ Clear(); }

	// Clears the current tImageSVG before loading. Returns success. If false is returned, the object is invalid.
	bool Load(const tString& svgFile, const LoadParams& params = LoadParams());
	bool Load(const uint8* svgFileInMemory, int numBytes, const LoadParams& params = LoadParams());

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise it
	// just copies the data out. After this call the object's ColourSpace is set to sRGB.
	bool Set(tPixel4b* pixels, int width, int height, bool steal = false) override;
	bool Set(tFrame* frame, bool steal = true) override;
	bool Set(tPicture& picture, bool steal = true) override;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																							{ return Pixels ? true : false; }

	int GetWidth() const																									{ return Width; }
	int GetHeight() const																									{ return Height; }

	// After this call you are the owner of the pixels and must eventually delete[] them. This tImageSVG object is
	// invalid afterwards.
	tPixel4b* StealPixels();
	tFrame* GetFrame(bool steal = true) override;
	tPixel4b* GetPixels() const																								{ return Pixels; }

	// A place to store XMP metadata. SVG files may carry this in an x:xmpmeta element, conventionally inside a
	// <metadata> element. This field is populated by the Load calls.
	tMetaData MetaData;

private:
	// Populates MetaData by locating the XMP in the <metadata> section (its <x:xmpmeta> element) and handing that
	// fragment to tMetaData, which parses it via TinyEXIF -- the same consumer used for the PNG/WEBP chunks. We do not
	// parse the whole document as an XML DOM because the only linked XML parser (tinyxml2) rejects the xpacket
	// processing instructions that wrap an SVG's XMP. Returns true if XMP was found and parsed. Failing to find or
	// parse XMP is not an error for the image itself -- most SVGs carry no metadata at all.
	bool PopulateMetaData(const uint8* svgFileInMemory, int numBytes);

	int Width			= 0;
	int Height			= 0;
	tPixel4b* Pixels	= nullptr;
};


// Implementation below this line.


inline void tImageSVG::Clear()
{
	Width		= 0;
	Height		= 0;
	delete[]		Pixels;
	Pixels		= nullptr;

	MetaData.Clear();
	tBaseImage::Clear();
}


}