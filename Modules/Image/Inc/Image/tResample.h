// tResample.h
//
// Resample an image using various filers like nearest-neighbour, bilinear, and bicubic.
//
// Copyright (c) 2020 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Math/tColour.h>
namespace tImage
{


enum class tResampleFilter
{
	NearestNeighbour,
	Bilinear,
	Bicubic
};


enum class tResampleEdgeMode
{
	Clamp,
	Wrap
};


// Resample the image using the supplied filter. All channels are treated equally. With some resamplers the alpha
// channels gets multiplied into the colours, we do not. This simplicity has some repercussions -- specifically the
// texture author should extend the colours into the areas where the alpha is 0 to make sure rescaling near these
// borders does not introduce colour artifacts when upscaling.
//
// The edge mode is either clamp or wrap. In wrap node if a pixel to the right (or up) is needed for the resample and
// we are at the edge of the image, it is taken from the other side. Some libraries also support a 'reflect' mode but
// since I'm not sure when this is useful, it is being excluded.
bool Resample
(
	tPixel* src, int srcW, int srcH,
	tPixel* dst, int dstW, int dstH,
	tResampleFilter = tResampleFilter::Bilinear,
	tResampleEdgeMode = tResampleEdgeMode::Clamp
);


}