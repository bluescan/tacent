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
tCommand::tOption Username("Username or e-mail.", 'u', "user");
tCommand::tOption Password("Password.", 'p', "pass");


int main(int argc, char** argv)
{
	tCommand::tParse(argc, argv);
	// tCommand::tPrintUsage();

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
