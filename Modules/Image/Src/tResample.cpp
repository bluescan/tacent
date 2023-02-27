// tResample.cpp
//
// Resample an image using various filers like nearest-neighbour, box, bilinear, and various bicubics.
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

#include "Image/tResample.h"
#include "System/tPrint.h"
using namespace tMath;


namespace tImage
{
	enum class FilterDirection
	{
		Horizontal,
		Vertical
	};

	struct FilterParams
	{
		FilterParams() : RatioH(0.0f), RatioV(0.0f) { }
		union { float RatioH; float CubicCoeffB; float LanczosA; };
		union { float RatioV; float CubicCoeffC; };
	};

	tPixel KernelFilterNearest			(const tPixel* src, int srcW, int srcH, float x, float y, FilterDirection, tResampleEdgeMode, const FilterParams&);
	tPixel KernelFilterBox				(const tPixel* src, int srcW, int srcH, float x, float y, FilterDirection, tResampleEdgeMode, const FilterParams&);
	tPixel KernelFilterBilinear			(const tPixel* src, int srcW, int srcH, float x, float y, FilterDirection, tResampleEdgeMode, const FilterParams&);
	tPixel KernelFilterBicubic			(const tPixel* src, int srcW, int srcH, float x, float y, FilterDirection, tResampleEdgeMode, const FilterParams&);
	tPixel KernelFilterLanczos			(const tPixel* src, int srcW, int srcH, float x, float y, FilterDirection, tResampleEdgeMode, const FilterParams&);

	int GetSrcIndex(int idx, int count, tResampleEdgeMode);
	float ComputeCubicWeight(float x, float b, float c);
	float ComputeLanczosWeight(float x, float a);
}


const char* tImage::tResampleFilterNames[int(tResampleFilter::NumFilters)+1] =
{
	"Nearest Neighbour",
	"Box",
	"Bilinear",
	"Bicubic Standard",
	"Bicubic CatmullRom",
	"Bicubic Mitchell",
	"Bicubic Cardinal",
	"Bicubic BSpline",
	"Lanczos Narrow",
	"Lanczos Normal",
	"Lanczos Wide",
	"None"
};


const char* tImage::tResampleFilterNamesSimple[int(tResampleFilter::NumFilters)+1] =
{
	"nearest",
	"box",
	"bilinear",
	"bicubic",
	"bicubic_catmullrom",
	"bicubic_mitchell",
	"bicubic_cardinal",
	"bicubic_bspline",
	"lanczos_narrow",
	"lanczos",
	"lanczos_wide",
	"none"
};


const char* tImage::tResampleEdgeModeNames[int(tResampleEdgeMode::NumEdgeModes)+1] =
{
	"Clamp",
	"Wrap",
	"None"
};


const char* tImage::tResampleEdgeModeNamesSimple[int(tResampleEdgeMode::NumEdgeModes)+1] =
{
	"clamp",
	"wrap",
	"none"
};


inline int tImage::GetSrcIndex(int idx, int count, tResampleEdgeMode edgeMode)
{
	tAssert(count > 0);
	switch (edgeMode)
	{
		case tResampleEdgeMode::Clamp:
			return tClamp(idx, 0, count-1);

		case tResampleEdgeMode::Wrap:
			return tMod(idx, count);
	}

	return -1;
}


