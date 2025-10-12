// tColour.cpp
//
// Colour and pixel classes. Both a 32 bit integral representation as well as a 4 component floating point one can be
// found in this file.
//
// Copyright (c) 2006, 2011, 2017, 2020, 2022-2025 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tString.h>
#include <Foundation/tHash.h>
#include <Math/tColour.h>
using namespace tMath;


const char* tColourProfileNames[] =
{
	"LDRsRGB_LDRlA",	// RGB is sRGB space. Alpha in linear. LDR (0.0 to 1.0) for both.
	"LDRgRGB_LDRlA",
	"LDRlRGBA",
	"HDRlRGB_LDRlA",
	"HDRlRGBA",
	"Auto"
};
tStaticAssert(tNumElements(tColourProfileNames) == int(tColourProfile::NumProfiles));


const char* tColourProfileShortNames[] =
{
	"sRGB",
	"gRGB",
	"lRGB",
	"HDRa",
	"HDRA",
	"Auto"
};
tStaticAssert(tNumElements(tColourProfileShortNames) == int(tColourProfile::NumProfiles));


const char* tGetColourProfileName(tColourProfile profile)
{
	if (profile == tColourProfile::Unspecified)		return "Unspecified";
	return tColourProfileNames[int(profile)];
}


const char* tGetColourProfileShortName(tColourProfile profile)
{
	if (profile == tColourProfile::None)			return "None";
	return tColourProfileShortNames[int(profile)];
}


const char* tAlphaModeNames[] =
{
	"Normal",
	"PreMult"
};
tStaticAssert(tNumElements(tAlphaModeNames) == int(tAlphaMode::NumModes));


const char* tAlphaModeShortNames[] =
{
	"Norm",
	"Mult"
};
tStaticAssert(tNumElements(tAlphaModeShortNames) == int(tAlphaMode::NumModes));


const char* tGetAlphaModeName(tAlphaMode mode)
{
	if (mode == tAlphaMode::Unspecified)		return "Unspecified";
	return tAlphaModeNames[int(mode)];
}


const char* tGetAlphaModeShortName(tAlphaMode mode)
{
	if (mode == tAlphaMode::None)				return "None";
	return tAlphaModeShortNames[int(mode)];
}


const char* tChannelTypeNames[] =
{
	"UnsignedIntNormalized",
	"SignedIntNormalized",
	"UnsignedInt",
	"SignedInt",
	"UnsignedFloat",
	"SignedFloat"
};
tStaticAssert(tNumElements(tChannelTypeNames) == int(tChannelType::NumTypes));


const char* tChannelTypeShortNames[] =
{
	"UNORM",
	"SNORM",
	"UINT",
	"SINT",
	"UFLOAT",
	"SFLOAT"
};
tStaticAssert(tNumElements(tChannelTypeShortNames) == int(tChannelType::NumTypes));


const char* tGetChannelTypeName(tChannelType type)
{
	if (type == tChannelType::Unspecified)		return "Unspecified";
	return tChannelTypeNames[int(type)];
}


const char* tGetChannelTypeShortName(tChannelType type)
{
	if (type == tChannelType::NONE)				return "NONE";
	return tChannelTypeShortNames[int(type)];
}


tChannelType tGetChannelType(const char* name)
{
	if (!name || (name[0] == '\0'))
		return tChannelType::Invalid;

	for (int t = 0; t < int(tChannelType::NumTypes); t++)
		if (tStd::tStrcmp(tChannelTypeShortNames[t], name) == 0)
			return tChannelType(t);

	for (int t = 0; t < int(tChannelType::NumTypes); t++)
		if (tStd::tStrcmp(tChannelTypeNames[t], name) == 0)
			return tChannelType(t);

	return tChannelType::Invalid;
}


