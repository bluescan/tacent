// TestImage.cpp
//
// Image module tests.
//
// Copyright (c) 2017, 2019, 2020, 2021 Tristan Grimmer.
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
	if (!tSystem::tDirExists("TestData/"))
		tSkipUnit(ImageTGA)

	// Test direct loading classes.
	tImageTGA imgTGA("TestData/WhiteBorderRLE.tga");
	tRequire(imgTGA.IsValid());

	tImageAPNG imgAPNG("TestData/Flame.apng");
	tRequire(imgAPNG.IsValid());

	tImageDDS imgDDS("TestData/TestDXT1.dds");
	tRequire(imgDDS.IsValid());

	tImageEXR imgEXR("TestData/Desk.exr");
	tRequire(imgEXR.IsValid());

	tImageGIF imgGIF("TestData/8-cell-simple.gif");
	tRequire(imgGIF.IsValid());

	tImageHDR imgHDR("TestData/mpi_atrium_3.hdr");
	tRequire(imgHDR.IsValid());

	tImageICO imgICO("TestData/UpperBounds.ico");
	tRequire(imgICO.IsValid());

	tImageJPG imgJPG("TestData/WiredDrives.jpg");
	tRequire(imgJPG.IsValid());

	tImageTIFF imgTIFF("TestData/Tiff_NoComp.tif");
	tRequire(imgTIFF.IsValid());

	tImageWEBP imgWEBP("TestData/RockyBeach.webp");
	tRequire(imgWEBP.IsValid());
}


tTestUnit(ImageSave)
{
	if (!tSystem::tDirExists("TestData/"))
		tSkipUnit(Image)

	tPicture newPngA("TestData/Xeyes.png");
	newPngA.Save("TestData/WrittenNewA.png");
	tRequire( tSystem::tFileExists("TestData/WrittenNewA.png"));

	tPicture newPngB("TestData/TextCursor.png");
	newPngB.Save("TestData/WrittenNewB.png");
	tRequire( tSystem::tFileExists("TestData/WrittenNewB.png"));

	tPicture apngPicForSave("TestData/Flame.apng");
	apngPicForSave.SaveWEBP("TestData/WrittenFlameOneFrame.webp");
	tRequire(tSystem::tFileExists("TestData/WrittenFlameOneFrame.webp"));

	// Test writing webp images.
	tPicture exrPicForSave("TestData/Desk.exr");
	exrPicForSave.SaveWEBP("TestData/WrittenDesk.webp");
	tRequire(tSystem::tFileExists("TestData/WrittenDesk.webp"));
}


tTestUnit(ImageTexture)
{
	if (!tSystem::tDirExists("TestData/"))
		tSkipUnit(Image)

	// Test dxt1 texture.
	tTexture dxt1Tex("TestData/TestDXT1.dds");
	tRequire(dxt1Tex.IsValid());

	tChunkWriter writer("TestData/WrittenTestDXT1.tac");
	dxt1Tex.Save(writer);
	tRequire( tSystem::tFileExists("TestData/WrittenTestDXT1.tac") );

	tChunkReader reader("TestData/WrittenTestDXT1.tac");
	dxt1Tex.Load( reader.Chunk() );
	tRequire(dxt1Tex.IsValid());

	// Test cubemap.
	tTexture cubemap("TestData/CubemapLayoutGuide.dds");
	tRequire(cubemap.IsValid());

	// Test jpg to texture. This will do conversion to BC1.
	tTexture bc1Tex("TestData/WiredDrives.jpg", true);
	tRequire(bc1Tex.IsValid());
	tChunkWriter chunkWriterBC1("TestData/WrittenBC1.tac");
	bc1Tex.Save(chunkWriterBC1);
	tRequire( tSystem::tFileExists("TestData/WrittenBC1.tac"));

	// Test ico with alpha to texture. This will do conversion to BC3.
	tTexture bc3Tex("TestData/UpperBounds.ico", true);
	tRequire(bc3Tex.IsValid());
	tChunkWriter chunkWriterBC3("TestData/WrittenBC3.tac");
	bc3Tex.Save(chunkWriterBC3);
	tRequire( tSystem::tFileExists("TestData/WrittenBC3.tac"));
}