bool tImage::Resample
(
	tPixel* src, int srcW, int srcH,
	tPixel* dst, int dstW, int dstH,
	tResampleFilter resampleFilter,
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

	float ratioH = (dstW > 1) ? (float(srcW) - 1.0f) / float(dstW - 1) : 1.0f;
	float ratioV = (dstH > 1) ? (float(srcH) - 1.0f) / float(dstH - 1) : 1.0f;

	// Decide what filer kernel to use. Different kernels may set different values in FilterParams.
	FilterParams params;
	typedef tPixel (*KernelFilterFn)(const tPixel* src, int srcW, int srcH, float x, float y, FilterDirection, tResampleEdgeMode, const FilterParams&);
	KernelFilterFn kernel;
	switch (resampleFilter)
	{
		case tResampleFilter::Nearest:
			kernel = KernelFilterNearest;
			break;

		case tResampleFilter::Box:
			params.RatioH = ratioH;
			params.RatioV = ratioV;
			kernel = KernelFilterBox;
			break;

		case tResampleFilter::Bilinear:
			kernel = KernelFilterBilinear;
			break;

		case tResampleFilter::Bicubic_Standard:		// Cardinal.				B=0		C=3/4
			params.CubicCoeffB = 0.0f;
			params.CubicCoeffC = 3.0f/4.0f;
			kernel = KernelFilterBicubic;
			break;

		case tResampleFilter::Bicubic_CatmullRom:	// Cardinal.				B=0		C=1/2
			params.CubicCoeffB = 0.0f;
			params.CubicCoeffC = 1.0f/2.0f;
			kernel = KernelFilterBicubic;
			break;

		case tResampleFilter::Bicubic_Mitchell:		// Balanced.				B=1/3	C=1/3
			params.CubicCoeffB = 1.0f/3.0f;
			params.CubicCoeffC = 1.0f/3.0f;
			kernel = KernelFilterBicubic;
			break;

		case tResampleFilter::Bicubic_Cardinal:		// Pure Cardinal.			B=0		C=1
			params.CubicCoeffB = 0.0f;
			params.CubicCoeffC = 1.0f;
			kernel = KernelFilterBicubic;
			break;

		case tResampleFilter::Bicubic_BSpline:		// Pure BSpline. Blurry.	B=1		C=0
			params.CubicCoeffB = 1.0f;
			params.CubicCoeffC = 0.0f;
			kernel = KernelFilterBicubic;
			break;

		case tResampleFilter::Lanczos_Narrow:		// Lanczos. Ringy/Sharp.	A=2
			params.LanczosA = 2.0f;
			kernel = KernelFilterLanczos;
			break;

		case tResampleFilter::Lanczos_Normal:		// Lanczos. Ringy/Sharp.	A=3
			params.LanczosA = 3.0f;
			kernel = KernelFilterLanczos;
			break;

		case tResampleFilter::Lanczos_Wide:			// Lanczos. Ringy/Sharp.	A=4
			params.LanczosA = 4.0f;
			kernel = KernelFilterLanczos;
			break;

		case tResampleFilter::Invalid:
		default:
			return false;
	}	

	// By convention do horizontal first. Outer loop is for each src row.
	// hri stands for hozontal-resized-image.
	tPixel* hri = new tPixel[dstW*srcH];
	for (int r = 0; r < srcH; r++)
	{
		// Fill in each dst pixel for the src row,
		float y = float(r);
		for (int c = 0; c < dstW; c++)
		{
			tPixel& dstPixel = hri[dstW*r + c];
			float x = float(c) * ratioH;
			dstPixel = kernel(src, srcW, srcH, x, y, FilterDirection::Horizontal, edgeMode, params);
		}
	}

	// Vertical resampling. Source is the horizontally resized image.
	for (int c = 0; c < dstW; c++)
	{
		float x = float(c);
		for (int r = 0; r < dstH; r++)
		{
			tPixel& dstPixel = dst[dstW*r + c];
			float y = float(r) * ratioV;
			dstPixel = kernel(hri, dstW, srcH, x, y, FilterDirection::Vertical, edgeMode, params);
		}
	}

	delete[] hri;
	return true;
}


tPixel tImage::KernelFilterNearest
(
	const tPixel* src, int srcW, int srcH, float x, float y,
	FilterDirection dir, tResampleEdgeMode edgeMode, const FilterParams& params
)
{
	int ix = tClamp(int(x + 0.5f), 0, srcW-1);
	int iy = tClamp(int(y + 0.5f), 0, srcH-1);
	return src[srcW*iy + ix];
}


