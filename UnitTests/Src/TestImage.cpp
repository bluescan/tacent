// TestImage.cpp
//
// Image module tests.
//
// Copyright (c) 2017, 2019-2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Image/tTexture.h>
#include <Image/tImageDDS.h>
#include <Image/tImageKTX.h>
#include <Image/tImageEXR.h>
#include <Image/tImageGIF.h>
#include <Image/tImageHDR.h>
#include <Image/tImageICO.h>
#include <Image/tImageJPG.h>
#include <Image/tImagePNG.h>
#include <Image/tImageAPNG.h>
#include <Image/tImageTGA.h>
#include <Image/tImageWEBP.h>
#include <Image/tImageXPM.h>
#include <System/tFile.h>
#include "UnitTests.h"
using namespace tImage;
namespace tUnitTest
{


tTestUnit(ImageLoad)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageLoad)

	// Test direct loading classes.
	tImageTGA imgTGA("TestData/Images/WhiteBorderRLE.tga");
	tRequire(imgTGA.IsValid());

	tImageAPNG imgAPNG("TestData/Images/Flame.apng");
	tRequire(imgAPNG.IsValid());

	tImageDDS imgDDS("TestData/Images/DDS/BC1DXT1_RGB_Legacy.dds");
	tRequire(imgDDS.IsValid());

	tImageEXR imgEXR("TestData/Images/Desk.exr");
	tRequire(imgEXR.IsValid());

	tImageGIF imgGIF("TestData/Images/8-cell-simple.gif");
	tRequire(imgGIF.IsValid());

	tImageHDR imgHDR("TestData/Images/mpi_atrium_3.hdr");
	tRequire(imgHDR.IsValid());

	tImageICO imgICO("TestData/Images/UpperBounds.ico");
	tRequire(imgICO.IsValid());

	tImageJPG imgJPG("TestData/Images/WiredDrives.jpg");
	tRequire(imgJPG.IsValid());

	tImageTIFF imgTIFF("TestData/Images/Tiff_NoComp.tif");
	tRequire(imgTIFF.IsValid());

	tImageWEBP imgWEBP("TestData/Images/RockyBeach.webp");
	tRequire(imgWEBP.IsValid());
}


tTestUnit(ImageSave)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageSave)

	tPicture newPngA("TestData/Images/Xeyes.png");
	newPngA.Save("TestData/Images/WrittenNewA.png");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenNewA.png"));

	tPicture newPngB("TestData/Images/TextCursor.png");
	newPngB.Save("TestData/Images/WrittenNewB.png");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenNewB.png"));

	tPicture apngPicForSave("TestData/Images/Flame.apng");
	apngPicForSave.SaveWEBP("TestData/Images/WrittenFlameOneFrame.webp");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenFlameOneFrame.webp"));

	// Test writing webp images.
	tPicture exrPicForSave("TestData/Images/Desk.exr");
	exrPicForSave.SaveWEBP("TestData/Images/WrittenDesk.webp");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenDesk.webp"));
}


tTestUnit(ImageTexture)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageTexture)

	// Test dxt1 texture.
	tTexture dxt1Tex("TestData/Images/DDS/BC1DXT1_RGB_Legacy.dds");
	tRequire(dxt1Tex.IsValid());

	tChunkWriter writer("TestData/Images/Written_BC1DXT1_RGB_Legacy.tac");
	dxt1Tex.Save(writer);
	tRequire( tSystem::tFileExists("TestData/Images/Written_BC1DXT1_RGB_Legacy.tac") );

	tChunkReader reader("TestData/Images/Written_BC1DXT1_RGB_Legacy.tac");
	dxt1Tex.Load( reader.Chunk() );
	tRequire(dxt1Tex.IsValid());

	// Test cubemap.
	tTexture cubemap("TestData/Images/DDS/CubemapLayoutGuide.dds");
	tRequire(cubemap.IsValid());

	// Test jpg to texture. This will do conversion to BC1.
	tTexture bc1Tex("TestData/Images/WiredDrives.jpg", true);
	tRequire(bc1Tex.IsValid());
	tChunkWriter chunkWriterBC1("TestData/Images/Written_WiredDrives_BC1.tac");
	bc1Tex.Save(chunkWriterBC1);
	tRequire( tSystem::tFileExists("TestData/Images/Written_WiredDrives_BC1.tac"));

	// Test ico with alpha to texture. This will do conversion to BC3.
	tTexture bc3Tex("TestData/Images/UpperBounds.ico", true);
	tRequire(bc3Tex.IsValid());
	tChunkWriter chunkWriterBC3("TestData/Images/Written_UpperBounds_BC3.tac");
	bc3Tex.Save(chunkWriterBC3);
	tRequire( tSystem::tFileExists("TestData/Images/Written_UpperBounds_BC3.tac"));
}


