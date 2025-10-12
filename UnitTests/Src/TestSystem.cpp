// TestSystem.cpp
//
// System module tests.
//
// Copyright (c) 2017, 2019-2025 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tVersion.cmake.h>
#include <Foundation/tAssert.h>
#include <Foundation/tMemory.h>
#include <Foundation/tHash.h>
#include <Math/tVector2.h>
#include <Math/tVector3.h>
#include <Math/tVector4.h>
#include <Math/tQuaternion.h>
#include <Math/tMatrix4.h>
#include <System/tCmdLine.h>
#include <System/tTask.h>
#include <System/tMachine.h>
#include <System/tRegex.h>
#include <System/tScript.h>
#include <System/tChunk.h>
#include <System/tTime.h>
#include "UnitTests.h"
#pragma warning (disable: 4723)
using namespace tSystem;
using namespace tMath;
extern tCmdLine::tOption OptionShared;


namespace tUnitTest
{


tTestUnit(CmdLine)
{
	tPrintf("Testing tCmdLine command line parsing.\n");

	// Although not necessarily a common use case, it's fine to have options and parameters as stack
	// variables. As long as they are in scope when tParse is called, they will get populated.
	tCmdLine::tParam fromFile(1, "fromFile");
	tCmdLine::tParam toFile(2, "toFile");
	tCmdLine::tOption log("Specify log file.", "log", 'l', 1);
	tCmdLine::tOption overwrite("Overwrite dest.", "overwrite", 0);
	tCmdLine::tOption recurse("Recursive.", 'R', 0);
	tCmdLine::tOption program("Program mode.", 'p', 0);
	tCmdLine::tOption time("Print timestamp.", "time", 't', 0);
	tCmdLine::tOption stop("Stop early.", "stop", 's', 0);
	tCmdLine::tParam inputFiles(0, "InputFiles", "Multiple file parameters");
	tCmdLine::tParam param3(3, "param3");	// Param because unrecognized option. See command-line string.
	tCmdLine::tParam param4(4, "param4");	// Param because in quotes. See command-line string.

	// Normally you would call tParse from main with argc and argv. The call below allows one to test command lines
	// by entering the command line arguments directly as a string.
	// tCmdLine::tParse("-R --overwrite fileA.txt -pt fileB.txt --log log.txt -l log2.txt --notthere --enj");

	// This is another way of entering a test command line. The true means the first entry is the program name.
	tCmdLine::tParse(u8"UnitTests.exe -R --overwrite fileA.txt -pt fileB.txt --log log.txt -l log2.txt --notthere --enj '-R'", true);

	tCmdLine::tPrintSyntax();

	// There are a few differnt ways of calling PrintUsage:
	// tCmdLine::tPrintUsage();
	// tCmdLine::tPrintUsage(tVersion::Major, tVersion::Minor);
	// tCmdLine::tPrintUsage(tVersion::Major);
	tCmdLine::tPrintUsage(tVersion::Major, tVersion::Minor, tVersion::Revision);
	// tCmdLine::tPrintUsage("Tony Tekhead", tVersion::Major, tVersion::Minor);
	// tCmdLine::tPrintUsage("Tony Tekhead", tVersion::Major, tVersion::Minor, tVersion::Revision);
	// tCmdLine::tPrintUsage("Version 42.67 By Patty Programmer");
	//
	// tString usageText;
	// tCmdLine::tStringUsageNI(usageText, tVersion::Major, tVersion::Minor, tVersion::Revision);
	// tPrintf("%s", usageText.Pod());

	tPrintf("OptionShared: %s\n", OptionShared.IsPresent() ? "true" : "false");
	tRequire(log.IsPresent());
	tRequire(!stop.IsPresent());
	tRequire(fromFile.IsPresent() && (fromFile.Get() == "fileA.txt"));
	tRequire(toFile.IsPresent()   && (toFile.Get()   == "fileB.txt"));
	tRequire(param3.IsPresent()   && (param3.Get()   == "--notthere"));
	tRequire(param4.IsPresent()   && (param4.Get()   == "-R"));
	tRequire(OptionShared.IsPresent());

	// More than one log entry simply adds to the numer option arguments. If an option took 2 args (A B) and was
	// specified twice, you would get A1 B1 A2 B2 for the arguments.
	tPrintf("Option log: %s\n", log.IsPresent() ? "present" : "absent");
	if (log.IsPresent())
		for (tStringItem* optArg = log.Args.First(); optArg; optArg = optArg->Next())
			tPrintf("    Log arg: %s\n", optArg->Pod());

	tPrintf("Param fromFile: %s\n", fromFile.IsPresent() ? "present" : "absent");
	if (fromFile.IsPresent())
		tPrintf("    FromFile: %s\n", fromFile.Get().Pod());

	tPrintf("Param toFile: %s\n", toFile.IsPresent() ? "present" : "absent");
	if (toFile.IsPresent())
		tPrintf("    toFile: %s\n", toFile.Get().Pod());

	tPrintf("Param 3: %s\n", param3.IsPresent() ? "present" : "absent");
	if (param3.IsPresent())
		tPrintf("    param3: %s\n", param3.Get().Pod());

	tPrintf("Param 4: %s\n", param4.IsPresent() ? "present" : "absent");
	if (param4.IsPresent())
		tPrintf("    param4: %s\n", param4.Get().Pod());

	tPrintf("Param inputFiles: %s\n", inputFiles.IsPresent() ? "present" : "absent");
	if (inputFiles.IsPresent())
		for (tStringItem* item = inputFiles.Values.First(); item; item = item->Next())
			tPrintf("    inputFiles: %s\n", item->Pod());
}


struct MyTask : public tTask
{
	MyTask() { }
	double Execute(double timeDelta) override;
	int ExecuteCount = 0;
	float LargestTimeDelta = 0.0f;
};


double MyTask::Execute(double timeDelta)
{
	ExecuteCount++;
	tPrintf("ExecuteCount: %d  TimeDelta: %f\n", ExecuteCount, timeDelta);
	if (timeDelta > LargestTimeDelta)
		LargestTimeDelta = timeDelta;

	double runAgainTime = 0.1; // Seconds.
	return runAgainTime;
}


tTestUnit(Task)
{
	int64 freq = tGetHardwareTimerFrequency();
	tTaskSetF tasks(freq, 0.1);
	MyTask* t1 = new MyTask();
	MyTask* t2 = new MyTask();

	tasks.Insert(t1);
	tasks.Insert(t2);

	tPrintf("\n\nStarting Execute Loop for 1.6 seconds.\n");
	for (int y = 0; y < 100; y++)
	{
		tSleep(16);
		int64 count = tGetHardwareTimerCount();
		tasks.Update(count);
	}

	tGoal(t1->LargestTimeDelta < 0.2f);
	tGoal(t2->LargestTimeDelta < 0.2f);
	tGoal(t1->ExecuteCount > 10);
	tGoal(t2->ExecuteCount > 10);

	int t1count = t1->ExecuteCount;
	int t2count = t2->ExecuteCount;

	tPrintf("\nRemoving task...\n");
	tasks.Remove(t1);

	tPrintf("\n\nStarting Execute Loop for 0.8 seconds.\n");
	for (int y = 0; y < 50; y++)
	{
		tSleep(16);
		int64 count = tGetHardwareTimerCount();
		tasks.Update(count);
	}

	tRequire(t1->ExecuteCount == t1count);
	tGoal(t2->LargestTimeDelta < 0.2f);
	tGoal(t2->ExecuteCount > t2count);

	tPrintf("\nExiting loop\n");
}


// This compares the output of tvsPrintf to the standard vsprintf. Some differences are intended while others are not.
bool PrintCompare(const char* format, ...)
{
	tPrint("\nComparing formatted output. Next three entries: (format, tPrintf, printf)\n");
	tPrint(format);
	va_list args;

	char tbuf[512];
	va_start(args, format);
	int tcount = tsvPrintf(tbuf, format, args);
	va_end(args);

	char nbuf[512];
	va_start(args, format);
	int ncount = vsprintf(nbuf, format, args);
	va_end(args);

	tPrint(tbuf);
	tPrint(nbuf);
	bool match = tStd::tStrcmp(tbuf, nbuf) ? false : true;
	tPrintf
	(
		"Str Match: %s  Len Match: %s\n", match ? "True" : "False",
		(tcount == ncount) ? "True" : "False"
	);
	return match;
}


// Tests the tPrintf formatting engine.
void PrintTest(const char* format, ...)
{
	tPrint("Next two entries: (format, tPrintf)\n");
	tPrint(format);
	va_list args;

	va_start(args, format);
	int tcount = tvPrintf(format, args);
	va_end(args);

	tPrintf("Char Count: %d\n\n", tcount);
}


template<typename T> static tString ConvertToString(T value)
{
	tString valStr = tsrPrint(value);
	return valStr;
}


tTestUnit(Print)
{
	tSetDefaultPrecision(6);

	// We test prints here. AKA print tests. How well does tPrintf work.
	tPrint("tPrintf Tests.\n");
	tRequire(PrintCompare("Hex %#010X\n", 0x0123ABCD));
	tRequire(PrintCompare("Hex %#010x\n", 0));
	tRequire(PrintCompare("Hex %04x\n", 0xFFFFF101));

	PrintTest("Pointer %p\n", 0xFFFFF101);
	PrintTest("Pointer %p\n", 0x00ABC710);
	PrintTest("Pointer %p\n", 0);

	PrintTest("Integer 64bit value neg forty-two:   ___%|64d___\n", int64(-42));
	tRequire(PrintCompare("Integer value neg forty-two:         ___%d___\n", -42));
	tRequire(PrintCompare("Integer value forty-two:             ___%d___\n", 42));

	uint8 u8 = 0xA7;
	PrintTest
	(
		"Binary  1010 0111 (8 bit):\n"
		"      __%08b__\n", u8
	);

	uint16 u16 = 0xA70F;
	PrintTest
	(
		"Binary  1010 0111 0000 1111 (16 bit):\n"
		"      __%16b__\n", u16
	);

	uint32 u32 = 0xA70F1234;
	PrintTest
	(
		"Binary  1010 0111 0000 1111 0001 0010 0011 0100 (32 bit):\n"
		"      __%32b__\n", u32
	);

	uint64 u64 = 0x170F1234B8F0B8F0LL;
	PrintTest
	(
		"Binary  0001 0111 0000 1111 0001 0010 0011 0100 1011 1000 1111 0000 1011 1000 1111 0000 (64 bit):\n"
		"      __%0_64|64b__\n", u64
	);

	PrintTest
	(
		"Octal   0001 3417 0443 2270 7413 4360 (64 bit):\n"
		"      __%0_24:2o__\n", u64
	);

	PrintTest
	(
		"Boolean true:\n"
		"      __%B__\n", true
	);

	PrintTest
	(
		"Boolean false:\n"
		"      __%B__\n", false
	);

	PrintTest
	(
		"Boolean true:\n"
		"      __%_B__\n", true
	);

	PrintTest
	(
		"Boolean false:\n"
		"      __%_B__\n", false
	);

	PrintTest
	(
		"Boolean true:\n"
		"      __%'B__\n", true
	);

	PrintTest
	(
		"Boolean false:\n"
		"      __%'B__\n", false
	);

	PrintTest
	(
		"Boolean true:\n"
		"      __%_08B__\n", true
	);

	PrintTest
	(
		"Boolean false:\n"
		"      __%010B__\n", false
	);

	PrintTest
	(
		"Boolean true:\n"
		"      __%2B__\n", true
	);

	PrintTest
	(
		"Boolean false:\n"
		"      __%12B__\n", false
	);

	tRequire(PrintCompare("Octal value forty-nine:              ___%#o___\n", 49));
	tRequire(PrintCompare("Percent symbol.                      ___%%___\n"));

	// I prefer the behaviour of windows printf here. If char after % is invalid, just print the character and
	// do NOT print the percent. The only way to get a percent should be %%. Clang and MSVC behave differently.
	#ifdef PLATFORM_WINDOWS
	tRequire(PrintCompare("Invalid char after percent.          ___%^___\n"));
	tRequire(PrintCompare("Invalid char after percent.          ___%%%^___\n"));
	#else
	PrintTest            ("Invalid char after percent.          ___%^___\n");
	PrintTest            ("Invalid char after percent.          ___%%%^___\n");
	#endif
	
	tRequire(PrintCompare("Float value forty-two:               ___%f___\n", float(42.0f)));
	tRequire(PrintCompare("Float value neg forty-two:           ___%f___\n", float(-42.0f)));
	tRequire(PrintCompare("Double value forty-two:              ___%f___\n", double(42.0)));
	tRequire(PrintCompare("Double value neg forty-two:          ___%f___\n", double(-42.0)));

	tRequire(PrintCompare("Float 42 width 10 leading 0:         ___%010f___\n", float(42.0f)));
	tRequire(PrintCompare("Int 42 width 10 leading 0:           ___%010d___\n", 42));
	tRequire(PrintCompare("Float width 10 lead 0 Left:          ___%-010f___\n", float(42.0f)));
	tRequire(PrintCompare("Int width 10 lead 0 Left:            ___%-010d___\n", 42));

	tRequire(PrintCompare("Int 1234 with prec 6:                ___%.6d___\n", 1234));
	tRequire(PrintCompare("Float value forty-two width 10:      ___%010f___\n", float(42.0f)));

	tVec3 v3b; v3b.x = 1.0f; v3b.y = 2.0f; v3b.z = 3.0f;
	tVector2 v2(1.0f, 2.0f);
	tVector3 v3(1.0f, 2.0f, 3.0f);
	tVector4 v4(1.0f, 2.0f, 3.0f, 4.0f);

	PrintTest("Vector 2D:                           ___%:2v___\n", v2.Pod());
	PrintTest("Vector 3D pod:                       ___%.3v___\n", tPod(v3));
	PrintTest("Vector 3D base:                      ___%:3v___\n", v3b);
	PrintTest("Vector 4D:                           ___%:4v___\n", tPod(v4));
	PrintTest("Vector 4D %%06.2:4v:                 ___%06.2:4v___\n", tPod(v4));
	PrintTest("Vector 4D Alternative:               ___%_:4v___\n", v4.Pod());

	tQuaternion quat(8.0f, 7.0f, 6.0f, 5.0f);
	tStaticAssert(sizeof(tQuaternion) == 16);
	tStaticAssert(sizeof(tVector4) == 16);
	tStaticAssert(sizeof(tVector3) == 12);
	tStaticAssert(sizeof(tVector2) == 8);
	tStaticAssert(sizeof(tMatrix2) == 16);
	tStaticAssert(sizeof(tMatrix4) == 64);
	PrintTest("Quaternion: %q\n", tPod(quat));
	PrintTest("Quaternion Alternate: %_q\n", tPod(quat));

	tMatrix4 mat;
	mat.Identity();
	tVector4 c4(1.0f, 2.0f, 3.0f, 4.0f);
	mat.C4 = c4;

	PrintTest("Matrix 4x4 Normal:\n%05.2m\n", tPod(mat));
	PrintTest("Matrix 4x4 Decorated:\n%_m\n", mat.Pod());

	tMatrix2 mat2x2;
	mat2x2.Identity();
	PrintTest("Matrix 2x2 Normal:\n%:4m\n", tPod(mat2x2));
	PrintTest("Matrix 2x2 Decorated:\n%_:4m\n", tPod(mat2x2));

	tString test("This is the tString.");
	tRequire(PrintCompare("tString: %s\n", tPod(test)));
	tRequire(PrintCompare("Reg String: %s\n", "A regular string"));

	tRequire(PrintCompare("Char %c\n", 65));					// A
	tRequire(PrintCompare("Char %c %c %c\n", 65, 66, 67));		// A B C
	tRequire(PrintCompare("Char %4c %6c %8c\n", 65, 66, 67));	// A B C

	// Using the 0 prefix works differently on Linux vs Windows so we can't PrintCompare.
	// Tacent behaves (on purpose) like Windows where the leading 0s are printed even though
	// the type is not integral.
	#ifdef PLATFORM_WINDOWS
	tRequire(PrintCompare("Char %04c %06c %08c\n", 65, 66, 67));
	#else
	PrintTest("Char %04c %06c %08c\n", 65, 66, 67);
	#endif

	#ifdef PLATFORM_WINDOWS
	tPrintf("Windows non-POD tString print.\n");
	tString str = "This sentence is the tString.";
	PrintTest("The string is '%t'. This is a number:%d.\n", str, 42);

	tPrintf("Windows non-POD tMatrix4 print.\n");
	PrintTest("Matrix Decorated:\n%_m\n", mat);

	tString strA("This is string A");
	tString strB("This is string B");

	// Note that you may NOT pass an tStringItem for the %t format specifier.
	tPrintf("StringA:%t  StringB:%t\n", strA, strB);
	char buff[512];
	tsPrintf(buff, "StringA:%t  StringB:%t\n", strA, strB);
	tPrintf("tsPrintf buffer:%s\n", buff);

	#else
	tPrintf("Non-windows platform. Skipping all non-POD print tests.\n");
	#endif

	// Test counting and string printf.
	tVector3 vv(1.0f, 2.0f, 3.0f);
	char buf[256];
	tStd::tMemset(buf, 1, 256);
	int len = tsPrintf(buf, "Vector in string is: %v", tPod(vv));
	tPrintf("Str: [%s] LenRet:%d LenAct:%d\n", buf, len, strlen(buf));

	tStd::tMemset(buf, 'Z', 256);
	tsPrintf(buf, 24, "string len 24 vec: %v", tPod(vv));
	tPrintf("Str: [%s] LenRet:%d LenAct:%d\n", buf, len, strlen(buf));

	tPrint("Buffer contains:\n");
	tPrint("123456789012345678901234567890\n");
	for (int i = 0; i < 30; i++)
		if (!buf[i])
			tPrintf("~");
		else
			tPrintf("%c", buf[i]);

	tStd::tMemset(buf, 'Z', 256);
	tsPrintf(buf, 24, "v: %4.2v", tPod(vv));
	tPrint("\n\nBuffer contains:\n");
	tPrint("123456789012345678901234567890\n");
	for (int i = 0; i < 30; i++)
		if (!buf[i])
			tPrintf("~");
		else
			tPrintf("%c", buf[i]);

	tPrintf("\n\n");

	float negValFlt = -0.65f;
	tRequire(PrintCompare("Lead Zero With Negative Float:%07.3f\n", negValFlt));

	tPrintf("\n\n");
	int negValInt = -42;
	tRequire(PrintCompare("Lead Zero With Negative Int:%07d\n", negValInt));

	// Test special floating-point bitpatterns.
	tRequire(PrintCompare("Float PSNAN: %f\n", tStd::tFloatPSNAN()));
	tRequire(PrintCompare("Float NSNAN: %f\n", tStd::tFloatNSNAN()));
	tRequire(PrintCompare("Float PQNAN: %f\n", tStd::tFloatPQNAN()));
	tRequire(PrintCompare("Float IQNAN: %f\n", tStd::tFloatIQNAN()));
	tRequire(PrintCompare("Float NQNAN: %f\n", tStd::tFloatNQNAN()));
	tRequire(PrintCompare("Float PINF : %f\n", tStd::tFloatPINF()));
	tRequire(PrintCompare("Float NINF : %f\n", tStd::tFloatNINF()));

	tRequire(PrintCompare("tSqrt(-1.0f): %08.3f\n", tSqrt(-1.0f)));
	float fone = 1.0f; float fzero = 0.0f;
	tRequire(PrintCompare("fone/fzero: %08.3f\n", fone/fzero));
	tRequire(PrintCompare("fzero/fzero: %08.3f\n", fzero/fzero));

	tRequire(PrintCompare("Double PSNAN: %f\n", tStd::tDoublePSNAN()));
	tRequire(PrintCompare("Double NSNAN: %f\n", tStd::tDoubleNSNAN()));
	tRequire(PrintCompare("Double PQNAN: %f\n", tStd::tDoublePQNAN()));
	tRequire(PrintCompare("Double IQNAN: %f\n", tStd::tDoubleIQNAN()));
	tRequire(PrintCompare("Double NQNAN: %f\n", tStd::tDoubleNQNAN()));
	tRequire(PrintCompare("Double PINF : %f\n", tStd::tDoublePINF()));
	tRequire(PrintCompare("Double NINF : %f\n", tStd::tDoubleNINF()));

	tPrintf("SpaceForPos and Leading zeros:% 08.3f\n", 65.5775f);
	tRequire(PrintCompare("SpaceForPos and Leading zeros:% 08.3f\n", 65.5775f));

	tRequire(PrintCompare("Test %%f:%f\n", 65.12345678f));	
	tRequire(PrintCompare("Test %%e:%e\n", 65e24));
	tRequire(PrintCompare("Test %%e:%e\n", 123456789.123456789f));
	tRequire(PrintCompare("Test %%e:%e\n", 12345678900.0f));
	tRequire(PrintCompare("Test %%e:%e\n", 1.0f));
	tRequire(PrintCompare("Test %%g:%g\n", 1234567.123456789f));
	tRequire(PrintCompare("Test %%g:%g\n", 65.12345678f));
	tRequire(PrintCompare("Test %%g:%g\n", 651.2345678f));

	tSetDefaultPrecision(4);

	tFileHandle handle = tOpenFile("TestData/Written.log", "wt");
	ttfPrintf(handle, "Log: Here is some timestamped log data. Index = %d\n", 42);
	ttfPrintf(handle, "Warning: And a second log line.\n");
	tCloseFile(handle);

	// Test tsPrint to convert various types to strings easily.
	tRequire(ConvertToString(int8(62)) 				== "62");
	tRequire(ConvertToString(int8(-62))				== "-62");
	tRequire(ConvertToString(uint8(0x0A))			== "0x0A");

	tRequire(ConvertToString(int16(63)) 			== "63");
	tRequire(ConvertToString(int16(-63))			== "-63");
	tRequire(ConvertToString(uint16(0xAF98))		== "0xAF98");

	tRequire(ConvertToString(64) 					== "64");
	tRequire(ConvertToString(-64)					== "-64");
	tRequire(ConvertToString(uint32(0xF123ABCD))	== "0xF123ABCD");

	tRequire(ConvertToString(65ll)					== "65");
	tRequire(ConvertToString(-65ll)					== "-65");
	tRequire(ConvertToString(66ull)					== "0x0000000000000042");

	tRequire(ConvertToString(tint128(67))			== "67");
	tRequire(ConvertToString(tint128(-67))			== "-67");
	tRequire(ConvertToString(tuint128(68))			== "0x00000000000000000000000000000044");

	tRequire(ConvertToString(tint256(69))			== "69");
	tRequire(ConvertToString(tint256(-69))			== "-69");
	tRequire(ConvertToString(tuint256(70))			== "0x0000000000000000000000000000000000000000000000000000000000000046");

	tRequire(ConvertToString(tint512(71))			== "71");
	tRequire(ConvertToString(tint512(-71))			== "-71");
	tRequire(ConvertToString(tuint512(72))			== "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000048");

	tRequire(ConvertToString(137.1f)				== "137.1000");
	tRequire(ConvertToString(137.2)					== "137.2000");
	tRequire(ConvertToString(true)					== "true");
	tRequire(ConvertToString(false)					== "false");

	tRequire(ConvertToString(tVector2(1.0f, 2.0f))					== "(1.0000, 2.0000)");
	tRequire(ConvertToString(tVector3(1.0f, 2.0f, 3.0f))			== "(1.0000, 2.0000, 3.0000)");
	tRequire(ConvertToString(tVector4(1.0f, 2.0f, 3.0f, 4.0f))		== "(1.0000, 2.0000, 3.0000, 4.0000)");
	tRequire(ConvertToString(tQuaternion(1.0f, 2.0f, 3.0f, 4.0f))	== "(1.0000, 2.0000, 3.0000, 4.0000)");
}


void RegexPattern(const char* pattern, const char* test, const char* desc)
{
	tPrintf("%s\n", desc);
	tRegex regex(pattern);
	bool perfectMatch = regex.IsMatch(test);
	tPrintf("Pattern:%s  Test String:%s  Perfect Match:%s\n", pattern, test, perfectMatch ? "Yes" : "No");

	tList<tRegex::Match> matches;
	regex.Search(test, matches);
	for (tRegex::Match* m = matches.First(); m; m = m->Next())
		tPrintf("Submatch index:%d  Length:%d  String:%s\n", m->IndexStart, m->Length, m->GetString(test).Pod());
	tPrintf("\n");
}


tTestUnit(Regex)
{
	tString pattern = "[ABC][DEF]";
	tRegex regex(pattern);
	tString test;
	bool match = false;;

	test = "AB";
	match = regex.IsMatch(test);
	tPrintf("Pattern:%s  Test:%s  Perfect Match:%s\n", pattern.Pod(), test.Pod(), match ? "Yes" : "No");
	tRequire(!match);

	test = "BF";
	match = regex.IsMatch(test);
	tPrintf("Pattern:%s  Test:%s  Perfect Match:%s\n", pattern.Pod(), test.Pod(), match ? "Yes" : "No");
	tRequire(match);

	RegexPattern(".....", "Hello World", "Test '.' to match any character.");
	RegexPattern("(H..).(o..)", "Hello World", "Test '()' groupings.");
	RegexPattern("l+", "Hello World", "Test '+' to match the preceding pattern element one or more times.");
	RegexPattern("Hellp?o World", "Hello World", "Test '?' to match the preceding pattern element zero or one times.");
	RegexPattern("Hellp?o World", "Hellpo World", "Test '?' to match the preceding pattern element zero or one times.");
	RegexPattern("Hellp?o World", "Hellppo World", "Test '?' to match the preceding pattern element zero or one times.");
	RegexPattern("z*bar*en*ess", "barrenness", "Test '*' to match the preceding pattern element zero or more times.");
	RegexPattern("a{4}A", "aaaA", "Test {n} to match exactly n times.");
	RegexPattern("a{4}A", "aaaaA", "Test {n} to match exactly n times.");
	RegexPattern("a{4}", "aaaaa", "Test {n} to match exactly n times.");
	RegexPattern("Ab{3,}C", "AbbC", "Test {n,} to match n or more times.");
	RegexPattern("Ab{3,}C", "AbbbC", "Test {n,} to match n or more times.");
	RegexPattern("Ab{3,}C", "AbbbbC", "Test {n,} to match n or more times.");
	RegexPattern("H{2,4}", "H", "Test {n,m} to match from n to m times.");
	RegexPattern("H{2,4}", "HH", "Test {n,m} to match from n to m times.");
	RegexPattern("H{2,4}", "HHH", "Test {n,m} to match from n to m times.");
	RegexPattern("H{2,4}", "HHHH", "Test {n,m} to match from n to m times.");
	RegexPattern("H{2,4}", "HHHHH", "Test {n,m} to match from n to m times.");
	RegexPattern("Vow[AEIO]", "Vow", "Test [...] to match one item inside.");
	RegexPattern("Vow[AEIO]", "VowI", "Test [...] to match one item inside.");
	RegexPattern("One|Two|Three", "One", "Test | to match alternate possibilities.");
	RegexPattern("One|Two|Three", "Four", "Test | to match alternate possibilities.");
	RegexPattern("Req(One|Two|Three)", "ReqTwo", "Test | to match alternate possibilities.");
	RegexPattern("Req(One|Two|Three)", "ReqFour", "Test | to match alternate possibilities.");
	RegexPattern("llo\\b", "Hello", "Test \\b to match word boundary.");
	RegexPattern("ne\\b two\\b three", "one two three", "Test \\b to match word boundary.");

	// Now using a single regex object.
	tRegex rex("[ \\t]*[A-Za-z0-9\\:]+[ \\t]+[A-Za-z0-9\\:]+[ \\t]*\\([A-Za-z0-9\\:\\*\\&\\:\\, \\t]*\\)[ \\t]*");
	bool isMatch1 = rex.IsMatch("void Foo::Foo(int a, char* b)");
	tPrintf("Test1.  Should pass.  Result:%s\n", isMatch1 ? "Pass" : "Fail");
	tRequire(isMatch1);
	bool isMatch2 = rex.IsMatch("int* Foo::Foo(int a, char* b)");
	tPrintf("Test2.  Should fail.  Result:%s\n", isMatch2 ? "Pass" : "Fail");
	tRequire(!isMatch2);

	RegexPattern("\\w\\w\\w \\W\\W\\W", "a2B !@#", "Test \\w alphanumeric and \\W non-alphanumeric.");
	RegexPattern("\\w", "_", "Test \\w alphanumeric with underscore.");
	RegexPattern("\\W", "_", "Test \\W non-alpha-numeric with underscore.");
	RegexPattern("[^A-Za-z0-9_][A-Za-z0-9_][A-Za-z0-9_][A-Za-z0-9_][A-Za-z0-9_]", "@Dd4_", "Test ^ the not operator.");
	RegexPattern("[^A-Za-z0-9_][A-Za-z0-9_][A-Za-z0-9_][A-Za-z0-9_][A-Za-z0-9_]", "_Dd4_", "Test ^ the not operator.");
	RegexPattern("[^A-Za-z0-9_][A-Za-z0-9_][A-Za-z0-9_][A-Za-z0-9_][A-Za-z0-9_]", "bDd4_", "Test ^ the not operator.");
	RegexPattern("\\w*\\s[\\w]*\\s[\\w]*", "one two\tTHR33", "Test \\s whitespace operator.");
	RegexPattern("\\S*", "aw#$", "Test \\S non-whitespace operator.");
	RegexPattern("\\S*", "aw\n$", "Test \\S non-whitespace operator.");
	RegexPattern("\\d*\\D*", "72635JHWas", "Test \\d digit and \\D non-digit.");
	RegexPattern("\\d*", "7263A4190", "Test \\d digit.");
	RegexPattern("^Hello", "Hello", "Test ^ to match beginning of the string.");
	RegexPattern("^Hello ^World", "Hello World", "Test ^ to match beginning of the string.");
	RegexPattern("World$", "World", "Test $ to match end of the string.");
	RegexPattern("World$", "Hello World", "Test $ to match end of the string.");
	RegexPattern("\\a\\a\\a\\A\\A\\A", "abC123", "Test \\a to match letters and \\A to match non-letters.");
	RegexPattern("\\a\\a\\a\\A\\A\\A", "123abC", "Test \\a to match letters and \\A to match non-letters.");
}


tTestUnit(Script)
{
	if (!tDirExists("TestData/"))
		tSkipUnit(Script)

	{
		tExprWriter ws("TestData/WrittenConfig.cfg");
		ws.Rem("This is a test config file.");
		ws.CR();
		ws.Comp("PosX", 10);
		ws.Comp("PosY", 20);
		ws.Comp("SizeW", 30);
		ws.Comp("SizeH", 40);
		ws.Comp("FloatVal", 50.123456789f);
		ws.Comp("DoubleVal", 60.111122223333444455556666777788889999);
		ws.Comp("Vec3", tVector3(1.0f, 2.0f, 3.0f));

		tMatrix2 mat2x2(11,21,12,22);
		ws.Comp("Mat2x2", mat2x2);

		tMatrix4 mat4x4(11,21,31,41, 12,22,32,42, 13,23,33,43, 14,24,34,44);
		ws.Comp("Mat4x4", mat4x4);
	}

	{
		tExprReader rs("TestData/WrittenConfig.cfg");
		for (tExpression e = rs.First(); e.Valid(); e = e.Next())
		{
			tPrintf("ExpressionString: ___%s___\n", e.GetExpressionString().Pod());
			switch (e.Command().Hash())
			{
				case tHash::tHashCT("PosX"):
					tRequire(int(e.Item1()) == 10);
					break;

				case tHash::tHashCT("PosY"):
					tRequire(int(e.Item1()) == 20);
					break;

				case tHash::tHashCT("SizeW"):
					tRequire(int(e.Item1()) == 30);
					break;

				case tHash::tHashCT("SizeH"):
					tRequire(int(e.Item1()) == 40);
					break;

				case tHash::tHashCT("FloatVal"):
				{
					float readval = e.Item1();
					tPrintf("Read float as: %f\n", readval);
					tRequire(readval == 50.123456789f);
					break;
				}

				case tHash::tHashCT("DoubleVal"):
				{
					double readval = e.Item1();
					tPrintf("Read double as: %f\n", readval);
					tRequire(readval == 60.111122223333444455556666777788889999);
					break;
				}

				case tHash::tHashCT("Vec3"):
				{
					tVector3 readval = e.Item1();
					tPrintf("Read vec3 as: %v\n", readval);
					tRequire(readval == tVector3(1.0f, 2.0f, 3.0f));
					break;
				}

				case tHash::tHashCT("Mat2x2"):
				{
					tMatrix2 readval = e.Item1();
					tPrintf("Read mat2x2 as: %:4m\n", readval.Pod());
					tRequire(readval == tMatrix2(11,21,12,22));
					break;
				}

				case tHash::tHashCT("Mat4x4"):
				{
					tMatrix4 readval = e.Item1();
					tPrintf("Read mat4x4 as: %m\n", readval.Pod());
					tRequire(readval == tMatrix4(11,21,31,41, 12,22,32,42, 13,23,33,43, 14,24,34,44));
					break;
				}
			}
		}
	}

	{
		tExprWriter ws("TestData/WrittenScript.txt");

		ws.WriteComment();
		ws.WriteComment("A comment!!");
		ws.WriteComment();
		ws.NewLine();

		ws.BeginExpression();
		ws.WriteAtom("A");
		ws.BeginExpression();
		ws.WriteAtom("B");
		ws.WriteAtom("C");
		ws.EndExpression();
		ws.EndExpression();

		ws.NewLine();
		ws.BeginExpression();
		ws.Indent();
		ws.NewLine();
			ws.WriteAtom("A longer atom");
			ws.BeginExpression();
			ws.WriteAtom( tString("M") );
			ws.WriteAtom(-3.0f);
			ws.WriteAtom(300000000000000000.0f);
			ws.WriteAtom(-4);
			ws.WriteAtom(true);
			ws.EndExpression();
			ws.Dedent();
			ws.NewLine();
		ws.EndExpression();
	}

	tPrintf("Testing reading a script.\n");
	int numExceptions = 0;
	try
	{
		tExprReader rs("TestData/TestScript.txt");

		tExpression arg = rs.Arg0();			// [A [6.8 42 True]]

		tExpression cmd = arg.Command();		// A
		tString cmdstr = cmd.GetAtomString();
		tPrintf("The first command is %s\n", cmdstr.Pod());
		tExpression a = arg.Arg1();
		tExpression c = a.Command();
		tExpression d = a.Arg1();
		tExpression e = d.Next();
		tPrintf("c:%f d:%d e:%d\n", c.GetAtomFloat(), d.GetAtomInt(), e.GetAtomBool());

		tExpression arg2 = arg.Next();			// K
		tPrintf("Second main arg %s\n", arg2.GetAtomString().Pod());

		tExpression arg3 = arg2.Next();			// [d	e[ f g]]

		tExpression arg4 = arg3.Next();			// [[H I] "This is a bigger atom" ]
		tExpression cmd4 = arg4.Command();		// [H I]	
		tPrintf("Command4 is-atom: %d\n", cmd4.IsAtom());
		tRequire(!cmd4.IsAtom());

		tExpression arg5 = cmd4.Next();			// "This is a bigger atom"
		tExpression arg5to = arg4.Arg1();		// "This is a bigger atom"

		tPrintf("Last atom %s\n", arg5.GetAtomString().Pod());
		tPrintf("Last atomdup %s\n", arg5to.GetAtomString().Pod());
		tRequire(arg5.GetAtomString() == arg5to.GetAtomString());

		// Lets test variable number of args.
		tExpression argvar = arg4.Next();

		tExpression varcmd = argvar.Command();
		tExpr vararg = argvar.Arg1();
		while (vararg.IsValid())
		{
			tPrintf("Variable arg val :");
			if (vararg.IsAtom())
			{
				tPrintf("%d\n", vararg.GetAtomInt());
				tPrintf("Using implicit cast %d\n", int(vararg));
			}
			else
				tPrintf("Not Atom\n");
			vararg = vararg.Next();
		}

		tExpression quotetest = argvar.Next();
		tExpression quoted = quotetest.Command();
		tPrintf("Quoted atom:%s\n", quoted.GetAtomString().Pod());
		tRequire(quoted.GetAtomString() == "quoted");

		tExpression notquoted = quoted.Next();
		tPrintf("NotQuoted atom:%s\n", notquoted.GetAtomString().Pod());
		tRequire(notquoted.GetAtomString() == "notquoted");

		tExpression vectors = quotetest.Next();
		tVector2 v1 = vectors.Item0().GetAtomVector2();
		tVector2 v2 = vectors.Item1().GetAtomVector2();
		tPrintf("Vector1: (%f, %f)\n", v1.x, v1.y);
		tPrintf("Vector2: (%f, %f)\n", v2.x, v2.y);

		tVector3 v3 = vectors.Item2().GetAtomVector3();
		tVector3 v4 = vectors.Item3().GetAtomVector3();
		tPrintf("Vector3: (%f, %f, %f)\n", v3.x, v3.y, v3.z);
		tPrintf("Vector4: (%f, %f, %f)\n", v4.x, v4.y, v4.z);

		tVector4 v5 = vectors.Item4().GetAtomVector4();
		tVector4 v6 = vectors.Item5().GetAtomVector4();
		tPrintf("Vector5: (%f, %f, %f, %f)\n", v5.x, v5.y, v5.z, v5.w);
		tPrintf("Vector6: (%f, %f, %f, %f)\n", v6.x, v6.y, v6.z, v6.w);

		tExpression mat2x2expA = vectors.Next();
		tExpression mat2x2expB = mat2x2expA.Next();
		tExpression mat4x4expA = mat2x2expB.Next();

		tMatrix2 mat2x2A = mat2x2expA;
		tMatrix2 mat2x2B = mat2x2expB;
		tMatrix4 mat4x4A = mat4x4expA;
		tPrintf("Mat2x2A: %:4m\n", mat2x2A);
		tPrintf("Mat2x2B: %:4m\n", mat2x2B);
		tPrintf("Mat4x4A: %m\n", mat4x4A);

		// This should generate an exception. Need to test that too.
		arg3.GetAtomString();
	}
	catch (tScriptError error)
	{
		numExceptions++;
		tPrintf(error.Message.Chr());
		tPrintf("\n");
	}
	tRequire(numExceptions == 1);
}


tTestUnit(Chunk)
{
	if (!tDirExists("TestData/"))
		tSkipUnit(Chunk)

	tPrintf("Testing writing a chunk file.\n");
	{
		tChunkWriter c("TestData/WrittenChunk.bin");
		c.Begin(0x02424242, 64);
		c.Write(tString("Does this work?"));
		c.Write( int8(0x12) );
		c.Write( int16(0x1234) );
		c.Write( int32(0x12345678) );
		c.Write( int64(0x1234567812345678) );
		c.End();

		c.Begin(0x03434343, 32);
		c.Write(tString("Next chunk..."));
		c.Write( int8(0x12) );
		c.Write( int16(0x1234) );
		c.Write( int32(0x12345678) );
		c.Write( int64(0x1234567812345678) );
		c.End();
	}
	tRequire(tFileExists("TestData/WrittenChunk.bin"));

	tPrintf("Testing reading a chunk file.\n");
	{
		tChunkReader c("TestData/WrittenChunk.bin");
		tChunk ch = c.GetFirstChunk();
		while (ch.Valid())
		{
			tPrintf("Chunk ID %x\n", ch.ID());
			tRequire((ch.ID() == 0x02424242) || (ch.ID() == 0x03434343));
			tPrintf("Data: %s\n", ch.GetData());
			tRequire((tString((char*)ch.GetData()) == "Does this work?") || (tString((char*)ch.GetData()) == "Next chunk..."));
			ch = ch.GetNextChunk();
		}
	}

	tPrintf("Another way to read.\n");
	{
		tChunkReader c("TestData/WrittenChunk.bin");
		for (tChunk ch = c.GetFirstChunk(); ch != ch.GetLastChunk(); ch = ch.GetNextChunk())
		{
			tPrintf("Chunk ID %x\n", ch.ID());
			tPrintf("Data %s\n", ch.GetData());
		}
	}

	tPrintf("Reading but managing the memory myself.\n");
	{
		uint8* buffer = (uint8*)tMem::tMalloc(tChunkReader::GetBufferSizeNeeded("TestData/WrittenChunk.bin"), tChunkReader::GetBufferAlignmentNeeded());
		tChunkReader c("TestData/WrittenChunk.bin", buffer);

		for (tChunk ch = c.GetFirstChunk(); ch != ch.GetLastChunk(); ch = ch.GetNextChunk())
		{
			tPrintf("Chunk ID %x\n", ch.ID());
			tPrintf("Data %s\n", ch.GetData());
		}
		tMem::tFree(buffer);
	}
}


// Test global init of tFileTypes.
tSystem::tFileTypes FileTypesGlobal
(
	tFileType::APNG,
	tFileType::BMP,
	tFileType::JPG,
	tFileType::TIFF,
	tFileType::EOL
);


tTestUnit(FileTypes)
{
	tFileTypes fileTypes;
	fileTypes.
		Add(tFileType::JPG).
		Add(tFileType::PNG).
		Add(tFileType::EXR).
		Add(tFileType::TIFF).
		Add(tFileType::PNG);

	// Check for uniqueness.
	tRequire(fileTypes.Count() == 4);

	tExtensions extensions(fileTypes, false);

	// There should be 6 extensions. 2 for JPG, 2 for TIFF, 1 for PNG (it's unique) and 1 for EXR.
	tRequire(extensions.Count() == 6);

	tPrintf("Found extensions:\n");
	for (tStringItem* ext = extensions.First(); ext; ext = ext->Next())
		tPrintf("Extension: %s\n", ext->Chr());
	tPrintf("Found extensions done.\n");

	// Test copy cons.
	tFileTypes fileTypesCopy(fileTypes);
	tExtensions extensionsCopy(extensions);

	// Test implicit type conversion of string literal.
	tList<tStringItem> foundFiles;
	tSystem::tFindFiles(foundFiles, "TestData/", "bin");

	tExtensions extsAll(FileTypesGlobal, false);
	tPrintf("All extensions:\n");
	for (tStringItem* ext = extsAll.First(); ext; ext = ext->Next())
		tPrintf("Ext: %s\n", ext->Chr());

	tExtensions extsCom(FileTypesGlobal, true);
	tPrintf("\nCommon extensions:\n");
	for (tStringItem* ext = extsCom.First(); ext; ext = ext->Next())
		tPrintf("Ext: %s\n", ext->Chr());
	tPrintf("\n");

	// Test selection utilities.
	for (tFileTypes::tFileTypeItem* item = fileTypes.First(); item; item = item->Next())
		item->Selected = true;
	fileTypes.Add(tFileType::HDR);
	fileTypes.Add(tFileType::ICO);
	tRequire(fileTypes.AnySelected());

	tFileTypes selected;
	selected.AddSelected(fileTypes);
	tPrintf("Selected Types:\n");
	for (tFileTypes::tFileTypeItem* item = selected.First(); item; item = item->Next())
		tPrintf("SelectedType: %s\n", tGetFileTypeName(item->FileType).Chr());

	tPrintf("Selected String (comsp, nomax):[%s]\n", fileTypes.GetSelectedString().Chr());
	tPrintf("Selected String (comsp, max 3):[%s]\n", fileTypes.GetSelectedString(tFileTypes::Separator::CommaSpace, 3).Chr());
	tPrintf("Selected String (space, max 5):[%s]\n", fileTypes.GetSelectedString(tFileTypes::Separator::Space, 5).Chr());
	tPrintf("Selected String (comma, max 2):[%s]\n", fileTypes.GetSelectedString(tFileTypes::Separator::Comma, 2).Chr());

	selected.Clear();
	selected.AddSelected(FileTypesGlobal, true);
	tPrintf("Selected Types (Global All):\n");
	for (tFileTypes::tFileTypeItem* item = fileTypes.First(); item; item = item->Next())
		tPrintf("SelectedType: %s\n", tGetFileTypeName(item->FileType).Chr());
}


bool ListsContainSameItems(const tList<tStringItem>& a, const tList<tStringItem>& b)
{
	if (a.GetNumItems() != b.GetNumItems())
		return false;

	for (tStringItem* ia = a.First(); ia; ia = ia->Next())
	{
		if (!b.Contains(*ia))
			return false;
	}

	return true;
}


tTestUnit(Directories)
{
	tString homeDir = tGetHomeDir();
	tPrintf("Home Dir is: %s\n", homeDir.Chr());
	tRequire(!homeDir.IsEmpty());

	tString progDir = tGetProgramDir();
	tPrintf("Program Dir is: %s\n", progDir.Chr());
	tRequire(!progDir.IsEmpty());

	tString progPath = tGetProgramPath();
	tPrintf("Program Path is: %s\n", progPath.Chr());
	tRequire(!progPath.IsEmpty());

	tString currDir = tGetCurrentDir();
	tPrintf("Curr Dir is: %s\n", currDir.Chr());
	tRequire(!currDir.IsEmpty());

	#ifdef PLATFORM_WINDOWS
	tString winDir = tGetWindowsDir();
	tPrintf("Windows Dir is: %s\n", winDir.Chr());
	tRequire(!winDir.IsEmpty());

	tString sysDir = tGetSystemDir();
	tPrintf("System Dir is: %s\n", sysDir.Chr());
	tRequire(!sysDir.IsEmpty());

	tString deskDir = tGetDesktopDir();
	tPrintf("Desktop Dir is: %s\n", deskDir.Chr());
	tRequire(!deskDir.IsEmpty());
	#endif

	// Returns the relative location of path from basePath. Returns an empty string if it fails.
	tString basePath, fullPath, relPath;
	#ifdef PLATFORM_WINDOWS
	basePath = "C:/TopLeVel/";
	fullPath = "C:/TopLeveL/SubLevel/";
	#elif defined(PLATFORM_LINUX)
	basePath = "/TopLevel/";
	fullPath = "/TopLevel/SubLevel/";
	#endif
	relPath = tGetRelativePath(basePath, fullPath);
	tPrintf("Rel Path is: %s\n", relPath.Chr());
	tRequire((relPath == "SubLevel/") && (relPath.Length() == 9));

	#ifdef PLATFORM_WINDOWS
	basePath = "C:/TopLevel/a/b/";
	fullPath = "C:/TopLEVEL/x/y/z/";
	#elif defined(PLATFORM_LINUX)
	basePath = "/TopLevel/a/b/";
	fullPath = "/TopLevel/x/y/z/";
	#endif
	relPath = tGetRelativePath(basePath, fullPath);
	tPrintf("Rel Path is: %s\n", relPath.Chr());
	tRequire((relPath == "../../x/y/z/") && (relPath.Length() == 12));
}


tTestUnit(File)
{
	if (!tDirExists("TestData/"))
		tSkipUnit(File)

	tRequire(!tFileExists("TestData/ProbablyDoesntExist.txt"));

	#ifdef PLATFORM_WINDOWS
	tSetHidden("TestData/.HiddenFile.txt");
	#endif

	// This file is now hidden in both Linux and Windows.
	tRequire(tIsHidden("TestData/.HiddenFile.txt"));

	tList<tFileInfo> dirs;
	tPrintf("tFindDirs Backend::Stndrd\n");
	tFindDirs(dirs, "TestData/", true, tSystem::Backend::Stndrd);
	for (tFileInfo* i = dirs.First(); i; i = i->Next())
	{
		std::tm localTime = tConvertTimeToLocal(i->ModificationTime);
		tString timestr = tSystem::tConvertTimeToString(localTime);

		tPrintf("Dir: %s LastModTime: %s\n", i->FileName.Chr(), timestr.Chr());
		tPrintf("Dir: %s Hidden: %s\n", i->FileName.Chr(), i->Hidden ? "true" : "false");
	}

	dirs.Empty();
	tPrintf("tFindDirs Backend::Native\n");
	tFindDirs(dirs, "TestData/", true, tSystem::Backend::Native);
	for (tFileInfo* i = dirs.First(); i; i = i->Next())
	{
		std::tm localTime = tConvertTimeToLocal(i->ModificationTime);
		tString timestr = tSystem::tConvertTimeToString(localTime);

		tPrintf("Dir: %s LastModTime: %s\n", i->FileName.Chr(), timestr.Chr());
		tPrintf("Dir: %s Hidden: %s\n", i->FileName.Chr(), i->Hidden ? "true" : "false");
	}

	tList<tStringItem> filesStd;
	tFindFiles(filesStd, "TestData/", false, Backend::Stndrd);
	for (tStringItem* file = filesStd.Head(); file; file = file->Next())
		tPrintf("Found file standard: %s\n", file->Text());

	tList<tStringItem> filesNat;
	tFindFiles(filesNat, "TestData/", false, Backend::Native);
	for (tStringItem* file = filesNat.Head(); file; file = file->Next())
		tPrintf("Found file native: %s\n", file->Text());

	tRequire(ListsContainSameItems(filesStd, filesNat));

	tExtensions extensions;
	extensions.Add(tFileType::TIFF).Add(tFileType::HDR);
	for (tStringItem* ext = extensions.First(); ext; ext = ext->Next())
		tPrintf("TIFF or HDR extension: %s\n", ext->Text());
	tRequire(extensions.Count() == 4);

	extensions.Clear();
	extensions.Add("bmp").Add("txT");
	extensions.Add("ZZZ");

	tList<tStringItem> filesMultStd;
	tFindFiles(filesMultStd, "TestData/", extensions, false, Backend::Stndrd);
	for (tStringItem* file = filesMultStd.Head(); file; file = file->Next())
		tPrintf("Found file standard (bmp, txt, zzz): %s\n", file->Text());

	tList<tStringItem> filesMultNat;
	tFindFiles(filesMultNat, "TestData/", extensions, false, Backend::Native);
	for (tStringItem* file = filesMultNat.Head(); file; file = file->Next())
		tPrintf("Found file native (bmp, txt, zzz): %s\n", file->Text());

	tRequire(ListsContainSameItems(filesMultStd, filesMultNat));

	tString testWinPath = "c:/ADir/file.txt";
	tRequire(tGetDir(testWinPath) == "c:/ADir/");

	tString testLinPath = "/ADir/file.txt";
	tRequire(tGetDir(testLinPath) == "/ADir/");
	
	tList<tStringItem> subDirs;
	tFindDirs(subDirs, "TestData/", true);
	for (tStringItem* subd = subDirs.Head(); subd; subd = subd->Next())
		tPrintf("SubDir: %s\n", subd->Text());

	// Create a directory. Create a file in it. Then delete the directory with the file in it.
	tCreateDir("TestData/CreatedDirectory/");
	tRequire(tDirExists("TestData/CreatedDirectory/"));
	tRequire(!tIsReadOnly("TestData/CreatedDirectory/"));

	tCreateFile("TestData/CreatedDirectory/CreatedFile.txt", "File Contents");
	tRequire(tFileExists("TestData/CreatedDirectory/CreatedFile.txt"));

	tDeleteDir("TestData/CreatedDirectory/");
	tRequire(!tDirExists("TestData/CreatedDirectory/"));

	// Create multiple directories in one go.
	tCreateDirs("TestData/CreatedA/CreatedB/CreatedC/");
	tRequire(tDirExists("TestData/CreatedA/CreatedB/CreatedC/"));

	tDeleteDir("TestData/CreatedA/");
	tRequire(!tDirExists("TestData/CreatedA/"));

	tString normalPath = "Q:/Projects/Calamity/Crypto/../../Reign/./Squiggle/";
	tPrintf("Testing GetSimplifiedPath on '%s'\n", normalPath.Pod());
	tString simpPath = tGetSimplifiedPath(normalPath);
	tPrintf("Simplified Path '%s'\n", simpPath.Pod());
	tRequire(simpPath == "Q:/Projects/Reign/Squiggle/");

	normalPath = "E:\\Projects\\Calamity\\Crypto";
	simpPath = tGetSimplifiedPath(normalPath, true);
	tRequire(simpPath == "E:/Projects/Calamity/Crypto/");

	normalPath = "E:\\Projects\\Calamity\\..\\Crypto.txt";
	simpPath = tGetSimplifiedPath(normalPath);
	tRequire(simpPath == "E:/Projects/Crypto.txt");

	normalPath = R"(\\MachineName\ShareName\Projects\Calamity\..\Crypto.txt)";
	simpPath = tGetSimplifiedPath(normalPath);
	tRequire(simpPath == "\\\\MachineName\\ShareName/Projects/Crypto.txt");

	normalPath = "/";
	simpPath = tGetSimplifiedPath(normalPath);
	tRequire(simpPath == "/");

	normalPath = "/";
	simpPath = tGetUpDir(normalPath);
	tRequire(simpPath == "/");

	normalPath = R"(\\machine\share/dir/subdir/file.txt)";
	simpPath = tGetDir(normalPath);
	tRequire(simpPath == "\\\\machine\\share/dir/subdir/");

	// Test some invalid ones.
	normalPath = "/Dir/../..";
	simpPath = tGetSimplifiedPath(normalPath);
	tRequire(simpPath == "/");

	normalPath = "z:/Dir/../..";
	simpPath = tGetSimplifiedPath(normalPath);
	tRequire(simpPath == "Z:/");
}


tTestUnit(FindRec)
{
	if (!tDirExists("TestData/"))
		tSkipUnit(File)

	#ifdef PLATFORM_WINDOWS
	tSetHidden("TestData/.HiddenFile.txt");
	#endif

	// This file is now hidden in both Linux and Windows.
	tRequire(tIsHidden("TestData/.HiddenFile.txt"));

	tList<tStringItem> filesStd;
	tList<tStringItem> filesNat;
	tList<tStringItem> dirsStd;
	tList<tStringItem> dirsNat;
	tList<tFileInfo> infosStd;
	tList<tFileInfo> infosNat;

	// Note the ordering of the results varies between native and standard backends. This is fine,
	// as order is not guaranteed. The below tests find files recursively.
	filesStd.Empty();
	tPrintf("\nRecursive Find Files. Incl Hidden. All Extensions. Standard Backend.\n");
	tFindFilesRec(filesStd, "TestData/", true, Backend::Stndrd);
	for (tStringItem* file = filesStd.Head(); file; file = file->Next())
		tPrintf("Found File: %s\n", file->Text());
	filesNat.Empty();
	tPrintf("\nRecursive Find Files. Incl Hidden. All Extensions. Native Backend.\n");
	tFindFilesRec(filesNat, "TestData/", true, Backend::Native);
	for (tStringItem* file = filesNat.Head(); file; file = file->Next())
		tPrintf("Found File: %s\n", file->Text());
	tRequire(filesStd.NumItems() == filesNat.NumItems());

	filesStd.Empty();
	tPrintf("\nRecursive Find Files. Incl Hidden. TGA Extensions. Standard Backend.\n");
	tFindFilesRec(filesStd, "TestData/", "tga", true, Backend::Stndrd);
	for (tStringItem* file = filesStd.Head(); file; file = file->Next())
		tPrintf("Found File: %s\n", file->Text());
	filesNat.Empty();
	tPrintf("\nRecursive Find Files. Incl Hidden. TGA Extensions. Native Backend.\n");
	tFindFilesRec(filesNat, "TestData/", "tga", true, Backend::Native);
	for (tStringItem* file = filesNat.Head(); file; file = file->Next())
		tPrintf("Found File: %s\n", file->Text());
	tRequire(filesStd.NumItems() == filesNat.NumItems());

	infosStd.Empty();
	tPrintf("\nRecursive Find Files (FileInfo). Excl Hidden. TGA and JPG Extensions. Standard Backend.\n");
	tExtensions exts( tFileTypes(tFileType::TGA, tFileType::JPG, tFileType::EOL) );
	tFindFilesRec(infosStd, "TestData/", exts, false, Backend::Stndrd);
	for (tFileInfo* info = infosStd.Head(); info; info = info->Next())
		tPrintf("Found File info: %s\n", info->FileName.Chr());
	infosNat.Empty();
	tPrintf("\nRecursive Find Files (FileInfo). Excl Hidden. TGA and JPG Extensions. Native Backend.\n");
	tFindFilesRec(infosNat, "TestData/", exts, false, Backend::Native);
	for (tFileInfo* info = infosNat.Head(); info; info = info->Next())
		tPrintf("Found File info: %s\n", info->FileName.Chr());
	tRequire(infosStd.NumItems() == infosNat.NumItems());

	// Below are tests for finding dirs.
	dirsStd.Empty();
	tPrintf("\nRecursive Find Dirs. Incl Hidden. Standard Backend.\n");
	tFindDirsRec(dirsStd, "TestData/", true, Backend::Stndrd);
	for (tStringItem* dir = dirsStd.Head(); dir; dir = dir->Next())
		tPrintf("Found Dir: %s\n", dir->Text());
	dirsNat.Empty();
	tPrintf("\nRecursive Find Dirs. Incl Hidden. Native Backend.\n");
	tFindDirsRec(dirsNat, "TestData/", true, Backend::Native);
	for (tStringItem* dir = dirsNat.Head(); dir; dir = dir->Next())
		tPrintf("Found Dir: %s\n", dir->Text());
	tRequire(dirsStd.NumItems() == dirsNat.NumItems());

	infosStd.Empty();
	tPrintf("\nRecursive Find Dirs (FileInfo). Excl Hidden. Standard Backend.\n");
	tFindDirsRec(infosStd, "TestData/", false, Backend::Stndrd);
	for (tFileInfo* info = infosStd.Head(); info; info = info->Next())
		tPrintf("Found Dir: %s\n", info->FileName.Chr());
	infosNat.Empty();
	tPrintf("\nRecursive Find Dirs (FileInfo). Excl Hidden. Native Backend.\n");
	tFindDirsRec(infosNat, "TestData/", false, Backend::Native);
	for (tFileInfo* info = infosNat.Head(); info; info = info->Next())
		tPrintf("Found Dir: %s\n", info->FileName.Chr());
	tRequire(infosStd.NumItems() == infosNat.NumItems());
}


#if defined(PLATFORM_WINDOWS)
tNetworkShareResult NetworkShareResult;
void GetNetworkSharesThreadEntry()
{
	int numSharesFromThread = tGetNetworkShares(NetworkShareResult);
	tPrintf("Thread returned %d shares\n", numSharesFromThread);
}
#endif


tTestUnit(Network)
{
	#if defined(PLATFORM_WINDOWS)

	// Getting network shares is a slow blocking call. We will offload the work
	// to a thread and print results as we get them.
	tPrintf("Offloading network share retrieval to new thread.\n");
	std::thread threadGetShares(GetNetworkSharesThreadEntry);

	// Mimic an update loop like there would be in an ImGui app or game.
	while (!NetworkShareResult.RequestComplete || !NetworkShareResult.ShareNames.IsEmpty())
	{
		tStringItem* share = NetworkShareResult.ShareNames.Remove();
		if (share)
		{
			tPrintf("Network Share: [%s] Exploded: ", share->Text());
			tList<tStringItem> exploded;
			tExplodeShareName(exploded, *share);
			for (tStringItem* exp = exploded.First(); exp; exp = exp->Next())
				tPrintf("[%s] ", exp->Text());
			tPrintf("\n");
			delete share;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	threadGetShares.join();
	tRequire(NetworkShareResult.RequestComplete);
	NetworkShareResult.Clear();

	#endif
}


tTestUnit(Time)
{
	uint64 epochTime = tGetTimeUTC();	//	h			d		y
	uint64 yearsSince1970 = epochTime / (	3600ULL *	24ULL *	365ULL);
	tPrintf("Years since 1970 UTC: %016|64d\n", yearsSince1970);

	// I wrote the following assert in 2020. Unless time moves backwards we should be good.
	tRequire(yearsSince1970 >= 50);

	float startTimeSeconds = tGetTime();
	double startTimeSecondsD = tGetTimeDouble();
	tPrintf("Start time seconds: %f %f\n", startTimeSeconds, startTimeSecondsD);
	tSleep(1000);
	float endTimeSeconds = tGetTime();
	double endTimeSecondsD = tGetTimeDouble();
	tPrintf("End time seconds: %f %f\n", endTimeSeconds, endTimeSecondsD);
	tPrintf("Elapsed time: %f %f\n", endTimeSeconds - startTimeSeconds, endTimeSecondsD - startTimeSecondsD);
	tRequire((endTimeSeconds - startTimeSeconds) > 0.99f);
	tRequire((endTimeSecondsD - startTimeSecondsD) > 0.99);

	tTimer timer;
	tPrintf("Timer running: %s\n", timer.IsRunning() ? "true" : "false");

	for (int c = 0; c < 100; c++)
		timer.Update(1.0f);

	tPrintf("100 seconds later.\n");
	tPrintf("Time (seconds)  : %f\n", timer.GetTime());
	tPrintf("Time (seconds)  : %f\n", timer.GetTime(tUnit::tTime::Second));
	tRequire(tApproxEqual(timer.GetTime(), 100.0f));

	timer.Stop();
	tPrintf("Timer running: %s\n", timer.IsRunning() ? "true" : "false");
	for (int c = 0; c < 100; c++)
		timer.Update(1.0f);

	tPrintf("These 100 seconds the timer was stopped.\n");
	tPrintf("Time (seconds)   : %f\n", timer.GetTime());
	tPrintf("Time (seconds)   : %f\n", timer.GetTime(tUnit::tTime::Second));
	tRequire(tApproxEqual(timer.GetTime(tUnit::tTime::Second), 100.0f));

	tPrintf("Time (minutes)   : %f\n", timer.GetTime(tUnit::tTime::Minute));
	tRequire(tApproxEqual(timer.GetTime(tUnit::tTime::Minute), 1.666666f));

	tPrintf("Time (millisecs) : %f\n", timer.GetTime(tUnit::tTime::Millisecond));
	tRequire(tApproxEqual(timer.GetTime(tUnit::tTime::Millisecond), 100000.0f));

	tPrintf("Time (microsecs) : %f\n", timer.GetTime(tUnit::tTime::Microsecond));
	tRequire(tApproxEqual(timer.GetTime(tUnit::tTime::Microsecond), 100000000.0f));

	tPrintf("Time (heleks)    : %f\n", timer.GetTime(tUnit::tTime::Helek));
	tRequire(tApproxEqual(timer.GetTime(tUnit::tTime::Helek), 30.0f));

	// Test conversions to strings in various formats.
	std::tm localTime = tGetTimeLocal();

	tString timeStandardStr = tConvertTimeToString(localTime, tTimeFormat::Standard);
	tPrintf("Local Time Standard Format: %s\n", timeStandardStr.Chr());

	tString timeExtendedStr = tConvertTimeToString(localTime, tTimeFormat::Extended);
	tPrintf("Local Time Extended Format: %s\n", timeExtendedStr.Chr());

	tString timeShortStr    = tConvertTimeToString(localTime, tTimeFormat::Short);
	tPrintf("Local Time    Short Format: %s\n", timeShortStr.Chr());

	tString timeFilenameStr = tConvertTimeToString(localTime, tTimeFormat::Filename);
	tPrintf("Local Time Filename Format: %s\n", timeFilenameStr.Chr());
}


tTestUnit(Machine)
{
	tString compName = tSystem::tGetComputerName();
	tPrintf("ComputerName:%s\n", compName.Chr());
	tRequire(!compName.IsEmpty());

	bool supportsSSE = tSystem::tSupportsSSE();
	bool supportsSSE2 = tSystem::tSupportsSSE2();
	tPrintf("CPU Support. SSE:%s SSE2:%s\n", supportsSSE ? "True" : "False", supportsSSE2 ? "True" : "False");

	int numCores = tSystem::tGetNumCores();
	tPrintf("Num Cores:%d\n", numCores);
	tRequire(numCores >= 1);

	tString pathEnvVar = tSystem::tGetEnvVar("PATH");
	tPrintf("PATH Env Var:%s\n", pathEnvVar.Chr());

	#ifdef PLATFORM_LINUX
	tPrintf("Testing XDG Base Directories\n");

	tString dataHome;
	bool dataHomeSet = tSystem::tGetXDGDataHome(dataHome);
	tPrintf("XDGDataHome Set:%'B Dir:%s\n", dataHomeSet, dataHome.Chr());
	tRequire(tIsAbsolutePath(dataHome));

	tString configHome;
	bool configHomeSet = tSystem::tGetXDGConfigHome(configHome);
	tPrintf("XDGConfigHome Set:%'B Dir:%s\n", configHomeSet, configHome.Chr());
	tRequire(tIsAbsolutePath(configHome));

	tString stateHome;
	bool stateHomeSet = tSystem::tGetXDGStateHome(stateHome);
	tPrintf("XDGStateHome Set:%'B Dir:%s\n", stateHomeSet, stateHome.Chr());
	tRequire(tIsAbsolutePath(stateHome));

	tString exeHome;
	tSystem::tGetXDGExeHome(exeHome);
	tPrintf("XDGExeHome Dir:%s\n", exeHome.Chr());
	tRequire(tIsAbsolutePath(exeHome));

	tList<tStringItem> dataDirs;
	bool dataDirsSet = tSystem::tGetXDGDataDirs(dataDirs);
	tPrintf("XDGDataDirs Set:%'B\n", dataDirsSet);
	for (tStringItem* dir = dataDirs.First(); dir; dir = dir->Next())
		tPrintf("   Dir:%s\n", dir->Chr());
	tRequire(!dataDirs.IsEmpty());

	tList<tStringItem> configDirs;
	bool configDirsSet = tSystem::tGetXDGConfigDirs(configDirs);
	tPrintf("XDGConfigDirs Set:%'B\n", configDirsSet);
	for (tStringItem* dir = configDirs.First(); dir; dir = dir->Next())
		tPrintf("   Dir:%s\n", dir->Chr());
	tRequire(!configDirs.IsEmpty());

	tString cacheHome;
	bool cacheHomeSet = tSystem::tGetXDGCacheHome(cacheHome);
	tPrintf("XDGCacheHome Set:%'B Dir:%s\n", cacheHomeSet, cacheHome.Chr());
	tRequire(tIsAbsolutePath(cacheHome));

	tString runtimeDir;
	bool runtimeDirSet = tSystem::tGetXDGRuntimeDir(runtimeDir);
	tPrintf("XDGRuntimeDir Set:%'B Dir:%s\n", runtimeDirSet, runtimeDir.Chr());
	tRequire(tIsAbsolutePath(runtimeDir) || runtimeDir.IsEmpty());
	#endif
}


}

