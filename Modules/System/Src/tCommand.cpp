// tCommand.cpp
//
// Parses a command line. A description of how to use the parser is in the header. Internally the first step is the
// expansion of combined single hyphen options. Next the parameters and options are parsed out. For each registered
// tOption and tParam object, its members are set to reflect the current command line when the tParse call is made.
// You may have more than one tOption that responds to the same option name. You may have more than one tParam that
// responds to the same parameter number.
//
// Copyright (c) 2017, 2020 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tFundamentals.h>
#include "System/tCommand.h"
#include "System/tFile.h"


namespace tCommand
{
	// Any single-hyphen combined arguments are expanded here. Ex. -abc becomes -a -b -c.
	void ExpandArgs(tList<tStringItem>& args);
	int IndentSpaces(int numSpaces);

	// I'm relying on zero initialization here. It's all zeroes before any items are constructed.
	tList<tParam> Params(tListMode::StaticZero);
	tList<tOption> Options(tListMode::StaticZero);
	tString Program;
	tString Empty;
}


tString tCommand::tGetProgram()
{
	return Program;
}


tCommand::tParam::tParam(int paramNumber, const char* name, const char* description) :
	ParamNumber(paramNumber),
	Param(),
	Name(),
	Description()
{
	if (name)
		Name = tString(name);
	else
		tsPrintf(Name, "Param%d", paramNumber);

	if (description)
		Description = tString(description);

	Params.Append(this);
}


tCommand::tOption::tOption(const char* description, char shortName, const char* longName, int numArgs) :
	ShortName(shortName),
	LongName(longName),
	Description(description),
	NumFlagArgs(numArgs),
	Args(tListMode::ListOwns),
	Present(false)
{
	Options.Append(this);
}


tCommand::tOption::tOption(const char* description, const char* longName, char shortName, int numArgs) :
	ShortName(shortName),
	LongName(longName),
	Description(description),
	NumFlagArgs(numArgs),
	Args(tListMode::ListOwns),
	Present(false)
{
	Options.Append(this);
}


tCommand::tOption::tOption(const char* description, char shortName, int numArgs) :
	ShortName(shortName),
	LongName(),
	Description(description),
	NumFlagArgs(numArgs),
	Args(tListMode::ListOwns),
	Present(false)
{
	Options.Append(this);
}


tCommand::tOption::tOption(const char* description, const char* longName, int numArgs) :
	ShortName(),
	LongName(longName),
	Description(description),
	NumFlagArgs(numArgs),
	Args(tListMode::ListOwns),
	Present(false)
{
	Options.Append(this);
}


int tCommand::IndentSpaces(int numSpaces)
{
	for (int s = 0; s < numSpaces; s++)
		tPrintf(" ");

	return numSpaces;
}


const tString& tCommand::tOption::ArgN(int n) const
{
	for (tStringItem* arg = Args.First(); arg; arg = arg->Next(), n--)
		if (n <= 1)
			return *arg;

	return Empty;
}


void tCommand::tParse(int argc, char** argv)
{
	if (argc <= 0)
		return;

	// Create a single line string of all the separate argv's. Arguments with quotes and spaces will come in as
	// distinct argv's, but they all get combined here. I don't believe two consecutive spaces will work.
	tString line;
	for (int a = 0; a < argc; a++)
	{
		char* arg = argv[a];
		if (!arg || (tStd::tStrlen(arg) == 0))
			continue;

		// Arg may have spaces within it. Such arguments need to be enclosed in quotes.
		tString argStr(arg);
		if (argStr.FindChar(' ') != -1)
			argStr = tString("\"") + argStr + tString("\"");

		line += argStr;
		if (a < (argc - 1))
			line += " ";
	}

	tParse(line, true);
}