tTestUnit(ImagePicture)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImagePicture)

	// Test generate layers.
	tPicture srcPic("TestData/Images/UpperB.bmp");
	tRequire(srcPic.IsValid());
	tPrintf("GenLayers Orig W=%d H=%d\n", srcPic.GetWidth(), srcPic.GetHeight());
	tList<tLayer> layers;
	srcPic.GenerateLayers(layers);
	int lev = 0;
	for (tLayer* lay = layers.First(); lay; lay = lay->Next(), lev++)
		tPrintf("GenLayers Mip:%02d W=%d H=%d\n", lev, lay->Width, lay->Height);
	tRequire(layers.GetNumItems() == 10);

	// Test tPicture loading bmp and saving as tga.
	tPicture bmpPicUB("TestData/Images/UpperB.bmp");
	tRequire(bmpPicUB.IsValid());
	bmpPicUB.Save("TestData/Images/WrittenUpperB.tga");
	bmpPicUB.Save("TestData/Images/WrittenUpperB.bmp");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenUpperB.tga"));

	tPicture bmpPicA("TestData/Images/Bmp_Alpha.bmp");
	tRequire(bmpPicA.IsValid());
	bmpPicA.Save("TestData/Images/WrittenBmp_Alpha.tga");
	bmpPicA.Save("TestData/Images/WrittenBmp_Alpha.bmp");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenBmp_Alpha.tga"));

	tPicture bmpPicL("TestData/Images/Bmp_Lambda.bmp");
	tRequire(bmpPicL.IsValid());
	bmpPicL.Save("TestData/Images/WrittenBmp_Lambda.tga");
	bmpPicL.Save("TestData/Images/WrittenBmp_Lambda.bmp");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenBmp_Lambda.tga"));

	tPicture bmpPicRL("TestData/Images/Bmp_RefLena.bmp");
	tRequire(bmpPicRL.IsValid());
	bmpPicRL.Save("TestData/Images/WrittenBmp_RefLena.tga");
	bmpPicRL.Save("TestData/Images/WrittenBmp_RefLena.bmp");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenBmp_RefLena.tga"));

	tPicture bmpPicRL101("TestData/Images/Bmp_RefLena101.bmp");
	tRequire(bmpPicRL101.IsValid());
	bmpPicRL101.Save("TestData/Images/WrittenBmp_RefLena101.tga");
	bmpPicRL101.Save("TestData/Images/WrittenBmp_RefLena101.bmp");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenBmp_RefLena101.tga"));

	tPicture bmpPicRLFlip("TestData/Images/Bmp_RefLenaFlip.bmp");
	tRequire(bmpPicRLFlip.IsValid());
	bmpPicRLFlip.Save("TestData/Images/WrittenBmp_RefLenaFlip.tga");
	bmpPicRLFlip.Save("TestData/Images/WrittenBmp_RefLenaFlip.bmp");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenBmp_RefLenaFlip.tga"));

	tPicture pngPicIcos("TestData/Images/Icos4D.png");
	tRequire(pngPicIcos.IsValid());
	pngPicIcos.Save("TestData/Images/WrittenBmp_Icos4D.bmp");
	pngPicIcos.Save("TestData/Images/WrittenBmp_Icos4D.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenBmp_Icos4D.tga"));

	// Test tPicture loading jpg and saving as tga.
	tPicture jpgPic("TestData/Images/WiredDrives.jpg");
	tRequire(jpgPic.IsValid());
	jpgPic.Save("TestData/Images/WrittenWiredDrives.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenWiredDrives.tga"));

	tPicture exrPic("TestData/Images/Desk.exr");
	tRequire(exrPic.IsValid());
	exrPic.Save("TestData/Images/WrittenDesk.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenDesk.tga"));

	tPicture apngPic("TestData/Images/Flame.apng", 100);
	tRequire(apngPic.IsValid());
	apngPic.Save("TestData/Images/WrittenFlame.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenFlame.tga"));

	// Test tPicture loading xpm and saving as tga.
	tPicture xpmPic("TestData/Images/Crane.xpm");
	tRequire(xpmPic.IsValid());
	xpmPic.Save("TestData/Images/WrittenCrane.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenCrane.tga"));

	// Test tPicture loading png (with alpha channel) and saving as tga (with alpha channel).
	tPicture pngPic("TestData/Images/Xeyes.png");
	tRequire(pngPic.IsValid());
	pngPic.SaveTGA("TestData/Images/WrittenXeyes.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenXeyes.tga"));

	// Test saving tPicture in other supported formats.
	pngPic.Save("TestData/Images/WrittenXeyesTGA.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenXeyesTGA.tga"));

	pngPic.Save("TestData/Images/WrittenXeyesBMP.bmp");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenXeyesBMP.bmp"));

	pngPic.Save("TestData/Images/WrittenXeyesJPG.jpg");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenXeyesJPG.jpg"));

	// Test tiff file loading and saving.
	tPicture tifPic_NoComp("TestData/Images/Tiff_NoComp.tif");
	tRequire(tifPic_NoComp.IsValid());
	tifPic_NoComp.Save("TestData/Images/WrittenTiff_NoComp.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenTiff_NoComp.tga"));

	tPicture tifPic_Pack("TestData/Images/Tiff_Pack.tif");
	tRequire(tifPic_Pack.IsValid());
	tifPic_Pack.Save("TestData/Images/WrittenTiff_Pack.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenTiff_Pack.tga"));

	tPicture tifPic_LZW("TestData/Images/Tiff_LZW.tif");
	tRequire(tifPic_LZW.IsValid());
	tifPic_LZW.Save("TestData/Images/WrittenTiff_LZW.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenTiff_LZW.tga"));

	tPicture tifPic_ZIP("TestData/Images/Tiff_ZIP.tif");
	tRequire(tifPic_ZIP.IsValid());
	tifPic_ZIP.Save("TestData/Images/WrittenTiff_ZIP.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenTiff_ZIP.tga"));

	tPicture exrPicForSave2("TestData/Images/Desk.exr");
	exrPicForSave2.SaveGIF("TestData/Images/WrittenDesk.gif");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenDesk.gif"));

	tPicture exrPicToSaveAsAPNG("TestData/Images/Desk.exr");
	exrPicToSaveAsAPNG.SaveAPNG("TestData/Images/WrittenDesk.apng");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenDesk.apng"));

	tPicture exrPicToSaveAsTIFF("TestData/Images/Desk.exr");
	exrPicToSaveAsTIFF.SaveTIFF("TestData/Images/WrittenDesk.tiff");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenDesk.tiff"));
}


static void PrintMetaDataTag(const tMetaData& metaData, tMetaTag tag)
{
	tString tagName = tGetMetaTagName(tag);
	tString tagDesc = tGetMetaTagDesc(tag);
	tPrintf("TagName [%s]\n", tagName.Chr());

	// Just want to print all on one line for now.
	tagDesc.Replace('\n', '_');
	tPrintf("TagDesc [%s]\n", tagDesc.Chr());

	const tMetaDatum& datum = metaData[tag];
	switch (datum.Type)
	{
		case tMetaDatum::DatumType::Invalid:
			tPrintf("TagNotSet\n");
			break;

		case tMetaDatum::DatumType::Uint32:
			tPrintf("RawValue(Uint32) [%08x]\n", datum.Uint32);
			break;

		case tMetaDatum::DatumType::Float:
			tPrintf("RawValue(Float)  [%f]\n", datum.Float);
			break;

		case tMetaDatum::DatumType::String:
			tPrintf("RawValue(String) [%s]\n", datum.String.Chr());
			break;
	}
	tString value = metaData.GetPrettyValue(tag);
	if (value.IsValid())
		tPrintf("PrettyValue      [%s]\n", value.Chr());

	tPrintf("\n");
}


