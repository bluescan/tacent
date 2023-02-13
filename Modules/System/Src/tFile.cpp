// tFile.cpp
//
// This file contains a class implementation of a file on a disk. It derives from tStream. By passing around tStreams
// any user code can be oblivious to the type of stream. It may be a file on disk, it may be a file in a custom
// filesystem, it may be a pipe, or even a network resource.
//
// A path can refer to either a file or a directory. All paths use forward slashes as the separator. Input paths can
// use backslashes, but consistency in using forward slashes is advised. Directory path specifications always end with
// a trailing slash. Without the trailing separator the path will be interpreted as a file.
//
// Copyright (c) 2004-2006, 2017, 2019-2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tPlatform.h>
#ifdef PLATFORM_WINDOWS
#include <io.h>
#include <Windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#elif defined(PLATFORM_LINUX)
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <fstream>
#include <dirent.h>			// For fast (C-style) directory entry queries.
#endif
#include <filesystem>
#include "System/tTime.h"
#include "System/tFile.h"


namespace tSystem
{
	// Conversions to tacent-standard paths. Forward slashes where possible.
	// Windows does not allow forward slashes when dealing with network shares, a path like
	// \\machinename\sharename/dir/subdir/
	// _must_ have two backslashes before the machine name and 1 backslash before the sharename.
	void tPathStd    (tString& path);	// "C:\Hello\There\" -> "C:/Hello/There/". "C:\Hello\There" -> "C:/Hello/There".
	void tPathStdDir (tString& path);	// "C:\Hello\There\" -> "C:/Hello/There/". "C:\Hello\There" -> "C:/Hello/There/".
	void tPathStdFile(tString& path);	// "C:\Hello\There\" -> "C:/Hello/There".  "C:\Hello\There" -> "C:/Hello/There".

	// Conversions to windows-standard paths. Backwards slashes. Not seen externally.
	void tPathWin    (tString& path);	// "C:/Hello/There/" -> "C:\Hello\There\". "C:/Hello/There" -> "C:\Hello\There".
	void tPathWinDir (tString& path);	// "C:/Hello/There/" -> "C:\Hello\There\". "C:/Hello/There" -> "C:\Hello\There\".
	void tPathWinFile(tString& path);	// "C:/Hello/There/" -> "C:\Hello\There".  "C:/Hello/There" -> "C:\Hello\There".

	std::time_t tConvertToPosixTime(std::filesystem::file_time_type);
	#ifdef PLATFORM_WINDOWS
	std::time_t tConvertToPosixTime(FILETIME);
	#endif

	void tPopulateFileInfo(tFileInfo& fileInfo, const std::filesystem::directory_entry&, const tString& filename);
	#ifdef PLATFORM_WINDOWS
	void tPopulateFileInfo(tFileInfo& fileInfo, Win32FindData&, const tString& filename);
	#endif

	const int MaxExtensionsPerFileType = 4;
	struct FileTypeExts
	{
		const char* Ext[MaxExtensionsPerFileType] = { nullptr, nullptr, nullptr, nullptr };
		bool HasExt(const tString& ext)																					{ for (int e = 0; e < MaxExtensionsPerFileType; e++) if (ext.IsEqualCI(Ext[e])) return true; return false; }
	};

	// It is important not to specify the array size here so we can static-assert.
	extern FileTypeExts FileTypeExtTable[];

	// Standard and Native implementations below.
	bool tFindDirs_Stndrd(tList<tStringItem>* dirs, tList<tFileInfo>* infos, const tString& dir, bool hidden);
	bool tFindDirs_Native(tList<tStringItem>* dirs, tList<tFileInfo>* infos, const tString& dir, bool hidden);

	bool tFindFiles_Stndrd(tList<tStringItem>* files, tList<tFileInfo>* infos, const tString& dir, const tExtensions*, bool hidden);
	bool tFindFiles_Native(tList<tStringItem>* files, tList<tFileInfo>* infos, const tString& dir, const tExtensions*, bool hidden);

	bool tFindDirsRec_Stndrd(tList<tStringItem>* dirs, tList<tFileInfo>* infos, const tString& dir, bool hidden);
	bool tFindDirsRec_Native(tList<tStringItem>* dirs, tList<tFileInfo>* infos, const tString& dir, bool hidden);

	bool tFindFilesRec_Stndrd(tList<tStringItem>* files, tList<tFileInfo>* infos, const tString& dir, const tExtensions*, bool hidden);
	bool tFindFilesRec_Native(tList<tStringItem>* files, tList<tFileInfo>* infos, const tString& dir, const tExtensions*, bool hidden);
}


inline void tSystem::tPathStd(tString& path)
{
	path.Replace('\\', '/');
	bool network = (path.Left(2) == "//");
	if (network)
	{
		path[0] = '\\'; path[1] = '\\';
		int shareSep = path.FindChar('/');
		if (shareSep != -1)
			path[shareSep] = '\\';
	}
}


inline void tSystem::tPathStdDir(tString& path)
{
	tPathStd(path);
	if (path[path.Length() - 1] != '/')
		path += "/";
}


inline void tSystem::tPathStdFile(tString& path)
{
	tPathStd(path);
	if (path[path.Length()-1] == '/')
		path.RemoveLast();
}


inline void tSystem::tPathWin(tString& path)
{
	path.Replace('/', '\\');
}


inline void tSystem::tPathWinDir(tString& path)
{
	tPathWin(path);
	if (path[path.Length() - 1] != '\\')
		path += "\\";
}


inline void tSystem::tPathWinFile(tString& path)
{
	tPathWin(path);
	if (path[path.Length()-1] == '\\')
		path.RemoveLast();
}


int tSystem::tGetFileSize(tFileHandle handle)
{
	if (!handle)
		return 0;

	tFileSeek(handle, 0, tSeekOrigin::End);
	int fileSize = tFileTell(handle);

	tFileSeek(handle, 0, tSeekOrigin::Beginning);			// Go back to beginning.
	return fileSize;
}


int tSystem::tFileSeek(tFileHandle handle, int offsetBytes, tSeekOrigin seekOrigin)
{
	int origin = SEEK_SET;
	switch (seekOrigin)
	{
		case tSeekOrigin::Beginning:
			origin = SEEK_SET;
			break;

		case tSeekOrigin::Current:
			origin = SEEK_CUR;
			break;

		case tSeekOrigin::End:
			origin = SEEK_END;
			break;
	}
	
	return fseek(handle, long(offsetBytes), origin);
}


tString tSystem::tGetFileFullName(const tString& file)
{
	tString filename(file);
	
	#if defined(PLATFORM_WINDOWS)
	tPathWin(filename);
	char ret[_MAX_PATH + 1];
	_fullpath(ret, file.Chr(), _MAX_PATH);
	tString retStr(ret);
	tPathStd(retStr);
	
	#else
	tPathStd(filename);
	char ret[PATH_MAX + 1];
	realpath(filename.Chr(), ret);
	tString retStr(ret);
	#endif

	return retStr;
}


tString tSystem::tGetFileName(const tString& file)
{
	tString retStr(file);
	tPathStd(retStr);
	return retStr.Right('/');
}


tString tSystem::tGetFileBaseName(const tString& file)
{
	tString r = tGetFileName(file);
	return r.Left('.');
}


tString tSystem::tGetSimplifiedPath(const tString& path, bool forceTreatAsDir)
{
	tString pth = path;
	tPathStd(pth);

	// We do support filenames at the end. However, if the name ends with a "." (or "..") we
	// know it is a folder and so add a trailing "/".
	if (pth[pth.Length()-1] == '.')
		pth += "/";

	if (forceTreatAsDir && (pth[pth.Length()-1] != '/'))
		pth += "/";

	if (tIsDrivePath(pth))
	{
		if ((pth[0] >= 'a') && (pth[0] <= 'z'))
			pth[0] = 'A' + (pth[0] - 'a');
	}

	// First we'll replace any "../" strings with "|".  Note that pipe indicators are not allowed
	// in filenames so we can safely use them.
	int numUps = pth.Replace("../", "|");

	// Now we can remove any "./" strings since all that's left will be up-directory markers.
	pth.Remove("./");
	if (!numUps)
		return pth;

	// We need to preserve leading '..'s so that paths like ../../Hello/There/ will work.
	int numLeading = pth.RemoveLeading("|");
	numUps -= numLeading;
	for (int nl = 0; nl < numLeading; nl++)
		pth = "../" + pth;

	tString simp;
	for (int i = 0; i < numUps; i++)
	{
		simp += pth.ExtractLeft('|');
		simp = tGetUpDir(simp);
	}

	tString res = simp + pth;
	return res;
}


tString tSystem::tGetAbsolutePath(const tString& path, const tString& basePath)
{
	tString pth(path);
	tPathStd(pth);
	if (tIsRelativePath(pth))
	{
		if (basePath.IsEmpty())
			pth = tGetCurrentDir() + pth;
		else
			pth = basePath + pth;
	}

	return tGetSimplifiedPath(pth);
}


tString tSystem::tGetRelativePath(const tString& basePath, const tString& path)
{
	// Below is the platform-agnostic implementation that doesn't use windows-only PathRelativePathTo. Only thing
	// plat-specific is no case-sensitivity on windows. First let's standardize both input paths to use the tacent
	// standard -- forward slashes and trailing-slash mandatory for directory paths.
	tString base(basePath);		tPathStdDir(base);
	tString full(path);			tPathStdDir(full);

	// Early exit on trivial paths.
	if (full.IsEmpty())			return tString();
	if (base.IsEmpty())			return full;

	// Find a common prefix and extract if from both paths.
	int lenBase = base.Length();
	int lenFull = full.Length();

	int last = 0;
	#if defined(PLATFORM_WINDOWS)
	// Case insensitive on windows.
	while (tStd::tChrlwr(base[last]) == tStd::tChrlwr(full[last]))
	#else
	while (base[last] == full[last])
	#endif
		last++;

	// It's ok to call extract with 0 if nothing was the same. It just extracts nothing.
	base.ExtractLeft(last);
	full.ExtractLeft(last);

	// Now we count how many times we need to go up from base.
	tString rel;
	int numDirsUp = base.CountChar('/');
	for (int up = 0; up < numDirsUp; up++)
		rel += "../";

	// And now just append what's left in the full path to get us there.
	rel += full;
	return rel;
}


tString tSystem::tGetLinuxPath(const tString& path, const tString& mountPoint)
{
	tString pth(path);
	tPathStd(pth);
	if (tIsAbsolutePath(pth) && (pth.Length() > 1) && (pth[1] == ':') && !mountPoint.IsEmpty())
	{
		tString mnt = mountPoint;
		tPathStdDir(mnt);

		char drive = tStd::tChrlwr(pth[0]);
		pth.ExtractLeft(2);
		pth = mnt + tString(drive) + pth;
	}
	return pth;
}


tString tSystem::tGetDir(const tString& path)
{
	tString ret(path);
	tPathStd(ret);

	// If string is empty or there is no filename on the end of the path just return what we have.
	if (ret.IsEmpty() || (ret[ret.Length()-1] == '/'))
		return ret;

	int lastSlash = ret.FindChar('/', true);

	// If there is no path, treat it as if it were a stand-alone file and return the current directory.
	if (lastSlash == -1)
		return tString("./");

	// At this point, we know there was a slash and that it isn't the last character, so
	// we know we aren't going out of bounds when we insert our string terminator after the slash.
	ret.SetLength(lastSlash+1);
	return ret;
}


tString tSystem::tGetUpDir(const tString& path, int levels)
{
	if (path.IsEmpty())
		return tString();

	tString ret(path);

	bool isNetLoc = false;
	tPathStd(ret);

	// Can't go up from here.
	if (ret == "/")
		return ret;
	if (tIsDrivePath(ret))
	{
		if (ret.Length() == 2)
			return ret + "/";
		if ((ret.Length() == 3) && (ret[2] == '/'))
			return ret;
	}

	#ifdef PLATFORM_WINDOWS
	// Are we a network location starting with two slashes?
	if ((ret.Length() >= 2) && (ret[0] == '/') && (ret[1] == '/'))
		isNetLoc = true;
	#endif

	if (isNetLoc)
	{
		ret[0] = '\\';
		ret[1] = '\\';
	}

	tString upPath = ret;
	upPath.RemoveLast();

	for (int i = 0; i < levels; i++)
	{
		int lastSlash = upPath.FindChar('/', true);

		if (isNetLoc && (upPath.CountChar('/') == 1))
			lastSlash = -1;

		if (lastSlash == -1)
			return tString();

		upPath.SetLength(lastSlash);
	}

	upPath += "/";

	if (isNetLoc)
	{
		ret[0] = '/';
		ret[1] = '/';
	}
	return upPath;
}


