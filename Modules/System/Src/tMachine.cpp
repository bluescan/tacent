// tMachine.cpp
//
// Hardware ans OS access functions like querying supported instruction sets, number or cores, computer name/ip.
// Includes parsing environment variables from the XDG Base Directory Specification (Linux-only).
//
// Copyright (c) 2004-2006, 2017, 2019-2022, 2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#include <intrin.h>
#else
#include <unistd.h>
#include <limits.h>
#include <sys/sysinfo.h>
#endif
#include <cstdlib>
#include "Foundation/tStandard.h"
#include "System/tFile.h"
#include "System/tMachine.h"


#ifdef PLATFORM_LINUX
namespace tSystem
{
	bool tGetXDGSingleEnvVar(tString& xdgEnvVar, const tString& xdgEnvVarName, const tString& xdgEnvVarDefault);
	bool tGetXDGMultipleEnvVar(tList<tStringItem>& xdgEnvVars, const tString& xdgEnvVarName, const tString& xdgEnvVarDefaults);
};
#endif


bool tSystem::tSupportsSSE()
{
	#ifdef PLATFORM_WINDOWS
	int cpuInfo[4];
	int infoType = 1;
	__cpuid(cpuInfo, infoType);

	int features = cpuInfo[3];

	// SSE feature bit is 25.
	if (features & (1 << 25))
		return true;
	else
		return false;
	#elif defined(PLATFORM_LINUX)
	// @todo Implement
	return true;
	#endif
}


bool tSystem::tSupportsSSE2()
{
	#ifdef PLATFORM_WINDOWS
	int cpuInfo[4];
	int infoType = 1;
	__cpuid(cpuInfo, infoType);

	int features = cpuInfo[3];

	// SSE2 feature bit is 26.
	if (features & (1 << 26))
		return true;
	else
		return false;

	#elif defined(PLATFORM_LINUX)
	// @todo Implement
	return true;
	#endif
}


tString tSystem::tGetComputerName()
{
	#ifdef PLATFORM_WINDOWS

	ulong nameSize = 128;
	#ifdef TACENT_UTF16_API_CALLS
	char16_t name[128];
	WinBool success = GetComputerName(LPWSTR(name), &nameSize);
	if (success)
	{
		tString nameUTF8;
		nameUTF8.SetUTF16(name);
		return nameUTF8;
	}

	#else
	char name[128];
	WinBool success = GetComputerName(name, &nameSize);
	if (success)
		return name;
	#endif

	#else
	char hostname[HOST_NAME_MAX];
	int err = gethostname(hostname, HOST_NAME_MAX);
	if (!err)
		return hostname;

	#endif
	return tString();
}


tString tSystem::tGetEnvVar(const tString& envVarName)
{
	if (envVarName.IsEmpty())
		return tString();

	// The tString constructor handles possible nullptr input from getenv.
	return tString(std::getenv(envVarName.Chr()));
}


int tSystem::tGetNumCores()
{
	// Lets cache this value as it never changes.
	static int numCores = 0;
	if (numCores > 0)
		return numCores;

	#ifdef PLATFORM_WINDOWS
	SYSTEM_INFO sysinfo;
	tStd::tMemset(&sysinfo, 0, sizeof(sysinfo));
	GetSystemInfo(&sysinfo);

	// dwNumberOfProcessors is unsigned, so can't say just > 0.
	if ((sysinfo.dwNumberOfProcessors == 0) || (sysinfo.dwNumberOfProcessors == -1))
		numCores = 1;
	else
		numCores = sysinfo.dwNumberOfProcessors;

	#else
	numCores = get_nprocs_conf();
	if (numCores < 1)
		numCores = 1;
	
	#endif
	return numCores;
}