tTestUnit(ImageMetaData)
{
	if (!tSystem::tDirExists("TestData/Images/EXIF_XMP"))
		tSkipUnit(ImageMetaData)

	#if 0
	tList<tStringItem> images;
	tSystem::tFindFiles(images, "TestData/Images/EXIF_XMP/", "jpg");
	for (tStringItem* file = images.First(); file; file = file->Next())
	{
		tImageJPG tmpImg;
		tPrintf("OpeningFile:%s\n", file->Chr());
		tmpImg.Load(*file);
	}
	return;
	#endif

	tImageJPG jpgWithMeta("TestData/Images/EXIF_XMP/HasLatLong.jpg");
	tRequire(jpgWithMeta.MetaData.IsValid());
	tMetaData& metaData = jpgWithMeta.MetaData;

	PrintMetaDataTag(metaData, tMetaTag::Make);
	PrintMetaDataTag(metaData, tMetaTag::Model);
	PrintMetaDataTag(metaData, tMetaTag::SerialNumber);
	PrintMetaDataTag(metaData, tMetaTag::MakeModelSerial);
	PrintMetaDataTag(metaData, tMetaTag::LatitudeDD);
	PrintMetaDataTag(metaData, tMetaTag::LatitudeDMS);
	PrintMetaDataTag(metaData, tMetaTag::LongitudeDD);
	PrintMetaDataTag(metaData, tMetaTag::LongitudeDMS);
	PrintMetaDataTag(metaData, tMetaTag::Altitude);
	PrintMetaDataTag(metaData, tMetaTag::AltitudeRelRef);
	PrintMetaDataTag(metaData, tMetaTag::AltitudeRel);
	PrintMetaDataTag(metaData, tMetaTag::Roll);
	PrintMetaDataTag(metaData, tMetaTag::Pitch);
	PrintMetaDataTag(metaData, tMetaTag::Yaw);
	PrintMetaDataTag(metaData, tMetaTag::VelX);
	PrintMetaDataTag(metaData, tMetaTag::VelY);
	PrintMetaDataTag(metaData, tMetaTag::VelZ);
	PrintMetaDataTag(metaData, tMetaTag::Speed);

	jpgWithMeta.Load("TestData/Images/EXIF_XMP/HasUTCDateTime.jpg");

	PrintMetaDataTag(metaData, tMetaTag::GPSSurvey);
	PrintMetaDataTag(metaData, tMetaTag::GPSTimeStamp);

	// Go back to original file.
	jpgWithMeta.Load("TestData/Images/EXIF_XMP/HasLatLong.jpg");

	PrintMetaDataTag(metaData, tMetaTag::ShutterSpeed);
	PrintMetaDataTag(metaData, tMetaTag::ExposureTime);
	PrintMetaDataTag(metaData, tMetaTag::ExposureBias);
	PrintMetaDataTag(metaData, tMetaTag::FStop);
	PrintMetaDataTag(metaData, tMetaTag::ExposureProgram);
	PrintMetaDataTag(metaData, tMetaTag::ISO);
	PrintMetaDataTag(metaData, tMetaTag::Aperture);
	PrintMetaDataTag(metaData, tMetaTag::Brightness);
	PrintMetaDataTag(metaData, tMetaTag::MeteringMode);

	jpgWithMeta.Load("TestData/Images/EXIF_XMP/NoFlashComp.jpg");
	PrintMetaDataTag(metaData, tMetaTag::FlashHardware);
	PrintMetaDataTag(metaData, tMetaTag::FlashUsed);
	PrintMetaDataTag(metaData, tMetaTag::FlashStrobe);
	PrintMetaDataTag(metaData, tMetaTag::FlashMode);
	PrintMetaDataTag(metaData, tMetaTag::FlashRedEye);
	jpgWithMeta.Load("TestData/Images/EXIF_XMP/HasLatLong.jpg");

	PrintMetaDataTag(metaData, tMetaTag::FocalLength);
	PrintMetaDataTag(metaData, tMetaTag::Orientation);
	PrintMetaDataTag(metaData, tMetaTag::LengthUnit);
	PrintMetaDataTag(metaData, tMetaTag::XPixelsPerUnit);
	PrintMetaDataTag(metaData, tMetaTag::YPixelsPerUnit);
	PrintMetaDataTag(metaData, tMetaTag::BitsPerSample);
	PrintMetaDataTag(metaData, tMetaTag::ImageWidth);
	PrintMetaDataTag(metaData, tMetaTag::ImageHeight);
	PrintMetaDataTag(metaData, tMetaTag::ImageWidthOrig);
	PrintMetaDataTag(metaData, tMetaTag::ImageHeightOrig);
	PrintMetaDataTag(metaData, tMetaTag::DateTimeChange);
	PrintMetaDataTag(metaData, tMetaTag::DateTimeOrig);
	PrintMetaDataTag(metaData, tMetaTag::DateTimeDigit);

	jpgWithMeta.Load("TestData/Images/EXIF_XMP/HasAuthorNotes.jpg");

	PrintMetaDataTag(metaData, tMetaTag::Software);
	PrintMetaDataTag(metaData, tMetaTag::Description);
	PrintMetaDataTag(metaData, tMetaTag::Copyright);

	// Test loading/saving with compensation for exif orientation tags.
	for (int i = 0; i < 9; i++)
	{
		tString file;
		tsPrintf(file, "Landscape_%d.jpg", i);
		tImageJPG jpg(tString("TestData/Images/ExifOrientation/") + file);
		jpg.Save(tString("TestData/Images/ExifOrientation/Written") + file);
	}

	for (int i = 0; i < 9; i++)
	{
		tString file;
		tsPrintf(file, "Portrait_%d.jpg", i);
		tImageJPG jpg(tString("TestData/Images/ExifOrientation/") + file);
		jpg.Save(tString("TestData/Images/ExifOrientation/Written") + file);
	}
}


