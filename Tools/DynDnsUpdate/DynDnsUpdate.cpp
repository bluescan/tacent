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


namespace DynDns
{
	//								Default
	enum class eRecord			{	IPV4,		IPV6 };
	enum class eProtocol		{	HTTPS,		HTTP };
	enum class eMode			{	Changed,	Always };

	void ParseEnvironmentBlock(tExpr& block);
	void ParseUpdateBlock(tExpr& block);
	void ReadCurrentState();
	void UpdateAllServices();		// This does the updates. It's the workhorse.
	bool RunCurl
	(
		eProtocol protocol, const tString& username, const tString& password,
		const tString& service, const tString& domain, const tString& ipaddr
	);
	void WriteCurrentState();

	// Environment state variables.
	tString StateFile = "DynDnsUpdate.ips";
	tString LogFile = "DynDnsUpdate.log";
	enum class eLogVerbosity
	{
		None,
		Normal,
		High
	};
	eLogVerbosity Verbosity = eLogVerbosity::Normal;
	tString IpLookup = "ifconfig.co";
	tString Curl = "curl.exe";

	struct UpdateBlock : public tLink<UpdateBlock>
	{
		tString Domain;
		tString Service;
		eRecord Record			= eRecord::IPV4;
		eProtocol Protocol		= eProtocol::HTTPS;
		tString Username;
		tString Password;
		eMode Mode				= eMode::Changed;
		
		tString LastUpdateIP;
	};
	tList<UpdateBlock> UpdateBlocks;
}


void DynDns::ParseEnvironmentBlock(tExpr& block)
{
	for (tExpr entry = block.Item1(); entry.IsValid(); entry = entry.Next())
	{
		if (tString(entry.Cmd()) == "statefile")
			StateFile = entry.Arg1().GetAtomString();

		else if (tString(entry.Cmd()) == "logfile")
			LogFile = entry.Arg1().GetAtomString();

		else if (tString(entry.Cmd()) == "verbosity")
		{
			tString verb = entry.Arg1().GetAtomString();
			if (verb == "verbose")
				Verbosity = eLogVerbosity::High;
			else if (verb == "none")
				Verbosity = eLogVerbosity::None;
		}

		else if (tString(entry.Cmd()) == "iplookup")
			IpLookup = entry.Arg1().GetAtomString();

		else if (tString(entry.Cmd()) == "curl")
			Curl = entry.Arg1().GetAtomString();
	}
}

void DynDns::ParseUpdateBlock(tExpr& block)
{
	UpdateBlock* update = new UpdateBlock();

	for (tExpr entry = block.Item1(); entry.IsValid(); entry = entry.Next())
	{
		if (tString(entry.Cmd()) == "domain")
			update->Domain = entry.Arg1().GetAtomString();

		else if (tString(entry.Cmd()) == "service")
			update->Service = entry.Arg1().GetAtomString();

		else if (tString(entry.Cmd()) == "record")
		{
			tString rec = entry.Arg1().GetAtomString();
			if ((rec == "ipv6") || (rec == "AAAA"))
				update->Record = eRecord::IPV6;
		}

		else if (tString(entry.Cmd()) == "protocol")
		{
			tString prot = entry.Arg1().GetAtomString();
			if (prot == "http")
				update->Protocol = eProtocol::HTTP;
		}

		else if (tString(entry.Cmd()) == "username")
			update->Username = entry.Arg1().GetAtomString();

		else if (tString(entry.Cmd()) == "password")
			update->Password = entry.Arg1().GetAtomString();

		else if (tString(entry.Cmd()) == "mode")
		{
			tString mode = entry.Arg1().GetAtomString();
			if (mode == "always")
				update->Mode = eMode::Always;
		}
	}

	UpdateBlocks.Append(update);
}


void DynDns::ReadCurrentState()
{
	if (!tFileExists(StateFile))
		return;

	tScriptReader state(StateFile);
	tExpr entry = state.Arg0();
	while (entry.IsValid())
	{
		tString domain = entry.Item0();
		eRecord record = (tString(entry.Item1()) == "ipv6") ? eRecord::IPV6 : eRecord::IPV4;
		tString ip = entry.Item2();

		for (UpdateBlock* block = UpdateBlocks.First(); block; block = block->Next())
		{
			if ((block->Domain == domain) && (block->Record == record))
				block->LastUpdateIP = ip;
		}

		entry = entry.Next();
	}
}


