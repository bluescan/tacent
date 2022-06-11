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
#include "Math/tVector3.h"
#include "TinyEXIF/TinyEXIF.h"
using namespace tImage;
using namespace tMath;


const char* tMetaTagNames[] =
{
	// Camera Hardware Tag Names.
	"Make",
	"Model",
	"Serial Number",

	// Geo Location Tag Names.
	"Latitude DD",
	"Longitude DD",
	"Latitude",
	"Longitude",
	"Altitude",
	"Altitude Ref",
	"Altitude Rel",
	"Roll",
	"Pitch",
	"Yaw",
	"VelX",
	"VelY",
	"VelZ",
	"Speed",
	"GPS Survey",
	"GPS Time Stamp",

	// Camera Settings Tag Names.
	"Shutter Speed",
	"Exposure Time",
	"Exposure Bias",
	"F-Stop",
	"Exposure Program",
	"ISO",
	"Aperture",
	"Brightness",
	"Metering Mode",
	"Flash",
	"Focal Length",
	"Orientation",
	"X-Pixels Per Unit",
	"Y-Pixels Per Unit",
	"Length Unit",
	"Bits Per Sample",
	"Image Width",
	"Image Height",
	"Image Width Orig",
	"Image Height Orig",
	"Date/Time Change",
	"Date/Time Orig",
	"Date/Time Digitized",

	// Authoring Note Tag Names.
	"Software",
	"Description",
	"Copyright"
};
tStaticAssert(tNumElements(tMetaTagNames) == int(tMetaTag::NumTags));


const char* tImage::tGetMetaTagName(tMetaTag tag)
{
	return tMetaTagNames[int(tag)];
}


const char* tImage::tGetMetaTagDesc(tMetaTag tag)
{
	return tMetaTagNames[int(tag)];
}


bool tMetaData::Set(const uint8* rawJpgImageData, int numBytes)
{
	Clear();
	TinyEXIF::EXIFInfo exifInfo;
	int errorCode = exifInfo.parseFrom(rawJpgImageData, numBytes);
	if (errorCode)
		return false;

	SetTags_CamHardware(exifInfo);
	SetTags_GeoLocation(exifInfo);
	SetTags_CamSettings(exifInfo);
	SetTags_AuthorNotes(exifInfo);

	// @todo This function will get quite large.
	return IsValid();
}


void tMetaData::SetTags_CamHardware(const TinyEXIF::EXIFInfo& exifInfo)
{
	// Make
	std::string make = exifInfo.Make;
	if (!make.empty())
	{
		Data[ int(tMetaTag::Make) ].Set(make.c_str());
		NumTagsValid++;
	}

	// Model
	std::string model = exifInfo.Model;
	if (!model.empty())
	{
		Data[ int(tMetaTag::Model) ].Set(model.c_str());
		NumTagsValid++;
	}
	
	// SerialNumber
	std::string serial = exifInfo.SerialNumber;
	if (!serial.empty())
	{
		Data[ int(tMetaTag::SerialNumber) ].Set(serial.c_str());
		NumTagsValid++;
	}
}