bool tSystem::tFileExists(const tString& file)
{
	#if defined(PLATFORM_WINDOWS)
	tString filename(file);
	tPathWin(filename);

	int length = filename.Length();
	if (filename[ length - 1 ] == ':')
		filename += "\\*";

	uint prevErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	Win32FindData fd;
	#ifdef TACENT_UTF16_API_CALLS	
	tStringUTF16 fileUTF16(filename);
	WinHandle h = FindFirstFile(fileUTF16.GetLPWSTR(), &fd);
	#else
	WinHandle h = FindFirstFile(filename.Chr(), &fd);
	#endif
	SetErrorMode(prevErrorMode);
	if (h == INVALID_HANDLE_VALUE)
		return false;

	FindClose(h);
	if (fd.dwFileAttributes & _A_SUBDIR)
		return false;
	
	return true;

	#else
	tString filename(file);
	tPathStd(filename);

	struct stat statbuf;
	return stat(filename.Chr(), &statbuf) == 0;

	#endif
}


bool tSystem::tDirExists(const tString& dir)
{
	if (dir.IsEmpty())
		return false;
		
	tString dirname = dir;
	
	#if defined(PLATFORM_WINDOWS)
	tPathWinFile(dirname);

	// Can't quite remember what the * does. Needs testing.
	int length = dirname.Length();
	if (dirname[ length - 1 ] == ':')
		dirname += "\\*";

	uint prevErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	Win32FindData fd;
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 dirUTF16(dirname);
	WinHandle h = FindFirstFile(dirUTF16.GetLPWSTR(), &fd);
	#else
	WinHandle h = FindFirstFile(dirname.Chr(), &fd);
	#endif

	SetErrorMode(prevErrorMode);
	if (h == INVALID_HANDLE_VALUE)
		return false;

	FindClose(h);
	if (fd.dwFileAttributes & _A_SUBDIR)
		return true;

	return false;

	#else
	tPathStdFile(dirname);
	std::filesystem::file_status fstat = std::filesystem::status(dirname.Chr());

	return std::filesystem::is_directory(fstat);
	#endif
}


int tSystem::tGetFileSize(const tString& file)
{
	if (file.IsEmpty())
		return 0;

	#ifdef PLATFORM_WINDOWS
	tString filename(file);
	tPathWin(filename);
	uint prevErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	Win32FindData fd;
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 fileUTF16(file);
	WinHandle h = FindFirstFile(fileUTF16.GetLPWSTR(), &fd);
	#else
	WinHandle h = FindFirstFile(filename.Chr(), &fd);
	#endif

	// If file doesn't exist, h will be invalid.
	if (h == INVALID_HANDLE_VALUE)
	{
		SetErrorMode(prevErrorMode);
		return 0;
	}

	FindClose(h);
	SetErrorMode(prevErrorMode);
	return fd.nFileSizeLow;
	#else

	tFileHandle handle = tOpenFile(file, "rb");
	int size = tGetFileSize(handle);
	tCloseFile(handle);

	return size;
	#endif
}


bool tSystem::tIsReadOnly(const tString& path)
{
	tString pathname(path);

	#if defined(PLATFORM_WINDOWS)
	tPathWinFile(pathname);

	// The docs for this should be clearer!  GetFileAttributes returns INVALID_FILE_ATTRIBUTES if it
	// fails.  Rather dangerously, and undocumented, INVALID_FILE_ATTRIBUTES has a value of 0xFFFFFFFF.
	// This means that all attribute are apparently true!  This is very lame.  Thank goodness there aren't
	// 32 possible attributes, or there could be real problems.  Too bad it didn't just return 0 on error...
	// especially since they specifically have a FILE_ATTRIBUTES_NORMAL flag that is non-zero!
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 path16(pathname);
	ulong attribs = GetFileAttributes(path16.GetLPWSTR());
	#else
	ulong attribs = GetFileAttributes(pathname.Chr());
	#endif
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	return (attribs & FILE_ATTRIBUTE_READONLY) ? true : false;

	#else
	tPathStd(pathname);

	struct stat st;
	int errCode = stat(pathname.Chr(), &st);
	if (errCode != 0)
		return false;

	bool w = (st.st_mode & S_IWUSR) ? true : false;
	bool r = (st.st_mode & S_IRUSR) ? true : false;
	return r && !w;

	#endif
}


bool tSystem::tSetReadOnly(const tString& path, bool readOnly)
{
	tString pathname(path);

	#if defined(PLATFORM_WINDOWS)
	tPathWinFile(pathname);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 path16(pathname);
	ulong attribs = GetFileAttributes(path16.GetLPWSTR());
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;
	if (!(attribs & FILE_ATTRIBUTE_READONLY) && readOnly)
		SetFileAttributes(path16.GetLPWSTR(), attribs | FILE_ATTRIBUTE_READONLY);
	else if ((attribs & FILE_ATTRIBUTE_READONLY) && !readOnly)
		SetFileAttributes(path16.GetLPWSTR(), attribs & ~FILE_ATTRIBUTE_READONLY);
	attribs = GetFileAttributes(path16.GetLPWSTR());

	#else
	ulong attribs = GetFileAttributes(pathname.Chr());
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;
	if (!(attribs & FILE_ATTRIBUTE_READONLY) && readOnly)
		SetFileAttributes(pathname.Chr(), attribs | FILE_ATTRIBUTE_READONLY);
	else if ((attribs & FILE_ATTRIBUTE_READONLY) && !readOnly)
		SetFileAttributes(pathname.Chr(), attribs & ~FILE_ATTRIBUTE_READONLY);
	attribs = GetFileAttributes(pathname.Chr());
	#endif

	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	if (!!(attribs & FILE_ATTRIBUTE_READONLY) == readOnly)
		return true;

	return false;

	#else
	tPathStd(pathname);
	
	struct stat st;
	int errCode = stat(pathname.Chr(), &st);
	if (errCode != 0)
		return false;
	
	uint32 permBits = st.st_mode;

	// Set user R and clear user w. Leave rest unchanged.
	permBits |= S_IRUSR;
	permBits &= ~S_IWUSR;
	errCode = chmod(pathname.Chr(), permBits);
	
	return (errCode == 0);

	#endif
}


bool tSystem::tIsHidden(const tString& path)
{
	if (path.IsEmpty())
		return false;

	#if defined(PLATFORM_LINUX)
	// In Linux it's all based on whether the filename starts with a dot. We ignore files called "." or ".."
	if (tIsFile(path))
	{
		tString fileName = tGetFileName(path);
		if ((fileName != ".") && (fileName != "..") && (fileName[0] == '.'))
			return true;
	}
	else
	{
		tString dirName = path;
		dirName.RemoveLast();
		dirName = tGetFileName(dirName);
		if ((dirName != ".") && (dirName != "..") && (dirName[0] == '.'))
			return true;
	}
	return false;

	#elif defined(PLATFORM_WINDOWS)
	// In windows it's all based on the attribute.
	tString pathName(path);
	tPathWinFile(pathName);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 pathName16(pathName);
	ulong attribs = GetFileAttributes(pathName16.GetLPWSTR());
	#else
	ulong attribs = GetFileAttributes(pathName.Chr());
	#endif
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	return (attribs & FILE_ATTRIBUTE_HIDDEN) ? true : false;

	#else
	return false;

	#endif
}


#if defined(PLATFORM_WINDOWS)
bool tSystem::tSetHidden(const tString& path, bool hidden)
{
	tString pth(path);
	tPathWinFile(pth);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 pth16(pth);
	ulong attribs = GetFileAttributes(pth16.GetLPWSTR());
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;
	if (!(attribs & FILE_ATTRIBUTE_HIDDEN) && hidden)
		SetFileAttributes(pth16.GetLPWSTR(), attribs | FILE_ATTRIBUTE_HIDDEN);
	else if ((attribs & FILE_ATTRIBUTE_HIDDEN) && !hidden)
		SetFileAttributes(pth16.GetLPWSTR(), attribs & ~FILE_ATTRIBUTE_HIDDEN);
	attribs = GetFileAttributes(pth16.GetLPWSTR());

	#else
	ulong attribs = GetFileAttributes(pth.Chr());
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;
	if (!(attribs & FILE_ATTRIBUTE_HIDDEN) && hidden)
		SetFileAttributes(pth.Chr(), attribs | FILE_ATTRIBUTE_HIDDEN);
	else if ((attribs & FILE_ATTRIBUTE_HIDDEN) && !hidden)
		SetFileAttributes(pth.Chr(), attribs & ~FILE_ATTRIBUTE_HIDDEN);
	attribs = GetFileAttributes(pth.Chr());
	#endif

	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	if (!!(attribs & FILE_ATTRIBUTE_HIDDEN) == hidden)
		return true;

	return false;
}


bool tSystem::tIsSystem(const tString& file)
{
	tString filename(file);
	tPathWinFile(filename);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 filename16(filename);
	ulong attribs = GetFileAttributes(filename16.GetLPWSTR());
	#else
	ulong attribs = GetFileAttributes(filename.Chr());
	#endif

	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	return (attribs & FILE_ATTRIBUTE_SYSTEM) ? true : false;
}


bool tSystem::tSetSystem(const tString& file, bool system)
{
	tString filename(file);
	tPathWinFile(filename);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 file16(filename);
	ulong attribs = GetFileAttributes(file16.GetLPWSTR());
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;
	if (!(attribs & FILE_ATTRIBUTE_SYSTEM) && system)
		SetFileAttributes(file16.GetLPWSTR(), attribs | FILE_ATTRIBUTE_SYSTEM);
	else if ((attribs & FILE_ATTRIBUTE_SYSTEM) && !system)
		SetFileAttributes(file16.GetLPWSTR(), attribs & ~FILE_ATTRIBUTE_SYSTEM);
	attribs = GetFileAttributes(file16.GetLPWSTR());

	#else
	ulong attribs = GetFileAttributes(filename.Chr());
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;
	if (!(attribs & FILE_ATTRIBUTE_SYSTEM) && system)
		SetFileAttributes(filename.Chr(), attribs | FILE_ATTRIBUTE_SYSTEM);
	else if ((attribs & FILE_ATTRIBUTE_SYSTEM) && !system)
		SetFileAttributes(filename.Chr(), attribs & ~FILE_ATTRIBUTE_SYSTEM);
	attribs = GetFileAttributes(filename.Chr());
	#endif

	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	if (!!(attribs & FILE_ATTRIBUTE_SYSTEM) == system)
		return true;

	return false;
}


bool tSystem::tDriveExists(const tString& driveName)
{
	tString drive = driveName;
	drive.ToUpper();

	char driveLet = drive[0];
	if ((driveLet > 'Z') || (driveLet < 'A'))
		return false;

	ulong driveBits = GetLogicalDrives();
	if (driveBits & (0x00000001 << (driveLet-'A')))
		return true;

	return false;
}
#endif // PLATFORM_WINDOWS


bool tSystem::tIsFileNewer(const tString& filea, const tString& fileb)
{
	#if defined(PLATFORM_WINDOWS)
	tString fileA(filea);
	tPathWin(fileA);

	tString fileB(fileb);
	tPathWin(fileB);

	Win32FindData fd;
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 fileA16(fileA);
	WinHandle h = FindFirstFile(fileA16.GetLPWSTR(), &fd);
	#else
	WinHandle h = FindFirstFile(fileA.Chr(), &fd);
	#endif
	if (h == INVALID_HANDLE_VALUE)
		throw tFileError("Invalid file handle for file: " + fileA);

	FileTime timeA = fd.ftLastWriteTime;
	FindClose(h);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 fileB16(fileB);
	h = FindFirstFile(fileB16.GetLPWSTR(), &fd);
	#else
	h = FindFirstFile(fileB.Chr(), &fd);
	#endif
	if (h == INVALID_HANDLE_VALUE)
		throw tFileError("Invalid file handle for file: " + fileB);

	FileTime timeB = fd.ftLastWriteTime;
	FindClose(h);

	if (CompareFileTime(&timeA, &timeB) > 0)
		return true;

	#elif defined(PLAYFORM_LINUX)
	tToDo("Implement tISFileNewer.");

	#endif
	return false;
}