tTestUnit(ImageRotation)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageRotation)

	// Test writing rotated images.
	tPicture aroPic("TestData/Images/RightArrow.png");
	tRequire(aroPic.IsValid());

	tPrintf("Image dimensions before rotate: W:%d H:%d\n", aroPic.GetWidth(), aroPic.GetHeight());
	float angleDelta = tMath::tDegToRad(30.0f);
	int numRotations = 12;
	for (int rotNum = 0; rotNum < numRotations; rotNum++)
	{
		tPicture rotPic(aroPic);
		float angle = float(rotNum) * tMath::TwoPi / numRotations;
		rotPic.RotateCenter(angle, tColouri::transparent);

		tPrintf("Rotated %05.1f Dimensions: W:%d H:%d\n", tMath::tRadToDeg(angle), rotPic.GetWidth(), rotPic.GetHeight());
		tString writeFile;
		tsPrintf(writeFile, "TestData/Images/WrittenRightArrow_NoResampRot%03d.tga", int(tMath::tRadToDeg(angle)));
		rotPic.Save(writeFile);
	}

	// Test resampled (high quality) rotations.
	for (int rotNum = 0; rotNum < numRotations; rotNum++)
	{
		tPicture rotPic(aroPic);
		float angle = float(rotNum) * tMath::TwoPi / numRotations;
		rotPic.RotateCenter(angle, tColouri::transparent, tImage::tResampleFilter::Bilinear, tImage::tResampleFilter::None);

		tPrintf("Rotated %05.1f Dimensions: W:%d H:%d\n", tMath::tRadToDeg(angle), rotPic.GetWidth(), rotPic.GetHeight());
		tString writeFile;
		tsPrintf(writeFile, "TestData/Images/WrittenRightArrow_BilinearResampleRot%03d.tga", int(tMath::tRadToDeg(angle)));
		rotPic.Save(writeFile);
	}

	tPrintf("Test 'plane' rotation.\n");
	tPicture planePic("TestData/Images/plane.png");
	int w = planePic.GetWidth();
	int h = planePic.GetHeight();
	planePic.RotateCenter(-tMath::PiOver4, tColouri::transparent);
}


tTestUnit(ImageCrop)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageCrop)

	// Crop black pixels ignoring alpha (RGB channels only).
	tPicture planePic("TestData/Images/plane.png");
	int w = planePic.GetWidth();
	int h = planePic.GetHeight();
	planePic.Crop(tColouri::black, tComp_RGB);
	planePic.Crop(w, h, tPicture::Anchor::MiddleMiddle, tColouri::transparent);
	planePic.Save("TestData/Images/WrittenPlane.png");
}


tTestUnit(ImageDetection)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageDetection)

	// Test APNG detection.
	bool isAnimA = tImageAPNG::IsAnimatedPNG("TestData/Images/TextCursor.png");
	tRequire(!isAnimA);

	bool isAnimB = tImageAPNG::IsAnimatedPNG("TestData/Images/Icos4D.apng");
	tRequire(isAnimB);

	bool isAnimC = tImageAPNG::IsAnimatedPNG("TestData/Images/Icos4D.png");
	tRequire(isAnimC);
}


tTestUnit(ImageFilter)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageFilter)

	for (int filt = 0; filt < int(tResampleFilter::NumFilters); filt++)
		tPrintf("Filter Name %d: %s\n", filt, tResampleFilterNames[filt]);

	tPicture resamplePicNearest("TestData/Images/TextCursor.png");		// 512x256.
	resamplePicNearest.Resample(800, 300, tResampleFilter::Nearest);
	resamplePicNearest.SaveTGA("TestData/Images/WrittenResampledNearest.tga");

	tPicture resamplePicBox("TestData/Images/TextCursor.png");		// 512x256.
	resamplePicBox.Resample(800, 300, tResampleFilter::Box);
	resamplePicBox.SaveTGA("TestData/Images/WrittenResampledBox.tga");

	tPicture resamplePicBilinear("TestData/Images/TextCursor.png");	// 512x256.
	resamplePicBilinear.Resample(800, 300, tResampleFilter::Bilinear);
	resamplePicBilinear.SaveTGA("TestData/Images/WrittenResampledBilinear.tga");

	tPicture resamplePicBicubicStandard("TestData/Images/TextCursor.png");	// 512x256.
	resamplePicBicubicStandard.Resample(800, 300, tResampleFilter::Bicubic_Standard);
	resamplePicBicubicStandard.SaveTGA("TestData/Images/WrittenResampledBicubicStandard.tga");

	tPicture resamplePicBicubicCatmullRom("TestData/Images/TextCursor.png");	// 512x256.
	resamplePicBicubicCatmullRom.Resample(800, 300, tResampleFilter::Bicubic_CatmullRom);
	resamplePicBicubicCatmullRom.SaveTGA("TestData/Images/WrittenResampledBicubicCatmullRom.tga");

	tPicture resamplePicBicubicMitchell("TestData/Images/TextCursor.png");	// 512x256.
	resamplePicBicubicMitchell.Resample(800, 300, tResampleFilter::Bicubic_Mitchell);
	resamplePicBicubicMitchell.SaveTGA("TestData/Images/WrittenResampledBicubicMitchell.tga");

	tPicture resamplePicBicubicCardinal("TestData/Images/TextCursor.png");	// 512x256.
	resamplePicBicubicCardinal.Resample(800, 300, tResampleFilter::Bicubic_Cardinal);
	resamplePicBicubicCardinal.SaveTGA("TestData/Images/WrittenResampledBicubicCardinal.tga");

	tPicture resamplePicBicubicBSpline("TestData/Images/TextCursor.png");	// 512x256.
	resamplePicBicubicBSpline.Resample(800, 300, tResampleFilter::Bicubic_BSpline);
	resamplePicBicubicBSpline.SaveTGA("TestData/Images/WrittenResampledBicubicBSpline.tga");

	tPicture resamplePicLanczosNarrow("TestData/Images/TextCursor.png");	// 512x256.
	resamplePicLanczosNarrow.Resample(800, 300, tResampleFilter::Lanczos_Narrow);
	resamplePicLanczosNarrow.SaveTGA("TestData/Images/WrittenResampledLanczosNarrow.tga");

	tPicture resamplePicLanczosNormal("TestData/Images/TextCursor.png");	// 512x256.
	resamplePicLanczosNormal.Resample(800, 300, tResampleFilter::Lanczos_Normal);
	resamplePicLanczosNormal.SaveTGA("TestData/Images/WrittenResampledLanczosNormal.tga");

	tPicture resamplePicLanczosWide("TestData/Images/TextCursor.png");	// 512x256.
	resamplePicLanczosWide.Resample(800, 300, tResampleFilter::Lanczos_Wide);
	resamplePicLanczosWide.SaveTGA("TestData/Images/WrittenResampledLanczosWide.tga");
}