tTestUnit(ImagePicture)
{
	if (!tSystem::tDirExists("TestData/"))
		tSkipUnit(Image)

	// Test generate layers.
	tPicture srcPic("TestData/UpperB.bmp");
	tRequire(srcPic.IsValid());
	tPrintf("GenLayers Orig W=%d H=%d\n", srcPic.GetWidth(), srcPic.GetHeight());
	tList<tLayer> layers;
	srcPic.GenerateLayers(layers);
	int lev = 0;
	for (tLayer* lay = layers.First(); lay; lay = lay->Next(), lev++)
		tPrintf("GenLayers Mip:%02d W=%d H=%d\n", lev, lay->Width, lay->Height);
	tRequire(layers.GetNumItems() == 10);

	// Test tPicture loading bmp and saving as tga.
	tPicture bmpPicUB("TestData/UpperB.bmp");
	tRequire(bmpPicUB.IsValid());
	bmpPicUB.Save("TestData/WrittenUpperB.tga");
	bmpPicUB.Save("TestData/WrittenUpperB.bmp");
	tRequire( tSystem::tFileExists("TestData/WrittenUpperB.tga"));

	tPicture bmpPicA("TestData/Bmp_Alpha.bmp");
	tRequire(bmpPicA.IsValid());
	bmpPicA.Save("TestData/WrittenBmp_Alpha.tga");
	bmpPicA.Save("TestData/WrittenBmp_Alpha.bmp");
	tRequire( tSystem::tFileExists("TestData/WrittenBmp_Alpha.tga"));

	tPicture bmpPicL("TestData/Bmp_Lambda.bmp");
	tRequire(bmpPicL.IsValid());
	bmpPicL.Save("TestData/WrittenBmp_Lambda.tga");
	bmpPicL.Save("TestData/WrittenBmp_Lambda.bmp");
	tRequire( tSystem::tFileExists("TestData/WrittenBmp_Lambda.tga"));

	tPicture bmpPicRL("TestData/Bmp_RefLena.bmp");
	tRequire(bmpPicRL.IsValid());
	bmpPicRL.Save("TestData/WrittenBmp_RefLena.tga");
	bmpPicRL.Save("TestData/WrittenBmp_RefLena.bmp");
	tRequire( tSystem::tFileExists("TestData/WrittenBmp_RefLena.tga"));

	tPicture bmpPicRL101("TestData/Bmp_RefLena101.bmp");
	tRequire(bmpPicRL101.IsValid());
	bmpPicRL101.Save("TestData/WrittenBmp_RefLena101.tga");
	bmpPicRL101.Save("TestData/WrittenBmp_RefLena101.bmp");
	tRequire( tSystem::tFileExists("TestData/WrittenBmp_RefLena101.tga"));

	tPicture bmpPicRLFlip("TestData/Bmp_RefLenaFlip.bmp");
	tRequire(bmpPicRLFlip.IsValid());
	bmpPicRLFlip.Save("TestData/WrittenBmp_RefLenaFlip.tga");
	bmpPicRLFlip.Save("TestData/WrittenBmp_RefLenaFlip.bmp");
	tRequire( tSystem::tFileExists("TestData/WrittenBmp_RefLenaFlip.tga"));

	tPicture pngPicIcos("TestData/Icos4D.png");
	tRequire(pngPicIcos.IsValid());
	pngPicIcos.Save("TestData/WrittenBmp_Icos4D.bmp");
	pngPicIcos.Save("TestData/WrittenBmp_Icos4D.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenBmp_Icos4D.tga"));

	// Test tPicture loading jpg and saving as tga.
	tPicture jpgPic("TestData/WiredDrives.jpg");
	tRequire(jpgPic.IsValid());
	jpgPic.Save("TestData/WrittenWiredDrives.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenWiredDrives.tga"));

	tPicture exrPic("TestData/Desk.exr");
	tRequire(exrPic.IsValid());
	exrPic.Save("TestData/WrittenDesk.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenDesk.tga"));

	tPicture apngPic("TestData/Flame.apng", 100);
	tRequire(apngPic.IsValid());
	apngPic.Save("TestData/WrittenFlame.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenFlame.tga"));

	// Test tPicture loading xpm and saving as tga.
	tPicture xpmPic("TestData/Crane.xpm");
	tRequire(xpmPic.IsValid());
	xpmPic.Save("TestData/WrittenCrane.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenCrane.tga"));

	// Test tPicture loading png (with alpha channel) and saving as tga (with alpha channel).
	tPicture pngPic("TestData/Xeyes.png");
	tRequire(pngPic.IsValid());
	pngPic.SaveTGA("TestData/WrittenXeyes.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenXeyes.tga"));

	// Test saving tPicture in other supported formats.
	pngPic.Save("TestData/WrittenXeyesTGA.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenXeyesTGA.tga"));

	pngPic.Save("TestData/WrittenXeyesBMP.bmp");
	tRequire( tSystem::tFileExists("TestData/WrittenXeyesBMP.bmp"));

	pngPic.Save("TestData/WrittenXeyesJPG.jpg");
	tRequire( tSystem::tFileExists("TestData/WrittenXeyesJPG.jpg"));

	// Test tiff file loading and saving.
	tPicture tifPic_NoComp("TestData/Tiff_NoComp.tif");
	tRequire(tifPic_NoComp.IsValid());
	tifPic_NoComp.Save("TestData/WrittenTiff_NoComp.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenTiff_NoComp.tga"));

	tPicture tifPic_Pack("TestData/Tiff_Pack.tif");
	tRequire(tifPic_Pack.IsValid());
	tifPic_Pack.Save("TestData/WrittenTiff_Pack.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenTiff_Pack.tga"));

	tPicture tifPic_LZW("TestData/Tiff_LZW.tif");
	tRequire(tifPic_LZW.IsValid());
	tifPic_LZW.Save("TestData/WrittenTiff_LZW.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenTiff_LZW.tga"));

	tPicture tifPic_ZIP("TestData/Tiff_ZIP.tif");
	tRequire(tifPic_ZIP.IsValid());
	tifPic_ZIP.Save("TestData/WrittenTiff_ZIP.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenTiff_ZIP.tga"));

	tPicture exrPicForSave2("TestData/Desk.exr");
	exrPicForSave2.SaveGIF("TestData/WrittenDesk.gif");
	tRequire(tSystem::tFileExists("TestData/WrittenDesk.gif"));

	tPicture exrPicToSaveAsAPNG("TestData/Desk.exr");
	exrPicToSaveAsAPNG.SaveAPNG("TestData/WrittenDesk.apng");
	tRequire(tSystem::tFileExists("TestData/WrittenDesk.apng"));

	tPicture exrPicToSaveAsTIFF("TestData/Desk.exr");
	exrPicToSaveAsTIFF.SaveTIFF("TestData/WrittenDesk.tiff");
	tRequire(tSystem::tFileExists("TestData/WrittenDesk.tiff"));
}


