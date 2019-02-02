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
#include <Build/tProcess.h>


using namespace tStd;
using namespace tBuild;
using namespace tSystem;
tCommand::tOption Force("Force an update even if no change detected.", 'f', "force");
tCommand::tOption OverrideAddr("Override the address that gets sent. It will autodetect ipv4 or ipv6. You can add an additional option to do both.", 'o', "override");
tCommand::tParam ConfigFile(1, "ConfigFile");

//http://update.spdyn.de/nic/update?hostname=twookes.spdns.org&myip=2001:569:71a5:9100:b5d0:9d40:8f81:9b92
int main(int argc, char** argv)
{
	tCommand::tParse(argc, argv);
	tCommand::tPrintUsage();

	tString result;
	ulong exitCode = 0;
	tProcess curlIPV6("curl -6 ifconfig.co", tGetCurrentDir(), result, &exitCode);
	tPrintf("Your IPV6 is: %s\n", result.ConstText());

	result.Clear();
	tProcess curlIPV4("curl -4 ifconfig.co", tGetCurrentDir(), result, &exitCode);
	tPrintf("Your IPV4 is: %s\n", result.ConstText());


	int waitChar = getchar();
	return 0;
}