// Uses C++11 aggregate initialization syntax.
const tColour4b tColour4b::black		= { 0x00, 0x00, 0x00, 0xFF };
const tColour4b tColour4b::white		= { 0xFF, 0xFF, 0xFF, 0xFF };
const tColour4b tColour4b::pink			= { 0xFF, 0x80, 0x80, 0xFF };
const tColour4b tColour4b::red			= { 0xFF, 0x00, 0x00, 0xFF };
const tColour4b tColour4b::green		= { 0x00, 0xFF, 0x00, 0xFF };
const tColour4b tColour4b::blue			= { 0x00, 0x00, 0xFF, 0xFF };
const tColour4b tColour4b::grey			= { 0x80, 0x80, 0x80, 0xFF };
const tColour4b tColour4b::lightgrey	= { 0xC0, 0xC0, 0xC0, 0xFF };
const tColour4b tColour4b::darkgrey		= { 0x40, 0x40, 0x40, 0xFF };
const tColour4b tColour4b::cyan			= { 0x00, 0xFF, 0xFF, 0xFF };
const tColour4b tColour4b::magenta		= { 0xFF, 0x00, 0xFF, 0xFF };
const tColour4b tColour4b::yellow		= { 0xFF, 0xFF, 0x00, 0xFF };
const tColour4b tColour4b::transparent	= { 0x00, 0x00, 0x00, 0x00 };