bool tSystem::tFilesIdentical(const tString& fileA, const tString& fileB)
{
	auto localCloseFiles = [](tFileHandle a, tFileHandle b)
	{
		tCloseFile(a);
		tCloseFile(b);
	};

	tFileHandle fa = tOpenFile(fileA, "rb");
	tFileHandle fb = tOpenFile(fileB, "rb");
	if (!fa || !fb)
	{
		localCloseFiles(fa, fb);
		return false;
	}

	int faSize = tGetFileSize(fa);
	int fbSize = tGetFileSize(fb);
	if (faSize != fbSize)
	{
		localCloseFiles(fa, fb);
		return false;
	}

	uint8* bufA = new uint8[faSize];
	uint8* bufB = new uint8[fbSize];

	int numReadA = tReadFile(fa, bufA, faSize);
	int numReadB = tReadFile(fb, bufB, fbSize);
	tAssert((faSize + fbSize) == (numReadA + numReadB));

	for (int i = 0; i < faSize; i++)
	{
		if (bufA[i] != bufB[i])
		{
			localCloseFiles(fa, fb);
			delete[] bufA;
			delete[] bufB;
			return false;
		}
	}

	localCloseFiles(fa, fb);
	delete[] bufA;
	delete[] bufB;
	return true;
}


bool tSystem::tCopyFile(const tString& destFile, const tString& srcFile, bool overWriteReadOnly)
{
	#if defined(PLATFORM_WINDOWS)

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 src16(srcFile);
	tStringUTF16 dest16(destFile);
	int success = ::CopyFile(src16.GetLPWSTR(), dest16.GetLPWSTR(), 0);

	#else
	int success = ::CopyFile(srcFile.Chr(), destFile.Chr(), 0);
	#endif

	if (!success && overWriteReadOnly)
	{
		tSetReadOnly(destFile, false);
		#ifdef TACENT_UTF16_API_CALLS
		success = ::CopyFile(src16.GetLPWSTR(), dest16.GetLPWSTR(), 0);
		#else
		success = ::CopyFile(srcFile.Chr(), destFile.Chr(), 0);
		#endif
	}
	return success ? true : false;

	#else
	std::filesystem::path pathFrom(srcFile.Chr());
	std::filesystem::path pathTo(destFile.Chr());
	bool success = std::filesystem::copy_file(pathFrom, pathTo);
	if (!success && overWriteReadOnly)
	{
		tSetReadOnly(destFile, false);
		success = std::filesystem::copy_file(pathFrom, pathTo);
	}
		
	return success;

	#endif
}


bool tSystem::tRenameFile(const tString& dir, const tString& oldPathName, const tString& newPathName)
{
	#if defined(PLATFORM_WINDOWS)
	tString fullOldName = dir + oldPathName;
	tPathWin(fullOldName);

	tString fullNewName = dir + newPathName;
	tPathWin(fullNewName);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 fullOldName16(fullOldName);
	tStringUTF16 fullNewName16(fullNewName);
	int success = ::MoveFile(fullOldName16.GetLPWSTR(), fullNewName16.GetLPWSTR());

	#else
	int success = ::MoveFile(fullOldName.Chr(), fullNewName.Chr());
	#endif
	return success ? true : false;

	#else
	tString fullOldName = dir + oldPathName;
	tPathStd(fullOldName);
	std::filesystem::path oldp(fullOldName.Chr());

	tString fullNewName = dir + newPathName;
	tPathStd(fullNewName);
	std::filesystem::path newp(fullNewName.Chr());

	std::error_code ec;
	std::filesystem::rename(oldp, newp, ec);
	return !bool(ec);

	#endif
}


bool tSystem::tCreateFile(const tString& file)
{
	tFileHandle f = tOpenFile(file.Chr(), "wt");
	if (!f)
		return false;

	tCloseFile(f);
	return true;
}


bool tSystem::tCreateFile(const tString& file, const tString& contents)
{
	uint32 len = contents.Length();
	return tCreateFile(file, (uint8*)contents.Chr(), len);
}


bool tSystem::tCreateFile(const tString& file, uint8* data, int length)
{
	tFileHandle dst = tOpenFile(file.Chr(), "wb");
	if (!dst)
		return false;

	// Sometimes this needs to be done, for some mysterious reason.
	tFileSeek(dst, 0, tSeekOrigin::Beginning);

	// Write data and close file.
	int numWritten = tWriteFile(dst, data, length);
	tCloseFile(dst);

	// Make sure it was created and an appropriate amount of bytes were written.
	bool verify = tFileExists(file);
	return verify && (numWritten >= length);
}


bool tSystem::tCreateFile(const tString& file, char8_t* data, int length, bool writeBOM)
{
	tFileHandle dst = tOpenFile(file.Chr(), "wb");
	if (!dst)
		return false;
	tFileSeek(dst, 0, tSeekOrigin::Beginning);
	if (writeBOM)
	{
		char8_t bom[4];
		int bomLen = tStd::tUTF8c(bom, tStd::cCodepoint_BOM);
		tAssert(bomLen == 3);
		int bomWritten = tWriteFile(dst, bom, 3);
		if (bomWritten != bomLen)
		{
			tCloseFile(dst);
			return false;
		}
	}

	// Write data and close file.
	int numWritten = tWriteFile(dst, data, length);
	tCloseFile(dst);

	// Make sure it was created and an appropriate amount of bytes were written.
	bool verify = tFileExists(file);
	return verify && (numWritten >= length);
}


bool tSystem::tCreateFile(const tString& file, char16_t* data, int length, bool writeBOM)
{
	tFileHandle dst = tOpenFile(file.Chr(), "wb");
	if (!dst)
		return false;
	tFileSeek(dst, 0, tSeekOrigin::Beginning);
	if (writeBOM)
	{
		char16_t bom = char16_t(tStd::cCodepoint_BOM);
		int bomWritten = tWriteFile(dst, &bom, 1);
		if (bomWritten != 1)
		{
			tCloseFile(dst);
			return false;
		}
	}
	// Write data and close file.
	int numWritten = tWriteFile(dst, data, length);
	tCloseFile(dst);

	// Make sure it was created and an appropriate amount of bytes were written.
	bool verify = tFileExists(file);
	return verify && (numWritten >= length);
}


bool tSystem::tCreateFile(const tString& file, char32_t* data, int length, bool writeBOM)
{
	tFileHandle dst = tOpenFile(file.Chr(), "wb");
	if (!dst)
		return false;
	tFileSeek(dst, 0, tSeekOrigin::Beginning);
	if (writeBOM)
	{
		int bomWritten = tWriteFile(dst, &tStd::cCodepoint_BOM, 1);
		if (bomWritten != 1)
		{
			tCloseFile(dst);
			return false;
		}
	}

	// Write data and close file.
	int numWritten = tWriteFile(dst, data, length);
	tCloseFile(dst);

	// Make sure it was created and an appropriate amount of bytes were written.
	bool verify = tFileExists(file);
	return verify && (numWritten >= length);
}


bool tSystem::tDeleteFile(const tString& file, bool deleteReadOnly, bool useRecycleBin)
{
	#ifdef PLATFORM_WINDOWS
	tString filename(file);
	tPathWin(filename);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 filename16(filename);
	if (deleteReadOnly)
		SetFileAttributes(filename16.GetLPWSTR(), FILE_ATTRIBUTE_NORMAL);
	#else
	if (deleteReadOnly)
		SetFileAttributes(filename.Chr(), FILE_ATTRIBUTE_NORMAL);
	#endif

	if (!useRecycleBin)
	{
		#ifdef TACENT_UTF16_API_CALLS
		if (DeleteFile(filename16.GetLPWSTR()))
		#else
		if (DeleteFile(filename.Chr()))
		#endif
			return true;
		else
			return false;
	}
	else
	{
		tString filenamePlusChar = file + "Z";
		#ifdef TACENT_UTF16_API_CALLS
		tStringUTF16 filenameDoubleNull16(filenamePlusChar);
		*(filenameDoubleNull16.Units() + filenameDoubleNull16.Length() - 1) = 0;
		#else
		tString filenameDoubleNull(filenamePlusChar);

		// This is ok. tString allows multiple nulls.
		filenameDoubleNull[filenameDoubleNull.Length()-1] = '\0';
		#endif

		SHFILEOPSTRUCT operation;
		tStd::tMemset(&operation, 0, sizeof(operation));
		operation.wFunc = FO_DELETE;
		#ifdef TACENT_UTF16_API_CALLS
		operation.pFrom = filenameDoubleNull16.GetLPWSTR();
		#else
		operation.pFrom = filenameDoubleNull.Chr();
		#endif
		operation.fFlags = FOF_ALLOWUNDO | FOF_NO_UI | FOF_NORECURSION;
		int errCode = SHFileOperation(&operation);
		return errCode ? false : true;
	}

	return true;

	#else
	if (!deleteReadOnly && tIsReadOnly(file))
		return true;

	std::filesystem::path p(file.Chr());

	if (useRecycleBin)
	{
		tString homeDir = tGetHomeDir();
		tString recycleDir = homeDir + ".local/share/Trash/files/";
		if (tDirExists(recycleDir))
		{
			tString toFile = recycleDir + tGetFileName(file);
			std::filesystem::path toPath(toFile.Chr());
			std::error_code ec;
			std::filesystem::rename(p, toPath, ec);
			return ec ? false : true;
		}

		return false;
	}

	return std::filesystem::remove(p);
	#endif
}


uint8* tSystem::tLoadFile(const tString& file, uint8* buffer, int* fileSize, bool appendEOF)
{
	tFileHandle f = tOpenFile(file.Chr(), "rb");
	if (!f)
	{
		if (fileSize)
			*fileSize = 0;
		return nullptr;
	}

	int size = tGetFileSize(f);
	if (fileSize)
		*fileSize = size;

	if (size == 0)
	{
		// It is perfectly valid to load a file with no data (0 bytes big).
		// In this case we always return 0 even if a non-zero buffer was passed in.
		// The fileSize member will already be set if necessary.
		tCloseFile(f);
		return nullptr;
	}

	bool bufferAllocatedHere = false;
	if (!buffer)
	{
		int bufSize = appendEOF ? size+1 : size;
		buffer = new uint8[bufSize];
		bufferAllocatedHere = true;
	}

	int numRead = tReadFile(f, buffer, size);			// Load the entire thing into memory.
	tAssert(numRead == size);

	if (appendEOF)
		buffer[numRead] = EOF;

	tCloseFile(f);
	return buffer;
}


bool tSystem::tLoadFile(const tString& file, tString& dst, char convertZeroesTo)
{
	if (!tFileExists(file))
	{
		dst.Clear();
		return false;
	}

	int filesize = tGetFileSize(file);
	if (filesize == 0)
	{
		dst.Clear();
		return true;
	}

	dst.SetLength(filesize, false);
	uint8* check = tLoadFile(file, (uint8*)dst.Text());
	if ((check != (uint8*)dst.Text()) || !check)
	{
		dst.Clear();
		return false;
	}

	if (convertZeroesTo != '\0')
	{
		for (int i = 0; i < filesize; i++)
			if (dst[i] == '\0')
				dst[i] = convertZeroesTo;
	}

	return true;
}


uint8* tSystem::tLoadFileHead(const tString& file, int& bytesToRead, uint8* buffer)
{
	tFileHandle f = tOpenFile(file, "rb");
	if (!f)
	{
		bytesToRead = 0;
		return buffer;
	}

	int size = tGetFileSize(f);
	if (!size)
	{
		tCloseFile(f);
		bytesToRead = 0;
		return buffer;
	}

	bytesToRead = (size < bytesToRead) ? size : bytesToRead;

	bool bufferAllocatedHere = false;
	if (!buffer)
	{
		buffer = new uint8[bytesToRead];
		bufferAllocatedHere = true;
	}

	// Load the first bytesToRead into memory.  We assume complete failure if the
	// number we asked for was not returned.
	int numRead = tReadFile(f, buffer, bytesToRead);
	if (numRead != bytesToRead)
	{
		if (bufferAllocatedHere)
		{
			delete[] buffer;
			buffer = 0;
		}

		tCloseFile(f);
		bytesToRead = 0;
		return buffer;
	}

	tCloseFile(f);
	return buffer;
}


tString tSystem::tGetHomeDir()
{
	tString home;
	#if defined(PLATFORM_LINUX)
	const char* homeDir = getenv("HOME");
	if (!homeDir)
		homeDir = getpwuid(getuid())->pw_dir;		
	if (!homeDir)
		return home;
	home.Set(homeDir);
	if (home[home.Length()-1] != '/')
		home += '/';

	#elif defined(PLATFORM_WINDOWS)
	wchar_t* pathBuffer = nullptr;
	hResult result = SHGetKnownFolderPath(FOLDERID_Profile, 0, 0, &pathBuffer);
	if ((result != S_OK) || !pathBuffer)
		return home;
	home.Set((char16_t*)pathBuffer);
	CoTaskMemFree(pathBuffer);
	tPathStdDir(home);
	#endif

	return home;
}