tPixel tImage::KernelFilterBox
(
	const tPixel* src, int srcW, int srcH, float x, float y,
	FilterDirection dir, tResampleEdgeMode edgeMode, const FilterParams& params
)
{
	float ratio = (dir == FilterDirection::Horizontal) ? params.RatioH : params.RatioV;
	int pixelDist = int(ratio + 1.0f);
	float maxDist = ratio;
	float weightTotal = 0.0f;
	tVector4 sampleTotal = tVector4::zero;

	for (int ks = 1 - pixelDist; ks <= pixelDist; ks++)
	{
		int ix = (dir == FilterDirection::Horizontal) ? int(x) + ks : int(x);
		int iy = (dir == FilterDirection::Horizontal) ? int(y)      : int(y) + ks;

		float dist = tAbs( (dir == FilterDirection::Horizontal) ? x - float(ix) : y - float(iy) );
		float weight = 0.0f;

		int srcX = GetSrcIndex(ix ,srcW, edgeMode);
		int srcY = GetSrcIndex(iy ,srcH, edgeMode);
		tPixel srcPixel = src[srcW*srcY + srcX];

		if (ratio >= 1.0f)
		{
			weight = 1.0f - tMin(maxDist, dist)/maxDist;
		}
		else
		{
			if (dist >= (0.5f - ratio))
				weight = 1.0f - dist;
			else
				return srcPixel;		// Box is inside src pixel. Done.
		}

		sampleTotal.x += srcPixel.R * weight;
		sampleTotal.y += srcPixel.G * weight;
		sampleTotal.z += srcPixel.B * weight;
		sampleTotal.w += srcPixel.A * weight;
		weightTotal += weight;
	}

	// Renormalize sampleTotal back to [0, 256).
	sampleTotal /= weightTotal;
	return tPixel
	(
		tClamp(int(tRound(sampleTotal.x)), 0, 255),
		tClamp(int(tRound(sampleTotal.y)), 0, 255),
		tClamp(int(tRound(sampleTotal.z)), 0, 255),
		tClamp(int(tRound(sampleTotal.w)), 0, 255)
	);
}


tPixel tImage::KernelFilterBilinear
(
	const tPixel* src, int srcW, int srcH, float x, float y,
	FilterDirection dir, tResampleEdgeMode edgeMode, const FilterParams& params
)
{
	int ix = int(x);
	int iy = int(y);

	int srcXa = GetSrcIndex(ix  , srcW, edgeMode);
	int srcYa = GetSrcIndex(iy  , srcH, edgeMode);
	int srcXb = GetSrcIndex(ix+1, srcW, edgeMode);
	int srcYb = GetSrcIndex(iy+1, srcH, edgeMode);

	tPixel a = src[srcW*srcYa + srcXa];
	tPixel b = (dir == FilterDirection::Horizontal) ?
		src[srcW*srcYa + srcXb] :
		src[srcW*srcYb + srcXa];

	float weight = (dir == FilterDirection::Horizontal) ? float(x)-ix : float(y)-iy;

	tVector4 av, bv, rv;
	a.GetDenorm(av);
	b.GetDenorm(bv);
	rv = av*(1.0f-weight) + bv*weight;
	tiClamp(tiRound(rv.x), 0.0f, 255.0f);
	tiClamp(tiRound(rv.y), 0.0f, 255.0f);
	tiClamp(tiRound(rv.z), 0.0f, 255.0f);
	tiClamp(tiRound(rv.w), 0.0f, 255.0f);

	return tPixel(int(rv.x), int(rv.y), int(rv.z), int(rv.w));
}


// This function is the cubic filter workhorse. It implements the weight function k(x) found
// at https://entropymine.com/imageworsener/bicubic/
// If that site ever goes down, the original paper is from Mitchell and Netravali (1988).
float tImage::ComputeCubicWeight(float x, float b, float c)
{
	float xa = tAbs(x);

	// Case 3. Early exit the 'otherwise' case.
	if (xa >= 2.0f)
		return 0.0f;

	// Common terms in the other two cases.
	float c6  = 6.0f*c;
	float xc  = tCube(xa);
	float b12 = 12.0f*b;
	float xs  = tSquare(xa);
	float r = 0.0f;

	if (xa < 1.0f)
	{
		// Case 1.
		float b9 = 9.0f*b;
		float b2 = 2.0f*b;
		r = (12.0f-b9-c6)*xc + (-18.0f+b12+c6)*xs + (6.0f-b2);
	}
	else
	{
		// Case 2.
		float b6 = 6.0f*b;
		float c30 = 30.0f*c;
		float c48 = 48.0f*c;
		float b8 = 8.0f*b;
		float c24 = 24.0f*c;
		r = (-b-c6)*xc + (b6+c30)*xs + (-b12-c48)*xa + (b8+c24);
	}

	return tClampMin(r/6.0f, 0.0f);
}


