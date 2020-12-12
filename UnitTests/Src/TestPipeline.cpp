// TestPipeline.cpp
//
// Pipeline module tests.
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

#include <System/tFile.h>
#include <System/tTime.h>
#include <Pipeline/tProcess.h>
#include <Pipeline/tRule.h>
#include "UnitTests.h"
using namespace tPipeline;
namespace tUnitTest
{


struct TestRule : public tPipeline::tRule
{
	TestRule(int v) : SubVal(v) { }
	int SubVal = 2;
};


tTestUnit(Process)
{
	if (!tSystem::tDirExists("TestData/"))
		tSkipUnit(Process)

	// Currenty tProcess only works on windows.
	#ifdef PLATFORM_WINDOWS
	ulong exitCode;
	tString output;

	try
	{
		// This constructor blocks. It fills in the exitCode if you supply it. Output is appended to the output string.
		tProcess("cmd.exe dir", "TestData/", output, &exitCode);
		tPrintf("Output:\n[\n%s\n]\n", output.Pod());
	}
	catch (tError error)
	{
		tPrintf("%s\n", error.Message.Pod());
	}
	tRequire(exitCode == 0);

	try
	{
		tProcess("cmd.exe dir", "TestData/DoesNotExist/", output, &exitCode);
		tPrintf("Output:\n[\n%s\n]\n", output.Pod());
	}
	catch (tError error)
	{
		tPrintf("%s\n", error.Message.Pod());
		tPrintf("We expect an error here since an invalid directory was passed on purpose.\n");
	}
	tRequire(exitCode != 0);
	#endif
}


tTestUnit(Rule)
{
	tItList<TestRule> rules;
	rules.Append(new TestRule(2));
	rules.Append(new TestRule(4));

	for (auto rule : rules)
		tPrintf("RuleSubVal: %d\n", rule.SubVal);

	for (tItList<TestRule>::Iter rule = rules.First(); rule; ++rule)
		tPrintf("RuleSubVal: %d\n", rule->SubVal);
	
	auto rule = rules.First();
	while (rule)
	{
		tPrintf("RuleSubVal: %d\n", rule->SubVal);
		rule++;
	}

	tItList<TestRule> localRules(tListMode::UserOwns);
	TestRule tr(12);
	localRules.Append(&tr);
	tSystem::tCreateFile("TestData/WrittenOlderFile.txt", "This is the older file contents.");
	tSystem::tSleep(2000);
	tSystem::tCreateFile("TestData/WrittenNewerFile.txt", "This is the newer file contents.");
	tr.SetTarget("WrittenOlderFile.txt");
	localRules.Head()->AddDep("TestData/WrittenNewerFile.txt");
	tRequire(tr.OutOfDate());
}


}