tTestUnit(ImageRotation)
{
	if (!tSystem::tDirExists("TestData/"))
		tSkipUnit(Image)

	// Test writing rotated images.
	tPicture aroPic("TestData/RightArrow.png");
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
		tsPrintf(writeFile, "TestData/WrittenRightArrow_NoResampRot%03d.tga", int(tMath::tRadToDeg(angle)));
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
		tsPrintf(writeFile, "TestData/WrittenRightArrow_BilinearResampleRot%03d.tga", int(tMath::tRadToDeg(angle)));
		rotPic.Save(writeFile);
	}

	tPrintf("Test 'plane' rotation.\n");
	tPicture planePic("TestData/plane.png");
	int w = planePic.GetWidth();
	int h = planePic.GetHeight();
	planePic.RotateCenter(-tMath::PiOver4, tColouri::transparent);
}


tTestUnit(ImageCrop)
{
	if (!tSystem::tDirExists("TestData/"))
		tSkipUnit(Image)

	// Crop black pixels ignoring alpha (RGB channels only).
	tPicture planePic("TestData/plane.png");
	int w = planePic.GetWidth();
	int h = planePic.GetHeight();
	planePic.Crop(tColouri::black, tMath::ColourChannel_RGB);
	planePic.Crop(w, h, tPicture::Anchor::MiddleMiddle, tColouri::transparent);
	planePic.Save("TestData/WrittenPlane.png");
}


