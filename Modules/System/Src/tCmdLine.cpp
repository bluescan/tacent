// tCmdLine.cpp
//
// Parses a command line. A description of how to use the parser is in the header. Internally the first step is the
// expansion of combined single hyphen options. Next the parameters and options are parsed out. For each registered
// tOption and tParam object, its members are set to reflect the current command line when the tParse call is made.
// You may have more than one tOption that responds to the same option name. You may have more than one tParam that
// responds to the same parameter number. You may also collect all parameters in a single tParam by setting the
// parameter number to -1.
//
// Copyright (c) 2017, 2020, 2023-2025 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tFundamentals.h>
#include "System/tCmdLine.h"
#include "System/tFile.h"


namespace tCmdLine
{
	// Any single-hyphen combined option arguments are expanded here. Ex. -abc becomes -a -b -c.
	void ExpandArgs(tList<tStringItem>& args);
	int IndentSpaces(tString* dest, int numSpaces);

	void SyntaxInternal(tString* dest, int width);
	void VersionAuthorInternal(tString& verAuthDest, const char8_t* author, int major, int minor = -1, int revision = -1);
	void UsageInternal(tString* dest, bool indent, const char8_t* verAuth, const char8_t* desc);

	// I'm relying on zero initialization here. It's all zeroes before any items are constructed.
	tList<tParam> Params(tListMode::StaticZero);
	tList<tOption> Options(tListMode::StaticZero);
	tString Program;
	tString Empty;
}


tString tCmdLine::tGetProgram()
{
	return Program;
}


tCmdLine::tParam::tParam(int paramNumber, const char* name, const char* description, bool exclude) :
	ParamNumber(paramNumber),
	Values(tListMode::ListOwns),
	Name(),
	Description(),
	ExcludeFromUsage(exclude)
{
	tAssert(ParamNumber >= 0);
	if (name)
		Name.Set(name);
	else
		tsPrintf(Name, "Param%d", paramNumber);

	if (description)
		Description = tString(description);

	Params.Append(this);
}


tCmdLine::tOption::tOption(const char* description, char shortName, const char* longName, int numArgs, bool exclude) :
	ShortName(shortName),
	LongName(longName),
	Description(description),
	NumArgsPerOption(numArgs),
	Args(tListMode::ListOwns),
	Present(false),
	ExcludeFromUsage(exclude)
{
	Options.Append(this);
}


tCmdLine::tOption::tOption(const char* description, const char* longName, char shortName, int numArgs, bool exclude) :
	ShortName(shortName),
	LongName(longName),
	Description(description),
	NumArgsPerOption(numArgs),
	Args(tListMode::ListOwns),
	Present(false),
	ExcludeFromUsage(exclude)
{
	Options.Append(this);
}


tCmdLine::tOption::tOption(const char* description, char shortName, int numArgs, bool exclude) :
	ShortName(shortName),
	LongName(),
	Description(description),
	NumArgsPerOption(numArgs),
	Args(tListMode::ListOwns),
	Present(false),
	ExcludeFromUsage(exclude)
{
	Options.Append(this);
}


tCmdLine::tOption::tOption(const char* description, const char* longName, int numArgs, bool exclude) :
	ShortName(),
	LongName(longName),
	Description(description),
	NumArgsPerOption(numArgs),
	Args(tListMode::ListOwns),
	Present(false),
	ExcludeFromUsage(exclude)
{
	Options.Append(this);
}


int tCmdLine::IndentSpaces(tString* dest, int numSpaces)
{
	for (int s = 0; s < numSpaces; s++)
		tsaPrintf(dest, " ");

	return numSpaces;
}


const tString& tCmdLine::tOption::ArgN(int n) const
{
	for (tStringItem* arg = Args.First(); arg; arg = arg->Next(), n--)
		if (n <= 1)
			return *arg;

	return Empty;
}


void tCmdLine::tParse(int argc, char** argv, bool sortOptions)
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

	tParse(line, true, sortOptions);
}


void tCmdLine::tParse(int argc, char16_t** argv, bool sortOptions)
{
	if (argc <= 0)
		return;

	// Create a single line string of all the separate argv's. Arguments with quotes and spaces will come in as
	// distinct argv's, but they all get combined here. I don't believe two consecutive spaces will work.
	tString line;
	for (int a = 0; a < argc; a++)
	{
		char16_t* arg16 = argv[a];
		if (!arg16)
			continue;

		// Arg may have spaces within it. Such arguments need to be enclosed in quotes.
		// We can construct a UTF-8 tString from a UTF-16 char16_t pointer.
		tString argStr(arg16);
		if (argStr.IsEmpty())
			continue;

		if (argStr.FindChar(' ') != -1)
			argStr = tString("\"") + argStr + tString("\"");

		line += argStr;
		if (a < (argc - 1))
			line += " ";
	}

	tParse(line, true, sortOptions);
}


