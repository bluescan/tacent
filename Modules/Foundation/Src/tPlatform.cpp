// tPlatform.cpp
//
// Tacent platform defines, architecture, and endianness detection. The Tacent library has some preprocessor define
// requirements. One of PLATFORM_NNN, ARCHITECTURE_NNN, and CONFIG_NNN need to be defined. If you haven't bothered
// to define these in the project file with a /D switch, an attempt is made to define them automatically for you.
//
// Copyright (c) 2004-2006, 2015, 2017, 2020, 2021 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include "Foundation/tPlatform.h"
#include "Foundation/tString.h"


tPlatform tGetPlatform()
{
	#if defined(PLATFORM_WINDOWS)
	return tPlatform::Windows;
	#elif defined(PLATFORM_LINUX)
	return tPlatform::Linux;
	#elif defined(PLATFORM_MACOS)
	return tPlatform::MacOS;
	#elif defined(PLATFORM_ANDROID)
	return tPlatform::Android;
	#elif defined(PLATFORM_IOS)
	return tPlatform::iOS;
	#else
	return tPlatform::Invalid;
	#endif
}


tPlatform tGetPlatform(const tString& name)
{
	for (int p = 0; p < int(tPlatform::NumPlatforms); p++)
		if (name == tGetPlatformName(tPlatform(p)))
			return tPlatform(p);

	return tPlatform::Invalid;
}


const char* tGetPlatformName(tPlatform plat)
{
	const static char* platNames[] =
	{
		"Windows",
		"Linux",
		"MacOS",
		"Android",
		"iOS",
		"All",
		"Invalid",
	};
	tStaticAssert( ((sizeof(platNames)/sizeof(*platNames)) - 2) == int(tPlatform::NumPlatforms) );

	if (plat == tPlatform::Invalid)
		return platNames[int(tPlatform::NumPlatforms)+1];

	return platNames[int(plat)];
}


const char* tGetPlatformNameShort(tPlatform plat)
{
	const static char* platNames[] =
	{
		"Win",
		"Lin",
		"OSX",
		"And",
		"iOS",
		"All",
		"N/A",
	};
	tStaticAssert( ((sizeof(platNames)/sizeof(*platNames)) - 2) == int(tPlatform::NumPlatforms) );

	if (plat == tPlatform::Invalid)
		return platNames[int(tPlatform::NumPlatforms)+1];

	return platNames[int(plat)];
}


tArchitecture tGetArchitecture()
{
	#if defined(ARCHITECTURE_X86)
	return tArchitecture::x86;

	#elif defined(ARCHITECTURE_X64)
	return tArchitecture::x64;

	#elif defined(ARCHITECTURE_A32)
	return tArchitecture::A32;

	#elif defined(ARCHITECTURE_A64)
	return tArchitecture::A64;

	#else
	return tArchitecture::Invalid;
	#endif
}


const char* tGetArchitectureName(tArchitecture arch)
{
	const static char* archNames[] =
	{
		"x86",
		"x64",
		"A32",
		"A64",
		"Invalid"
	};
	tStaticAssert( ((sizeof(archNames)/sizeof(*archNames)) - 1) == int(tArchitecture::NumArchitectures) );

	if (arch == tArchitecture::Invalid)
		return archNames[int(tArchitecture::NumArchitectures)];

	return archNames[int(arch)];
}


const char* tGetArchitectureNameLong(tArchitecture arch)
{
	const static char* archNames[] =
	{
		"x86|INTEL32|x86|32bit",
		"x64|AMD64|x86-64|64bit",
		"A32|ARM32|ARM|32bit",
		"A64|ARM64|AArch64|64bit",
		"Invalid"
	};
	tStaticAssert( ((sizeof(archNames)/sizeof(*archNames)) - 1) == int(tArchitecture::NumArchitectures) );

	if (arch == tArchitecture::Invalid)
		return archNames[int(tArchitecture::NumArchitectures)];

	return archNames[int(arch)];
}


tConfiguration tGetConfiguration()
{
	#if defined(CONFIG_DEBUG)
	return tConfiguration::Debug;

	#elif defined(CONFIG_DEVELOP)
	return tConfiguration::Develop;

	#elif defined(CONFIG_PROFILE)
	return tConfiguration::Profile;

	#elif defined(CONFIG_RELEASE)
	return tConfiguration::Release;

	#elif defined(CONFIG_SHIP)
	return tConfiguration::Ship;

	#else
	return tConfiguration::Invalid;
	#endif
}


const char* tGetConfigurationName(tConfiguration config)
{
	const static char* configNames[] =
	{
		"Debug",
		"Develop",
		"Profile",
		"Release",
		"Ship",
		"Invalid"
	};
	tStaticAssert( ((sizeof(configNames)/sizeof(*configNames)) - 1) == int(tConfiguration::NumConfigurations) );

	if (config == tConfiguration::Invalid)
		return configNames[int(tConfiguration::NumConfigurations)];

	return configNames[int(config)];
}


tEndianness tGetEndianness(tPlatform plat)
{
	switch (plat)
	{
		case tPlatform::Windows:
		case tPlatform::Linux:
		case tPlatform::MacOS:
		case tPlatform::Android:
		case tPlatform::iOS:
			return tEndianness::Little;
	}

	return tEndianness::Invalid;
}
