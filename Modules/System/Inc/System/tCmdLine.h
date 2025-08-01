// tCmdLine.h
//
// Parses a command line. A command line takes the form:
// program [arg1 arg2 arg3 ...]
//
// ARGUMENTS:
// Arguments are separated by spaces. An argument must be enclosed in quotes (single or double) if it has spaces in it
// or you want the argument to start with a hyphen literal. Hat (^) escape sequences can be used to put either type of
// quote inside. If you need to specify file paths you may use forward or back slashes. An ARGUMENT is either an OPTION
// or PARAMETER.
//
// OPTIONS:
// An option is simply an argument that starts with a hyphen (-). An option has a short syntax and a long syntax.
// Short syntax is a - followed by a single non-hyphen character. The long form is -- followed by a word. All options
// support either long, short, or both forms. Options may have 0 or more arguments separated by spaces. Options can be
// specified in any order. Short form options may be combined: Eg. -al expands to -a -l.
//
// FLAGS:
// If an option takes zero arguments it is called a flag. You can only test for a FLAGs presence or lack thereof.
//
// PARAMETERS:
// A parameter is simply an argument that is not one of the available options. It can be read as a string and parsed
// however is needed (converted to an integer, float etc.) Order is important when specifying parameters. If you need a
// hyphen in a parameter at the start you will need put the parameter in quotes. For example, a filename _can_ start
// with -. Note that arguments that start with a hyphen but are not recognized as a valid option just get turned into
// parameters. This means interpreting a hyphen directly instead of as an option specifier will happen automatically if
// there are no options matching what comes after the hyphen. Eg. 'tool -.85 --add 33 -87.98 --notpresent' work just
// fine as long as there are no options that have a short form with digits or a decimal. In this example the -.85 will
// be the first parameter, --notpresent will be the second. The --add is assumed to take in two number arguments.
//
// ESCAPES:
// In some cases you may need a particular character to appear inside an argument. For example you
// may need a single or double quote to apprear inside a parameter. The hat (^) followed by the character you need is
// used for this purpose. Eg: ^^ yields ^ | ^' yields ' | ^" yields "
//
// VARIABLE ARGUMENTS:
// A variable number of space-separated parameters may be specified if the tool supports them. The parsing system will
// collect them all up if the parameter number is unset (-1).
// A variable number of option arguments is not directly supported due to the more complex parsing that would be needed.
// The same result is achieved by entering the same option more than once.
// Eg. tool -I /patha/include/ -I /pathb/include
//
// EXAMPLE:
// mycopy -R --overwrite fileA.txt -pat fileB.txt --log log.txt
//
// The fileA.txt and fileB.txt in the above example are parameters (assuming the overwrite option is a flag). fileA.txt
// is the first parameter and fileB.txt is the second.
//
// The '--log log.txt' is an option with a single argument, log.txt. Options that are Flags may be combined. The -pat
// in the example expands to -p -a -t. It is suggested only to combine flag options as only the last option would get
// any arguments.
//
// DESIGN:
// A powerful feature of the design of this parsing system is separation of concerns. In a typical system the knowledge
// of all the different command line parameters and options is needed in a single place, often in main() where argc and
// argv are passed in. These values need to somehow be passed all over the place in a large system. With tCmdLine you
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
// Copyright (c) 2017, 2020, 2023-2025 Tristan Grimmer.
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


namespace tCmdLine
{
	struct tParam : public tLink<tParam>
	{
		// Parameter number starts at 1. Set it to which parameter you want from the command line. For example, set to
		// 2 if you want this object to receive the 2nd parameter. If you want ALL command-line paramters collected
		// here you must explicitly set number to 0. If you do this, Values gets populated with every parameter. Name
		// and desc are optional and are used when printing the tool usage. Exclude means exclude from the usage print. 
		tParam(int number,			const char* name = nullptr,	const char* desc = nullptr,	bool exclude = false);
		tParam(const char* desc,	const char* name,			int number,					bool exclude = false);

		tString Get() const																								{ return Values.IsEmpty() ? tString() : *Values.First(); }
		void Set(const tString& value)																					{ if (Values.First()) Values.First()->Set(value); else Values.Append(new tStringItem(value)); }
		bool IsPresent() const																							{ return !Values.IsEmpty(); }
		operator bool() const																							{ return IsPresent(); }

		// 1 based. 0 means all.
		int ParamNumber;

		// This usually has a single item (if ParamNumber >= 1). Only if ParamNumber == 0 does this get populated with
		// every parameter in the command line. There may be an arbitrary number of them in this case. This list is not
		// in static-zero initialization mode because it is never populated before main (there is no init order issue).
		tList<tStringItem> Values;
		tString Name;
		tString Description;
		bool ExcludeFromUsage;
	};

