// tMetaData.cpp
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

#include "Image/tMetaData.h"
#include "System/tPrint.h"
#include "TinyEXIF/TinyEXIF.h"
using namespace tImage;


bool tMetaData::Set(const uint8* rawJpgImageData, int numBytes)
{
	Clear();
	TinyEXIF::EXIFInfo exifInfo;
	int errorCode = exifInfo.parseFrom(rawJpgImageData, numBytes);
	if (errorCode)
		return false;

	if (exifInfo.GeoLocation.hasLatLon())
	{
		double lat = exifInfo.GeoLocation.Latitude;
		Data[ int(tMetaTag::LatitudeDD) ].Set(float(lat));
		NumTagsValid++;

		// @todo
		tPrintf("Photo Latitude: %f\n", lat);
	}

	// @todo This function will get quite large.

	return IsValid();
}