void tCommand::ExpandArgs(tList<tStringItem>& args)
{
	tList<tStringItem> expArgs(tListMode::ListOwns);
	while (tStringItem* arg = args.Remove())
	{
		if ((arg->Length() < 2) || ((*arg)[0] != '-') || (((*arg)[0] == '-') && ((*arg)[1] == '-')))
		{
			expArgs.Append(arg);
			continue;
		}
		// It's now a single hyphen with something after it.

		bool recognized = false;
		for (tOption* option = Options.First(); option; option = option->Next())
		{
			if ( option->ShortName == tString((*arg)[1]) )
			{
				recognized = true;
				break;
			}
		}

		// Unrecognized options are left unmodified. Means you can put -10 and have it treated as a parameter.
		// as long as you don't have an option with shortname "1".
		if (!recognized)
		{
			expArgs.Append(arg);
			continue;
		}

		// By now it's a single hyphen and is expandble. eg. -abc -> -a -b -c
		for (int flag = 1; flag < arg->Length(); flag++)
		{
			tString newArg = "-" + tString((*arg)[flag]);
			expArgs.Append(new tStringItem(newArg));
		}

		delete arg;
	}

	// Repopulate args.
	while (tStringItem* arg = expArgs.Remove())
		args.Append(arg);
}


static bool ParamSortFn(const tCommand::tParam& a, const tCommand::tParam& b)
{
	return (a.ParamNumber < b.ParamNumber);
}


static bool OptionSortFnShort(const tCommand::tOption& a, const tCommand::tOption& b)
{
	return tStd::tStrcmp(a.ShortName.Pod(), b.ShortName.Pod()) < 0;
}


static bool OptionSortFnLong(const tCommand::tOption& a, const tCommand::tOption& b)
{
	return tStd::tStrcmp(a.LongName.Pod(), b.LongName.Pod()) < 0;
}


void tCommand::tParse(const char* commandLine, bool fullCommandLine)
{
	// At this point the constructors for all tOptions and tParams will have been called and both Params and Options
	// lists are populated. Options can be specified in any order, but we're going to order them alphabetically by short
	// flag name so they get printed nicely by tPrintUsage. Params must be printed in order based on their param num
	// so we'll do that sort here too.
	Params.Sort(ParamSortFn);
	Options.Sort(OptionSortFnShort);
	Options.Sort(OptionSortFnLong);

	tString line(commandLine);

	// Mark both kinds of escaped quotes that may be present. These may be found when the caller
	// wants a quote inside a string on the command line.
	line.Replace("\\'", tStd::SeparatorAStr);
	line.Replace("\\\"", tStd::SeparatorBStr);

	// Mark the spaces and hyphens inside normal (non escaped) quotes.
	bool inside = false;
	for (char* ch = line.Text(); *ch; ch++)
	{
		if ((*ch == '\'') || (*ch == '\"'))
			inside = !inside;

		if (!inside)
			continue;

		if (*ch == ' ')
			*ch = tStd::SeparatorC;

		if (*ch == '-')
			*ch = tStd::SeparatorD;
	}

	line.Remove('\'');
	line.Remove('\"');

	tList<tStringItem> args(tListMode::ListOwns);
	tStd::tExplode(args, line, ' ');

	// Now that the arguments are exploded into separate elements we replace the separators with the correct characters.
	for (tStringItem* arg = args.First(); arg; arg = arg->Next())
	{
		arg->Replace(tStd::SeparatorA, '\'');
		arg->Replace(tStd::SeparatorB, '\"');
		arg->Replace(tStd::SeparatorC, ' ');
	}

	// Set the program name as typed in the command line.
	if (fullCommandLine)
	{
		tStringItem* prog = args.Remove();
		Program.Set(prog->ConstText());
		delete prog;
	}
	else
	{
		Program.Clear();
	}

	ExpandArgs(args);

	// Process all options.
	for (tStringItem* arg = args.First(); arg; arg = arg->Next())
	{
		for (tOption* option = Options.First(); option; option = option->Next())
		{
			if ( (*arg == tString("--") + option->LongName) || (*arg == tString("-") + option->ShortName) )
			{
				option->Present = true;
				for (int optArgNum = 0; optArgNum < option->NumFlagArgs; optArgNum++)
				{
					arg = arg->Next();
					tStringItem* argItem = new tStringItem(*arg);
					argItem->Replace(tStd::SeparatorD, '-');
					option->Args.Append(argItem);
				}
			}
		}
	}

	// Now we're going to create a list of just the parameters by skipping any options as we encounter them.
	// For any option that we know about we'll also have to skip its option arguments.
	tList<tStringItem> commandLineParams(tListMode::ListOwns);
	for (tStringItem* arg = args.First(); arg; arg = arg->Next())
	{
		tStringItem* candidate = arg;

		// This loop skips any options for the current arg.
		for (tOption* option = Options.First(); option; option = option->Next())
		{
			if (*(arg->Text()) == '-')
			{
				tString flagArg = *arg;

				// We only skip flags we recognize.
				if ( (flagArg == tString("--") + option->LongName) || (flagArg == tString("-") + option->ShortName) )
				{
					candidate = nullptr;
					for (int optArgNum = 0; optArgNum < option->NumFlagArgs; optArgNum++)
						arg = arg->Next();
				}
			}
		}

		if (candidate)
			commandLineParams.Append(new tStringItem(*candidate));
	}

	// Process all parameters.
	int paramNumber = 1;
	for (tStringItem* arg = commandLineParams.First(); arg; arg = arg->Next(), paramNumber++)
	{
		arg->Replace(tStd::SeparatorD, '-');
		for (tParam* param = Params.First(); param; param = param->Next())
		{
			if (param->ParamNumber == paramNumber)
				param->Param = *arg;
		}
	}
}