const tColour4s tColour4s::black		= { 0x0000, 0x0000, 0x0000, 0xFFFF };
const tColour4s tColour4s::white		= { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
const tColour4s tColour4s::pink			= { 0xFFFF, 0x8000, 0x8000, 0xFFFF };
const tColour4s tColour4s::red			= { 0xFFFF, 0x0000, 0x0000, 0xFFFF };
const tColour4s tColour4s::green		= { 0x0000, 0xFFFF, 0x0000, 0xFFFF };
const tColour4s tColour4s::blue			= { 0x0000, 0x0000, 0xFFFF, 0xFFFF };
const tColour4s tColour4s::grey			= { 0x8000, 0x8000, 0x8000, 0xFFFF };
const tColour4s tColour4s::lightgrey	= { 0xC000, 0xC000, 0xC000, 0xFFFF };
const tColour4s tColour4s::darkgrey		= { 0x4000, 0x4000, 0x4000, 0xFFFF };
const tColour4s tColour4s::cyan			= { 0x0000, 0xFFFF, 0xFFFF, 0xFFFF };
const tColour4s tColour4s::magenta		= { 0xFFFF, 0x0000, 0xFFFF, 0xFFFF };
const tColour4s tColour4s::yellow		= { 0xFFFF, 0xFFFF, 0x0000, 0xFFFF };
const tColour4s tColour4s::transparent	= { 0x0000, 0x0000, 0x0000, 0x0000 };


const tColour3f tColour3f::invalid		= { -1.0f, -1.0f, -1.0f };
const tColour3f tColour3f::black		= { 0.00f, 0.00f, 0.00f };
const tColour3f tColour3f::white		= { 1.00f, 1.00f, 1.00f };
const tColour3f tColour3f::hotpink		= { 1.00f, 0.50f, 0.50f };
const tColour3f tColour3f::red			= { 1.00f, 0.00f, 0.00f };
const tColour3f tColour3f::green		= { 0.00f, 1.00f, 0.00f };
const tColour3f tColour3f::blue			= { 0.00f, 0.00f, 1.00f };
const tColour3f tColour3f::grey			= { 0.50f, 0.50f, 0.50f };
const tColour3f tColour3f::lightgrey	= { 0.75f, 0.75f, 0.75f };
const tColour3f tColour3f::darkgrey		= { 0.25f, 0.25f, 0.25f };
const tColour3f tColour3f::cyan			= { 0.00f, 1.00f, 1.00f };
const tColour3f tColour3f::magenta		= { 1.00f, 0.00f, 1.00f };
const tColour3f tColour3f::yellow		= { 1.00f, 1.00f, 0.00f };


const tColour4f tColour4f::invalid		= { -1.0f, -1.0f, -1.0f, -1.0f };
const tColour4f tColour4f::black		= { 0.00f, 0.00f, 0.00f, 1.00f };
const tColour4f tColour4f::white		= { 1.00f, 1.00f, 1.00f, 1.00f };
const tColour4f tColour4f::hotpink		= { 1.00f, 0.50f, 0.50f, 1.00f };
const tColour4f tColour4f::red			= { 1.00f, 0.00f, 0.00f, 1.00f };
const tColour4f tColour4f::green		= { 0.00f, 1.00f, 0.00f, 1.00f };
const tColour4f tColour4f::blue			= { 0.00f, 0.00f, 1.00f, 1.00f };
const tColour4f tColour4f::grey			= { 0.50f, 0.50f, 0.50f, 1.00f };
const tColour4f tColour4f::lightgrey	= { 0.75f, 0.75f, 0.75f, 1.00f };
const tColour4f tColour4f::darkgrey		= { 0.25f, 0.25f, 0.25f, 1.00f };
const tColour4f tColour4f::cyan			= { 0.00f, 1.00f, 1.00f, 1.00f };
const tColour4f tColour4f::magenta		= { 1.00f, 0.00f, 1.00f, 1.00f };
const tColour4f tColour4f::yellow		= { 1.00f, 1.00f, 0.00f, 1.00f };
const tColour4f tColour4f::transparent	= { 0.00f, 0.00f, 0.00f, 0.00f };


void tMath::tRGBToHSV(int& h, int& s, int& v, int r, int g, int b, tAngleMode angleMode)
{
	double min = double( tMin(r, g, b) );
	double max = double( tMax(r, g, b) );
	double delta = max - min;

	v = int(max);
	if (!delta)
	{
		h = s = 0;
		return;
	}

	double temp = delta/max;
	s = int(temp*255.0);

	if (r == int(max))
		temp = double(g-b) / delta;
	else if (g == int(max))
		temp = 2.0 + double(b-r) / delta;
	else
		temp = 4.0 + double(r-g) / delta;

	// Compute hue in correct angle units.
	tAssert((angleMode == tAngleMode::Degrees) || (angleMode == tAngleMode::Norm256));
	double fullCircle = 360.0;
	if (angleMode == tAngleMode::Norm256)
		fullCircle = 256.0;

	temp *= fullCircle / 6.0;
	if (temp < 0.0)
		temp += fullCircle;

	if (temp >= fullCircle)
		temp = 0;

	h = int(temp);
}


void tMath::tHSVToRGB(int& r, int& g, int& b, int h, int s, int v, tAngleMode angleMode)
{
	tAssert((angleMode == tAngleMode::Degrees) || (angleMode == tAngleMode::Norm256));
	int fullCircle = 360;
	if (angleMode == tAngleMode::Norm256)
		fullCircle = 256;

	while (h >= fullCircle)
		h -= fullCircle;

	while (h < 0)
		h += fullCircle;

	tiClamp(h, 0, fullCircle-1);
	tiClamp(s, 0, 255);
	tiClamp(v, 0, 255);

	if (!h && !s)
	{
		r = g = b = v;
		return;
	}

	double max = double(v);
	double delta = max*s / 255.0;
	double min = max - delta;
	double hue = double(h);
	double circle = double(fullCircle);
	double oneSixthCircle	= circle/6.0;		// 60 degrees.
	double oneThirdCircle	= circle/3.0;		// 120 degrees.
	double oneHalfCircle	= circle/2.0;		// 180 degrees.
	double twoThirdCircle	= (2.0*circle)/3.0;	// 240 degrees.
	double fiveSixthCircle	= (5.0*circle)/6.0;	// 300 degrees.

	if (h > fiveSixthCircle || h <= oneSixthCircle)
	{
		r = int(max);
		if (h > fiveSixthCircle)
		{
			g = int(min);
			hue = (hue - circle)/oneSixthCircle;
			b = int(min - hue*delta);
		}
		else
		{
			b = int(min);
			hue = hue / oneSixthCircle;
			g = int(hue*delta + min);
		}
	}
	else if (h > oneSixthCircle && h < oneHalfCircle)
	{
		g = int(max);
		if (h < oneThirdCircle)
		{
			b = int(min);
			hue = (hue/oneSixthCircle - 2.0) * delta;
			r = int(min - hue);
		}
		else
		{
			r = int(min);
			hue = (hue/oneSixthCircle - 2.0) * delta;
			b = int(min + hue);
		}
	}
	else
	{
		b = int(max);
		if (h < twoThirdCircle)
		{
			r = int(min);
			hue = (hue/oneSixthCircle - 4.0) * delta;
			g = int(min - hue);
		}
		else
		{
			g = int(min);
			hue = (hue/oneSixthCircle - 4.0) * delta;
			r = int(min + hue);
		}
	}
}


void tMath::tRGBToHSV(float& h, float& s, float& v, float r, float g, float b, tAngleMode angleMode)
{
	float min = tMin(r, g, b);
	float max = tMax(r, g, b);

	v = max;
	float delta = max - min;
	if (max > 0.0f)
	{
		s = (delta / max);
	}
	else
	{
		// Max is 0 so we're black with v = 0.
		// Hue and Sat are irrelevant at this point but we zero them to be clean.
		s = 0.0f;
		h = 0.0f;
	}

	if (r >= max)
		h = (g - b) / delta;				// Between yellow & magenta.
	else if (g >= max)
		h = 2.0f + (b - r) / delta;			// Between cyan & yellow.
	else
		h = 4.0f + (r - g) / delta;			// Between magenta & cyan.

	float fullCircle = 360.0f;
	switch (angleMode)
	{
		case tAngleMode::Radians:			fullCircle = fTwoPi;	break;
		case tAngleMode::Degrees:			fullCircle = 360.0f;	break;
		case tAngleMode::Norm256:			fullCircle = 256.0f;	break;
		case tAngleMode::NormOne:			fullCircle = 1.0f;		break;
	}

	h *= fullCircle / 6.0f;
	if (h < 0.0f)
		h += fullCircle;
}


void tMath::tHSVToRGB(float& r, float& g, float& b, float h, float s, float v, tAngleMode angleMode)
{
	// If sat is zero we always ignore the hue. That is, we're a shade of grey on the vertical line.
	if (s <= 0.0f)
	{
		r = v;	g = v;	b = v;
		return;
	}

	float fullCircle = 360.0f;
	switch (angleMode)
	{
		case tAngleMode::Radians:			fullCircle = fTwoPi;	break;
		case tAngleMode::Degrees:			fullCircle = 360.0f;	break;
		case tAngleMode::Norm256:			fullCircle = 256.0f;	break;
		case tAngleMode::NormOne:			fullCircle = 1.0f;		break;
	}

	if (h >= fullCircle)
		h = 0.0f;
	h /= (fullCircle/6.0f);

	int i = int(h);
	float rem = h - i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - (s * rem));
	float t = v * (1.0f - (s * (1.0f - rem)));

	switch (i)
	{
		case 0:		r = v;	g = t;	b = p;	break;
		case 1:		r = q;	g = v;	b = p;	break;
		case 2:		r = p;	g = v;	b = t;	break;
		case 3:		r = p;	g = q;	b = v;	break;
		case 4:		r = t;	g = p;	b = v;	break;
		case 5:
		default:	r = v;	g = p;	b = q;	break;
	}
}


