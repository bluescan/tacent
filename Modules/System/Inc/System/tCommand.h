// tCommand.h
//
// Parses a command line. A command line takes the form:
// program.exe [arg1 arg2 arg3 ...]
//
// Arguments are separated by spaces. An argument must be enclosed in quotes
// (single or double) if it has a space or hyphen in it. Use escape sequences to
// put either type of quote inside. If you need to specify paths, it is suggested
// to use forward slashes, although backslashes will work so long as the filename
// does not have a single or double quote next.
//
// An argument may be an 'option' or a 'parameter'.
//
// Options:
// An option has a short syntax and a long syntax. Short syntax is a - followed by
// a single non-hyphen character. The long form is -- followed by a word. All
// options support either long, short, or both forms. Options may have 0 or more
// arguments. If an option takes zero arguments it is called a flag and you can
// only test for its presence or lack of. Options can be specified in any order.
// Short form options may be combined: Eg. -al expands to -a -l
//
// Parameters:
// A parameter is simply an argument that does not start with a - or --. It can be
// read as a string and parsed arbitrarily (converted to an integer or float etc.)
// Order is important when specifying parameters.
//
// Example:
// mycopy.exe -R --overwrite fileA.txt -pat fileB.txt --log log.txt
//
// The fileA.txt and fileB.txt in the above example are parameters (assuming
// the overwrite option is a flag). fileA.txt is the first parameter and
// fileB.txt is the second.
//
// The '--log log.txt' is an option with a single argument, log.txt. Flags may be
// combined. The -pat in the example expands to -p -a -t. It is suggested only to
// combine flag options as only the last option would get any arguments.
//
// If you wish to interpret a hyphen directly instead of as an option specifier
// this will happen automatically if there are no options matching what comes
// after the hyphen. Eg. 'tool.exe -.85 --add 33 -87.98 -notpresent' works just
// fine as long as there are no options that have a short form with digits or a
// decimal. In this example the -.85 will be the first parameter, --notpresent
// will be the second, and the --add takes in two number arguments.
//
// Variable argument options are not supported due to the extra syntax that would
// be needed. The same result is achieved by entering the same option more than
// once. Eg. tool.exe -I /patha/include/ -I /pathb/include
//
// A powerful feature of the design of this parsing system is separation of concerns. In a typical system the knowledge
// of all the different command line parameters and options is needed in a single place, often in main() where argc and
// argv are passed in. These values need to somehow be passed all over the place in a large system. With tCommand you
// specify which options and parameters you care about only in the cpp file you are working in.
//
// To use the command line class, you start by registering your options and parameters. This is done using the tOption
// and tParam types to create static objects. After main calls the parse function, your objects get populated
// appropriately. For example,
//
// FileA.cpp:
// tParam FromFile(1, "FromFile");	// The 1 means this is the first parameter. The description is optional.
// tParam ToFile(2, "ToFile");		// The 2 means this is the second parameter. The description is optional.
// tOption("log", 'l', 1, "Specify log file");	// The 1 means there is one option argument to --log or -l.
//
// FileB.cpp:
// tOption ProgramOption('p', 0, "Program mode.");
// tOption AllOption('a', "ALL", 0, "Include all widgets.");
// tOption TimeOption("time", 't', 0, "Print timestamp.");
//
// Main.cpp:
// tParse(argc, argv);
//
// Internal Processing. The first step is the expansion of combined single hyphen options. Next the parameters and
// options are parsed out. For each registered tOption and tParam object, its members are set to reflect the current
// command line when the tParse call is made. You may have more than one tOption that responds to the same option name.
// You may have more than one tParam that responds to the same parameter number.
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

#pragma once
#include <Foundation/tList.h>
#include <Foundation/tString.h>


namespace tCommand
{
	struct tParam : public tLink<tParam>
	{
		// This constructor allows you to make an array of parameters if you have a lot of them.
		// Param number starts at 1.
		tParam();
		tParam(int paramNumber, const char* name = nullptr, const char* description = nullptr);
		tParam(const char* description, const char* paramName, int paramNumber);
		tString Get() const																								{ return Param; }
		bool IsPresent() const																							{ return !Param.IsEmpty(); }
		operator bool() const																							{ return IsPresent(); }

		int ParamNumber;				// 1 based.
		tString Param;
		tString Name;
		tString Description;
	};

	struct tOption : public tLink<tOption>
	{
		tOption(const char* description, char shortName, const char* longName, int numArgs = 0);
		tOption(const char* description, const char* longName, char shortName, int numArgs = 0);
		tOption(const char* description, char shortName, int numArgs = 0);
		tOption(const char* description, const char* longName, int numArgs = 0);

		// These validity checking functions only return true if the option was found in the command line and all
		// arguments were successfully parsed.
		bool IsPresent() const																							{ return Present; }
		operator bool() const																							{ return IsPresent(); }

		// These argument accessors all return a reference to a static empty string if 'n' is out of range or the
		// option is invalid. GetArgs will return false on invalid.
		const tString& Arg1() const																						{ return ArgN(1); }
		const tString& Arg2() const																						{ return ArgN(2); }
		const tString& Arg3() const																						{ return ArgN(3); }
		const tString& Arg4() const																						{ return ArgN(4); }
		const tString& ArgN(int n) const;																				// n must be >= 1.
		bool GetArgs(tList<tStringItem>& args) const;
		int GetNumArgs() const																							{ return Args.Count(); }
		int GetNumFlagArgs() const																						{ return NumFlagArgs; }

		tString ShortName;
		tString LongName;
		tString Description;

		// This is _not_ the number of args that necessarily gets collected in the Args list. It is the number of args
		// for each instance of the flag in the command line.
		int NumFlagArgs;

		// Important note here. If you have an option that takes 1 argument and it is listed in the command line
		// multiple times like "-i fileA -i fileB", then they will collect in the Args list in multiples
		// of 1. In general the arguments collect in multiples of NumFlagArgs.
		tList<tStringItem> Args;
		bool Present;
	};

	void tParse(int argc, char** argv);
	void tParse(const char* commandLine, bool fullCommandLine = false);
	void tPrintUsage(int versionMajor, int versionMinor = -1, int revision = -1);
	void tPrintUsage(const char* author, int versionMajor, int versionMinor = -1, int revision = -1);
	void tPrintUsage(const char* author, const char* desc, int versionMajor, int versionMinor = -1, int revision = -1);
	void tPrintUsage(const char* versionAuthor = nullptr, const char* desc = nullptr);
	void tPrintSyntax();

	tString tGetProgram();			// Returns the program name assuming you have already called tParse.
	int tGetNumPresentParameters();	// Returns the number of present parameters assuming you have already called tParse.
}


// Implementation below this line.


inline tCommand::tParam::tParam(const char* description, const char* paramName, int paramNumber) :
	tParam(paramNumber, paramName, description)
{
}


inline bool tCommand::tOption::GetArgs(tList<tStringItem>& args) const
{
	if (!IsPresent())
		return false;

	for (tStringItem* srcArg = Args.First(); srcArg; srcArg = srcArg->Next())
		args.Append(new tStringItem(*srcArg));

	return true;
}