bool tSystem::tOpenSystemFileExplorer(const tString& dir, const tString& file)
{
	#ifdef PLATFORM_WINDOWS
	tString fullName = dir + file;
	HWND hWnd = ::GetActiveWindow();

	// Just open an explorer window if the dir is invalid.
	if (!tSystem::tDirExists(dir))
	{
		// 20D04FE0-3AEA-1069-A2D8-08002B30309D is the CLSID of "This PC" on Windows.
		#ifdef TACENT_UTF16_API_CALLS
		ShellExecute(hWnd, LPWSTR(u"open"), LPWSTR(u"explorer"), LPWSTR(u"/n,::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"), 0, SW_SHOWNORMAL);
		#else
		ShellExecute(hWnd, "open", "explorer", "/n,::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", 0, SW_SHOWNORMAL);
		#endif
		return false;
	}

	if (tSystem::tFileExists(fullName))
	{
		fullName.Replace('/', '\\');
		tString options;
		tsPrintf(options, "/select,\"%s\"", fullName.Chr());
		#ifdef TACENT_UTF16_API_CALLS
		tStringUTF16 optionsUTF16(options);
		ShellExecute(hWnd, LPWSTR(u"open"), LPWSTR(u"explorer"), optionsUTF16.GetLPWSTR(), 0, SW_SHOWNORMAL);
		#else
		ShellExecute(hWnd, "open", "explorer", options.Chr(), 0, SW_SHOWNORMAL);
		#endif
	}
	else
	{
		#ifdef TACENT_UTF16_API_CALLS
		tStringUTF16 dirUTF16(dir);
		ShellExecute(hWnd, LPWSTR(u"open"), dirUTF16.GetLPWSTR(), 0, dirUTF16.GetLPWSTR(), SW_SHOWNORMAL);
		#else
		ShellExecute(hWnd, "open", dir.Chr(), 0, dir.Chr(), SW_SHOWNORMAL);
		#endif
	}
	return true;

	#elif defined(PLATFORM_LINUX)
	tString nautilus = "/usr/bin/nautilus";
	tString dolphin = "/usr/bin/dolphin";
	tString browser;

	if (tFileExists(nautilus))
		browser = nautilus;
	else if (tFileExists(dolphin))
		browser = dolphin;
	if (browser.IsEmpty())
		return false;

	tString sysStr;
	if (browser == nautilus)
		tsPrintf(sysStr, "%s %s%s &", browser.Chr(), dir.Chr(), file.Chr());
	else if (browser == dolphin)
		tsPrintf(sysStr, "%s --new-window --select %s%s &", browser.Chr(), dir.Chr(), file.Chr());

	system(sysStr.Chr());
	return true;

	#else
	return false;

	#endif
}


bool tSystem::tOpenSystemFileExplorer(const tString& fullFilename)
{
	return tOpenSystemFileExplorer(tSystem::tGetDir(fullFilename), tSystem::tGetFileName(fullFilename));
}


#if defined(PLATFORM_LINUX)
bool tSystem::tGetXDGSingleEnvVar(tString& xdgEnvVar, const tString& xdgEnvVarName, const tString& xdgEnvVarDefault)
{
	if (xdgEnvVarName.IsEmpty())
	{
		xdgEnvVar.Clear();
		return false;
	}
	xdgEnvVar = tGetEnvVar(xdgEnvVarName);
	bool envVarSet = xdgEnvVar.IsValid();
	if (envVarSet)
	{
		tPathStdDir(xdgEnvVar);

		// According to the spec xdgEnvVar should be an absolute path and ignored if relative.
		if (tIsRelativePath(xdgEnvVar))
			xdgEnvVar = xdgEnvVarDefault;
	}
	else
	{
		xdgEnvVar = xdgEnvVarDefault;
	}

	return envVarSet;
}


bool tSystem::tGetXDGMultipleEnvVar(tList<tStringItem>& xdgEnvVars, const tString& xdgEnvVarName, const tString& xdgEnvVarDefaults)
{
	xdgEnvVars.Empty();
	if (xdgEnvVarName.IsEmpty())
		return false;

	tString xdgEnvVar = tGetEnvVar(xdgEnvVarName);
	bool envVarSet = xdgEnvVar.IsValid();
	tList<tStringItem> paths;
	tStd::tExplode(paths, xdgEnvVar, ':');

	while (paths.Head())
	{
		tStringItem* path = paths.Remove();
		tPathStdDir(*path);

		// According to the spec 'path' should be an absolute path and ignored if relative.
		if (tIsRelativePath(*path))
			delete path;
		else
			xdgEnvVars.Append(path);
	}

	if (xdgEnvVars.IsEmpty())
	{
		tList<tStringItem> defaultPaths;
		tStd::tExplode(defaultPaths, xdgEnvVarDefaults, ':');
		while (defaultPaths.Head())
			xdgEnvVars.Append(defaultPaths.Remove());
	}

	return envVarSet;
}