tTestUnit(ImageDetection)
{
	if (!tSystem::tDirExists("TestData/"))
		tSkipUnit(Image)

	// Test APNG detection.
	bool isAnimA = tImageAPNG::IsAnimatedPNG("TestData/TextCursor.png");
	tRequire(!isAnimA);

	bool isAnimB = tImageAPNG::IsAnimatedPNG("TestData/Icos4D.apng");
	tRequire(isAnimB);

	bool isAnimC = tImageAPNG::IsAnimatedPNG("TestData/Icos4D.png");
	tRequire(isAnimC);
}


tTestUnit(ImageFilter)
{
	if (!tSystem::tDirExists("TestData/"))
		tSkipUnit(Image)

	for (int filt = 0; filt < int(tResampleFilter::NumFilters); filt++)
		tPrintf("Filter Name %d: %s\n", filt, tResampleFilterNames[filt]);

	tPicture resamplePicNearest("TestData/TextCursor.png");		// 512x256.
	resamplePicNearest.Resample(800, 300, tResampleFilter::Nearest);
	resamplePicNearest.SaveTGA("TestData/WrittenResampledNearest.tga");

	tPicture resamplePicBox("TestData/TextCursor.png");		// 512x256.
	resamplePicBox.Resample(800, 300, tResampleFilter::Box);
	resamplePicBox.SaveTGA("TestData/WrittenResampledBox.tga");

	tPicture resamplePicBilinear("TestData/TextCursor.png");	// 512x256.
	resamplePicBilinear.Resample(800, 300, tResampleFilter::Bilinear);
	resamplePicBilinear.SaveTGA("TestData/WrittenResampledBilinear.tga");

	tPicture resamplePicBicubicStandard("TestData/TextCursor.png");	// 512x256.
	resamplePicBicubicStandard.Resample(800, 300, tResampleFilter::Bicubic_Standard);
	resamplePicBicubicStandard.SaveTGA("TestData/WrittenResampledBicubicStandard.tga");

	tPicture resamplePicBicubicCatmullRom("TestData/TextCursor.png");	// 512x256.
	resamplePicBicubicCatmullRom.Resample(800, 300, tResampleFilter::Bicubic_CatmullRom);
	resamplePicBicubicCatmullRom.SaveTGA("TestData/WrittenResampledBicubicCatmullRom.tga");

	tPicture resamplePicBicubicMitchell("TestData/TextCursor.png");	// 512x256.
	resamplePicBicubicMitchell.Resample(800, 300, tResampleFilter::Bicubic_Mitchell);
	resamplePicBicubicMitchell.SaveTGA("TestData/WrittenResampledBicubicMitchell.tga");

	tPicture resamplePicBicubicCardinal("TestData/TextCursor.png");	// 512x256.
	resamplePicBicubicCardinal.Resample(800, 300, tResampleFilter::Bicubic_Cardinal);
	resamplePicBicubicCardinal.SaveTGA("TestData/WrittenResampledBicubicCardinal.tga");

	tPicture resamplePicBicubicBSpline("TestData/TextCursor.png");	// 512x256.
	resamplePicBicubicBSpline.Resample(800, 300, tResampleFilter::Bicubic_BSpline);
	resamplePicBicubicBSpline.SaveTGA("TestData/WrittenResampledBicubicBSpline.tga");

	tPicture resamplePicLanczosNarrow("TestData/TextCursor.png");	// 512x256.
	resamplePicLanczosNarrow.Resample(800, 300, tResampleFilter::Lanczos_Narrow);
	resamplePicLanczosNarrow.SaveTGA("TestData/WrittenResampledLanczosNarrow.tga");

	tPicture resamplePicLanczosNormal("TestData/TextCursor.png");	// 512x256.
	resamplePicLanczosNormal.Resample(800, 300, tResampleFilter::Lanczos_Normal);
	resamplePicLanczosNormal.SaveTGA("TestData/WrittenResampledLanczosNormal.tga");

	tPicture resamplePicLanczosWide("TestData/TextCursor.png");	// 512x256.
	resamplePicLanczosWide.Resample(800, 300, tResampleFilter::Lanczos_Wide);
	resamplePicLanczosWide.SaveTGA("TestData/WrittenResampledLanczosWide.tga");
}