tTestUnit(ImageMultiFrame)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageMultiFrame)

	// A multipage tiff.
	tPicture tifPic_Multipage_ZIP_P1("TestData/Images/Tiff_Multipage_ZIP.tif", 0);
	tRequire(tifPic_Multipage_ZIP_P1.IsValid());
	tifPic_Multipage_ZIP_P1.Save("TestData/Images/WrittenTiff_Multipage_ZIP_P1.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenTiff_Multipage_ZIP_P1.tga"));

	tPicture tifPic_Multipage_ZIP_P2("TestData/Images/Tiff_Multipage_ZIP.tif", 1);
	tRequire(tifPic_Multipage_ZIP_P2.IsValid());
	tifPic_Multipage_ZIP_P2.Save("TestData/Images/WrittenTiff_Multipage_ZIP_P2.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenTiff_Multipage_ZIP_P2.tga"));

	tPicture tifPic_Multipage_ZIP_P3("TestData/Images/Tiff_Multipage_ZIP.tif", 2);
	tRequire(tifPic_Multipage_ZIP_P3.IsValid());
	tifPic_Multipage_ZIP_P3.Save("TestData/Images/WrittenTiff_Multipage_ZIP_P3.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenTiff_Multipage_ZIP_P3.tga"));

	// tImageWEBP also supports saving multi-frame webp files.
	tImageAPNG apngSrc("TestData/Images/Flame.apng");
	tImageWEBP webpDst( apngSrc.Frames, true);
	webpDst.Save("TestData/Images/WrittenFlameManyFrames.webp");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenFlameManyFrames.webp"));

	tImageAPNG apngSrc2("TestData/Images/Icos4D.apng");
	tImageWEBP webpDst2( apngSrc2.Frames, true);
	webpDst2.Save("TestData/Images/WrittenIcos4DManyFrames.webp");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenIcos4DManyFrames.webp"));

	// tImageGIF supports saving multi-frame gif files.
	tImageAPNG apngSrc3("TestData/Images/Icos4D.apng");
	tImageGIF gifDst(apngSrc3.Frames, true);
	gifDst.Save("TestData/Images/WrittenIcos4DManyFrames.gif");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenIcos4DManyFrames.gif"));

	// tImageAPNG supports saving multi-frame apng files.
	tImageAPNG apngSrc4("TestData/Images/Icos4D.apng");
	tImageAPNG apngDst(apngSrc4.Frames, true);
	apngDst.Save("TestData/Images/WrittenIcos4DManyFrames.apng");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenIcos4DManyFrames.apng"));

	// Load a multipage tiff with no page duration info.
	tPrintf("Test multipage TIFF load.\n");
	tImageTIFF tiffMultipage("TestData/Images/Tiff_Multipage_ZIP.tif");
	tRequire(tiffMultipage.IsValid());

	// Create a multipage tiff with page duration info.
	tImageAPNG apngSrc5("TestData/Images/Icos4D.apng");
	tImageTIFF tiffDst(apngSrc5.Frames, true);
	tiffDst.Save("TestData/Images/WrittenIcos4DManyFrames.tiff");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenIcos4DManyFrames.tiff"));

	// Load a multipage tiff with page duration info since it was saved from Tacent.
	tImageTIFF tiffWithDur("TestData/Images/WrittenIcos4DManyFrames.tiff");
	tiffWithDur.Save("TestData/Images/WrittenIcos4DManyFrames2.tiff");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenIcos4DManyFrames2.tiff"));
}


tTestUnit(ImageGradient)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageGradient)

	const int width = 640;
	const int height = 90;
	tPixel* pixels = nullptr;

	// Gradient black to white.
	pixels = new tPixel[width*height];
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			pixels[y*width + x] = tColour(256*x / width, 256*x / width, 256*x / width, 255);
	tImageTGA blackToWhite(pixels, width, height, true);
	tRequire(blackToWhite.IsValid());
	blackToWhite.Save("TestData/Images/Written_Gradient_BlackToWhite.tga", tImageTGA::tFormat::Bit24, tImageTGA::tCompression::RLE);

	// Gradient black to transparent.
	pixels = new tPixel[width*height];
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			pixels[y*width + x] = tColour(0, 0, 0, 255 - 256*x / width);
	tImageTGA blackToTrans(pixels, width, height, true);
	tRequire(blackToTrans.IsValid());
	blackToTrans.Save("TestData/Images/Written_Gradient_BlackToTrans.tga", tImageTGA::tFormat::Bit32, tImageTGA::tCompression::RLE);

	// Gradient transparent to white.
	pixels = new tPixel[width*height];
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			pixels[y*width + x] = tColour(255, 255, 255, 256*x / width);
	tImageTGA transToWhite(pixels, width, height, true);
	tRequire(transToWhite.IsValid());
	transToWhite.Save("TestData/Images/Written_Gradient_TransToWhite.tga", tImageTGA::tFormat::Bit32, tImageTGA::tCompression::RLE);

	// Gradient red to yellow to green to cyan to blue to magenta to red.
	const int section = width / 6;
	pixels = new tPixel[width*height];
	for (int y = 0; y < height; y++)
	{
		// Red to yellow.
		for (int x = 0; x < section; x++)
			pixels[y*width + section*0+x+0] = tColour(255, 256*x/section, 0, 255);

		// Yellow to Green.
		for (int x = 0; x < section+1; x++)
			pixels[y*width + section*1+x+0] = tColour(255-256*x/section, 255, 0, 255);

		// Green to Cyan.
		for (int x = 0; x < section+1; x++)
			pixels[y*width + section*2+x+1] = tColour(0, 255, 256*x/section, 255);

		// Cyan to Blue.
		for (int x = 0; x < section+1; x++)
			pixels[y*width + section*3+x+2] = tColour(0, 255-256*x/section, 255, 255);

		// Blue to Magenta.
		for (int x = 0; x < section+1; x++)
			pixels[y*width + section*4+x+3] = tColour(256*x/section, 0, 255, 255);

		// Magenta to Red.
		for (int x = 0; x < section; x++)
			pixels[y*width + section*5+x+4] = tColour(255, 0, 255-256*x/section, 255);
	}
	tImageTGA redToRed(pixels, width, height, true);
	tRequire(redToRed.IsValid());
	redToRed.Save("TestData/Images/Written_Gradient_RedToRed.tga", tImageTGA::tFormat::Bit24, tImageTGA::tCompression::RLE);	
}