tString tSystem::tGetProgramDir()
{
	#if defined(PLATFORM_WINDOWS)

	#ifdef TACENT_UTF16_API_CALLS
	// No need to add one here. Reserving space does it for us.
	tStringUTF16 result16(MAX_PATH);
	// Except for windows XP (which I don't care about TBH), the result is always null-terminated.
	ulong l = GetModuleFileName(0, result16.GetLPWSTR(), MAX_PATH);
	result16.SetLength( tStd::tStrlen(result16.Chars()) );
	tString result(result16);
	#else
	char resBuf[MAX_PATH+1];
	ulong l = GetModuleFileName(0, resBuf, MAX_PATH);
	tString result(resBuf);
	#endif

	tPathStd(result);
	int bi = result.FindChar('/', true);
	tAssert(bi != -1);

	result.SetLength(bi + 1);
	return result;

	#elif defined(PLATFORM_LINUX)
	char resBuf[PATH_MAX+1];
	int numWritten = readlink("/proc/self/exe", resBuf, PATH_MAX);
	if ((numWritten == -1) || (numWritten > PATH_MAX))
		return tString();
	resBuf[numWritten] = '\0';

	tString result(resBuf);
	int bi = result.FindChar('/', true);
	tAssert(bi != -1);
	result.SetLength(bi + 1);
	return result;

	#else
	return tString();

	#endif
}


tString tSystem::tGetProgramPath()
{
	#if defined(PLATFORM_WINDOWS)

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 result16(MAX_PATH);
	ulong l = GetModuleFileName(0, result16.GetLPWSTR(), MAX_PATH);
	result16.SetLength( tStd::tStrlen(result16.Chars()) );
	tString result(result16);

	#else
	char resBuf[MAX_PATH+1];
	ulong l = GetModuleFileName(0, resBuf, MAX_PATH);
	tString result(resBuf);
	#endif

	tPathStd(result);
	return result;

	#elif defined(PLATFORM_LINUX)
	char resBuf[PATH_MAX+1];
	int numWritten = readlink("/proc/self/exe", resBuf, PATH_MAX);
	if ((numWritten == -1) || (numWritten > PATH_MAX))
		return tString();
	resBuf[numWritten] = '\0';
	tString result(resBuf);
	return result;

	#else
	return tString();

	#endif
}


tString tSystem::tGetCurrentDir()
{
	#ifdef PLATFORM_WINDOWS

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 r16(MAX_PATH);
	GetCurrentDirectory(MAX_PATH, r16.GetLPWSTR());
	r16.SetLength( tStd::tStrlen(r16.Chars()) );
	tString r(r16);

	#else
	char rbuf[MAX_PATH+1];
	GetCurrentDirectory(MAX_PATH, rbuf);
	tString r(rbuf);
	#endif

	#else
	char rbuf[PATH_MAX+1];
	getcwd(rbuf, PATH_MAX);
	tString r(rbuf);
	#endif

	tPathStdDir(r);
	return r;
}


bool tSystem::tSetCurrentDir(const tString& directory)
{
	if (directory.IsEmpty())
		return false;

	tString dir = directory;

	#ifdef PLATFORM_WINDOWS
	tPathWin(dir);
	tString cd;

	// "." and ".." get left alone.
	if ((dir == ".") || (dir == ".."))
	{
		cd = dir;
	}
	else
	{
		if (dir.FindChar(':') != -1)
			cd = dir;
		else
			cd = ".\\" + dir;

		if (cd[cd.Length() - 1] != '\\')
			cd += "\\";
	}

	// So there is no dialog asking user to insert a floppy.
	uint prevErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 cd16(cd);
	int success = SetCurrentDirectory(cd16.GetLPWSTR());
	#else
	int success = SetCurrentDirectory(cd.Chr());
	#endif
	SetErrorMode(prevErrorMode);

	return success ? true : false;

	#else
	tPathStd(dir);
	int errCode = chdir(dir.Chr());
	return (errCode == 0);

	#endif
}


#if defined(PLATFORM_WINDOWS)
tString tSystem::tGetWindowsDir()
{
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 windir16(MAX_PATH);
	GetWindowsDirectory(windir16.GetLPWSTR(), MAX_PATH);
	windir16.SetLength( tStd::tStrlen(windir16.Chars()) );
	tString windir(windir16);
	#else
	char windirbuf[MAX_PATH+1];
	GetWindowsDirectory(windirbuf, MAX_PATH);
	tString windir(windirbuf);
	#endif

	tPathStdDir(windir);
	return windir;
}


tString tSystem::tGetSystemDir()
{
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 sysdir16(MAX_PATH);
	GetSystemDirectory(sysdir16.GetLPWSTR(), MAX_PATH);
	sysdir16.SetLength( tStd::tStrlen(sysdir16.Chars()) );
	tString sysdir(sysdir16);
	#else
	char sysdirbuf[MAX_PATH+1];
	GetSystemDirectory(sysdirbuf, MAX_PATH);
	tString sysdir(sysdirbuf);
	#endif

	tPathStdDir(sysdir);
	return sysdir;
}


tString tSystem::tGetDesktopDir()
{
	tString desktop;
	wchar_t* pathBuffer = nullptr;
	hResult result = SHGetKnownFolderPath(FOLDERID_Desktop, 0, 0, &pathBuffer);
	if ((result != S_OK) || !pathBuffer)
		return desktop;

	desktop.Set((char16_t*)pathBuffer);
	CoTaskMemFree(pathBuffer);

	tPathStdDir(desktop);
	return desktop;
}


void tSystem::tGetDrives(tList<tStringItem>& drives)
{
	ulong ad = GetLogicalDrives();

	char driveLet = 'A';
	for (int i = 0; i < 26; i++)
	{
		if (ad & 0x00000001)
		{
			tStringItem* drive = new tStringItem(driveLet);
			*drive += ":";
			drives.Append(drive);
		}

		driveLet++;
		ad = ad >> 1;
	}
}


bool tSystem::tGetDriveInfo(tDriveInfo& driveInfo, const tString& drive, bool getDisplayName, bool getVolumeAndSerial)
{
	tString driveRoot = drive;
	driveRoot.ToUpper();

	if (driveRoot.Length() == 1)							// Assume string was of form "C"
		driveRoot += ":\\";
	else if (driveRoot.Length() == 2)						// Assume string was of form "C:"
		driveRoot += "\\";
	else													// Assume string was of form "C:/" or "C:\"
		tPathWin(driveRoot);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 driveRoot16(driveRoot);
	uint driveType = GetDriveType(driveRoot16.GetLPWSTR());
	#else
	uint driveType = GetDriveType(driveRoot.Chr());
	#endif
	switch (driveType)
	{
		case DRIVE_NO_ROOT_DIR:
			return false;

		case DRIVE_REMOVABLE:
			if ((driveRoot == "A:\\") || (driveRoot == "B:\\"))
				driveInfo.DriveType = tDriveType::Floppy;
			else
				driveInfo.DriveType = tDriveType::Removable;
			break;

		case DRIVE_FIXED:
			driveInfo.DriveType = tDriveType::HardDisk;
			break;

		case DRIVE_REMOTE:
			driveInfo.DriveType = tDriveType::Network;
			break;

		case DRIVE_CDROM:
			driveInfo.DriveType = tDriveType::Optical;
			break;

		case DRIVE_RAMDISK:
			driveInfo.DriveType = tDriveType::RamDisk;
			break;

		case DRIVE_UNKNOWN:
		default:
			driveInfo.DriveType = tDriveType::Unknown;
			break;
	}

	if (getDisplayName)
	{
		// Here we try getting the name by using the shell api.  It should give the
		// same name as seen by windows explorer.
		SHFILEINFO fileInfo;
		fileInfo.szDisplayName[0] = '\0';
		SHGetFileInfo
		(
			#ifdef TACENT_UTF16_API_CALLS
			driveRoot16.GetLPWSTR(),
			#else
			driveRoot.Chr(),
			#endif
			0,
			&fileInfo,
			sizeof(SHFILEINFO),
			SHGFI_DISPLAYNAME
		);
		#ifdef TACENT_UTF16_API_CALLS
		driveInfo.DisplayName.SetUTF16((char16_t*)fileInfo.szDisplayName);
		#else
		driveInfo.DisplayName = fileInfo.szDisplayName;
		#endif
	}

	if (getVolumeAndSerial)
	{
		#ifdef TACENT_UTF16_API_CALLS
		tStringUTF16 volumeInfoName(256);
		#else
		tString volumeInfoName(256);
		#endif
		ulong componentLength = 0;
		ulong flags = 0;
		ulong serial = 0;
		int success = GetVolumeInformation
		(
			#ifdef TACENT_UTF16_API_CALLS
			driveRoot16.GetLPWSTR(),
			volumeInfoName.GetLPWSTR(),
			#else
			driveRoot.Chr(),
			volumeInfoName.Txt(),
			#endif
			256,
			&serial,
			&componentLength,
			&flags,
			0,							// File system name not needed.
			0							// Buffer for system name is 0 long.
		);

		#ifdef TACENT_UTF16_API_CALLS
		driveInfo.VolumeName.SetUTF16(volumeInfoName.Units());
		#else
		driveInfo.VolumeName.Set(volumeInfoName.Units());
		#endif
		driveInfo.SerialNumber = serial;
	}

	return true;
}


bool tSystem::tSetVolumeName(const tString& drive, const tString& newVolumeName)
{
	tString driveRoot = drive;
	driveRoot.ToUpper();

	if (driveRoot.Length() == 1)			// Assume string was of form "C"
		driveRoot += ":\\";
	else if (driveRoot.Length() == 2)		// Assume string was of form "C:"
		driveRoot += "\\";
	else									// Assume string was of form "C:/" or "C:\"
		tPathWin(driveRoot);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 driveRoot16(driveRoot);
	tStringUTF16 newVolumeName16(newVolumeName);
	int success = SetVolumeLabel(driveRoot16.GetLPWSTR(), newVolumeName16.GetLPWSTR());
	#else
	int success = SetVolumeLabel(driveRoot.Chr(), newVolumeName.Chr());
	#endif

	return success ? true : false;
}


namespace tWindowsShares
{
	tString tGetDisplayName(LPITEMIDLIST pidl, IShellFolder*, DWORD type);
	void tEnumerateRec(tSystem::tNetworkShareResult&, IShellFolder*, int levels, bool retrieveMachinesWithNoShares);
	LPMALLOC Malloc;
}


tString tWindowsShares::tGetDisplayName(LPITEMIDLIST pidl, IShellFolder* folderInterface, DWORD type)
{
	STRRET strRet;

	// Request the string as a char* although Windows will likely ignore the request.
	strRet.uType = STRRET_CSTR;

	// Call GetDisplayNameOf() to fill in the STRRET structure.
	HRESULT hr = folderInterface->GetDisplayNameOf(pidl, type, &strRet);
	if (!SUCCEEDED(hr))
		return tString();

	// Extract the string based on the value of the uType member of STRRET.
	switch (strRet.uType)
	{
		case STRRET_CSTR:
			return tString(strRet.cStr);

		case STRRET_WSTR:
			return tString((char16_t*)strRet.pOleStr);
		
		case STRRET_OFFSET :
			return tString(((char*)pidl) + strRet.uOffset);
	}
	return tString();
}


