// tMachine.h
//
// Hardware ans OS access functions like querying supported instruction sets, number or cores, computer name/ip.
// Includes parsing environment variables from the XDG Base Directory Specification (Linux-only).
//
// Copyright (c) 2004-2006, 2017, 2020, 2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Foundation/tString.h>
#include "System/tThrow.h"
#include "System/tPrint.h"
namespace tSystem
{

	
// Returns true if the processor and the OS support Streaming SIMD Extensions.
bool tSupportsSSE();
bool tSupportsSSE2();

// Returns the computer's name.
tString tGetComputerName();

// Returns an environment variable value.
tString tGetEnvVar(const tString& envVarName);

// Returns the number of cores (processors) the current machine has.
int tGetNumCores();

// Opens the OS's file explorer for the folder and file specified. If file doesn't exist, no file will be selected.
// If dir doesn't exist, an explorer window is opened at a location decided by the system.
bool tOpenSystemFileExplorer(const tString& dir, const tString& file);
bool tOpenSystemFileExplorer(const tString& fullFilename);

#if defined(PLATFORM_LINUX)
// These functions return the XDG environment variables in the first argument. They also respect all default values as
// specified by https://specifications.freedesktop.org/basedir-spec/latest/. True is returned if the envirenment
// variable was set. False is returned if the env var was unset and the default had to be used. The paths are returned
// in Tacent's path format with trailing / for directories. All paths are absolute as defined by the spec. Clients using
// these functions should NOT place data directly in the returned directories -- they should make a subdirectory,
// usually the name of the app in lowercase, and put their files there.
bool tGetXDGDataHome(tString& xdgDataHome);					// $XDG_DATA_HOME
bool tGetXDGConfigHome(tString& xdgConfigHome);				// $XDG_CONFIG_HOME
bool tGetXDGStateHome(tString& xdgStateHome);				// $XDG_STATE_HOME
void tGetXDGExeHome(tString& xdgExeHome);					// Not defined by an XDG env variable. Always: $HOME/.local/bin/
bool tGetXDGDataDirs(tList<tStringItem>& xdgDataDirs);		// $XDG_DATA_DIRS. Priority list of directories to search.
bool tGetXDGConfigDirs(tList<tStringItem>& xdgConfigDirs);	// $XDG_CONFIG_DIRS. Priority list of directories to search.
bool tGetXDGCacheHome(tString& xdgCacheHome);				// $XDG_CACHE_HOME
bool tGetXDGRuntimeDir(tString& xdgRuntimeDir);				// $XDG_RUNTIME_DIR
#endif

}
