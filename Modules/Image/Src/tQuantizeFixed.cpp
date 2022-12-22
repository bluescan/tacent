// tQuantizeFFFixed.cpp
//
// This module implements quantization of an image based on a fixed palette of colours as well as a function to perform
// an exact palettization if the number of unique pixel colours is less-than or equal to the number of colours available
// to the palette.
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

#include <Math/tColour.h>
#include "Foundation/tMap.h"
#include "System/tPrint.h"
#include "Image/tQuantize.h"
namespace tImage {


namespace tQuantizeFixed
{
	int FindIndexOfClosestColour_Redmean(const tColour3i* searchSpace, int searchSize, const tColour3i& colour);
	int FindIndexOfExactColour(const tColour3i* searchSpace, int searchSize, const tColour3i& colour);
}


int tQuantizeFixed::FindIndexOfClosestColour_Redmean(const tColour3i* searchSpace, int searchSize, const tColour3i& colour)
{
	float closest = 1000.0f;
	int closestIndex = -1;

	for (int i = 0; i < searchSize; i++)
	{
		float diff = tMath::tColourDiffRedmean(colour, searchSpace[i]);
		if (diff < closest)
		{
			closest = diff;
			closestIndex = i;
		}
	}
	return closestIndex;
}


int tQuantizeFixed::FindIndexOfExactColour(const tColour3i* searchSpace, int searchSize, const tColour3i& colour)
{
	for (int i = 0; i < searchSize; i++)
		if (colour == searchSpace[i])
			return i;

	return -1;
}



//
// The functions below make up the external interface.
//


bool tQuantizeFixed::QuantizeImageExact
(
	int numColours, int width, int height, const tPixel3* pixels,
	tColour3i* destPalette, uint8* destIndices
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
	tStd::tMemset(destPalette, 0, numColours*sizeof(tColour3i));
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


bool tQuantizeFixed::QuantizeImage
(
	int numColours, int width, int height, const tPixel3* pixels,
	tColour3i* destPalette, uint8* destIndices,
	bool checkExact
)
{
	return true;
}


}