tColour4b tMath::tGetColour(const char* colourName)
{
	tString lowerName(colourName);
	lowerName.ToLower();
	uint32 colourHash = tHash::tHashStringFast32(lowerName);
	tColour4b colour = tColour4b::white;

	// This switch uses compile-time hashes. Collisions will be automatically detected by the compiler.
	switch (colourHash)
	{
		case tHash::tHashCT("none"):	colour = 0xFFFFFFFF;	break;
		case tHash::tHashCT("black"):	colour = 0x000000FF;	break;
		default:												break;
	}

	return colour;
}


float tMath::tColourDiffRedmean(const tColour3b& aa, const tColour3b& bb)
{
	tVector3 a; aa.GetDenorm(a);
	tVector3 b; bb.GetDenorm(b);

	float rhat = (a.x + b.x) / 2.0f;

	float dR2 = tSquare(a.x - b.x);
	float dG2 = tSquare(a.y - b.y);
	float dB2 = tSquare(a.z - b.z);

	float term1 = (2.0f + rhat/256.0f)*dR2;
	float term2 = 4.0f * dG2;
	float term3 = (2.0f + ((255.0f-rhat)/256.0f)) * dB2;

	return tSqrt(term1 + term2 + term3);
}


float tMath::tColourDiffRedmean(const tColour4b& aa, const tColour4b& bb)
{
	tVector3 a; aa.GetDenorm(a);
	tVector3 b; bb.GetDenorm(b);

	float rhat = (a.x + b.x) / 2.0f;

	float dR2 = tSquare(a.x - b.x);
	float dG2 = tSquare(a.y - b.y);
	float dB2 = tSquare(a.z - b.z);

	float term1 = (2.0f + rhat/256.0f)*dR2;
	float term2 = 4.0f * dG2;
	float term3 = (2.0f + ((255.0f-rhat)/256.0f)) * dB2;

	return tSqrt(term1 + term2 + term3);
}