void tWindowsShares::tEnumerateRec(tSystem::tNetworkShareResult& shareResults, IShellFolder* folderInterface, int depth, bool retrieveMachinesWithNoShares)
{
	LPITEMIDLIST pidl;
	LPENUMIDLIST enumList;
	DWORD enumFlags = SHCONTF_FOLDERS;				// Could add | SHCONTF_NONFOLDERS to enumerate non-folders.
	DWORD result = folderInterface->EnumObjects(0, enumFlags, &enumList);
	if (result != NOERROR)
    	return;

	tString displayName;
	result = enumList->Next(1, &pidl, 0);			// Get the pidl for the first item in the folder.
	while (result != S_FALSE)
	{
		int currentDepth = depth;
		if (result != NOERROR)
			break;

		// There are a few possible ways to display the result. We use ForParsing. Examlpes:
		// SHGDN_NORMAL			-> ShareName (\\MACHINENAME)
		// SHGDN_INFOLDER		-> ShareName
		// SHGDN_FORPARSING		-> \\MACHINENAME\ShareName
		displayName = tGetDisplayName(pidl, folderInterface, SHGDN_FORPARSING);
		if (!displayName.IsEmpty())
		{
			tString padStr;
			displayName = padStr + displayName;

			// If we're at a leaf we need to add our result.
			if ((currentDepth == 1) || retrieveMachinesWithNoShares)
			{
				shareResults.ShareNames.Append(new tStringItem(displayName));
				shareResults.NumSharesFound++;
			}
		}

		currentDepth--;

		// See if this shell item is a folder.
		DWORD attr = SFGAO_FOLDER;
		folderInterface->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &attr);

		// Terminate recursion when depth reaches 0.
		if ((currentDepth > 0) && (attr & SFGAO_FOLDER) == SFGAO_FOLDER)
		{
			LPSHELLFOLDER shellFolder;

			// Get the IShellFolder for the pidl.
			int res = folderInterface->BindToObject(pidl, 0, IID_IShellFolder, (void**)&shellFolder);
			if (res == NOERROR)
			{
				tEnumerateRec(shareResults, shellFolder, currentDepth, retrieveMachinesWithNoShares);	// Recurse.
				shellFolder->Release();
			}
		}
		Malloc->Free(pidl);
		result = enumList->Next(1, &pidl, 0);				// Next pidl.
	}

	enumList->Release();
}


int tSystem::tGetNetworkShares(tNetworkShareResult& shareResults, bool retrieveMachinesWithNoShares)
{
	shareResults.Clear();
	SHGetMalloc(&tWindowsShares::Malloc);

	// To enumerate everything you could use the desktop folder. In our case we only want the shares.
	// To do this we use SHGetSpecialFolderLocation with the special class ID specifier CSIDL_NETWORK.
	// LPSHELLFOLDER desktopFolder; SHGetDesktopFolder(&desktopFolder);
	LPITEMIDLIST pidlSystem = nullptr;
	HRESULT hr = SHGetSpecialFolderLocation(0, CSIDL_NETWORK, &pidlSystem);
    if (!SUCCEEDED(hr))
	{
		shareResults.RequestComplete = true;
		return 0;
	}

	IShellFolder* shellFolder = nullptr;
    LPCITEMIDLIST pidlRelative = nullptr;
	
	hr = SHBindToObject(0, pidlSystem, 0, IID_IShellFolder, (void**)&shellFolder);
    if (!SUCCEEDED(hr))
	{
		CoTaskMemFree(pidlSystem);
		shareResults.RequestComplete = true;
		return 0;
	}
	tAssert(shellFolder && pidlSystem);

	// To get network shares we need to go to a depth of 2. The first level contains the machine names.
	// The second level contains the share names.
	const int depth = 2;
	tWindowsShares::tEnumerateRec(shareResults, shellFolder, depth, retrieveMachinesWithNoShares);

	// Free all used memory.
	// Free the IShellFolder for the special folder location (CSIDL_NETWORK).
	// For desktop it would be desktopFolder->Release();
	shellFolder->Release();
	CoTaskMemFree(pidlSystem);
	tWindowsShares::Malloc->Release();

	shareResults.RequestComplete = true;
	return shareResults.NumSharesFound;
}


void tSystem::tExplodeShareName(tList<tStringItem>& exploded, const tString& shareName)
{
	exploded.Empty();
	tString share(shareName);
	share.ExtractLeft("\\\\");
	tStd::tExplode(exploded, share, '\\');
}
#endif // PLATFORM_WINDOWS


// When more than one extension maps to the same filetype (like jpg and jpeg), always put the more common extension
// first in the extensions array. It is important not to specify the array size here so we can static-assert that we
// have entered the correct number of entries.
tSystem::FileTypeExts tSystem::FileTypeExtTable[] =
{
//	Extensions							Filetype
	{ "tga" },							// TGA
	{ "bmp" },							// BMP
	{ "qoi" },							// QOI
	{ "png" },							// PNG
	{ "apng" },							// APNG
	{ "gif" },							// GIF
	{ "webp" },							// WEBP
	{ "xpm" },							// XPM
	{ "jpg", "jpeg" },					// JPG
	{ "tif", "tiff" },					// TIFF
	{ "dds" },							// DDS
	{ "ktx" },							// KTX
	{ "ktx2" },							// KTX2
	{ "astc" },							// ASTC
	{ "hdr", "rgbe" },					// HDR
	{ "exr" },							// EXR
	{ "pcx" },							// PCX
	{ "wbmp" },							// WBMP
	{ "wmf" },							// WMF
	{ "jp2" },							// JP2
	{ "jpc" },							// JPC
	{ "ico" },							// ICO
	{ "tac" },							// TAC
	{ "cfg" },							// CFG
	{ "ini" },							// INI
};
tStaticAssert(tNumElements(tSystem::FileTypeExtTable) == int(tSystem::tFileType::NumFileTypes));


tString tSystem::tGetFileExtension(const tString& file)
{
	tString ext = file.Right('.'); 
	if(ext == file)
		ext.Clear();

	return ext;
}


tSystem::tFileType tSystem::tGetFileTypeFromExtension(const tString& ext)
{
	if (ext.IsEmpty())
		return tFileType::Unknown;

	for (int t = 0; t < int(tFileType::NumFileTypes); t++)
		if (FileTypeExtTable[t].HasExt(ext))
			return tFileType(t);

	return tFileType::Unknown;
}


tSystem::tFileType tSystem::tGetFileTypeFromExtension(const char* ext)
{
	// tString constructor can handle nullptr.
	return tGetFileTypeFromExtension(tString(ext));
}


tSystem::tFileType tSystem::tGetFileType(const tString& file)
{
	if (file.IsEmpty())
		return tFileType::Unknown;

	tString ext = tGetFileExtension(file);
	return tGetFileTypeFromExtension(ext);
}


void tSystem::tGetExtensions(tList<tStringItem>& extensions, tFileType fileType)
{
	if (fileType == tFileType::Invalid)
		return;

	FileTypeExts& exts = FileTypeExtTable[ int(fileType) ];
	for (int e = 0; e < MaxExtensionsPerFileType; e++)
		if (exts.Ext[e])
			extensions.Append(new tStringItem(exts.Ext[e]));
}


void tSystem::tGetExtension(tList<tStringItem>& extensions, tFileType fileType)
{
	if (fileType == tFileType::Unknown)
		return;

	FileTypeExts& exts = FileTypeExtTable[ int(fileType) ];
	if (exts.Ext[0])
		extensions.Append(new tStringItem(exts.Ext[0]));
}


tString tSystem::tGetExtension(tFileType fileType)
{
	if (fileType == tFileType::Unknown)
		return tString();

	FileTypeExts& exts = FileTypeExtTable[ int(fileType) ];

	// The tString constructor can handle nullptr.
	return tString(exts.Ext[0]);
}


std::time_t tSystem::tConvertToPosixTime(std::filesystem::file_time_type ftime)
{
	using namespace std::chrono;

	// This is the new C++20 way, although different compilers have not implemented the same code paths.
	// Using clock_cast seems a bit more portable than something like to_sys which is not implemented for all clocks
	// on some systems (MSVC, for example, chose only to do it for the utc clock. In any case clock_cast is probably
	// the way to go. Example non-portable code: Update: apparently not portable. Need the ifdefs.
	#ifdef PLATFORM_WINDOWS
	auto systemTimePoint = clock_cast<std::chrono::system_clock>(ftime);
	return system_clock::to_time_t(systemTimePoint);

	#else

	// This is what we should be using, but couldn't get snap to work with it. Well, actually
	// I could get it compiling by using core22 (it has recent enough compiler) but then there were glx issues. 
	// return system_clock::to_time_t(file_clock::to_sys(ftime));

	// This is the _old_ way.
	auto sctp = time_point_cast<system_clock::duration>(ftime - std::filesystem::file_time_type::clock::now() + system_clock::now());
	return system_clock::to_time_t(sctp);
	#endif
}


#ifdef PLATFORM_WINDOWS
std::time_t tSystem::tConvertToPosixTime(FILETIME filetime)
{
	LARGE_INTEGER date;
	date.HighPart = filetime.dwHighDateTime;
	date.LowPart = filetime.dwLowDateTime;

	// Milliseconds.
	LARGE_INTEGER adjust;
	adjust.QuadPart = 11644473600000 * 10000;
	
	// Removes the diff between 1970 and 1601.
	date.QuadPart -= adjust.QuadPart;

	// Converts back from 100-nanoseconds (Windows) to seconds (Posix).
	return date.QuadPart / 10000000;
}
#endif


void tSystem::tPopulateFileInfo(tFileInfo& fileInfo, const std::filesystem::directory_entry& entry, const tString& filename)
{
	fileInfo.FileName = filename;
	std::error_code errCode;
	fileInfo.FileSize = entry.file_size(errCode);
	if (errCode)
	{
		fileInfo.FileSize = 0;
		errCode.clear();
	}

	// CreationTime not vailable from std::filesystem. Needs to remain -1 (unset).
	fileInfo.CreationTime = -1;

	// ModificationTime.
	std::filesystem::file_time_type ftime = entry.last_write_time(errCode);
	if (!errCode)
		fileInfo.ModificationTime = tConvertToPosixTime(ftime);

	// AccessTime not vailable from std::filesystem. Needs to remain -1 (unset).
	fileInfo.AccessTime = -1;

	// ReadOnly flag.
	std::filesystem::file_status status = entry.status(errCode);
	if (!errCode)
	{
		std::filesystem::perms pms = status.permissions();
		bool w = ((pms & std::filesystem::perms::owner_write) != std::filesystem::perms::none);
		bool r = ((pms & std::filesystem::perms::owner_read) != std::filesystem::perms::none);
		fileInfo.ReadOnly = (r && !w);
	}

	// Hidden.
	// For Linux it just checks for leading '.' do is fast.
	// For Windows there's no way I can see to use std::filesystem to get this... so it uses WinAPI calls.
	// It's actually not that important. We have a native implementation of FindDirs for Windows
	// anyway. We don't (yet) have a native one for Linux, but at least this part is fast.
	fileInfo.Hidden = tIsHidden(filename);

	// Directoryness.
	std::error_code dec;
	fileInfo.Directory = entry.is_directory(dec);
}


