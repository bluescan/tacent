// tQuantize.h
//
// Generic interface for quantizing colours (creating colour palettes).
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

#pragma once
#include <Image/tPixelFormat.h>
namespace tImage
{


enum class tQuantizeMethod
{
	Fixed,					// Supports from 2 to 256 colours. Low quality because used predefined palettes.
	Spatial,				// AKA scolorq. Supports from 2 to 256 colours. Good for 32 colours or fewer.
	Neu,					// AKA NeuQuant. Supports from 64 to 256 colours. Best for 128 to 256.
	Wu						// AKA XiaolinWu. Supports from 2 to 256 colours. Best for 128 to 256.
};


//tColour3i* tQuantizeColours(tPixel3* pixels, int width, int height, int tQuantizeMethod);
namespace tQuantizeSpatial
{
	bool Quantize(int numColours, int width, int height, const tPixel3* pixels, tColour3i* destPalette, uint8* destIndices);
}


}