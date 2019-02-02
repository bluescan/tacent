// DynDnsUpdate.cpp
//
// Dynamic DNS Updater.
//
// Copyright (c) 2019 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tVersion.h>
#include <System/tCommand.h>
#include <System/tFile.h>
#include <System/tScript.h>
#include <Build/tProcess.h>


using namespace tStd;
using namespace tBuild;
using namespace tSystem;
tCommand::tOption Help("Display help.", 'h', "help");
tCommand::tOption Force("Force an update even if no change detected.", 'f', "force");
tCommand::tOption OverrideAddr("Override the address that gets sent. It will autodetect ipv4 or ipv6. You can add an additional option to do both.", 'o', "override", 1);
tCommand::tParam ConfigFile(1, "ConfigFile", "The DynDnsUpdate config file. Defaults to DynDnsUpdate.cfg");


// Environment state variables.
tString StateFile = "DynDnsUpdate.ips";
tString LogFile = "DynDnsUpdate.log";
enum class LogVerbosity
{
	None,
	Normal,
	High
};
LogVerbosity Verbosity = LogVerbosity::Normal;
tString IpService = "ifconfig.co";
tString Curl = "curl.exe";


void ParseEnvironmentBlock(tExpr& block)
{
	for (tExpr entry = block.Item1(); entry.IsValid(); entry = entry.Next())
	{
		if (tString(entry.Cmd()) == "statefile")
			StateFile = entry.Arg1().GetAtomString();

		if (tString(entry.Cmd()) == "logfile")
		{
			tString verb = entry.Arg1().GetAtomString();
			if (verb == "verbose")
				Verbosity = LogVerbosity::High;

			else if (verb == "none")
				Verbosity = LogVerbosity::None;

			LogFile = entry.Arg2().GetAtomString();
		}

		if (tString(entry.Cmd()) == "ipservice")
			IpService = entry.Arg1().GetAtomString();

		if (tString(entry.Cmd()) == "curl")
			Curl = entry.Arg1().GetAtomString();
	}
}


int main(int argc, char** argv)
{
	try
	{
		tCommand::tParse(argc, argv);
		if ((argc <= 1) || Help)
		{
			tCommand::tPrintUsage();
			return 0;
		}

		tString configFile = "DynDnsUpdate.cfg";
		if (ConfigFile)
			configFile = ConfigFile.Get();

		if (!tFileExists(configFile))
		{
			tPrintf("No config file found. Default config name is DynDnsUpdate.cfg or enter preferred config file in command line.\n");
			return 1;
		}

		tScriptReader cfg(configFile);

		tExpr block = cfg.Arg0();
		while (block.IsValid())
		{
			tString blockType = block.Item0().GetAtomString();
			if (blockType == "environment")
			{
				ParseEnvironmentBlock(block);
			}

			else if (blockType == "update")
			{
			}


			//tPrintf("Block name:%s\n", .ConstText());
			block = block.Next();
		}

		tString result;
		ulong exitCode = 0;
		tProcess curlIPV6("curl -6 ifconfig.co", tGetCurrentDir(), result, &exitCode);
		tPrintf("Your IPV6 is: %s\n", result.ConstText());

		result.Clear();
		tProcess curlIPV4("curl -4 ifconfig.co", tGetCurrentDir(), result, &exitCode);
		tPrintf("Your IPV4 is: %s\n", result.ConstText());
	}
	catch (tError error)
	{
		tPrintf("Error:\n%s\n", error.Message.ConstText());
		return 1;
	}


	//int waitChar = getchar();
	return 0;
}