bool tSystem::tGetXDGDataHome(tString& xdgDataHome)
{
	// From https://specifications.freedesktop.org/basedir-spec/latest/
	// There is a single base directory relative to which user-specific data files should be written.
	// This directory is defined by the environment variable $XDG_DATA_HOME.
	tString defaultPath = tGetHomeDir() + ".local/share/";
	return tGetXDGSingleEnvVar(xdgDataHome, "XDG_DATA_HOME", defaultPath);
}


bool tSystem::tGetXDGConfigHome(tString& xdgConfigHome)
{
	// From https://specifications.freedesktop.org/basedir-spec/latest/
	// There is a single base directory relative to which user-specific configuration files should be written.
	// This directory is defined by the environment variable $XDG_CONFIG_HOME.
	tString defaultPath = tGetHomeDir() + ".config/";
	return tGetXDGSingleEnvVar(xdgConfigHome, "XDG_CONFIG_HOME", defaultPath);
}


bool tSystem::tGetXDGStateHome(tString& xdgStateHome)
{
	// From https://specifications.freedesktop.org/basedir-spec/latest/
	// There is a single base directory relative to which user-specific state data should be written.
	// This directory is defined by the environment variable $XDG_STATE_HOME.
	tString defaultPath = tGetHomeDir() + ".local/state/";
	return tGetXDGSingleEnvVar(xdgStateHome, "XDG_STATE_HOME", defaultPath);
}


void tSystem::tGetXDGExeHome(tString& xdgExeHome)
{
	// From https://specifications.freedesktop.org/basedir-spec/latest/
	// There is a single base directory relative to which user-specific executable files may be written.
	xdgExeHome = tGetHomeDir() + ".local/bin/";
}


bool tSystem::tGetXDGDataDirs(tList<tStringItem>& xdgDataDirs)
{
	// From https://specifications.freedesktop.org/basedir-spec/latest/
	// There is a set of preference ordered base directories relative to which data files should be searched.
	// This set of directories is defined by the environment variable $XDG_DATA_DIRS.
	tString defaultPaths = "/usr/local/share/:/usr/share/";
	return tGetXDGMultipleEnvVar(xdgDataDirs, "XDG_DATA_DIRS", defaultPaths);
}


bool tSystem::tGetXDGConfigDirs(tList<tStringItem>& xdgConfigDirs)
{
	// From https://specifications.freedesktop.org/basedir-spec/latest/
	// There is a set of preference ordered base directories relative to which configuration files should be searched.
	// This set of directories is defined by the environment variable $XDG_CONFIG_DIRS.
	tString defaultPaths = "/etc/xdg/";
	return tGetXDGMultipleEnvVar(xdgConfigDirs, "XDG_CONFIG_DIRS", defaultPaths);	
}


bool tSystem::tGetXDGCacheHome(tString& xdgCacheHome)
{
	// From https://specifications.freedesktop.org/basedir-spec/latest/
	// There is a single base directory relative to which user-specific non-essential (cached) data should be written.
	// This directory is defined by the environment variable $XDG_CACHE_HOME.
	tString defaultPath = tGetHomeDir() + ".cache/";
	return tGetXDGSingleEnvVar(xdgCacheHome, "XDG_CACHE_HOME", defaultPath);
}


bool tSystem::tGetXDGRuntimeDir(tString& xdgRuntimeDir)
{
	// From https://specifications.freedesktop.org/basedir-spec/latest/
	// There is a single base directory relative to which user-specific runtime files and other file objects should be placed.
	// This directory is defined by the environment variable $XDG_RUNTIME_DIR.
	//
	// The defalut is empty on purpose for this one.
	tString defaultPath;
	return tGetXDGSingleEnvVar(xdgRuntimeDir, "XDG_RUNTIME_DIR", defaultPath);
}
#endif