#ifdef PLATFORM_WINDOWS
void tSystem::tPopulateFileInfo(tFileInfo& fileInfo, Win32FindData& fd, const tString& filename)
{
	fileInfo.FileName = filename;
	fileInfo.CreationTime = tConvertToPosixTime(fd.ftCreationTime);
	fileInfo.ModificationTime = tConvertToPosixTime(fd.ftLastWriteTime);
	fileInfo.AccessTime = tConvertToPosixTime(fd.ftLastAccessTime);

	// Occasionally, a file does not have a valid modification or access time.  The fileInfo struct
	// may, erronously, contain a modification or access time that is smaller than the creation time!
	// This happens, for example, with some files that have been transferred from a USB camera.
	// In this case, we simply use the creation time for the modification or access times.
	// Actually, this happens quite a bit, sometimes even if the times are valid!
	//
	// On seconds thought... we're going to leave the modification date alone.  Although I don't agree
	// with how the OS deals with this, I think the behaviour is best left untouched for mod time.
	// Basically, a file copied from, say, a mem card will get a current creation time, but the mod date
	// will be left at whatever the original file was.  Silly, although perhaps a little defensible.
	// We still correct any possible problems with access date though.
	if (fileInfo.AccessTime < fileInfo.CreationTime)
		fileInfo.AccessTime = fileInfo.CreationTime;

	fileInfo.FileSize = fd.nFileSizeHigh;
	fileInfo.FileSize <<= 32;
	fileInfo.FileSize |= fd.nFileSizeLow;

	fileInfo.ReadOnly = (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? true : false;
	fileInfo.Hidden = (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ? true : false;
	fileInfo.Directory = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
}
#endif


bool tSystem::tGetFileInfo(tFileInfo& fileInfo, const tString& path)
{
	// We want the info cleared in case an error or early-exit occurs.
	fileInfo.Clear();
	tString file(path);

	#ifdef PLATFORM_WINDOWS
	// Seems like FindFirstFile cannot deal with a trailing backslash when
	// trying to access directory information.  We remove it here.
	tPathWinFile(file);
	Win32FindData fd;
	#ifdef TACENT_UTF16_API_CALLS
		tStringUTF16 file16(file);
		WinHandle h = FindFirstFile(file16.GetLPWSTR(), &fd);
		#else
		WinHandle h = FindFirstFile(file.Chr(), &fd);
	#endif
	if (h == INVALID_HANDLE_VALUE)
		return false;

	// This fully fills in fileInfo, including the filename.
	tPopulateFileInfo(fileInfo, fd, path);
	FindClose(h);
	return true;

	#else
	tPathStd(file);
	fileInfo.FileName = file;
	fileInfo.Hidden = tIsHidden(file);		// On Linux just looks for a leading . in filename.

	struct stat statBuf;
	int errCode = stat(file.Chr(), &statBuf);
	if (errCode)
		return false;

	// Figure out read-onlyness.
	bool w = (statBuf.st_mode & S_IWUSR) ? true : false;
	bool r = (statBuf.st_mode & S_IRUSR) ? true : false;
	fileInfo.ReadOnly = (r && !w);

	fileInfo.FileSize = statBuf.st_size;
	fileInfo.Directory = ((statBuf.st_mode & S_IFMT) == S_IFDIR) ? true : false;

	fileInfo.CreationTime = statBuf.st_ctime;		// @todo I think this is not creation time.
	fileInfo.ModificationTime = statBuf.st_mtime;
	fileInfo.AccessTime = statBuf.st_atime;
	if (fileInfo.AccessTime < fileInfo.CreationTime)
		fileInfo.AccessTime = fileInfo.CreationTime;

	return true;
	#endif
}


#ifdef PLATFORM_WINDOWS
bool tSystem::tGetFileDetails(tFileDetails& details, const tString& path)
{
	tString ffn = path;
	tPathWinFile(ffn);

	tString fileName = tSystem::tGetFileName(ffn);
	tString fileDir = tSystem::tGetDir(ffn);
	tPathWin(fileDir);

	if ((fileName.Length() == 2) && (fileName[1] == ':'))
	{
		fileName += "\\";
		fileDir = "";
	}

	// This interface is used for freeing up PIDLs.
	lpMalloc mallocInterface = 0;
	hResult result = SHGetMalloc(&mallocInterface);
	if (!mallocInterface)
		return false;

	// Get the desktop interface.
	IShellFolder* desktopInterface = 0;
	result = SHGetDesktopFolder(&desktopInterface);
	if (!desktopInterface)
	{
		mallocInterface->Release();
		return false;
	}

	// IShellFolder::ParseDisplayName requires the path name in Unicode wide characters.
	OleChar olePath[MaxPath];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, fileDir.Chr(), -1, olePath, MaxPath);

	// Parse path for absolute PIDL, and connect to target folder.
	lpItemIdList pidl = 0;
	result = desktopInterface->ParseDisplayName(0, 0, olePath, 0, &pidl, 0);
	if (result != S_OK)
	{
		desktopInterface->Release();
		mallocInterface->Release();
		return false;
	}

	IShellFolder2* shellFolder2 = 0;
	result = desktopInterface->BindToObject
	(
		pidl, 0,
		IID_IShellFolder2, (void**)&shellFolder2
	);

	// Release what we no longer need.
	desktopInterface->Release();
	mallocInterface->Free(pidl);

	if ((result != S_OK) || !shellFolder2)
	{
		mallocInterface->Release();
		return false;
	}

	OleChar unicodeName[MaxPath];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, fileName.Chr(), -1, unicodeName, MaxPath);
	lpItemIdList localPidl = 0;
	result = shellFolder2->ParseDisplayName(0, 0, unicodeName, 0, &localPidl, 0);
	if (result != S_OK)
	{
		mallocInterface->Release();
		shellFolder2->Release();
		return false;
	}

	int columnIndexArray[] =
	{
		// 0,				Name (Not Needed)
		1,					// Size / Type (Logical Drives)
		2,					// Type / Total Size (Logical Drives)
		3,					// Date Modified / Free Space (Logical Drives)
		4,					// Date Created / File System (Logical Drives)
		5,					// Date Accessed / Comments (Logical Drives)
		6,					// Attributes
		// 7,				Status (Not Needed)
		// 8,				Owner (Not Needed)
		9,					// Author
		10,					// Title
		11,					// Subject
		12,					// Category
		13,					// Pages
		14,					// Comments
		15,					// Copyright
		16,					// Artist
		17,					// Album Title
		18,					// Year
		19,					// Track Number
		20,					// Genre
		21,					// Duration
		22,					// Bit Rate
		23,					// Protected
		24,					// Camera Model
		25,					// Date Picture Taken
		26,					// Dimensions
		// 27,				Blank
		// 28,				Blank
		29,					// Episode Name
		30,					// Program Description
		// 31,				Blank
		32,					// Audio Sample Size
		33,					// Audio Sample Rate
		34,					// Channels
		// 35,				File State (Too Slow)
		// 36,				Rev (Useful but Too Slow)
		// 37,				Action (Too Slow)
		38,					// Company
		39,					// Description
		40,					// File VErsion
		41,					// Product Name
		42					// Product Version
	};

	const int maxDetailColumnsToTry = sizeof(columnIndexArray)/sizeof(*columnIndexArray);
	for (int c = 0; c < maxDetailColumnsToTry; c++)
	{
		int col = columnIndexArray[c];
		SHELLDETAILS shellDetail;

		// Get title.
		result = shellFolder2->GetDetailsOf(0, col, &shellDetail);
		if (result == S_OK)
		{
			#ifdef TACENT_UTF16_API_CALLS
			tStringUTF16 title(33);
			StrRetToBuf(&shellDetail.str, localPidl, title.GetLPWSTR(), 32);
			title.SetLength( tStd::tStrlen(title.Chars()) );

			#else
			char titlebuf[33];
			StrRetToBuf(&shellDetail.str, localPidl, titlebuf, 32);
			tString title(titlebuf);
			#endif

			// Get detail.
			result = shellFolder2->GetDetailsOf(localPidl, col, &shellDetail);
			if (result == S_OK)
			{
				#ifdef TACENT_UTF16_API_CALLS
				tStringUTF16 detail(33);
				StrRetToBuf(&shellDetail.str, localPidl, detail.GetLPWSTR(), 32);
				detail.SetLength( tStd::tStrlen(detail.Chars()) );
				#else
				char detailBuf[33];
				StrRetToBuf(&shellDetail.str, localPidl, detailBuf, 32);
				tString detail(detailBuf);
				#endif

				// We only add the detail to the list if both title and detail are present.
				if (title.IsValid() && detail.IsValid())
				{
					details.DetailTitles.Append(new tStringItem(title));
					details.Details.Append(new tStringItem(detail));
				}
			}
		}
	}

	mallocInterface->Free(localPidl);

	// Release all remaining interface pointers.
	mallocInterface->Release();
	shellFolder2->Release();

	return true;
}


void tSystem::tSetFileOpenAssoc(const tString& program, const tString& extension, const tString& programOptions)
{
	tString baseName = tSystem::tGetFileBaseName(program);
	baseName.ToLower();

	tString keyString = "Software\\Classes\\Tacent_";
	keyString += baseName;
	keyString += "\\shell\\open\\command";

	HKEY key;
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 keyString16(keyString);
	if (RegCreateKeyEx(HKEY_CURRENT_USER, keyString16.GetLPWSTR(), 0, 0, 0, KEY_SET_VALUE, 0, &key, 0) == ERROR_SUCCESS)
	#else
	if (RegCreateKeyEx(HKEY_CURRENT_USER, keyString.Chr(), 0, 0, 0, KEY_SET_VALUE, 0, &key, 0) == ERROR_SUCCESS)
	#endif
	{
		// Create value string and set it.
		tString options = programOptions;
		if (options.IsEmpty())
			options = " ";
		else
			options = tString(" ") + options + " ";
		tString valString = tString("\"") + tSystem::tGetSimplifiedPath(program) + "\"" + options + "\"%1\"";
		tPathWin(valString);
		#ifdef TACENT_UTF16_API_CALLS
		RegSetValueEx(key, LPWSTR(u""), 0, REG_SZ, (uint8*)valString.Chr(), valString.Length()+1);
		#else
		RegSetValueEx(key, "", 0, REG_SZ, (uint8*)valString.Chr(), valString.Length()+1);
		#endif
		RegCloseKey(key);
	}

	tString ext = extension;
	ext.ToLower();
	keyString = "Software\\Classes\\.";
	keyString += ext;
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 keyString16B(keyString);
	if (RegCreateKeyEx(HKEY_CURRENT_USER, keyString16B.GetLPWSTR(), 0, 0, 0, KEY_SET_VALUE, 0, &key, 0) == ERROR_SUCCESS)
	#else
	if (RegCreateKeyEx(HKEY_CURRENT_USER, keyString.Chr(), 0, 0, 0, KEY_SET_VALUE, 0, &key, 0) == ERROR_SUCCESS)
	#endif
	{
		tString valString = "Tacent_";
		valString += baseName;

		// REG_SZ means that the values in arg 5 is not UTF-16.
		#ifdef TACENT_UTF16_API_CALLS
		RegSetValueEx(key, LPWSTR(u""), 0, REG_SZ, (const BYTE*)valString.Chr(), valString.Length()+1);
		#else
		RegSetValueEx(key, "", 0, REG_SZ, (uint8*)valString.Chr(), valString.Length()+1);
		#endif
		RegCloseKey(key);
	}
}


void tSystem::tSetFileOpenAssoc(const tString& program, const tList<tStringItem>& extensions, const tString& programOptions)
{
	for (auto ext = extensions.First(); ext; ext = ext->Next())
		tSetFileOpenAssoc(program, *ext, programOptions);
}


tString tSystem::tGetFileOpenAssoc(const tString& extension)
{
	if (extension.IsEmpty())
		return tString();

	HKEY key;
	tString ext = extension;
	ext.ToLower();
	tString keyString = "Software\\Classes\\.";
	keyString += ext;
	tString appName;
	char appNameBuf[128];
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 keyString16A(keyString);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, keyString16A.GetLPWSTR(), 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
	#else
	if (RegOpenKeyEx(HKEY_CURRENT_USER, keyString.Chr(), 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
	#endif
	{
		ulong numBytesIO = 127;
		#ifdef TACENT_UTF16_API_CALLS
		RegGetValue(key, LPCWSTR(u""), 0, RRF_RT_REG_SZ | RRF_ZEROONFAILURE, 0, appNameBuf, &numBytesIO);
		#else
		RegGetValue(key, "", 0, RRF_RT_REG_SZ | RRF_ZEROONFAILURE, 0, appNameBuf, &numBytesIO);
		#endif
		appName.Set(appNameBuf);
		RegCloseKey(key);
	}

	if (appName.IsEmpty())
		return tString();

	keyString = "Software\\Classes\\";
	keyString += appName;
	keyString += "\\shell\\open\\command";
	tString exeName;
	char exeNameBuf[256];
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 keyString16B(keyString);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, keyString16B.GetLPWSTR(), 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
	#else
	if (RegOpenKeyEx(HKEY_CURRENT_USER, keyString.Chr(), 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
	#endif
	{
		ulong numBytesIO = 255;
		#ifdef TACENT_UTF16_API_CALLS
		RegGetValue(key, LPCWSTR(u""), 0, RRF_RT_REG_SZ | RRF_ZEROONFAILURE, 0, exeNameBuf, &numBytesIO);
		#else
		RegGetValue(key, "", 0, RRF_RT_REG_SZ | RRF_ZEROONFAILURE, 0, exeNameBuf, &numBytesIO);
		#endif
		exeName.Set(exeNameBuf);
		RegCloseKey(key);
	}

	return exeName;
}
#endif // PLATFORM_WINDOWS


bool tSystem::tFindDirs_Stndrd(tList<tStringItem>* dirs, tList<tFileInfo>* infos, const tString& dir, bool hidden)
{
	tString dirPath(dir);
	if (dirPath.IsEmpty())
		dirPath = (char*)std::filesystem::current_path().u8string().c_str();

	std::error_code errorCode;
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dirPath.Text(), errorCode))
	{
		if (errorCode || (entry == std::filesystem::directory_entry()))
		{
			errorCode.clear();
			continue;
		}

		// For now we're skipping symlinks (they return false for is_directory).
		std::error_code direc;
		if (!entry.is_directory(direc))
			continue;

		if (direc)
			continue;

		tString foundDir((char*)entry.path().u8string().c_str());
		tPathStdDir(foundDir);

		if (!hidden && tIsHidden(foundDir))
			continue;

		if (dirs)
			dirs->Append(new tStringItem(foundDir));

		if (infos)
		{
			tFileInfo* fileInfo = new tFileInfo();
			tPopulateFileInfo(*fileInfo, entry, foundDir);
			infos->Append(fileInfo);
		}
	}

	return true;
}


bool tSystem::tFindDirs_Native(tList<tStringItem>* dirs, tList<tFileInfo>* infos, const tString& dir, bool hidden)
{
	#if defined(PLATFORM_WINDOWS)
	// First lets massage fileName a little.
	tString massagedName = dir;
	if ((massagedName[massagedName.Length() - 1] == '/') || (massagedName[massagedName.Length() - 1] == '\\'))
		massagedName += "*.*";

	Win32FindData fd;
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 massagedName16(massagedName);
	WinHandle h = FindFirstFile(massagedName16.GetLPWSTR(), &fd);
	#else
	WinHandle h = FindFirstFile(massagedName.Chr(), &fd);
	#endif
	if (h == INVALID_HANDLE_VALUE)
		return false;

	tString path = tGetDir(massagedName);
	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || hidden)
			{
				// If the directory name is not "." or ".." then it's a real directory.
				// Note that you cannot just check for the first character not being "."  Some directories (and files)
				// may have a name that starts with a dot, especially if they were copied from a unix machine.
				#ifdef TACENT_UTF16_API_CALLS
				tString fn((char16_t*)fd.cFileName);
				#else
				tString fn(fd.cFileName);
				#endif
				if ((fn != ".") && (fn != ".."))
				{
					if (dirs)
						dirs->Append(new tStringItem(path + fn + "/"));

					if (infos)
					{
						tFileInfo* fileInfo = new tFileInfo();
						tPopulateFileInfo(*fileInfo, fd, tString(path + fn + "/"));
						infos->Append(fileInfo);
					}
				}
			}
		}
	} while (FindNextFile(h, &fd));

	FindClose(h);
	if (GetLastError() != ERROR_NO_MORE_FILES)
		return false;
	return true;

	#elif defined(PLATFORM_LINUX)
	// @todo No Linux Native implementation. Use Standard.
	return tFindDirs_Stndrd(dirs, infos, dir, hidden);

	#else
	tAssert(!"tFindDirs_Native not implemented for platform.");
	return false;

	#endif
}


