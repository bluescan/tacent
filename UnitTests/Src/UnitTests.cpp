// UnitTests.cpp
//
// Tacent unit tests.
//
// Copyright (c) 2017, 2019-2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#ifdef PLATFORM_WINDOWS
#include <locale.h>
#endif
#include <Foundation/tVersion.cmake.h>
#include <System/tCmdLine.h>
#include "UnitTests.h"
#include "TestFoundation.h"
#include "TestMath.h"
#include "TestSystem.h"
#if !defined(ARCHITECTURE_ARM32) && !defined(ARCHITECTURE_ARM64)
#include "TestPipeline.h"
#include "TestImage.h"
#include "TestScene.h"
#endif


using namespace tStd;
tCmdLine::tOption OptionPrintAllOutput("Print all output.", 'a', "all");
tCmdLine::tOption OptionShared("Share and enjoy.", 'e', "enj");
tCmdLine::tOption OptionHelp("Display help.", "help", 'h', 0);
tCmdLine::tOption OptionNumber("Number option.", "num", 'n', 2);
tCmdLine::tOption OptionLongOnly("Long Only.", "longonly");
tCmdLine::tOption OptionShortOnly("Short Only.", 's');
tCmdLine::tParam Param2("Parameter Two", "Param2", 2);
tCmdLine::tParam Param1("Parameter One", "Param1", 1);


namespace tUnitTest
{
	int UnitRequirementNumber	= 0;
	int UnitGoalNumber			= 0;
	int UnitsSkipped			= 0;
	int TotalRequirements		= 0;
	int RequirementsPassed		= 0;
	int TotalGoals				= 0;
	int GoalsPassed				= 0;
}


int main(int argc, char** argv)
{
	#ifdef PLATFORM_WINDOWS
	setlocale(LC_ALL, ".UTF8");
	#endif

	// Try calling with a command line like:
	// UnitTests.exe -n -35 3.0 -10 hello20
	// UnitTests.exe --help
	// UnitTests.exe -h
	tCmdLine::tParse(argc, argv);

	if (OptionHelp)
	{
		tCmdLine::tPrintUsage
		(
			u8"Tristan Grimmer",
			u8"This program takes wingnuts and twists them into dingwags. This description\n"
			"should not end in a newline.",
			3, 12
		);
		tCmdLine::tPrintSyntax();
		return 0;
	}

	if (Param1)
		tPrintf("Param1:%s AsInt:%d\n", Param1.Get().Pod(), Param1.Get().AsInt32());

	if (Param2)
		tPrintf("Param2:%s AsInt:%d\n", Param2.Get().Pod(), Param2.Get().AsInt32());

	if (OptionNumber)
	{
		tPrintf("NumOption Arg1:%s AsInt:%d\n", OptionNumber.Arg1().Pod(), OptionNumber.Arg1().AsInt32());
		tPrintf("NumOption Arg2:%s AsFlt:%f\n", OptionNumber.Arg2().Pod(), OptionNumber.Arg2().GetAsFloat());
	}

	if (Param1 || Param2 || OptionNumber)
		return 0;

#ifdef UNIT_TEST_FORCE_PRINT_ALL_OUTPUT
	OptionPrintAllOutput.Present = true;
#endif

	if (OptionPrintAllOutput)
		tSystem::tSetChannels(tSystem::tChannel_All);
	else
		tSystem::tSetChannels(tSystem::tChannel_TestResult);

	tUnitTest::rPrintf
	(
		"Testing Tacent Version %d.%d.%d\n",
		tVersion::Major,
		tVersion::Minor, 
		tVersion::Revision
	);

	#if !defined(UNIT_TEST_ONLY_ONE_TEST)

	// Foundation tests.
	tTest(Types);
	tTest(Array);
	tTest(List);
	tTest(ListExtra);
	tTest(Map);
	tTest(Promise);
	tTest(Sort);
	tTest(BitArray);
	tTest(BitField);
	tTest(FixInt);
	tTest(String);
	tTest(RingBuffer);
	tTest(PriorityQueue);
	tTest(MemoryPool);
	tTest(Hash);
	tTest(UTF);
	tTest(Half);

	// Math tests.
	tTest(Fundamentals);
	tTest(Interval);
	tTest(Spline);
	tTest(Random);
	tTest(Matrix);
	tTest(Quaternion);
	tTest(Geometry);
	tTest(Colour);

	// System tests.
	tTest(CmdLine);
	tTest(Task);
	tTest(Print);
	tTest(Regex);
	tTest(Script);
	tTest(Rule);
	tTest(Chunk);
	tTest(FileTypes);
	tTest(Directories);
	tTest(File);
	tTest(FindRec);
	tTest(Network);
	tTest(Time);
	tTest(Machine);

	// Build tests.
	#ifdef PLATFORM_WINDOWS
	tTest(Process);
	#endif

	// Image tests.
	#if !defined(ARCHITECTURE_ARM32) && !defined(ARCHITECTURE_ARM64)
	tTest(ImageLoad);
	tTest(ImageSave);
	tTest(ImageTexture);
	tTest(ImagePicture);
	tTest(ImageQuantize);
	tTest(ImagePalette);
	tTest(ImageMetaData);
	tTest(ImageLosslessTransform);
	tTest(ImageRotation);
	tTest(ImageCrop);
	tTest(ImageAdjustment);
	tTest(ImageDetection);
	tTest(ImageFilter);
	tTest(ImageMultiFrame);
	tTest(ImageGradient);
	tTest(ImageDDS);
	tTest(ImageKTX1);
	tTest(ImageKTX2);
	tTest(ImageASTC);
	tTest(ImagePKM);
	#endif

	#else

	// If UNIT_TEST_ONLY_ONE_TEST is defined, this is the test.
	// tTest(Fundamentals);
	// tTest(Interval);
	// tTest(FileTypes);
	// tTest(Directories);
	// tTest(File);
	// tTest(FindRec);
	// tTest(Network);
	// tTest(Time);
	// tTest(CmdLine);
	// tTest(String);
	// tTest(List);
	// tTest(ListExtra);
	// tTest(Colour);
	// tTest(Print);
	// tTest(Map);
	// tTest(Promise);
	// tTest(Script);
	// tTest(Rule);
	#if !defined(ARCHITECTURE_ARM32) && !defined(ARCHITECTURE_ARM64)
	// tTest(ImageLoad);
	// tTest(ImageSave);
	// tTest(ImageTexture);
	// tTest(ImageMultiFrame);
	// tTest(ImagePicture);
	// tTest(ImageQuantize);
	// tTest(ImagePalette);
	// tTest(ImageFilter);
	// tTest(ImageGradient);
	tTest(ImageMetaData);
	// tTest(ImageLosslessTransform);
	// tTest(ImageRotation);
	// tTest(ImageCrop);
	// tTest(ImageAdjustment);
	// tTest(ImageDetection);
	// tTest(ImageDDS);
	// tTest(ImageKTX1);
	// tTest(ImageKTX2);
	// tTest(ImageASTC);
	// tTest(ImagePKM);
	#endif
	// tTest(UTF);
	// tTest(Hash);
	// tTest(BitArray);
	// tTest(BitField);
	// tTest(FixInt);
	// tTest(Half);

	#endif

	return tUnitTest::tTestResults();
}