void tMetaData::SetTags_GeoLocation(const TinyEXIF::EXIFInfo& exifInfo)
{
	// If we have LatLong we should have it in DD and DMS formats.
	if (exifInfo.GeoLocation.hasLatLon())
	{
		// LatitudeDD
		double lat = exifInfo.GeoLocation.Latitude;
		Data[ int(tMetaTag::LatitudeDD) 	].Set(float(lat));
		NumTagsValid++;

		// LatitudeDMS
		// The exifInfo should not have fraction values for the degreed and minutes if they did everythng right.
		int degLat = int ( tMath::tRound(exifInfo.GeoLocation.LatComponents.degrees) );
		int minLat = int ( tMath::tRound(exifInfo.GeoLocation.LatComponents.minutes) );
		int secLat = int ( tMath::tRound(exifInfo.GeoLocation.LatComponents.seconds) );
		char dirLat = exifInfo.GeoLocation.LatComponents.direction;
		tString dmsLat;
		tsPrintf(dmsLat, u8"%d°%d'%d\"%c", degLat, minLat, secLat, dirLat);
		Data[ int(tMetaTag::LatitudeDMS) 	].Set(dmsLat);
		NumTagsValid++;

		// LongitudeDD
		double lon = exifInfo.GeoLocation.Longitude;
		Data[ int(tMetaTag::LongitudeDD) 	].Set(float(lon));
		NumTagsValid++;

		// LongitudeDMS
		int degLon = int ( tMath::tRound(exifInfo.GeoLocation.LonComponents.degrees) );
		int minLon = int ( tMath::tRound(exifInfo.GeoLocation.LonComponents.minutes) );
		int secLon = int ( tMath::tRound(exifInfo.GeoLocation.LonComponents.seconds) );
		char dirLon = exifInfo.GeoLocation.LonComponents.direction;
		tString dmsLon;
		tsPrintf(dmsLon, u8"%d°%d'%d\"%c", degLon, minLon, secLon, dirLon);
		Data[ int(tMetaTag::LongitudeDMS) 	].Set(dmsLon);
		NumTagsValid++;
	}

	if (exifInfo.GeoLocation.hasAltitude())
	{
		double alt = exifInfo.GeoLocation.Altitude;
		Data[ int(tMetaTag::Altitude) 	].Set(float(alt));
		NumTagsValid++;
	}

	if (exifInfo.GeoLocation.hasRelativeAltitude())
	{
		int8 ref = exifInfo.GeoLocation.AltitudeRef;
		tString refStr("No Reference");
		switch (ref)
		{
			case 0:		refStr = "Above Sea Level";		break;
			case -1:	refStr = "Below Sea Level";		break;
		}
		Data[ int(tMetaTag::AltitudeRelGnd) ].Set(refStr);
		NumTagsValid++;

		double altRel = exifInfo.GeoLocation.RelativeAltitude;
		Data[ int(tMetaTag::AltitudeRel) ].Set(float(altRel));
		NumTagsValid++;
	}

	if (exifInfo.GeoLocation.hasOrientation())
	{
		double roll = exifInfo.GeoLocation.RollDegree;
		Data[ int(tMetaTag::Roll) ].Set(float(roll));
		NumTagsValid++;

		double pitch = exifInfo.GeoLocation.PitchDegree;
		Data[ int(tMetaTag::Pitch) ].Set(float(pitch));
		NumTagsValid++;

		double yaw = exifInfo.GeoLocation.YawDegree;
		Data[ int(tMetaTag::Yaw) ].Set(float(yaw));
		NumTagsValid++;
	}

	if (exifInfo.GeoLocation.hasSpeed())
	{
		tVector3 vel
		(
			float(exifInfo.GeoLocation.SpeedX),
			float(exifInfo.GeoLocation.SpeedY),
			float(exifInfo.GeoLocation.SpeedZ)
		);
		Data[ int(tMetaTag::VelX) ].Set(vel.x);
		Data[ int(tMetaTag::VelY) ].Set(vel.y);
		Data[ int(tMetaTag::VelZ) ].Set(vel.z);
		NumTagsValid += 3;

		Data[ int(tMetaTag::Speed) ].Set( vel.Length() );
		NumTagsValid++;
	}

	std::string survey = exifInfo.GeoLocation.GPSMapDatum;
	if (!survey.empty())
	{
		Data[ int(tMetaTag::GPSSurvey) ].Set(survey.c_str());
		NumTagsValid++;
	}

	// tString can handle nullptr.
	tString utcDate(exifInfo.GeoLocation.GPSDateStamp.c_str());
	tString utcTime(exifInfo.GeoLocation.GPSTimeStamp.c_str());
	if (utcDate.IsValid())
		utcDate.Replace(':', '-');
	if (utcTime.IsValid())
	{
		utcTime.ExtractRight('.');
		utcTime.Replace(' ', ':');
	}
	tString dateTime;
	if (utcDate.IsValid() && utcTime.IsValid())
		dateTime = utcDate + " " + utcTime;
	else if (utcDate.IsValid())
		dateTime = utcDate;
	else if (utcTime.IsValid())
		dateTime = utcTime;
	if (dateTime.IsValid())
	{
		Data[ int(tMetaTag::GPSTimeStamp) ].Set(dateTime);
		NumTagsValid++;
	}
}