bool tSystem::tFindDirs(tList<tStringItem>& dirs, const tString& dir, bool hidden, Backend backend)
{
	switch (backend)
	{
		case Backend::Stndrd: return tFindDirs_Stndrd(&dirs, nullptr, dir, hidden);
		case Backend::Native: return tFindDirs_Native(&dirs, nullptr, dir, hidden);
	}
	return false;
}


bool tSystem::tFindDirs(tList<tFileInfo>& dirs, const tString& dir, bool hidden, Backend backend)
{
	switch (backend)
	{
		case Backend::Stndrd: return tFindDirs_Stndrd(nullptr, &dirs, dir, hidden);
		case Backend::Native: return tFindDirs_Native(nullptr, &dirs, dir, hidden);
	}
	return false;
}


bool tSystem::tFindFiles_Stndrd(tList<tStringItem>* files, tList<tFileInfo>* infos, const tString& dir, const tExtensions* extensions, bool hidden)
{
	if (extensions && extensions->IsEmpty())
		return false;

	// Use current directory if no dirPath supplied.
	tString dirPath(dir);
	if (dirPath.IsEmpty())
		dirPath = (char*)std::filesystem::current_path().u8string().c_str();

	if (dirPath.IsEmpty())
		return false;

	std::error_code errorCode;
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dirPath.Text(), errorCode))
	{
		if (errorCode || (entry == std::filesystem::directory_entry()))
		{
			errorCode.clear();
			continue;
		}

		std::error_code rec;
		if (!entry.is_regular_file(rec))
			continue;
		if (rec)
			continue;

		tString foundFile((char*)entry.path().u8string().c_str());
		tPathStdFile(foundFile);
		tString foundExt = tGetFileExtension(foundFile);

		// If no extension match continue.
		if (extensions && !extensions->Contains(foundExt))
			continue;

		if (!hidden && tIsHidden(foundFile))
			continue;

		if (files)
			files->Append(new tStringItem(foundFile));

		if (infos)
		{
			tFileInfo* newFileInfo = new tFileInfo();
			tGetFileInfo(*newFileInfo, foundFile);
			infos->Append(newFileInfo);
		}
	}

	return true;
}


bool tSystem::tFindFiles_Native(tList<tStringItem>* files, tList<tFileInfo>* infos, const tString& dir, const tExtensions* extensions, bool hidden)
{
	if (extensions && extensions->IsEmpty())
		return false;

	tString dirStr(dir);
	if (dirStr.IsEmpty())
		dirStr = tGetCurrentDir();

	#ifdef PLATFORM_WINDOWS
	// FindFirstFile etc seem to like backslashes better.
	tPathWinDir(dirStr);

	// There's some complexity here with windows, but it's still very fast. We need to loop through all the
	// extensions doing the FindFirstFile business, while modifying the path appropriately for each one.
	// Insert a special empty extension if extensions is null. This will cause all file types to be included.
	tExtensions exts;
	if (extensions)
		exts.Add(*extensions);
	else
		exts.Extensions.Append(new tStringItem());

	bool allOk = true;
	for (tStringItem* extItem = exts.First(); extItem; extItem = extItem->Next())
	{
		tString ext(*extItem);
		tString path = dirStr + "*.";
		if (ext.IsEmpty())
			path += "*";
		else
			path += ext;

		Win32FindData fd;
		#ifdef TACENT_UTF16_API_CALLS
		tStringUTF16 path16(path);
		WinHandle h = FindFirstFile(path16.GetLPWSTR(), &fd);
		#else
		WinHandle h = FindFirstFile(path.Chr(), &fd);
		#endif
		if (h == INVALID_HANDLE_VALUE)
		{
			allOk = false;
			continue;
		}

		do
		{
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				// It's not a directory... so it's actually a real file.
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || hidden)
				{
					#ifdef TACENT_UTF16_API_CALLS
					tString fdFilename((char16_t*)fd.cFileName);
					#else
					tString fdFilename(fd.cFileName);
					#endif
					tString foundName = dirStr + fdFilename;
					tPathStd(foundName);

					tStringItem* newName = files ? new tStringItem(foundName) : nullptr;
					tFileInfo*   newInfo = infos ? new tFileInfo() : nullptr;
					if (newInfo)
						tPopulateFileInfo(*newInfo, fd, foundName);

					// Holy obscure and annoying FindFirstFile bug! FindFirstFile("*.abc", ...) will also find
					// files like file.abcd. This isn't correct I guess we have to check the extension here.
					// FileMask is required to specify an extension, even if it is ".*"
					if (path[path.Length() - 1] != '*')
					{
						tString foundExtension = tGetFileExtension(fdFilename);
						if (ext.IsEqualCI(foundExtension))
						{
							if (files)
								files->Append(newName);
							if (infos)
								infos->Append(newInfo);
						}
					}
					else
					{
						if (files)
							files->Append(newName);
						if (infos)
							infos->Append(newInfo);
					}
				}
			}
		} while (FindNextFile(h, &fd));

		FindClose(h);
		if (GetLastError() != ERROR_NO_MORE_FILES)
			return false;		
	}
	return allOk;

	#elif defined(PLATFORM_LINUX)
	tPathStdDir(dirStr);
	DIR* dirEnt = opendir(dirStr.Chr());
	if (dirStr.IsEmpty() || !dirEnt)
		return false;

	for (struct dirent* entry = readdir(dirEnt); entry; entry = readdir(dirEnt))
	{
		// Definitely skip directories.
		if (entry->d_type == DT_DIR)
			continue;

		// Sometimes it seems that d_type for a file is set to unknown.
		// Noticed this under Linux when some files are in mounted directories.
		if ((entry->d_type != DT_REG) && (entry->d_type != DT_UNKNOWN))
			continue;

		tString foundFile((char*)entry->d_name);
		foundFile = dirStr + foundFile;
		tString foundExt = tGetFileExtension(foundFile);

		// If extension list present and no match continue.
		if (extensions && !extensions->Contains(foundExt))
			continue;

		if (!hidden && tIsHidden(foundFile))
			continue;

		if (files)
			files->Append(new tStringItem(foundFile));

		if (infos)
		{
			tFileInfo* newFileInfo = new tFileInfo();

			// @todo If we had a linux native populate file info, we would call it here.
			tGetFileInfo(*newFileInfo, foundFile);
			infos->Append(newFileInfo);
		}
	}
	closedir(dirEnt);
	return true;

	#endif
}


bool tSystem::tFindFiles(tList<tStringItem>& files, const tString& dir, bool hidden, Backend backend)
{
	switch (backend)
	{
		// A nullptr for extensions will return all types.
		case Backend::Stndrd: return tFindFiles_Stndrd(&files, nullptr, dir, nullptr, hidden);
		case Backend::Native: return tFindFiles_Native(&files, nullptr, dir, nullptr, hidden);
	}
	return false;
}


bool tSystem::tFindFiles(tList<tStringItem>& files, const tString& dir, const tString& ext, bool hidden, Backend backend)
{
	tExtensions extensions;
	if (!ext.IsEmpty())
		extensions.Add(ext);

	switch (backend)
	{
		// A valid but empty tExtensions will return false which is what we want.
		case Backend::Stndrd: return tFindFiles_Stndrd(&files, nullptr, dir, &extensions, hidden);
		case Backend::Native: return tFindFiles_Native(&files, nullptr, dir, &extensions, hidden);
	}
	return false;
}


bool tSystem::tFindFiles(tList<tStringItem>& files, const tString& dir, const tExtensions& extensions, bool hidden, Backend backend)
{
	switch (backend)
	{
		// A valid but empty tExtensions will return false which is what we want.
		case Backend::Stndrd: return tFindFiles_Stndrd(&files, nullptr, dir, &extensions, hidden);
		case Backend::Native: return tFindFiles_Native(&files, nullptr, dir, &extensions, hidden);
	}
	return false;
}


bool tSystem::tFindFiles(tList<tFileInfo>& files, const tString& dir, bool hidden, Backend backend)
{
	switch (backend)
	{
		// A nullptr for extensions will return all types.
		case Backend::Stndrd: return tFindFiles_Stndrd(nullptr, &files, dir, nullptr, hidden);
		case Backend::Native: return tFindFiles_Native(nullptr, &files, dir, nullptr, hidden);
	}
	return false;
}


bool tSystem::tFindFiles(tList<tFileInfo>& files, const tString& dir, const tString& ext, bool hidden, Backend backend)
{
	tExtensions extensions;
	if (!ext.IsEmpty())
		extensions.Add(ext);

	switch (backend)
	{
		// A valid but empty tExtensions will return false which is what we want.
		case Backend::Stndrd: return tFindFiles_Stndrd(nullptr, &files, dir, &extensions, hidden);
		case Backend::Native: return tFindFiles_Native(nullptr, &files, dir, &extensions, hidden);
	}
	return false;
}


bool tSystem::tFindFiles(tList<tFileInfo>& files, const tString& dir, const tExtensions& extensions, bool hidden, Backend backend)
{
	switch (backend)
	{
		// A valid but empty tExtensions will return false which is what we want.
		case Backend::Stndrd: return tFindFiles_Stndrd(nullptr, &files, dir, &extensions, hidden);
		case Backend::Native: return tFindFiles_Native(nullptr, &files, dir, &extensions, hidden);
	}
	return false;
}


bool tSystem::tFindDirsRec_Stndrd(tList<tStringItem>* dirs, tList<tFileInfo>* infos, const tString& dir, bool hidden)
{
	tString dirPath(dir);
	if (dirPath.IsEmpty())
		dirPath = (char*)std::filesystem::current_path().u8string().c_str();

	// The std::filesystem API has a recursive iterator so we use it.
	std::error_code errorCode;
	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(dirPath.Chr(), errorCode))
	{
		// If error or entry is default entry, continue. Probably don't need to clear errorCode but just in case.
		if (errorCode || (entry == std::filesystem::directory_entry()))
		{
			errorCode.clear();
			continue;
		}

		std::error_code direc;
		if (!entry.is_directory(direc))
			continue;
		if (direc)
			continue;

		tString foundDir((char*)entry.path().u8string().c_str());
		tPathStdDir(foundDir);

		// All directories end in a slash in tacent.
		if (foundDir[foundDir.Length()-1] != '/')
			foundDir += "/";

		if (!hidden && tIsHidden(foundDir))
			continue;

		if (dirs)
			dirs->Append(new tStringItem(foundDir));

		if (infos)
		{
			tFileInfo* fileInfo = new tFileInfo();
			tPopulateFileInfo(*fileInfo, entry, foundDir);
			infos->Append(fileInfo);
		}
	}

	return true;
}


