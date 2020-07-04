// TestImage.cpp
//
// Image module tests.
//
// Copyright (c) 2017, 2019, 2020 Tristan Grimmer.
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
#include <Image/tImageTGA.h>
#include <Image/tImageJPG.h>
#include <Image/tImageWEBP.h>
#include <System/tFile.h>
#include "UnitTests.h"
using namespace tStd;
namespace tUnitTest
{


tTestUnit(Image)
{
	if (!tSystem::tDirExists("TestData/"))
		tSkipUnit(Image)

	// Test direct loading classes.
	tImage::tImageDDS imgDDS("TestData/TestDXT1.dds");
	tRequire(imgDDS.IsValid());

	tImage::tImageEXR imgEXR("TestData/Desk.exr");
	tRequire(imgEXR.IsValid());

	tImage::tImageGIF imgGIF("TestData/8-cell-simple.gif");
	tRequire(imgGIF.IsValid());

	tImage::tImageHDR imgHDR("TestData/mpi_atrium_3.hdr");
	tRequire(imgHDR.IsValid());

	tImage::tImageICO imgICO("TestData/UpperBounds.ico");
	tRequire(imgICO.IsValid());

	tImage::tImageTGA imgTGA("TestData/WhiteBorderRLE.tga");
	tRequire(imgTGA.IsValid());

	tImage::tImageJPG imgJPG("TestData/WiredDrives.jpg");
	tRequire(imgJPG.IsValid());

	tImage::tImageWEBP imgWEBP("TestData/RockyBeach.webp");
	tRequire(imgWEBP.IsValid());

	// Test dxt1 texture.
	tImage::tTexture dxt1Tex("TestData/TestDXT1.dds");
	tRequire(dxt1Tex.IsValid());

	tChunkWriter writer("TestData/WrittenTestDXT1.tac");
	dxt1Tex.Save(writer);
	tRequire( tSystem::tFileExists("TestData/WrittenTestDXT1.tac") );

	tChunkReader reader("TestData/WrittenTestDXT1.tac");
	dxt1Tex.Load( reader.Chunk() );
	tRequire(dxt1Tex.IsValid());

	// Test cubemap.
	tImage::tTexture cubemap("TestData/CubemapLayoutGuide.dds");
	tRequire(cubemap.IsValid());

	// Test jpg to texture. This will do conversion to BCTC.
	tGoal(!"BCTC needs to be re-enabled for texture creation.");
	//tImage::tTexture jpgTex("TestData/WiredDrives.jpg", true);
	//tRequire(jpgTex.IsValid());
	
	// Test tPicture loading jpg and saving as tga.
	tImage::tPicture jpgPic("TestData/WiredDrives.jpg");
	tRequire(jpgPic.IsValid());

	jpgPic.Save("TestData/WrittenWiredDrives.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenWiredDrives.tga"));

	// Test tPicture loading png (with alpha channel) and saving as tga (with alpha channel).
	tImage::tPicture pngPic("TestData/Xeyes.png");
	tRequire(pngPic.IsValid());

	pngPic.SaveTGA("TestData/WrittenXeyes.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenXeyes.tga"));

	// Test saving tPicture in all supported formats.
	pngPic.Save("TestData/WrittenXeyesTGA.tga");
	tRequire( tSystem::tFileExists("TestData/WrittenXeyesTGA.tga"));

	pngPic.Save("TestData/WrittenXeyesPNG.png");
	tRequire( tSystem::tFileExists("TestData/WrittenXeyesPNG.png"));

	pngPic.Save("TestData/WrittenXeyesBMP.bmp");
	tRequire( tSystem::tFileExists("TestData/WrittenXeyesBMP.bmp"));

	pngPic.Save("TestData/WrittenXeyesJPG.jpg");
	tRequire( tSystem::tFileExists("TestData/WrittenXeyesJPG.jpg"));

	#ifdef PLATFORM_WINDOWS
	pngPic.Save("TestData/WrittenXeyesGIF.gif");
	tRequire( tSystem::tFileExists("TestData/WrittenXeyesGIF.gif"));
	#else
	tGoal(!"Write to gif not working on this platform.");
	#endif
}


}