tPixel tImage::KernelFilterBicubic
(
	const tPixel* src, int srcW, int srcH, float x, float y,
	FilterDirection dir, tResampleEdgeMode edgeMode, const FilterParams& params
)
{
	float weightTotal = 0.0f;
	tVector4 sampleTotal = tVector4::zero;

	for (int ks = -2; ks < 2; ks++)
	{
		int ix = (dir == FilterDirection::Horizontal) ? int(x) + ks  : int(x);
		int iy = (dir == FilterDirection::Horizontal) ? int(y)       : int(y) + ks;

		float diff = (dir == FilterDirection::Horizontal) ? x - float(ix) : y - float(iy);
		float weight = ComputeCubicWeight(diff, params.CubicCoeffB, params.CubicCoeffC);

		int srcX = GetSrcIndex(ix, srcW, edgeMode);
		int srcY = GetSrcIndex(iy, srcH, edgeMode);
		tPixel srcPixel = src[srcW*srcY + srcX];

		sampleTotal.x += srcPixel.R * weight;
		sampleTotal.y += srcPixel.G * weight;
		sampleTotal.z += srcPixel.B * weight;
		sampleTotal.w += srcPixel.A * weight;
		weightTotal += weight;
	}

	// Renormalize sampleTotal back to [0, 256).
	sampleTotal /= weightTotal;
	return tPixel
	(
		tClamp(int(tRound(sampleTotal.x)), 0, 255),
		tClamp(int(tRound(sampleTotal.y)), 0, 255),
		tClamp(int(tRound(sampleTotal.z)), 0, 255),
		tClamp(int(tRound(sampleTotal.w)), 0, 255)
	);
}


inline float tImage::ComputeLanczosWeight(float x, float a)
{
	// See https://en.wikipedia.org/wiki/Lanczos_resampling
	return ((x >= -a) && (x <= a)) ? tSinc(x) * tSinc(x/a) : 0.0f;
}


tPixel tImage::KernelFilterLanczos
(
	const tPixel* src, int srcW, int srcH, float x, float y,
	FilterDirection dir, tResampleEdgeMode edgeMode, const FilterParams& params
)
{
	int pixelDist = int(params.LanczosA);
	float weightTotal = 0.0f;
	tVector4 sampleTotal = tVector4::zero;

	for (int ks = -pixelDist; ks < pixelDist; ks++)
	{
		int ix = (dir == FilterDirection::Horizontal) ? int(x) + ks : int(x);
		int iy = (dir == FilterDirection::Horizontal) ? int(y)      : int(y) + ks;

		float diff = (dir == FilterDirection::Horizontal) ? x - float(ix) : y - float(iy);
		float weight = ComputeLanczosWeight(tAbs(diff), params.LanczosA);

		int srcX = GetSrcIndex(ix, srcW, edgeMode);
		int srcY = GetSrcIndex(iy, srcH, edgeMode);
		tPixel srcPixel = src[srcW*srcY + srcX];

		sampleTotal.x += srcPixel.R * weight;
		sampleTotal.y += srcPixel.G * weight;
		sampleTotal.z += srcPixel.B * weight;
		sampleTotal.w += srcPixel.A * weight;
		weightTotal += weight;
	}

	// Renormalize totalSamples back to [0, 256).
	sampleTotal /= weightTotal;
	return tPixel
	(
		tClamp(int(tRound(sampleTotal.x)), 0, 255),
		tClamp(int(tRound(sampleTotal.y)), 0, 255),
		tClamp(int(tRound(sampleTotal.z)), 0, 255),
		tClamp(int(tRound(sampleTotal.w)), 0, 255)
	);
}
