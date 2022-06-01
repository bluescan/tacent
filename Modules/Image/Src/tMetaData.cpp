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
	double exposureTime = exifInfo.ExposureTime;
	Data[ int(tMetaTag::ExposureTime) ].Set(float(exposureTime));
	NumTagsValid++;

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

	double shutterSpeed = exifInfo.ShutterSpeedValue;
	Data[ int(tMetaTag::ShutterSpeed) ].Set(float(shutterSpeed));
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
}