// Helper for ImageDDS unit tests.
void DDSLoadDecodeSave(const tString& ddsfile, uint32 loadFlags = 0, bool saveAllMips = false)
{
	tString basename = tSystem::tGetFileBaseName(ddsfile);
	tString savename = basename + "_";
	savename += (loadFlags & tImageDDS::LoadFlag_Decode)			? "D" : "x";
	savename += ((loadFlags & tImageDDS::LoadFlag_GammaCompression) || (loadFlags & tImageDDS::LoadFlag_SRGBCompression)) ? "G" : "x";
	savename += (loadFlags & tImageDDS::LoadFlag_ReverseRowOrder)	? "R" : "x";
	savename += (loadFlags & tImageDDS::LoadFlag_SpreadLuminance)	? "S" : "x";
	tPrintf("DDS Load %s\n", savename.Chr());
	tString formatname = basename.Left('_');

	tImageDDS::LoadParams params;
	params.Flags = loadFlags;
	tImageDDS dds(ddsfile, params);
	tRequire(dds.IsValid());
	tPixelFormat fileformat = tGetPixelFormat(formatname.Chr());
	tPixelFormat ddsformat = dds.GetPixelFormat();
	tPixelFormat ddsformatsrc = dds.GetPixelFormatSrc();
	tRequire(fileformat == ddsformatsrc);
	if (loadFlags & tImageDDS::LoadFlag_Decode)
		tRequire(ddsformat == tPixelFormat::R8G8B8A8);
	else
		tRequire(ddsformat == fileformat);

	// If we asked to flip rows but it couldn't, print a message. The conditional is
	// only set if we requested and it couldn't be done.
	if (dds.IsResultSet(tImageDDS::ResultCode::Conditional_CouldNotFlipRows))
		tPrintf("Could not flip rows for %s\n", savename.Chr());

	tList<tImage::tLayer> layers;
	dds.StealLayers(layers);

	if (ddsformat != tPixelFormat::R8G8B8A8)
	{
		tPrintf("No tga save. Pixel format not R8G8B8A8\n");
	}
	else
	{
		if (saveAllMips)
		{
			int mipNum = 0;
			for (tLayer* layer = layers.First(); layer; layer = layer->Next(), mipNum++)
			{
				tImageTGA tga((tPixel*)layer->Data, layer->Width, layer->Height);
				tString mipName;
				tsPrintf(mipName, "Written_%s_Mip%02d.tga", savename.Chr(), mipNum);
				tga.Save(mipName);
			}
		}
		else
		{
			if (tLayer* layer = layers.First())
			{
				tImageTGA tga((tPixel*)layer->Data, layer->Width, layer->Height);
				tga.Save("Written_" + savename + ".tga");
			}
		}
	}
	tPrintf("\n");
}