void tCommand::tPrintSyntax()
{
	tString syntax =
R"U5AG3(Syntax Help:
tool.exe [arguments]

Arguments are separated by spaces. An argument must be enclosed in quotes
(single or double) if it has a space or hyphen in it. Use escape sequences to
put either type of quote inside. If you need to specify paths, it is suggested
to use forward slashes, although backslashes will work so long as the filename
does not have a single or double quote next.

An argument may be an 'option' or a 'parameter'.

Options:
An option has a short syntax and a long syntax. Short syntax is a - followed by
a single non-hyphen character. The long form is -- followed by a word. All
options support either long, short, or both forms. Options may have 0 or more
arguments. If an option takes zero arguments it is called a flag and you can
only test for its presence or lack of. Options can be specified in any order.
Short form options may be combined: Eg. -al expands to -a -l

Parameters:
A parameter is simply an argument that does not start with a - or --. It can be
read as a string and parsed arbitrarily (converted to an integer or float etc.)
Order is important when specifying parameters.

Example:
mycopy.exe -R --overwrite fileA.txt -pat fileB.txt --log log.txt

The fileA.txt and fileB.txt in the above example are parameters (assuming
the overwrite option is a flag). fileA.txt is the first parameter and
fileB.txt is the second.

The '--log log.txt' is an option with a single argument, log.txt. Flags may be
combined. The -pat in the example expands to -p -a -t. It is suggested only to
combine flag options as only the last option would get any arguments.

If you wish to interpret a hyphen directly instead of as an option specifier
this will happen automatically if there are no options matching what comes
after the hyphen. Eg. 'tool.exe -.85 --add 33 -87.98 -notpresent' works just
fine as long as there are no options that have a short form with digits or a
decimal. In this example the -.85 will be the first parameter, --notpresent
will be the second, and the --add takes in two number arguments.

Variable argument options are not supported due to the extra syntax that would
be needed. The same result is achieved by entering the same option more than
once. Eg. tool.exe -I /patha/include/ -I /pathb/include

)U5AG3";

	tPrintf("%s", syntax.Pod());
}


void tCommand::tPrintUsage(int versionMajor, int versionMinor, int revision)
{
	tPrintUsage(nullptr, versionMajor, versionMinor, revision);
}


void tCommand::tPrintUsage(const char* author, int versionMajor, int versionMinor, int revision)
{
	tPrintUsage(author, nullptr, versionMajor, versionMinor, revision);
}