tTestUnit(ImageMultiFrame)
{
	if (!tSystem::tDirExists("TestData/"))
		tSkipUnit(Image)

	// A multipage tiff.
	tPicture tifPic_Multipage_ZIP_P1("TestData/Tiff_Multipage_ZIP.tif", 0);
	tRequire(tifPic_Multipage_ZIP_P1.IsValid());
	tifPic_Multipage_ZIP_P1.Save("TestData/WrittenTiff_Multipage_ZIP_P1.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenTiff_Multipage_ZIP_P1.tga"));

	tPicture tifPic_Multipage_ZIP_P2("TestData/Tiff_Multipage_ZIP.tif", 1);
	tRequire(tifPic_Multipage_ZIP_P2.IsValid());
	tifPic_Multipage_ZIP_P2.Save("TestData/WrittenTiff_Multipage_ZIP_P2.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenTiff_Multipage_ZIP_P2.tga"));

	tPicture tifPic_Multipage_ZIP_P3("TestData/Tiff_Multipage_ZIP.tif", 2);
	tRequire(tifPic_Multipage_ZIP_P3.IsValid());
	tifPic_Multipage_ZIP_P3.Save("TestData/WrittenTiff_Multipage_ZIP_P3.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenTiff_Multipage_ZIP_P3.tga"));

	// tImageWEBP also supports saving multi-frame webp files.
	tImageAPNG apngSrc("TestData/Flame.apng");
	tImageWEBP webpDst( apngSrc.Frames, true);
	webpDst.Save("TestData/WrittenFlameManyFrames.webp");
	tRequire(tSystem::tFileExists("TestData/WrittenFlameManyFrames.webp"));

	tImageAPNG apngSrc2("TestData/Icos4D.apng");
	tImageWEBP webpDst2( apngSrc2.Frames, true);
	webpDst2.Save("TestData/WrittenIcos4DManyFrames.webp");
	tRequire(tSystem::tFileExists("TestData/WrittenIcos4DManyFrames.webp"));

	// tImageGIF supports saving multi-frame gif files.
	tImageAPNG apngSrc3("TestData/Icos4D.apng");
	tImageGIF gifDst(apngSrc3.Frames, true);
	gifDst.Save("TestData/WrittenIcos4DManyFrames.gif");
	tRequire(tSystem::tFileExists("TestData/WrittenIcos4DManyFrames.gif"));

	// tImageAPNG supports saving multi-frame apng files.
	tImageAPNG apngSrc4("TestData/Icos4D.apng");
	tImageAPNG apngDst(apngSrc4.Frames, true);
	apngDst.Save("TestData/WrittenIcos4DManyFrames.apng");
	tRequire(tSystem::tFileExists("TestData/WrittenIcos4DManyFrames.apng"));

	// Load a multipage tiff with no page duration info.
	tPrintf("Test multipage TIFF load.\n");
	tImageTIFF tiffMultipage("TestData/Tiff_Multipage_ZIP.tif");
	tRequire(tiffMultipage.IsValid());

	// Create a multipage tiff with page duration info.
	tImageAPNG apngSrc5("TestData/Icos4D.apng");
	tImageTIFF tiffDst(apngSrc5.Frames, true);
	tiffDst.Save("TestData/WrittenIcos4DManyFrames.tiff");
	tRequire(tSystem::tFileExists("TestData/WrittenIcos4DManyFrames.tiff"));

	// Load a multipage tiff with page duration info since it was saved from Tacent.
	tImageTIFF tiffWithDur("TestData/WrittenIcos4DManyFrames.tiff");
	tiffWithDur.Save("TestData/WrittenIcos4DManyFrames2.tiff");
	tRequire(tSystem::tFileExists("TestData/WrittenIcos4DManyFrames2.tiff"));
}


}
