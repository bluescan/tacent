// tResample.cpp
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
#include "Image/tResample.h"
using namespace tMath;


namespace tImage
{
	enum class FilterDirection
	{
		Horizontal,
		Vertical
	};
	tPixel KernelNearest(tPixel* src,int srcW, int srcH, float x, float y, FilterDirection, tResampleEdgeMode);
	tPixel KernelBilinear(tPixel* src,int srcW, int srcH, float x, float y, FilterDirection, tResampleEdgeMode);
	int GetSrcIndex(int idx, int count, tResampleEdgeMode);
}


inline int tImage::GetSrcIndex(int idx, int count, tResampleEdgeMode edgeMode)
{
	switch (edgeMode)
	{
		case tResampleEdgeMode::Clamp:
			return tClamp(idx, 0, count-1);

		case tResampleEdgeMode::Wrap:
		{
			if (idx >= count)
				return idx-count;
			else if (idx < 0)
				return count - idx;
			else
				return idx;
		}
	}

	return -1;
}


bool tImage::Resample
(
	tPixel* src, int srcW, int srcH,
	tPixel* dst, int dstW, int dstH,
	tResampleKernel resampleKernel,
	tResampleEdgeMode edgeMode
)
{
	if (!src || !dst || srcW<=0 || srcH<=0 || dstW<=0 || dstH<=0)
		return false;

	if ((srcW == dstW) && (srcH == dstH))
	{
		for (int p = 0; p < srcW*srcH; p++)
			dst[p] = src[p];

		return true;
	}

	// Decide what filer kernel to use.
	typedef tPixel (*KernelFn)(tPixel* src,int srcW, int srcH, float x, float y, FilterDirection, tResampleEdgeMode);
	KernelFn kernel;
	switch (resampleKernel)
	{
		case tResampleKernel::Nearest:
			kernel = KernelNearest;
			break;

		case tResampleKernel::Bilinear:
			kernel = KernelBilinear;
			break;

		default:
			return false;
	}	

	// By convention do horizontal first. Outer loop is for each src row.
	float wratio = (dstW > 1) ? (float(srcW) - 1.0f) / float(dstW - 1) : 1.0f;
	tPixel* horizResampledImage = new tPixel[dstW*srcH];
	for (int r = 0; r < srcH; r++)
	{
		// Fill in each dst pixel for the src row,
		float y = float(r);
		for (int c = 0; c < dstW; c++)
		{
			tPixel& dstPixel = horizResampledImage[dstW*r + c];
			float x = float(c) * wratio;
			dstPixel = kernel(src, srcW, srcH, x, y, FilterDirection::Horizontal, edgeMode);
		}
	}

	// Vertical resampling. Source is the horizontally scaled image.
	float hratio = (dstH > 1) ? (float(srcH) - 1.0f) / float(dstH - 1) : 1.0f;
	for (int c = 0; c < dstW; c++)
	{
		float x = float(c);
		for (int r = 0; r < dstH; r++)
		{
			tPixel& dstPixel = dst[dstW*r + c];
			float y = float(r) * hratio;
			dstPixel = kernel(horizResampledImage, dstW, srcH, x, y, FilterDirection::Vertical, edgeMode);
		}
	}

	delete[] horizResampledImage;
	return true;
}


tPixel tImage::KernelNearest(tPixel* src,int srcW, int srcH, float x, float y, FilterDirection dir, tResampleEdgeMode edgeMode)
{
	int ix = tClamp(int(x + 0.5f), 0, srcW-1);
	int iy = tClamp(int(y + 0.5f), 0, srcH-1);
	return src[srcW*iy + ix];
}


tPixel tImage::KernelBilinear(tPixel* src,int srcW, int srcH, float x, float y, FilterDirection dir, tResampleEdgeMode edgeMode)
{
	int ix = int(x);
	int iy = int(y);

	int srcXa = GetSrcIndex(ix  , srcW, edgeMode);
	int srcYa = GetSrcIndex(iy  , srcH, edgeMode);
	int srcXb = GetSrcIndex(ix+1, srcW, edgeMode);
	int srcYb = GetSrcIndex(iy+1, srcH, edgeMode);

	tPixel a = src[srcW*srcYa + srcXa];
	tPixel b = (dir == FilterDirection::Horizontal) ?
		src[srcW*srcYa  + srcXb] :
		src[srcW*srcYb + srcXa];

	float weight = (dir == FilterDirection::Horizontal) ? float(x)-ix : float(y)-iy;
	return a*(1.0f-weight) + b*weight;
}