#ifdef PLATFORM_WINDOWS
void tCmdLine::tParse(int argc, wchar_t** argv, bool sortOptions)
{
	tParse(argc, (char16_t**)argv, sortOptions);
}
#endif


void tCmdLine::ExpandArgs(tList<tStringItem>& args)
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

		// By now it's a single hyphen and is expandble. e.g. -abc -> -a -b -c
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


static bool ParamSortFn(const tCmdLine::tParam& a, const tCmdLine::tParam& b)
{
	return (a.ParamNumber < b.ParamNumber);
}


static bool OptionSortFnShort(const tCmdLine::tOption& a, const tCmdLine::tOption& b)
{
	return tStd::tStrcmp(a.ShortName.Pod(), b.ShortName.Pod()) < 0;
}


static bool OptionSortFnLong(const tCmdLine::tOption& a, const tCmdLine::tOption& b)
{
	return tStd::tStrcmp(a.LongName.Pod(), b.LongName.Pod()) < 0;
}


void tCmdLine::tParse(const char8_t* commandLine, bool fullCommandLine, bool sortOptions)
{
	// At this point the constructors for all tOptions and tParams will have been called and both Params and Options
	// lists are populated. Options can be specified in any order. By default we are going to order them alphabetically
	// by short flag name so they get printed nicely by tPrintUsage. Params must be printed in order based on their param num
	// so we'll do that sort here too. Param sorting cannot be disabled.
	Params.Sort(ParamSortFn);
	if (sortOptions)
	{
		Options.Sort(OptionSortFnShort);
		Options.Sort(OptionSortFnLong);
	}

	tString line(commandLine);

	// Mark both kinds of escaped quotes that may be present. These may be found when the caller
	// wants a quote inside a string on the command line.
	line.Replace(u8"^'", tStd::u8SeparatorAStr);		// Replaces ^'
	line.Replace(u8"^\"", tStd::u8SeparatorBStr);		// Replaces ^"
	line.Replace(u8"^^", tStd::u8SeparatorCStr);		// Replaces ^^

	// Mark the spaces and hyphens inside normal (non escaped) quotes.
	bool inside = false;
	for (char8_t* ch = line.Text(); *ch; ch++)
	{
		if ((*ch == '\'') || (*ch == '\"'))
			inside = !inside;

		if (!inside)
			continue;

		if (*ch == ' ')
			*ch = tStd::SeparatorD;

		if (*ch == '-')
			*ch = tStd::SeparatorE;
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
		arg->Replace(tStd::SeparatorC, '^');
		arg->Replace(tStd::SeparatorD, ' ');
	}

	// Set the program name as typed in the command line.
	if (fullCommandLine)
	{
		tStringItem* prog = args.Remove();
		Program.Set(prog->Chars());
		delete prog;
	}
	else
	{
		Program.Clear();
	}

	ExpandArgs(args);

	// Process all options. If there is more than one tOption that uses the same names, they all need to
	// be populated. That's why we need to loop through all arguments for each tOption.
	for (tOption* option = Options.First(); option; option = option->Next())
	{
		for (tStringItem* arg = args.First(); arg; arg = arg->Next())
		{
			if ( (*arg == tString("--") + option->LongName) || (*arg == tString("-") + option->ShortName) )
			{
				option->Present = true;
				for (int optArgNum = 0; optArgNum < option->NumArgsPerOption; optArgNum++)
				{
					arg = arg->Next();
					tStringItem* argItem = new tStringItem(*arg);
					argItem->Replace(tStd::SeparatorE, '-');
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
				tString optArg = *arg;

				// We only skip options we recognize.
				if ( (optArg == tString("--") + option->LongName) || (optArg == tString("-") + option->ShortName) )
				{
					candidate = nullptr;
					for (int optArgNum = 0; optArgNum < option->NumArgsPerOption; optArgNum++)
						arg = arg->Next();
				}
			}
		}

		if (candidate)
		{
			tStringItem* cmdArg = new tStringItem(*candidate);
			cmdArg->Replace(tStd::SeparatorE, '-');
			commandLineParams.Append(cmdArg);
		}
	}

	// Process all parameters. Note again that similarly to tOptions, we need to loop through all commandLineParams
	// arguments for every tParam. This is because more than one tParam may need to collect the same arg. In fact some
	// tParams may have their param number set to 0, in which case they (all) need to collect all parameter arguments.
	for (tParam* param = Params.First(); param; param = param->Next())
	{
		int paramNumber = 1;
		for (tStringItem* arg = commandLineParams.First(); arg; arg = arg->Next(), paramNumber++)
		{
			if ((param->ParamNumber == paramNumber) || (param->ParamNumber == 0))
			{
				param->Values.Append(new tStringItem(*arg));
			}
		}
	}
}


void tCmdLine::SyntaxInternal(tString* dest, int width)
{
	tString syntaxRaw =
R"SYNTAX(Command Line Syntax Guide
program [arg1 arg2 arg3 ...]

ARGUMENTS:
Arguments are separated by spaces. An argument must be enclosed in quotes (single or double) if it has spaces in it or you want the argument to start with a hyphen literal. Hat (^) escape sequences can be used to put either type of quote inside. If you need to specify file paths you may use forward or back slashes. An ARGUMENT is either an OPTION or PARAMETER.

OPTIONS:
An option is simply an argument that starts with a hyphen (-). An option has a short syntax and a long syntax. Short syntax is a - followed by a single non-hyphen character. The long form is -- followed by a word. All options support either long, short, or both forms. Options may have 0 or more arguments separated by spaces. Options can be specified in any order. Short form options may be combined: e.g. -al expands to -a -l.

FLAGS:
If an option takes zero arguments it is called a flag. You can only test for a FLAG's presence or lack thereof.

PARAMETERS:
A parameter is simply an argument that is not one of the available options. It can be read as a string and parsed however is needed (converted to an integer, float etc.) Order is important when specifying parameters. If you need a hyphen in a parameter at the start you will need put the parameter in quotes. For example, a filename _can_ start with -. Note that arguments that start with a hyphen but are not recognized as a valid option just get turned into parameters. This means interpreting a hyphen directly instead of as an option specifier will happen automatically if there are no options matching what comes after the hyphen. e.g. 'tool -.85 --add 33 -87.98 --notpresent' work just fine as long as there are no options that have a short form with digits or a decimal. In this example the -.85 will be the first parameter, --notpresent will be the second. The --add is assumed to take in two number arguments.

ESCAPES:
Sometimes you need a particular character to appear inside an argument. For example you may need a single or double quote to apprear inside a parameter. The hat (^) followed by the character you need is used for this purpose.
e.g. ^^ yields ^ | ^' yields ' | ^" yields "

VARIABLE ARGUMENTS:
A variable number of space-separated parameters may be specified if the tool supports them. The parsing system will collect them all up if the parameter number is unset (-1).
A variable number of option arguments is not directly supported due to the more complex parsing that would be needed. The same result is achieved by entering the same option more than once. Order is preserved. This can also be done with options that take more than one argument.
e.g. tool -I /patha/include/ -I /pathb/include

EXAMPLE:
mycopy -R --overwrite fileA.txt -pat fileB.txt --log log.txt

The fileA.txt and fileB.txt in the above example are parameters (assuming the overwrite option is a flag). fileA.txt is the first parameter and fileB.txt is the second.

The '--log log.txt' is an option with a single argument, log.txt. Flags may be combined. The -pat in the example expands to -p -a -t. It is suggested only to combine flag (boolean) options as only the last option would get any arguments.
)SYNTAX";

	// Support column width by processing each line of syntaxRaw and adding to the final syntax string.
	tString syntax;
	while (syntaxRaw.IsValid())
	{
		tString line = syntaxRaw.ExtractLeft('\n');
		if (line.IsEmpty())
		{
			syntax += "\n";
			continue;
		}

		// The separator is so we can test if the word we're extracting is the last of the line.
		line += tStd::SeparatorAStr;
		line += " ";
		int col = 0;
		do
		{
			tString word = line.ExtractLeft(' ');
			bool lastWord = false;
			if (word.FindChar(tStd::SeparatorA, true) != -1)
			{
				word.RemoveLast();
				lastWord = true;
			}

			int wlen = word.Length();
			if (wlen > width)
			{
				// If the word is super long we're stuck. Just add it and move on. We don't break single words.
				syntax += word;
				col = 0;
				continue;
			}
			if (col+wlen <= width)
			{
				syntax += word;
				col += wlen;

				// We only add the space after the word if it's not the last one of the input line.
				if (!lastWord)
				{
					syntax += " ";
					col++;
				}
			}
			else
			{
				// No room on the current output line. PPlace the word on the next line and add a space.
				syntax.RemoveLast();
				syntax += "\n";
				syntax += word;
				syntax += " ";
				col = wlen+1;
			}
		}
		while (line.IsValid());
		syntax += "\n";
	}

	tsaPrintf(dest, "%s", syntax.Pod());
}


