// UnitTests.cpp
//
// Tacent unit tests.
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

#include <Foundation/tVersion.cmake.h>
#include <System/tCommand.h>
#include "UnitTests.h"
#include "TestPipeline.h"
#include "TestFoundation.h"
#include "TestImage.h"
#include "TestMath.h"
#include "TestScene.h"
#include "TestSystem.h"


using namespace tStd;
tCommand::tOption PrintAllOutput("Print all output.", 'a', "all");
tCommand::tOption SharedOption("Share and enjoy.", 'e', "enj");
tCommand::tOption HelpOption("Display help.", "help", 'h', 0);
tCommand::tOption NumberOption("Number option.", "num", 'n', 2);
tCommand::tParam Param2("Parameter Two", "Param2", 2);
tCommand::tParam Param1("Parameter One", "Param1", 1);


namespace tUnitTest
{
	int UnitRequirementNumber = 0;
	int UnitGoalNumber = 0;
	int UnitsSkipped = 0;
	int TotalRequirements = 0;
	int RequirementsPassed = 0;
	int TotalGoals = 0;
	int GoalsPassed = 0;
}


int main(int argc, char** argv)
{
	// Try calling with a command line like:
	// UnitTests.exe -n -35 3.0 -10 hello20
	// UnitTests.exe --help
	// UnitTests.exe -h
	tCommand::tParse(argc, argv);

	if (HelpOption)
	{
		tCommand::tPrintUsage
		(
			"Tristan Grimmer",
			"This program takes wingnuts and twists them into dingwags. This description\n"
			"should not end in a newline.",
			3, 12
		);
		tCommand::tPrintSyntax();
		return 0;
	}

	if (Param1)
		tPrintf("Param1:%s AsInt:%d\n", Param1.Get().Pod(), Param1.Get().AsInt32());

	if (Param2)
		tPrintf("Param2:%s AsInt:%d\n", Param2.Get().Pod(), Param2.Get().AsInt32());

	if (NumberOption)
	{
		tPrintf("NumOption Arg1:%s AsInt:%d\n", NumberOption.Arg1().Pod(), NumberOption.Arg1().AsInt32());
		tPrintf("NumOption Arg2:%s AsFlt:%f\n", NumberOption.Arg2().Pod(), NumberOption.Arg2().GetAsFloat());
	}

	if (Param1 || Param2 || NumberOption)
		return 0;

#ifdef UNIT_TEST_FORCE_PRINT_ALL_OUTPUT
	PrintAllOutput.Present = true;
#endif

	if (PrintAllOutput)
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
	tTest(Sort);
	tTest(FixInt);
	tTest(Bitfield);
	tTest(String);
	tTest(RingBuffer);
	tTest(PriorityQueue);
	tTest(MemoryPool);

	// Math tests.
	tTest(Fundamentals);
	tTest(Spline);
	tTest(Hash);
	tTest(Random);
	tTest(Matrix);
	tTest(Quaternion);
	tTest(Geometry);

	// System tests.
	tTest(CmdLine);
	tTest(Task);
	tTest(Print);
	tTest(Regex);
	tTest(Script);
	tTest(Chunk);
	tTest(File);
	tTest(Time);
	tTest(Machine);

	#ifndef PLATFORM_LINUX
	// Build tests.
	tTest(Process);
	#endif

	// Image tests.
	tTest(Image);

	#else

	// If UNIT_TEST_ONLY_ONE_TEST is defined, this is the test.
	// tTest(File);
	// tTest(Script);
	// tTest(CmdLine);
	// tTest(String);
	// tTest(List);
	// tTest(ListExtra);
	// tTest(Image);
	tTest(CmdLine);

	#endif

	return tUnitTest::tTestResults();
}
