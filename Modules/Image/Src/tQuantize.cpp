// tQuantize.cpp
//
// This module implements exact palettization of an image for cases when full quantization of an image is not necessary.
// That is, when there will be no colour losses. Exact palettization is possible if the number of unique pixel colours
// is less-than or equal to the number of colours available to the palette. Additionally a function to convert from
// palette/index format back to straght pixels is provided.
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

#include <Math/tColour.h>
#include <Foundation/tMap.h>
#include "Image/tQuantize.h"
namespace tImage {


namespace tQuantize
{
	int FindIndexOfExactColour(const tColour3b* searchSpace, int searchSize, const tColour3b& colour);
}


const char* tQuantize::GetMethodName(Method method)
{
	switch (method)
	{
		case tQuantize::Method::Fixed:		return "Fixed";
		case tQuantize::Method::Spatial:	return "Scolorq";
		case tQuantize::Method::Neu:		return "Neuquant";
		case tQuantize::Method::Wu:			return "Wu";
	}
	return "Invalid";
}


int tQuantize::FindIndexOfExactColour(const tColour3b* searchSpace, int searchSize, const tColour3b& colour)
{
	for (int i = 0; i < searchSize; i++)
		if (colour == searchSpace[i])
			return i;

	return -1;
}


//
// The functions below make up the external interface.
//


bool tQuantize::QuantizeImageExact
(
	int numColours, int width, int height, const tPixel3* pixels,
	tColour3b* destPalette, uint8* destIndices
)
{
	if ((numColours < 2) || (numColours > 256) || (width <= 0) || (height <= 0) || !pixels || !destPalette || !destIndices)
		return false;

	// First we need to find how many unique colours are in the pixels.
	// We do this using a tMap which forces uniqueness on the key. We'll use the int value to count occurrences.
	tMap<tPixel3, int> pixelCountMap;
	for (int xy = 0; xy < width*height; xy++)
		pixelCountMap[ pixels[xy] ]++;

	// Test print counts of each unique colour.
	#if 0
	for (auto pair : pixelCountMap)
	{
		tPixel3& c = pair.Key();
		tPrintf("Key RGB %03d %03d %03d.  Value COUNT: %05d]\n", c.R, c.G, c.B, pair.Value());
	}
	#endif

	int numUnique = pixelCountMap.GetNumItems();
	if (numUnique > numColours)
		return false;

	// Populate the palette.
	tStd::tMemset(destPalette, 0, numColours*sizeof(tColour3b));
	int entry = 0;
	for (auto pair : pixelCountMap)
		destPalette[entry++] = pair.Key();
		
	// Now populate the indices by finding each pixel's colour in the palette.
	for (int p = 0; p < width*height; p++)
	{
		int idx = FindIndexOfExactColour(destPalette, numUnique, pixels[p]);
		tAssert(idx != -1);
		destIndices[p] = idx;
	}

	return true;
}


bool tQuantize::ConvertToPixels
(
	tPixel3* destPixels, int width, int height,
	const tColour3b* srcPalette, const uint8* srcIndices
)
{
	if (!destPixels || (width <= 0) || (height <= 0) || !srcPalette || !srcIndices)
		return false;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int index = x + y*width;
			int palIndex = srcIndices[index];
			tColour3b colour = srcPalette[palIndex];
			destPixels[index] = colour;
		}
	}

	return true;
}


bool tQuantize::ConvertToPixels
(
	tPixel4* destPixels, int width, int height,
	const tColour3b* srcPalette, const uint8* srcIndices, bool preserveDestAlpha
)
{
	if (!destPixels || (width <= 0) || (height <= 0) || !srcPalette || !srcIndices)
		return false;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int index = x + y*width;
			int palIndex = srcIndices[index];
			tColour3b colour = srcPalette[palIndex];
			if (preserveDestAlpha)
				destPixels[index].SetRGB(colour.R, colour.G, colour.B);
			else
				destPixels[index].Set(colour.R, colour.G, colour.B);
		}
	}

	return true;
}


}