void tCmdLine::tPrintSyntax(int width)
{
	SyntaxInternal(nullptr, width);
}


void tCmdLine::tStringSyntax(tString& dest, int width)
{
	SyntaxInternal(&dest, width);
}


void tCmdLine::VersionAuthorInternal(tString& verAuth, const char8_t* author, int major, int minor, int revision)
{
	tAssert(major >= 0);
	tAssert((minor >= 0) || (revision < 0));		// Not allowed a valid revision number if minor is not also valid.

	verAuth.Clear();
	tsaPrintf(verAuth, "Version %d", major);
	if (minor >= 0)
	{
		tsaPrintf(verAuth, ".%d", minor);
		if (revision >= 0)
			tsaPrintf(verAuth, ".%d", revision);
	}

	if (author)
		tsaPrintf(verAuth, " by %s", author);
}


void tCmdLine::UsageInternal(tString* dest, bool doIndent, const char8_t* verAuth, const char8_t* desc)
{
	tString exeName = "program";
	if (!tCmdLine::Program.IsEmpty())
	{
		exeName = tSystem::tGetFileName(tCmdLine::Program);
	}
	else
	{
		tString fullPath = tSystem::tGetProgramPath();
		tString fileName = tSystem::tGetFileName(fullPath);
		if (fileName.IsValid())
			exeName = fileName;
	}

	if (verAuth)
		tsaPrintf(dest, "%s %s\n\n", tPod(tSystem::tGetFileBaseName(exeName)), verAuth);

	if (Options.IsEmpty())
		tsaPrintf(dest, "USAGE: %s ", exeName.Pod());
	else
		tsaPrintf(dest, "USAGE: %s [options] ", exeName.Pod());

	for (tParam* param = Params.First(); param; param = param->Next())
	{
		if (param->ExcludeFromUsage)
			continue;

		if (!param->Name.IsEmpty() && (param->ParamNumber > 0))
			tsaPrintf(dest, "%s ", param->Name.Pod());
		else if (!param->Name.IsEmpty() && (param->ParamNumber == 0))
			tsaPrintf(dest, "[%s] ", param->Name.Pod());
		else if (param->ParamNumber > 0)
			tsaPrintf(dest, "param%d ", param->ParamNumber);
		else
			tsaPrintf(dest, "[params] ");
	}

	tsaPrintf(dest, "\n\n");
	if (desc)
	{
		tsaPrintf(dest, "%s", desc);
		tsaPrintf(dest, "\n\n");
	}

	int indent = 0;
	int numUsageOptions = 0;
	int numUsageParams = 0;

	// Loop through both options and parameters to figure out how far to indent.
	for (tOption* option = Options.First(); option; option = option->Next())
	{
		if (option->ExcludeFromUsage)
			continue;

		int numPrint = 0;
		if (!option->LongName.IsEmpty())
			numPrint += tcPrintf("--%s ", option->LongName.Pod());
		if (!option->ShortName.IsEmpty())
			numPrint += tcPrintf("-%s ", option->ShortName.Pod());

		if (option->NumArgsPerOption <= 2)
		{
			for (int a = 0; a < option->NumArgsPerOption; a++)
				numPrint += tcPrintf("arg%c ", '1'+a);
		}
		else
		{
			numPrint += tcPrintf("[%d args] ", option->NumArgsPerOption);
		}

		if (doIndent)
			indent = tMath::tMax(indent, numPrint);
		numUsageOptions++;
	}

	for (tParam* param = Params.First(); param; param = param->Next())
	{
		if (param->ExcludeFromUsage)
			continue;

		int numPrint = 0;
		if (!param->Name.IsEmpty() && (param->ParamNumber > 0))
			numPrint = tcPrintf("%s ", param->Name.Pod());
		else if (!param->Name.IsEmpty() && (param->ParamNumber == 0))
			numPrint = tcPrintf("[%s] ", param->Name.Pod());
		else if (param->ParamNumber > 0)
			numPrint = tcPrintf("param%d ", param->ParamNumber);
		else
			numPrint = tcPrintf("[params] ");

		if (doIndent)
			indent = tMath::tMax(indent, numPrint);
		numUsageParams++;
	}

	if (numUsageOptions > 0)
	{
		tsaPrintf(dest, "Options:\n");
		for (tOption* option = Options.First(); option; option = option->Next())
		{
			if (option->ExcludeFromUsage)
				continue;

			int numPrinted = 0;
			if (!option->LongName.IsEmpty())
				numPrinted += tsaPrintf(dest, "--%s ", option->LongName.Pod());
			if (!option->ShortName.IsEmpty())
				numPrinted += tsaPrintf(dest, "-%s ", option->ShortName.Pod());

			if (option->NumArgsPerOption <= 2)
			{
				for (int a = 0; a < option->NumArgsPerOption; a++)
					numPrinted += tsaPrintf(dest, "arg%c ", '1'+a);
			}
			else
			{
				numPrinted += tsaPrintf(dest, "[%d args] ", option->NumArgsPerOption);
			}

			if (doIndent)
				IndentSpaces(dest, indent-numPrinted);
			tsaPrintf(dest, ": %s\n", option->Description.Pod());
		}
		tsaPrintf(dest, "\n");
	}

	if (numUsageParams > 0)
	{
		tsaPrintf(dest, "Parameters:\n");
		for (tParam* param = Params.First(); param; param = param->Next())
		{
			if (param->ExcludeFromUsage)
				continue;

			int numPrinted = 0;
			if (!param->Name.IsEmpty() && (param->ParamNumber > 0))
				numPrinted = tsaPrintf(dest, "%s ", param->Name.Pod());
			else if (!param->Name.IsEmpty() && (param->ParamNumber == 0))
				numPrinted = tsaPrintf(dest, "[%s] ", param->Name.Pod());
			else if (param->ParamNumber > 0)
				numPrinted = tsaPrintf(dest, "param%d ", param->ParamNumber);
			else
				numPrinted = tsaPrintf(dest, "[params] ");

			if (doIndent)
				IndentSpaces(dest, indent - numPrinted);

			if (!param->Description.IsEmpty())
				tsaPrintf(dest, ": %s", param->Description.Pod());
			else
				tsaPrintf(dest, ": No description");

			tsaPrintf(dest, "\n");
		}
		tsaPrintf(dest, "\n");
	}
}


