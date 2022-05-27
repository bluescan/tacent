// tMetaData.h
//
// A class to store image meta-data. Some image formats allow comments and other metadata to be stored inside the
// image. For example, jpg files may contain EXIF or XMP meta-data. This class is basically a map of key/value strings
// that may be a member of some tImageXXX types, It currently knows how to parse EXIF and XMP meta-data.
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
#include <Foundation/tMap.h>
#include <Foundation/tString.h>
namespace tImage
{


// These are the common metadata tags, often extracted from either EXIF and/or XMP data.
enum class tMetaTag
{
	// Tag Name			Description													Type
	Invalid = -1,
	LatitudeDD,			// Decimal degrees.											float
	LatitudeDMS,		// Degrees, Minutes, Seconds, Direction. Eg. 42°33'56"N		string
	LongitudeDD,		// Decimal degrees.											float
	LongitudeDMS,		// Degrees, Minutes, Seconds, Direction. Eg. 160°59'4"W		string
	Make,				// Camera make. eg. "Canon".								string
	Model,				// Camera model. eg "Nicon Coolpix 5000".					string
	ExposureTime,
	FStop,
	ExposureProgram,
	ISO,
	ShutterSpeed,
	Aperture,
	ExposureBias,
	MeteringMode,
	Flash,
	FocalLength,
	Orientation,
	XResolution,
	YResolution,
	ResolutionUnit,
	ImageWidth,
	ImageHeight,
	ModifyDate,
	DateTimeOriginal,
	Copyright
};



class tMetaData
{
public:
	tMetaData() { }

	tMetaData(const tMetaData&);
	virtual ~tMetaData() { }

	bool IsValid() const { return (KeyValueMap.GetNumItems() > 0); }

private:
	tMap<tString, tString> KeyValueMap;
};


// Implementation below this line.


}