bool tSystem::tFindDirsRec_Native(tList<tStringItem>* dirs, tList<tFileInfo>* infos, const tString& dir, bool hidden)
{
	// Populate current dir dirs/infos.
	tList<tStringItem> currdirs;
	tList<tFileInfo> currinfos;
	tFindDirs_Native(&currdirs, &currinfos, dir, hidden);

	if (dirs)
	{
		for (tStringItem* ds = currdirs.First(); ds; ds = ds->Next())
			dirs->Append(new tStringItem(*ds));
	}

	if (infos)
	{
		while (tFileInfo* di = currinfos.Remove())
			infos->Append(di);
	}

	// Recurse.
	for (tStringItem* d = currdirs.First(); d; d = d->Next())
		tFindDirsRec_Native(dirs, infos, *d, hidden);

	return true;
}


bool tSystem::tFindDirsRec(tList<tStringItem>& dirs, const tString& dir, bool hidden, Backend backend)
{
	switch (backend)
	{
		case Backend::Stndrd: return tFindDirsRec_Stndrd(&dirs, nullptr, dir, hidden);
		case Backend::Native: return tFindDirsRec_Native(&dirs, nullptr, dir, hidden);
	}
	return false;
}


bool tSystem::tFindDirsRec(tList<tFileInfo>& dirs, const tString& dir, bool hidden, Backend backend)
{
	switch (backend)
	{
		case Backend::Stndrd: return tFindDirsRec_Stndrd(nullptr, &dirs, dir, hidden);
		case Backend::Native: return tFindDirsRec_Native(nullptr, &dirs, dir, hidden);
	}
	return false;
}


bool tSystem::tFindFilesRec_Stndrd(tList<tStringItem>* files, tList<tFileInfo>* infos, const tString& dir, const tExtensions* extensions, bool hidden)
{
	if (extensions && extensions->IsEmpty())
		return false;

	// Use current directory if no dirPath supplied.
	tString dirPath(dir);
	if (dirPath.IsEmpty())
		dirPath = (char*)std::filesystem::current_path().u8string().c_str();

	if (dirPath.IsEmpty())
		return false;

	std::error_code errorCode;
	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(dirPath.Text(), errorCode))
	{
		if (errorCode || (entry == std::filesystem::directory_entry()))
		{
			errorCode.clear();
			continue;
		}

		std::error_code rec;
		if (!entry.is_regular_file(rec))
			continue;
		if (rec)
			continue;

		tString foundFile((char*)entry.path().u8string().c_str());
		tPathStdFile(foundFile);
		tString foundExt = tGetFileExtension(foundFile);

		// If no extension match continue.
		if (extensions && !extensions->Contains(foundExt))
			continue;

		if (!hidden && tIsHidden(foundFile))
			continue;

		if (files)
			files->Append(new tStringItem(foundFile));

		if (infos)
		{
			tFileInfo* newFileInfo = new tFileInfo();
			tGetFileInfo(*newFileInfo, foundFile);
			infos->Append(newFileInfo);
		}
	}

	return true;
}


bool tSystem::tFindFilesRec_Native(tList<tStringItem>* files, tList<tFileInfo>* infos, const tString& dir, const tExtensions* extensions, bool hidden)
{
	// Populate current dir files/infos.
	tFindFiles_Native(files, infos, dir, extensions, hidden);

	// Recurse.
	tList<tStringItem> currdirs;
	tFindDirs_Native(&currdirs, nullptr, dir, hidden);
	for (tStringItem* d = currdirs.First(); d; d = d->Next())
		tFindFilesRec_Native(files, infos, *d, extensions, hidden);

	return true;
}


bool tSystem::tFindFilesRec(tList<tStringItem>& files, const tString& dir, bool hidden, Backend backend)
{
	switch (backend)
	{
		// A nullptr for extensions will return all types.
		case Backend::Stndrd: return tFindFilesRec_Stndrd(&files, nullptr, dir, nullptr, hidden);
		case Backend::Native: return tFindFilesRec_Native(&files, nullptr, dir, nullptr, hidden);
	}
	return false;
}


bool tSystem::tFindFilesRec(tList<tStringItem>& files, const tString& dir, const tString& ext, bool hidden, Backend backend)
{
	tExtensions extensions;
	if (!ext.IsEmpty())
		extensions.Add(ext);

	switch (backend)
	{
		// A valid but empty tExtensions will return false which is what we want.
		case Backend::Stndrd: return tFindFilesRec_Stndrd(&files, nullptr, dir, &extensions, hidden);
		case Backend::Native: return tFindFilesRec_Native(&files, nullptr, dir, &extensions, hidden);
	}
	return false;
}


bool tSystem::tFindFilesRec(tList<tStringItem>& files, const tString& dir, const tExtensions& extensions, bool hidden, Backend backend)
{
	switch (backend)
	{
		// A valid but empty tExtensions will return false which is what we want.
		case Backend::Stndrd: return tFindFilesRec_Stndrd(&files, nullptr, dir, &extensions, hidden);
		case Backend::Native: return tFindFilesRec_Native(&files, nullptr, dir, &extensions, hidden);
	}
	return false;
}


bool tSystem::tFindFilesRec(tList<tFileInfo>& files, const tString& dir, bool hidden, Backend backend)
{
	switch (backend)
	{
		// A nullptr for extensions will return all types.
		case Backend::Stndrd: return tFindFilesRec_Stndrd(nullptr, &files, dir, nullptr, hidden);
		case Backend::Native: return tFindFilesRec_Native(nullptr, &files, dir, nullptr, hidden);
	}
	return false;
}


bool tSystem::tFindFilesRec(tList<tFileInfo>& files, const tString& dir, const tString& ext, bool hidden, Backend backend)
{
	tExtensions extensions;
	if (!ext.IsEmpty())
		extensions.Add(ext);

	switch (backend)
	{
		// A valid but empty tExtensions will return false which is what we want.
		case Backend::Stndrd: return tFindFilesRec_Stndrd(nullptr, &files, dir, &extensions, hidden);
		case Backend::Native: return tFindFilesRec_Native(nullptr, &files, dir, &extensions, hidden);
	}
	return false;
}


bool tSystem::tFindFilesRec(tList<tFileInfo>& files, const tString& dir, const tExtensions& extensions, bool hidden, Backend backend)
{
	switch (backend)
	{
		// A valid but empty tExtensions will return false which is what we want.
		case Backend::Stndrd: return tFindFilesRec_Stndrd(nullptr, &files, dir, &extensions, hidden);
		case Backend::Native: return tFindFilesRec_Native(nullptr, &files, dir, &extensions, hidden);
	}
	return false;
}


bool tSystem::tCreateDir(const tString& dir)
{
	tString dirPath = dir;
	
	#if defined(PLATFORM_WINDOWS)
	tPathWin(dirPath);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 dirPath16(dirPath);
	bool success = ::CreateDirectory(dirPath16.GetLPWSTR(), 0) ? true : false;
	#else
	bool success = ::CreateDirectory(dirPath.Chr(), 0) ? true : false;

	#endif
	if (!success)
		success = tDirExists(dirPath);

	return success;

	#else
	tPathStdFile(dirPath);
	bool ok = std::filesystem::create_directory(dirPath.Chr());
	if (!ok)
		return tDirExists(dirPath);

	return ok;

	#endif
}


bool tSystem::tDeleteDir(const tString& dir, bool deleteReadOnly)
{
	#ifdef PLATFORM_WINDOWS
	// Are we done before we even begin?
	if (!tDirExists(dir))
		return false;

	tList<tStringItem> fileList;
	tFindFiles(fileList, dir);
	tStringItem* file = fileList.First();
	while (file)
	{
		tDeleteFile(*file, deleteReadOnly);		// We don't really care whether it succeeded or not.
		file = file->Next();
	}

	fileList.Empty();							// Clean up the file list.
	tString directory(dir);
	tPathWin(directory);

	Win32FindData fd;
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 directoryMod16(directory + "*.*");
	WinHandle h = FindFirstFile(directoryMod16.GetLPWSTR(), &fd);
	#else
	WinHandle h = FindFirstFile((directory + "*.*").Chr(), &fd);
	#endif
	if (h == INVALID_HANDLE_VALUE)
		return true;

	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// If the directory name is not "." or ".." then it's a real directory.
			// Note that you cannot just check for the first character not being "."  Some directories (and files)
			// may have a name that starts with a dot, especially if they were copied from a unix machine.
			#ifdef TACENT_UTF16_API_CALLS
			tString fn((char16_t*)fd.cFileName);
			#else
			tString fn(fd.cFileName);
			#endif
			if ((fn != ".") && (fn != ".."))
				tDeleteDir(dir + fn + "/", deleteReadOnly);
		}
	} while (FindNextFile(h, &fd));

	bool deleteFilesOK = (GetLastError() == ERROR_NO_MORE_FILES) ? true : false;
	FindClose(h);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 directory16(directory);
	if (deleteReadOnly)
		SetFileAttributes(directory16.GetLPWSTR(), FILE_ATTRIBUTE_NORMAL);	// Directories can be read-only too.
	#else
	if (deleteReadOnly)
		SetFileAttributes(directory.Chr(), FILE_ATTRIBUTE_NORMAL);	// Directories can be read-only too.
	#endif

	bool success = false;
	for (int delTry = 0; delTry < 32; delTry++)
	{
		#ifdef TACENT_UTF16_API_CALLS
		tStringUTF16 dir16(dir);
		if (RemoveDirectory(dir16.GetLPWSTR()))
		#else
		if (RemoveDirectory(dir.Chr()))
		#endif
		{
			success = true;
			break;
		}

		// In some cases we might need to wait just a little and try again.  This can even take up to 10 seconds or so.
		// This seems to happen a lot when the target manager is streaming music, say, from the folder.
		else if (GetLastError() == ERROR_DIR_NOT_EMPTY)
		{
			tSystem::tSleep(500);
		}
		else
		{
			tSystem::tSleep(10);
		}
	}

	if (!success || !deleteFilesOK)
		return false;

	#else
	// Are we done before we even begin?
	if (!tDirExists(dir))
		return false;

	if (tIsReadOnly(dir) && !deleteReadOnly)
		return true;

	std::filesystem::path p(dir.Chr());
	std::error_code ec;
	uintmax_t numRemoved = std::filesystem::remove_all(p, ec);
	if (ec)
		return false;

	#endif

	return true;
}


uint32 tSystem::tHashFileFast32(const tString& filename, uint32 iv)
{
	int dataSize = 0;
	uint8* data = tLoadFile(filename, nullptr, &dataSize);
	if (!data)
		return iv;

	uint32 hash = tHash::tHashDataFast32(data, dataSize, iv);
	delete[] data;
	return hash;
}


uint32 tSystem::tHashFile32(const tString& filename, uint32 iv)
{
	int dataSize = 0;
	uint8* data = tLoadFile(filename, nullptr, &dataSize);
	if (!data)
		return iv;

	uint32 hash = tHash::tHashData32(data, dataSize, iv);
	delete[] data;
	return hash;
}


uint64 tSystem::tHashFile64(const tString& filename, uint64 iv)
{
	int dataSize = 0;
	uint8* data = tLoadFile(filename, nullptr, &dataSize);
	if (!data)
		return iv;

	uint64 hash = tHash::tHashData64(data, dataSize, iv);
	delete[] data;
	return hash;
}


tuint256 tSystem::tHashFile256(const tString& filename, tuint256 iv)
{
	int dataSize = 0;
	uint8* data = tLoadFile(filename, nullptr, &dataSize);
	if (!data)
		return iv;

	tuint256 hash = tHash::tHashData256(data, dataSize, iv);
	delete[] data;
	return hash;
}


tuint128 tSystem::tHashFileMD5(const tString& filename, tuint128 iv)
{
	int dataSize = 0;
	uint8* data = tLoadFile(filename, nullptr, &dataSize);
	if (!data)
		return iv;

	tuint128 hash = tHash::tHashDataMD5(data, dataSize, iv);
	delete[] data;
	return hash;
}


tuint256 tSystem::tHashFileSHA256(const tString& filename, tuint256 iv)
{
	int dataSize = 0;
	uint8* data = tLoadFile(filename, nullptr, &dataSize);
	if (!data)
		return iv;

	tuint256 hash = tHash::tHashDataSHA256(data, dataSize, iv);
	delete[] data;
	return hash;
}