void tMetaData::SetTags_CamSettings(const TinyEXIF::EXIFInfo& exifInfo)
{
	// These are annoying because they are not independent.
	double shutterSpeed = exifInfo.ShutterSpeedValue;
	double exposureTime = exifInfo.ExposureTime;

	// Compute one from the other if necessary.
	if ((shutterSpeed <= 0.0) && (exposureTime > 0.0))
		shutterSpeed = 1.0 / exposureTime;
	else if ((exposureTime <= 0.0) && (shutterSpeed > 0.0))
		exposureTime = 1.0 / shutterSpeed;

	if (shutterSpeed > 0.0)
	{
		Data[ int(tMetaTag::ShutterSpeed) ].Set(float(shutterSpeed));
		NumTagsValid++;
	}

	if (exposureTime > 0.0)
	{
		Data[ int(tMetaTag::ExposureTime) ].Set(float(exposureTime));
		NumTagsValid++;
	}

	double exposureBias = exifInfo.ExposureBiasValue;
	Data[ int(tMetaTag::ExposureBias) ].Set(float(exposureBias));
	NumTagsValid++;

	double fstop = exifInfo.FNumber;
	Data[ int(tMetaTag::FStop) ].Set(float(fstop));
	NumTagsValid++;

	uint32 prog = exifInfo.ExposureProgram;
	Data[ int(tMetaTag::ExposureProgram) ].Set(prog);
	NumTagsValid++;

	uint32 iso = exifInfo.ISOSpeedRatings;
	Data[ int(tMetaTag::ISO) ].Set(iso);
	NumTagsValid++;

	double aperture = exifInfo.ApertureValue;
	Data[ int(tMetaTag::Aperture) ].Set(float(aperture));
	NumTagsValid++;

	double brightness = exifInfo.BrightnessValue;
	Data[ int(tMetaTag::Brightness) ].Set(float(brightness));
	NumTagsValid++;

	uint32 meterMode = exifInfo.MeteringMode;
	Data[ int(tMetaTag::MeteringMode) ].Set(meterMode);
	NumTagsValid++;

	uint32 flash = exifInfo.Flash;
	Data[ int(tMetaTag::Flash) ].Set(flash);
	NumTagsValid++;

	double focalLength = exifInfo.FocalLength;
	Data[ int(tMetaTag::FocalLength) ].Set(float(focalLength));
	NumTagsValid++;

	uint32 orientation = exifInfo.Orientation;
	Data[ int(tMetaTag::Orientation) ].Set(orientation);
	NumTagsValid++;

	double pixelsPerUnitX = exifInfo.XResolution;
	if (pixelsPerUnitX > 0.0)
	{
		Data[ int(tMetaTag::XPixelsPerUnit) ].Set(float(pixelsPerUnitX));
		NumTagsValid++;
	}

	double pixelsPerUnitY = exifInfo.YResolution;
	if (pixelsPerUnitY > 0.0)
	{
		Data[ int(tMetaTag::YPixelsPerUnit) ].Set(float(pixelsPerUnitY));
		NumTagsValid++;
	}

	uint32 lengthUnit = exifInfo.ResolutionUnit;
	if (lengthUnit)
	{
		Data[ int(tMetaTag::LengthUnit) ].Set(lengthUnit);
		NumTagsValid++;
	}

	uint32 bitsPerComponent = exifInfo.BitsPerSample;
	if (bitsPerComponent)
	{
		Data[ int(tMetaTag::BitsPerSample) ].Set(bitsPerComponent);
		NumTagsValid++;
	}

	uint32 imageWidth = exifInfo.ImageWidth;
	if (imageWidth > 0)
	{
		Data[ int(tMetaTag::ImageWidth) ].Set(imageWidth);
		NumTagsValid++;
	}

	uint32 imageHeight = exifInfo.ImageHeight;
	if (imageHeight > 0)
	{
		Data[ int(tMetaTag::ImageHeight) ].Set(imageHeight);
		NumTagsValid++;
	}

	uint32 imageWidthOrig = exifInfo.RelatedImageWidth;
	if (imageWidthOrig > 0)
	{
		Data[ int(tMetaTag::ImageWidthOrig) ].Set(imageWidthOrig);
		NumTagsValid++;
	}

	uint32 imageHeightOrig = exifInfo.RelatedImageHeight;
	if (imageHeightOrig > 0)
	{
		Data[ int(tMetaTag::ImageHeightOrig) ].Set(imageHeightOrig);
		NumTagsValid++;
	}

	tString dateTimeChange(exifInfo.DateTime.c_str());
	if (dateTimeChange.IsValid())
	{
		tString yyyymmdd = dateTimeChange.ExtractLeft(' ');
		yyyymmdd.Replace(':', '-');
		dateTimeChange = yyyymmdd + " " + dateTimeChange;

		Data[ int(tMetaTag::DateTimeChange) ].Set(dateTimeChange);
		NumTagsValid++;
	}

	tString dateTimeOrig(exifInfo.DateTimeOriginal.c_str());
	if (dateTimeOrig.IsValid())
	{
		tString yyyymmdd = dateTimeOrig.ExtractLeft(' ');
		yyyymmdd.Replace(':', '-');
		dateTimeOrig = yyyymmdd + " " + dateTimeOrig;

		Data[ int(tMetaTag::DateTimeOrig) ].Set(dateTimeOrig);
		NumTagsValid++;
	}

	tString dateTimeDig(exifInfo.DateTimeDigitized.c_str());
	if (dateTimeDig.IsValid())
	{
		tString yyyymmdd = dateTimeDig.ExtractLeft(' ');
		yyyymmdd.Replace(':', '-');
		dateTimeDig = yyyymmdd + " " + dateTimeDig;

		Data[ int(tMetaTag::DateTimeDigit) ].Set(dateTimeDig);
		NumTagsValid++;
	}
}


void tMetaData::SetTags_AuthorNotes(const TinyEXIF::EXIFInfo& exifInfo)
{
	// Software
	tString software = exifInfo.Software.c_str();
	if (software.IsValid())
	{
		Data[ int(tMetaTag::Software) ].Set(software);
		NumTagsValid++;
	}

	// Description
	tString description = exifInfo.ImageDescription.c_str();
	if (description.IsValid())
	{
		Data[ int(tMetaTag::Description) ].Set(description);
		NumTagsValid++;
	}

	// Copyright
	tString copyright = exifInfo.Copyright.c_str();
	if (copyright.IsValid())
	{
		Data[ int(tMetaTag::Copyright) ].Set(copyright);
		NumTagsValid++;
	}
}