void DynDns::UpdateAllServices()
{
	ulong exitCode = 0;

	tString ipv4;
	tProcess curlIPV4("curl -4 ifconfig.co", tGetCurrentDir(), ipv4, &exitCode);
	ipv4.Replace('\n', '\0');
	ipv4.Replace('\r', '\0');
	tPrintf("Your IPV4 is: ____%s____\n", ipv4.ConstText());

	tString ipv6;
	tProcess curlIPV6("curl -6 ifconfig.co", tGetCurrentDir(), ipv6, &exitCode);
	ipv6.Replace('\n', '\0');
	ipv6.Replace('\r', '\0');
	tPrintf("Your IPV6 is: ____%s____\n", ipv6.ConstText());

	// Update ipv4 blocks.
	if (ipv4.CountChar('.') == 3)
	{
		for (UpdateBlock* block = UpdateBlocks.First(); block; block = block->Next())
		{
			if (block->Record != eRecord::IPV4)
				continue;

			bool attemptUpdate =
				Force ||
				(block->Mode == eMode::Always) ||
				block->LastUpdateIP.IsEmpty() ||
				(block->LastUpdateIP != ipv4);

			if (attemptUpdate)
			{
				bool updated = RunCurl(block->Protocol, block->Username, block->Password, block->Service, block->Domain, ipv4);
				if (updated)
					block->LastUpdateIP = ipv4;
			}
			else
			{
				tPrintf("Skipping update.\n");
			}
		}
	}

	// Update ipv6 blocks.
	if (ipv6.CountChar(':') == 7)
	{
		for (UpdateBlock* block = UpdateBlocks.First(); block; block = block->Next())
		{
			if (block->Record != eRecord::IPV6)
				continue;

			bool attemptUpdate =
				Force ||
				(block->Mode == eMode::Always) ||
				block->LastUpdateIP.IsEmpty() ||
				(block->LastUpdateIP != ipv6);

			if (attemptUpdate)
			{
				bool updated = RunCurl(block->Protocol, block->Username, block->Password, block->Service, block->Domain, ipv6);
				if (updated)
					block->LastUpdateIP = ipv6;
			}
			else
			{
				tPrintf("Skipping update.\n");
			}
		}
	}
}


bool DynDns::RunCurl
(
	eProtocol protocol, const tString& username, const tString& password,
	const tString& service, const tString& domain, const tString& ipaddr
)
{
	tString user = username;	user.Replace("@", "%40");
	tString pass = password;	pass.Replace("@", "%40");
	tString prot = (protocol == eProtocol::HTTPS) ? "HTTPS" : "HTTP";
	tString cmd;
	tsPrintf(cmd, "%s \"%s://%s:%s@%s?hostname=%s&myip=%s\"",
		Curl.Pod(), prot.Pod(), user.Pod(), pass.Pod(), service.Pod(), domain.Pod(), ipaddr.Pod());

	tPrintf("CURL\n%s\n", cmd.Pod());

	ulong exitCode = 0;
	tString result;
	tProcess curl(cmd, tGetCurrentDir(), result, &exitCode);
	result.Replace('\n', '\0');
	result.Replace('\r', '\0');

	// Result may be "nochg 212.34.22.489" for success/no-change or "good 212.34.22.489" for success/change.
	tPrintf("Exitcode: %d Result: ____%s____\n", exitCode, result.Pod());
	bool success = false;
	if ((exitCode == 0) && ((result.FindString("good") != -1) || (result.FindString("nochg") != -1)))
		success = true;

	return success;
}


void DynDns::WriteCurrentState()
{
	tScriptWriter state(StateFile);

	state.WriteComment("DynDnsUpdate current state data.");
	state.NewLine();

	for (UpdateBlock* block = UpdateBlocks.First(); block; block = block->Next())
	{
		if (!block->LastUpdateIP.IsEmpty())
		{
			state.BeginExpression();
			state.WriteAtom(block->Domain);
			state.WriteAtom((block->Record == eRecord::IPV4) ? "ipv4" : "ipv6");
			state.WriteAtom(block->LastUpdateIP);
			state.EndExpression();
			state.NewLine();
		}
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
				DynDns::ParseEnvironmentBlock(block);

			else if (blockType == "update")
				DynDns::ParseUpdateBlock(block);

			//tPrintf("Block name:%s\n", .ConstText());
			block = block.Next();
		}

		DynDns::ReadCurrentState();
		DynDns::UpdateAllServices();
		DynDns::WriteCurrentState();
	}
	catch (tError error)
	{
		tPrintf("Error:\n%s\n", error.Message.ConstText());
		return 1;
	}


	//int waitChar = getchar();
	return 0;
}