void tCmdLine::tPrintUsage(int major, int minor, int revision)
{
	tPrintUsage(nullptr, major, minor, revision);
}


void tCmdLine::tPrintUsage(const char8_t* author, int major, int minor, int revision)
{
	tPrintUsage(author, nullptr, major, minor, revision);
}


void tCmdLine::tPrintUsage(const char8_t* author, const char8_t* desc, int major, int minor, int revision)
{
	tString verAuth;
	VersionAuthorInternal(verAuth, author, major, minor, revision);
	tPrintUsage(verAuth.Pod(), desc);
}


void tCmdLine::tPrintUsage(const char8_t* verAuth, const char8_t* desc)
{
	UsageInternal(nullptr, true, verAuth, desc);
}


void tCmdLine::tStringUsage(tString& dest, int major, int minor, int revision)
{
	tStringUsage(dest, nullptr, major, minor, revision);
}


void tCmdLine::tStringUsage(tString& dest, const char8_t* author, int major, int minor, int revision)
{
	tStringUsage(dest, author, nullptr, major, minor, revision);
}


void tCmdLine::tStringUsage(tString& dest, const char8_t* author, const char8_t* desc, int major, int minor, int revision)
{
	tString verAuth;
	VersionAuthorInternal(verAuth, author, major, minor, revision);
	tStringUsage(dest, verAuth.Pod(), desc);
}


void tCmdLine::tStringUsage(tString& dest, const char8_t* verAuth, const char8_t* desc)
{
	UsageInternal(&dest, true, verAuth, desc);
}


void tCmdLine::tStringUsageNI(tString& dest, int major, int minor, int revision)
{
	tStringUsageNI(dest, nullptr, major, minor, revision);
}


void tCmdLine::tStringUsageNI(tString& dest, const char8_t* author, int major, int minor, int revision)
{
	tStringUsageNI(dest, author, nullptr, major, minor, revision);
}


void tCmdLine::tStringUsageNI(tString& dest, const char8_t* author, const char8_t* desc, int major, int minor, int revision)
{
	tString verAuth;
	VersionAuthorInternal(verAuth, author, major, minor, revision);
	tStringUsageNI(dest, verAuth.Pod(), desc);
}


void tCmdLine::tStringUsageNI(tString& dest, const char8_t* verAuth, const char8_t* desc)
{
	UsageInternal(&dest, false, verAuth, desc);
}