void tCommand::tPrintUsage(const char* author, const char* desc, int versionMajor, int versionMinor, int revision)
{
	tAssert(versionMajor >= 0);
	tAssert((versionMinor >= 0) || (revision < 0));		// Not allowed a valid revision number if minor is not also valid.

	char verAuth[128];
	char* va = verAuth;
	va += tsPrintf(va, "Version %d", versionMajor);
	if (versionMinor >= 0)
	{
		va += tsPrintf(va, ".%d", versionMinor);
		if (revision >= 0)
			va += tsPrintf(va, ".%d", revision);
	}

	if (author)
		va += tsPrintf(va, " by %s", author);

	tPrintUsage(verAuth, desc);
}


void tCommand::tPrintUsage(const char* versionAuthorString, const char* desc)
{
	tString exeName = "Program.exe";
	if (!tCommand::Program.IsEmpty())
		exeName = tSystem::tGetFileName(tCommand::Program);

	if (versionAuthorString)
		tPrintf("%s %s\n\n", tPod(tSystem::tGetFileBaseName(exeName)), versionAuthorString);

	if (Options.IsEmpty())
		tPrintf("USAGE: %s ", exeName.Pod());
	else
		tPrintf("USAGE: %s [options] ", exeName.Pod());

	// Support 256 parameters.
	bool printedParamNum[256];
	tStd::tMemset(printedParamNum, 0, sizeof(printedParamNum));
	for (tParam* param = Params.First(); param; param = param->Next())
	{
		if ((param->ParamNumber < 256) && !printedParamNum[param->ParamNumber])
		{
			if (!param->Name.IsEmpty())
				tPrintf("%s ", param->Name.Pod());
			else
				tPrintf("param%d ", param->ParamNumber);
			printedParamNum[param->ParamNumber] = true;
		}
	}

	tPrintf("\n\n");
	if (desc)
		tPrintf("%s", desc);
	tPrintf("\n\n");

	int indent = 0;
	if (!Options.IsEmpty())
	{
		for (tOption* option = Options.First(); option; option = option->Next())
		{
			int numPrint = 0;
			if (!option->LongName.IsEmpty())
				numPrint += tcPrintf("--%s ", option->LongName.Pod());
			if (!option->ShortName.IsEmpty())
				numPrint += tcPrintf("-%s ", option->ShortName.Pod());
			for (int a = 0; a < option->NumFlagArgs; a++)
				numPrint += tcPrintf("arg%c ", '1'+a);

			indent = tMath::tMax(indent, numPrint);
		}
	}

	if (!Params.IsEmpty())
	{
		// Loop through them all to figure out how far to indent.
		for (tParam* param = Params.First(); param; param = param->Next())
		{
			int numPrint = 0;
			if (!param->Name.IsEmpty())
				numPrint = tcPrintf("%s ", param->Name.Pod());
			else
				numPrint = tcPrintf("param%d ", param->ParamNumber);
			indent = tMath::tMax(indent, numPrint);
		}
	}

	if (!Options.IsEmpty())
	{
		tPrintf("Options:\n");
		for (tOption* option = Options.First(); option; option = option->Next())
		{
			int numPrinted = 0;
			if (!option->LongName.IsEmpty())
				numPrinted += tPrintf("--%s ", option->LongName.Pod());
			if (!option->ShortName.IsEmpty())
				numPrinted += tPrintf("-%s ", option->ShortName.Pod());

			for (int a = 0; a < option->NumFlagArgs; a++)
				numPrinted += tPrintf("arg%c ", '1'+a);

			IndentSpaces(indent-numPrinted);
			tPrintf(" : %s\n", option->Description.Pod());
		}
		tPrintf("\n");
	}

	if (!Params.IsEmpty())
	{
		tPrintf("Parameters:\n");
		for (tParam* param = Params.First(); param; param = param->Next())
		{
			int numPrinted = 0;
			if (!param->Name.IsEmpty())
				numPrinted = tPrintf("%s ", param->Name.Pod());
			else
				numPrinted = tPrintf("param%d ", param->ParamNumber);

			IndentSpaces(indent - numPrinted);

			if (!param->Description.IsEmpty())
				tPrintf(" : %s", param->Description.Pod());

			tPrintf("\n");
		}
		tPrintf("\n");
	}
}