tTestUnit(ImageDDS)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageDDS)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/DDS/");

	uint32 decode = tImageDDS::LoadFlag_Decode;
	uint32 revrow = tImageDDS::LoadFlag_ReverseRowOrder;
	uint32 spread = tImageDDS::LoadFlag_SpreadLuminance;
	uint32 gammac = tImageDDS::LoadFlag_SRGBCompression;
	// uint32 gammac = tImageDDS::LoadFlag_GammaCompression;

	//
	// Block Compressed Formats.
	//
	tPrintf("Testing DDS Loading/Decoding. Legacy = No DX10 Header.\n\n");

	// BC1
	DDSLoadDecodeSave("BC1DXT1_RGB_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("BC1DXT1_RGB_Modern.dds", decode | revrow);

	// BC1a
	DDSLoadDecodeSave("BC1DXT1a_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("BC1DXT1a_RGBA_Modern.dds", decode | revrow);

	// BC2
	DDSLoadDecodeSave("BC2DXT2DXT3_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("BC2DXT2DXT3_RGBA_Modern.dds", decode | revrow);

	// BC3
	DDSLoadDecodeSave("BC3DXT4DXT5_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("BC3DXT4DXT5_RGBA_Modern.dds", decode | revrow);

	// BC4
	DDSLoadDecodeSave("BC4ATI1_R_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("BC4ATI1_R_Modern.dds", decode | revrow | spread);

	// BC5
	DDSLoadDecodeSave("BC5ATI2_RG_Modern.dds", decode | revrow);

	// BC6
	DDSLoadDecodeSave("BC6s_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("BC6u_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("BC6s_HDRRGB_Modern.dds", decode | revrow | gammac);
	DDSLoadDecodeSave("BC6u_HDRRGB_Modern.dds", decode | revrow | gammac);

	// BC7
	DDSLoadDecodeSave("BC7_RGBA_Modern.dds", decode | revrow, true);

	//
	// Uncompressed Integer Formats.
	//
	// A8
	DDSLoadDecodeSave("A8_A_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("A8_A_Modern.dds", decode | revrow);

	// L8
	DDSLoadDecodeSave("L8_L_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("L8_L_Legacy.dds", decode | revrow | spread);
	DDSLoadDecodeSave("R8_L_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("R8_L_Modern.dds", decode | revrow | spread);

	// B8G8R8
	DDSLoadDecodeSave("B8G8R8_RGB_Legacy.dds", decode | revrow);

	// B8G8R8A8
	DDSLoadDecodeSave("B8G8R8A8_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("B8G8R8A8_RGBA_Modern.dds", decode | revrow);

	// B5G6R5
	DDSLoadDecodeSave("B5G6R5_RGB_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("B5G6R5_RGB_Modern.dds", decode | revrow);

	// B4G4R4A4
	DDSLoadDecodeSave("B4G4R4A4_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("B4G4R4A4_RGBA_Modern.dds", decode | revrow);

	// B5G5R5A1
	DDSLoadDecodeSave("B5G5R5A1_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("B5G5R5A1_RGBA_Modern.dds", decode | revrow);

	//
	// Uncompressed Floating-Point (HDR) Formats.
	//
	// R16F
	DDSLoadDecodeSave("R16f_R_Legacy.dds", decode | revrow | gammac);
	DDSLoadDecodeSave("R16f_R_Modern.dds", decode | revrow | gammac);
	DDSLoadDecodeSave("R16f_R_Legacy.dds", decode | revrow | gammac | spread);
	DDSLoadDecodeSave("R16f_R_Modern.dds", decode | revrow | gammac | spread);

	// R16G16F
	DDSLoadDecodeSave("R16G16f_RG_Legacy.dds", decode | revrow | gammac);
	DDSLoadDecodeSave("R16G16f_RG_Modern.dds", decode | revrow | gammac);

	// R16G16B16A16F
	DDSLoadDecodeSave("R16G16B16A16f_RGBA_Legacy.dds", decode | revrow | gammac);
	DDSLoadDecodeSave("R16G16B16A16f_RGBA_Modern.dds", decode | revrow | gammac);

	// R32F
	DDSLoadDecodeSave("R32f_R_Legacy.dds", decode | revrow | gammac);
	DDSLoadDecodeSave("R32f_R_Modern.dds", decode | revrow | gammac);
	DDSLoadDecodeSave("R32f_R_Legacy.dds", decode | revrow | gammac | spread);
	DDSLoadDecodeSave("R32f_R_Modern.dds", decode | revrow | gammac | spread);

	// R32G32F
	DDSLoadDecodeSave("R32G32f_RG_Legacy.dds", decode | revrow | gammac);
	DDSLoadDecodeSave("R32G32f_RG_Modern.dds", decode | revrow | gammac);

	// R32G32B32A32F
	DDSLoadDecodeSave("R32G32B32A32f_RGBA_Legacy.dds", decode | revrow | gammac);
	DDSLoadDecodeSave("R32G32B32A32f_RGBA_Modern.dds", decode | revrow | gammac);

	// Do this all over again, but without decoding and tRequire the pixel-format to be as expected.
	// This time, since not decoding, it may be impossible to reverse the rows, so we can also expect
	// to get conditional valids if it couldn't be done (for some of the BC formats). We're only going
	// to bother with the modern-style dds files (for the most part) this time through.

	tPrintf("Testing DDS Loading/No-decoding.\n\n");
	DDSLoadDecodeSave("BC1DXT1_RGB_Modern.dds", revrow);		// Revrow should work for BC1.
	DDSLoadDecodeSave("BC1DXT1a_RGBA_Modern.dds");
	DDSLoadDecodeSave("BC2DXT2DXT3_RGBA_Modern.dds", revrow);
	DDSLoadDecodeSave("BC3DXT4DXT5_RGBA_Modern.dds", revrow);
	DDSLoadDecodeSave("BC4ATI1_R_Modern.dds", revrow);			// Should print warning and be unable to flip rows. May be able to implement.
	DDSLoadDecodeSave("BC5ATI2_RG_Modern.dds", revrow);			// Should print warning and be unable to flip rows. May be able to implement.
	DDSLoadDecodeSave("BC6s_RGB_Modern.dds", revrow);			// Should print warning and be unable to flip rows.
	DDSLoadDecodeSave("BC6u_RGB_Modern.dds");
	DDSLoadDecodeSave("BC6s_HDRRGB_Modern.dds");
	DDSLoadDecodeSave("BC6u_HDRRGB_Modern.dds", revrow);		// Should print warning and be unable to flip rows.
	DDSLoadDecodeSave("BC7_RGBA_Modern.dds", revrow);			// Should print warning and be unable to flip rows.
	DDSLoadDecodeSave("A8_A_Modern.dds");
	DDSLoadDecodeSave("R8_L_Modern.dds", revrow);
	DDSLoadDecodeSave("L8_L_Legacy.dds", revrow);
	DDSLoadDecodeSave("B8G8R8_RGB_Legacy.dds");					// Only legacy supports this format.
	DDSLoadDecodeSave("B8G8R8A8_RGBA_Modern.dds");
	DDSLoadDecodeSave("B5G6R5_RGB_Modern.dds", revrow);
	DDSLoadDecodeSave("B4G4R4A4_RGBA_Modern.dds", revrow);
	DDSLoadDecodeSave("B5G5R5A1_RGBA_Modern.dds");

	DDSLoadDecodeSave("R16f_R_Modern.dds", revrow);
	DDSLoadDecodeSave("R16f_R_Modern.dds");
	DDSLoadDecodeSave("R16G16f_RG_Modern.dds", revrow);
	DDSLoadDecodeSave("R16G16B16A16f_RGBA_Modern.dds");

	DDSLoadDecodeSave("R32f_R_Modern.dds", revrow);
	DDSLoadDecodeSave("R32f_R_Modern.dds");
	DDSLoadDecodeSave("R32G32f_RG_Modern.dds");
	DDSLoadDecodeSave("R32G32B32A32f_RGBA_Modern.dds", revrow);

	tSystem::tSetCurrentDir(origDir.Chr());
}


// Helper for ImageKTX unit tests.
void KTXLoadDecodeSave(const tString& ktxfile, uint32 loadFlags = 0, bool saveAllMips = false)
{
	tString basename = tSystem::tGetFileBaseName(ktxfile);
	tString savename = basename + "_";
	savename += (loadFlags & tImageKTX::LoadFlag_Decode)			? "D" : "x";
	savename += ((loadFlags & tImageKTX::LoadFlag_GammaCompression) || (loadFlags & tImageKTX::LoadFlag_SRGBCompression)) ? "G" : "x";
	savename += (loadFlags & tImageKTX::LoadFlag_ReverseRowOrder)	? "R" : "x";
	savename += (loadFlags & tImageKTX::LoadFlag_SpreadLuminance)	? "S" : "x";
	tPrintf("KTX Load %s\n", savename.Chr());
	tString formatname = basename.Left('_');

	tImageKTX::LoadParams params;
	params.Flags = loadFlags;
	tImageKTX ktx(ktxfile, params);
	tRequire(ktx.IsValid());
	tPixelFormat fileformat = tGetPixelFormat(formatname.Chr());
	tPixelFormat ktxformat = ktx.GetPixelFormat();
	tPixelFormat ktxformatsrc = ktx.GetPixelFormatSrc();
	tRequire(fileformat == ktxformatsrc);
	if (loadFlags & tImageKTX::LoadFlag_Decode)
		tRequire(ktxformat == tPixelFormat::R8G8B8A8);
	else
		tRequire(ktxformat == fileformat);

	// If we asked to flip rows but it couldn't, print a message. The conditional is
	// only set if we requested and it couldn't be done.
	if (ktx.IsResultSet(tImageKTX::ResultCode::Conditional_CouldNotFlipRows))
		tPrintf("Could not flip rows for %s\n", savename.Chr());

	tList<tImage::tLayer> layers;
	ktx.StealLayers(layers);

	if (ktxformat != tPixelFormat::R8G8B8A8)
	{
		tPrintf("No tga save. Pixel format not R8G8B8A8\n");
	}
	else
	{
		if (saveAllMips)
		{
			int mipNum = 0;
			for (tLayer* layer = layers.First(); layer; layer = layer->Next(), mipNum++)
			{
				tImageTGA tga((tPixel*)layer->Data, layer->Width, layer->Height);
				tString mipName;
				tsPrintf(mipName, "Written_%s_Mip%02d.tga", savename.Chr(), mipNum);
				tga.Save(mipName);
			}
		}
		else
		{
			if (tLayer* layer = layers.First())
			{
				tImageTGA tga((tPixel*)layer->Data, layer->Width, layer->Height);
				tga.Save("Written_" + savename + ".tga");
			}
		}
	}
	tPrintf("\n");
}


tTestUnit(ImageKTX)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageKTX)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/KTX2/");

	uint32 decode = tImageKTX::LoadFlag_Decode;
	uint32 revrow = tImageKTX::LoadFlag_ReverseRowOrder;
	uint32 spread = tImageKTX::LoadFlag_SpreadLuminance;
	uint32 gammac = tImageKTX::LoadFlag_SRGBCompression;
	// uint32 gammac = tImageKTX::LoadFlag_GammaCompression;

	//
	// Block Compressed Formats.
	//
	tPrintf("Testing KTX Loading/Decoding Using LibKTX %s\n\n", tImage::Version_LibKTX);
	tPrintf("D = Decode\n");
	tPrintf("G = Gamma Compression\n");
	tPrintf("R = Reverse Row Order\n");
	tPrintf("S = Spread Luminance\n");

	// BC1
	KTXLoadDecodeSave("BC1DXT1_RGB.ktx2", decode | revrow);

	// BC1a
	KTXLoadDecodeSave("BC1DXT1a_RGBA.ktx2", decode | revrow);

	// BC2
	KTXLoadDecodeSave("BC2DXT2DXT3_RGBA.ktx2", decode | revrow);

	// BC3
	KTXLoadDecodeSave("BC3DXT4DXT5_RGBA.ktx2", decode | revrow);

	// BC4
	KTXLoadDecodeSave("BC4ATI1_R.ktx2", decode | revrow);

	// BC5
	KTXLoadDecodeSave("BC5ATI2_RG.ktx2", decode | revrow);

	// BC6
	KTXLoadDecodeSave("BC6s_RGB.ktx2", decode | revrow | gammac);

	// BC7
	KTXLoadDecodeSave("BC7_RGBA.ktx2", decode | revrow, true);
	KTXLoadDecodeSave("BC7_RGBANoSuper.ktx2", decode | revrow, true);

	//
	// Uncompressed Integer Formats.
	//
	// R8
	KTXLoadDecodeSave("R8_A.ktx2", decode | revrow);

	// L8
	KTXLoadDecodeSave("R8_L.ktx2", decode | revrow);
	KTXLoadDecodeSave("R8_L.ktx2", decode | revrow | spread);

	// B8G8R8
	KTXLoadDecodeSave("B8G8R8_RGB.ktx2", decode | revrow);

	// B8G8R8A8
	KTXLoadDecodeSave("B8G8R8A8_RGBA.ktx2", decode | revrow);

	//
	// Uncompressed Floating-Point (HDR) Formats.
	//
	// R16F
	KTXLoadDecodeSave("R16f_R.ktx2", decode | revrow | gammac);
	KTXLoadDecodeSave("R16f_R.ktx2", decode | revrow | gammac | spread);

	// R16G16F
	KTXLoadDecodeSave("R16G16f_RG.ktx2", decode | revrow | gammac);

	// R16G16B16A16F
	KTXLoadDecodeSave("R16G16B16A16f_RGBA.ktx2", decode | revrow | gammac);

	// R32F
	KTXLoadDecodeSave("R32f_R.ktx2", decode | revrow | gammac);
	KTXLoadDecodeSave("R32f_R.ktx2", decode | revrow | gammac | spread);

	// R32G32F
	KTXLoadDecodeSave("R32G32f_RG.ktx2", decode | revrow | gammac);

	// R32G32B32A32F
	KTXLoadDecodeSave("R32G32B32A32f_RGBA.ktx2", decode | revrow | gammac);

	// Do this all over again, but without decoding and tRequire the pixel-format to be as expected.
	// This time, since not decoding, it may be impossible to reverse the rows, so we can also expect
	// to get conditional valids if it couldn't be done (for some of the BC formats).

	tPrintf("Testing KTX Loading/No-decoding.\n\n");
	KTXLoadDecodeSave("BC1DXT1_RGB.ktx2", revrow);				// Revrow should work for BC1.
	KTXLoadDecodeSave("BC1DXT1a_RGBA.ktx2");
	KTXLoadDecodeSave("BC2DXT2DXT3_RGBA.ktx2", revrow);
	KTXLoadDecodeSave("BC3DXT4DXT5_RGBA.ktx2", revrow);
	KTXLoadDecodeSave("BC4ATI1_R.ktx2", revrow);				// Should print warning and be unable to flip rows. May be able to implement.
	KTXLoadDecodeSave("BC5ATI2_RG.ktx2", revrow);				// Should print warning and be unable to flip rows. May be able to implement.
	KTXLoadDecodeSave("BC6s_RGB.ktx2", revrow);					// Should print warning and be unable to flip rows.
	KTXLoadDecodeSave("BC7_RGBA.ktx2", revrow);					// Should print warning and be unable to flip rows.
	KTXLoadDecodeSave("BC7_RGBANoSuper.ktx2", revrow);			// Should print warning and be unable to flip rows.

	KTXLoadDecodeSave("R8_A.ktx2");
	KTXLoadDecodeSave("R8_L.ktx2", revrow);
	KTXLoadDecodeSave("B8G8R8_RGB.ktx2");
	KTXLoadDecodeSave("B8G8R8A8_RGBA.ktx2");

	KTXLoadDecodeSave("R16f_R.ktx2", revrow);
	KTXLoadDecodeSave("R16f_R.ktx2");
	KTXLoadDecodeSave("R16G16f_RG.ktx2", revrow);
	KTXLoadDecodeSave("R16G16B16A16f_RGBA.ktx2");

	KTXLoadDecodeSave("R32f_R.ktx2", revrow);
	KTXLoadDecodeSave("R32f_R.ktx2");
	KTXLoadDecodeSave("R32G32f_RG.ktx2");
	KTXLoadDecodeSave("R32G32B32A32f_RGBA.ktx2", revrow);

	tSystem::tSetCurrentDir(origDir.Chr());
}


}