	struct tOption : public tLink<tOption>
	{
		// Opt is the single-character (short name) for the option. eg. -h. Name is the full (long name) for the option.
		// eg. --help.  If exclude is true, this option is not included in the usage print.
		tOption(const char* desc,	char opt,			const char* name,	int numArgs = 0,	bool exclude = false);
		tOption(const char* desc,	const char* name,	char opt,			int numArgs = 0,	bool exclude = false);
		tOption(const char* desc,	char opt,								int numArgs = 0,	bool exclude = false);
		tOption(const char* desc,	const char* name,						int numArgs = 0,	bool exclude = false);

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
		const tString& Arg5() const																						{ return ArgN(5); }
		const tString& Arg6() const																						{ return ArgN(6); }
		const tString& Arg7() const																						{ return ArgN(7); }
		const tString& Arg8() const																						{ return ArgN(8); }
		const tString& ArgN(int n) const;																				// n must be >= 1.
		bool GetArgs(tList<tStringItem>& args) const;
		int GetNumTotalArgs() const																						{ return Args.Count(); }
		int GetNumArgsPerOption() const																					{ return NumArgsPerOption; }

		tString ShortName;
		tString LongName;
		tString Description;

		// This is _not_ the number of option args that necessarily gets collected in the Args list. It is the number
		// of option args for each instance of the option in the command line. NumTotalArgs will be a multiple of this
		// number. Eg. '--plus a b --plus c d' would yield '--plus a b c d' when parsed. NumTotalArgs would be 4 and
		// NumArgsPerOption would be 2.
		int NumArgsPerOption;

		// Important note here. If you have an option that takes 1 argument and it is listed in the command line
		// multiple times like "-i fileA -i fileB", then they will collect in the Args list in multiples
		// of 1. In general the arguments collect in multiples of NumFlagArgs.
		tList<tStringItem> Args;
		bool Present;
		bool ExcludeFromUsage;
	};

	// All strings are UTF-8 for the next two functions -- including the char** one.
	void tParse(int argc, char** argv, bool sortOptions = true);
	void tParse(const char8_t* commandLine, bool fullCommandLine = false, bool sortOptions = true);

	// We also support parsing from UTF-16. Internally we just convert to UTF-8.
	void tParse(int argc, char16_t** argv, bool sortOptions = true);
#ifdef PLATFORM_WINDOWS
	void tParse(int argc, wchar_t** argv, bool sortOptions = true);
#endif

	void tPrintSyntax(int columnWidth = 80);
	void tPrintUsage(int versionMajor, int versionMinor = -1, int versionRevision = -1);
	void tPrintUsage(const char8_t* author, int versionMajor, int versionMinor = -1, int versionRevision = -1);
	void tPrintUsage(const char8_t* author, const char8_t* desc, int versionMajor, int versionMinor = -1, int versionRevision = -1);
	void tPrintUsage(const char8_t* versionAuthor = nullptr, const char8_t* desc = nullptr);

	// The following functions are the same as the tPrint ones above except they populate a tString instead of
	// printing. This is handy if you need, for example, to display usage instructins in a message box or someehere
	// other than direct stdout. For all these functions the dest string is appended to and not cleared first.
	void tStringSyntax(tString& dest, int columnWidth = 80);
	void tStringUsage(tString& dest, int versionMajor, int versionMinor = -1, int versionRevision = -1);
	void tStringUsage(tString& dest, const char8_t* author, int versionMajor, int versionMinor = -1, int versionRevision = -1);
	void tStringUsage(tString& dest, const char8_t* author, const char8_t* desc, int versionMajor, int versionMinor = -1, int versionRevision = -1);
	void tStringUsage(tString& dest, const char8_t* versionAuthor = nullptr, const char8_t* desc = nullptr);

	// These variants of tStringUsage are for cases where a monospaced font will not be used. In this case it makes
	// little sense to try to indent things nicely. NI = no indentation.
	void tStringUsageNI(tString& dest, int versionMajor, int versionMinor = -1, int versionRevision = -1);
	void tStringUsageNI(tString& dest, const char8_t* author, int versionMajor, int versionMinor = -1, int versionRevision = -1);
	void tStringUsageNI(tString& dest, const char8_t* author, const char8_t* desc, int versionMajor, int versionMinor = -1, int versionRevision = -1);
	void tStringUsageNI(tString& dest, const char8_t* versionAuthor = nullptr, const char8_t* desc = nullptr);

	// Returns the program name assuming you have already called tParse.
	tString tGetProgram();
}


// Implementation below this line.


inline tCmdLine::tParam::tParam(const char* description, const char* paramName, int paramNumber, bool exclude) :
	tParam(paramNumber, paramName, description, exclude)
{
}


inline bool tCmdLine::tOption::GetArgs(tList<tStringItem>& args) const
{
	if (!IsPresent())
		return false;

	for (tStringItem* srcArg = Args.First(); srcArg; srcArg = srcArg->Next())
		args.Append(new tStringItem(*srcArg));

	return true;
}
