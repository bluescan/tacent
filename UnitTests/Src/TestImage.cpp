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

	tImageDDS imgDDS("TestData/Images/TestDXT1.dds");
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
	tTexture dxt1Tex("TestData/Images/TestDXT1.dds");
	tRequire(dxt1Tex.IsValid());

	tChunkWriter writer("TestData/Images/WrittenTestDXT1.tac");
	dxt1Tex.Save(writer);
	tRequire( tSystem::tFileExists("TestData/Images/WrittenTestDXT1.tac") );

	tChunkReader reader("TestData/Images/WrittenTestDXT1.tac");
	dxt1Tex.Load( reader.Chunk() );
	tRequire(dxt1Tex.IsValid());

	// Test cubemap.
	tTexture cubemap("TestData/Images/CubemapLayoutGuide.dds");
	tRequire(cubemap.IsValid());

	// Test jpg to texture. This will do conversion to BC1.
	tTexture bc1Tex("TestData/Images/WiredDrives.jpg", true);
	tRequire(bc1Tex.IsValid());
	tChunkWriter chunkWriterBC1("TestData/Images/WrittenBC1.tac");
	bc1Tex.Save(chunkWriterBC1);
	tRequire( tSystem::tFileExists("TestData/Images/WrittenBC1.tac"));

	// Test ico with alpha to texture. This will do conversion to BC3.
	tTexture bc3Tex("TestData/Images/UpperBounds.ico", true);
	tRequire(bc3Tex.IsValid());
	tChunkWriter chunkWriterBC3("TestData/Images/WrittenBC3.tac");
	bc3Tex.Save(chunkWriterBC3);
	tRequire( tSystem::tFileExists("TestData/Images/WrittenBC3.tac"));
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


tTestUnit(ImageMetaData)
{
	if (!tSystem::tDirExists("TestData/Images/EXIF_XMP"))
		tSkipUnit(ImageMetaData)

//	tList<tStringItem> images;
//	tSystem::tFindFiles(images, "TestData/Images/EXIF_XMP/", "jpg");
//	for (tStringItem* file = images.First(); file; file = file->Next())
//	{
//		tImageJPG tmpImg;
//		tPrintf("OpeningFile:%s\n", file->Chars());
//		tmpImg.Load(*file);
//	}
	tImageJPG jpgWithMeta("TestData/Images/EXIF_XMP/HasLatLong.jpg");
//	tImageJPG jpgWithMeta("TestData/Images/EXIF_XMP/test1.jpg");
//	tImageJPG jpgWithMeta("TestData/Images/EXIF_XMP/sony-alpha-6000.jpg");
	tRequire(jpgWithMeta.MetaData.IsValid());

	tMetaData& metaData = jpgWithMeta.MetaData;
	tMetaDatum datum;

	datum = metaData[tMetaTag::Make];
	if (datum.IsValid())	tPrintf("Make: %s\n", datum.String.Chars());

	datum = metaData[tMetaTag::Model];
	if (datum.IsValid())	tPrintf("Model: %s\n", datum.String.Chars());

	datum = metaData[tMetaTag::SerialNumber];
	if (datum.IsValid())	tPrintf("SerialNumber: %s\n", datum.String.Chars());

	datum = metaData[tMetaTag::LatitudeDD];
	if (datum.IsValid())	tPrintf("LatitudeDD : %f\n", datum.Float);

	datum = metaData[tMetaTag::LatitudeDMS];
	if (datum.IsValid())	tPrintf("LatitudeDMS: %s\n", datum.String.Chars());

	datum = metaData[tMetaTag::LongitudeDD];
	if (datum.IsValid())	tPrintf("LongitudeDD : %f\n", datum.Float);

	datum = metaData[tMetaTag::LongitudeDMS];
	if (datum.IsValid())	tPrintf("LongitudeDMS: %s\n", datum.String.Chars());

	datum = metaData[tMetaTag::Altitude];
	if (datum.IsValid())	tPrintf("Altitude: %f m Above Sea Level\n", datum.Float);

	datum = metaData[tMetaTag::AltitudeRel];
	if (datum.IsValid())	tPrintf("AltitudeRel: %f m\n", datum.Float);

	datum = metaData[tMetaTag::AltitudeRelGnd];
	if (datum.IsValid())	tPrintf("AltitudeRelGnd: Ground is %s\n", datum.String.Chars());

	datum = metaData[tMetaTag::Roll];
	if (datum.IsValid())	tPrintf(u8"Roll %f°\n", datum.Float);

	datum = metaData[tMetaTag::Pitch];
	if (datum.IsValid())	tPrintf(u8"Pitch %f°\n", datum.Float);

	datum = metaData[tMetaTag::Yaw];
	if (datum.IsValid())	tPrintf(u8"Yaw %f°\n", datum.Float);

	datum = metaData[tMetaTag::VelX];
	if (datum.IsValid())	tPrintf("VelX %f m/s°\n", datum.Float);

	datum = metaData[tMetaTag::VelY];
	if (datum.IsValid())	tPrintf("VelY %f m/s°\n", datum.Float);

	datum = metaData[tMetaTag::VelZ];
	if (datum.IsValid())	tPrintf("VelZ %f m/s°\n", datum.Float);

	datum = metaData[tMetaTag::Speed];
	if (datum.IsValid())	tPrintf("Speed %f m/s°\n", datum.Float);

	jpgWithMeta.Load("TestData/Images/EXIF_XMP/HasUTCDateTime.jpg");
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
	planePic.Crop(tColouri::black, tMath::ColourChannel_RGB);
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


}
