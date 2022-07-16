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
	std::time_t tFileTimeToStdTime(std::filesystem::file_time_type tp);

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

	#ifdef PLATFORM_WINDOWS
	std::time_t tFileTimeToPosixEpoch(FILETIME);
	void tGetFileInfo(tFileInfo& fileInfo, Win32FindData&);
	#endif

	bool tFindFilesInternal(tList<tStringItem>& foundFiles, const tString& dir, const tExtensions*, bool includeHidden);
	bool tFindFilesFastInternal
	(
		tList<tStringItem>* foundFiles, tList<tFileInfo>* foundInfos,
		const tString& dir, const tExtensions*, bool hidden
	);

	const int MaxExtensionsPerFileType = 4;
	struct FileTypeExts
	{
		const char* Ext[MaxExtensionsPerFileType] = { nullptr, nullptr, nullptr, nullptr };
		bool HasExt(const tString& ext)																					{ for (int e = 0; e < MaxExtensionsPerFileType; e++) if (ext.IsEqualCI(Ext[e])) return true; return false; }
	};
	extern FileTypeExts FileTypeExtTable[int(tFileType::NumFileTypes)];
}


inline void tSystem::tPathStd(tString& path)
{
	path.Replace('\\', '/');
	bool network = (path.Left(2) == "//");
	if (network)
	{
		path[0] = '\\'; path[1] = '\\';
		int sharesep = path.FindChar('/');
		if (sharesep != -1)
			path[sharesep] = '\\';
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
	int len = path.Length();
	if (path[len-1] == '/')
		path[len-1] = '\0';
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
	int len = path.Length();
	if (path[len-1] == '\\')
		path[len-1] = '\0';
}


tFileHandle tSystem::tOpenFile(const char8_t* filename, const char* mode)
{
	return fopen((const char*)filename, mode);
}


tFileHandle tSystem::tOpenFile(const char* filename, const char* mode)
{
	return fopen(filename, mode);
}


void tSystem::tCloseFile(tFileHandle f)
{
	if (!f)
		return;

	fclose(f);
}


int tSystem::tReadFile(tFileHandle f, void* buffer, int sizeBytes)
{
	// Load the entire thing into memory.
	int numRead = int(fread((char*)buffer, 1, sizeBytes, f));
	return numRead;
}


int tSystem::tWriteFile(tFileHandle f, const void* buffer, int sizeBytes)
{
	int numWritten = int(fwrite((void*)buffer, 1, sizeBytes, f));
	return numWritten;
}


int tSystem::tWriteFile(tFileHandle f, const char8_t* buffer, int length)
{
	int numWritten = int(fwrite((void*)buffer, 1, length, f));
	return numWritten;
}


int tSystem::tWriteFile(tFileHandle f, const char16_t* buffer, int length)
{
	int numWritten = int(fwrite((void*)buffer, 2, length, f));
	return numWritten;
}


int tSystem::tWriteFile(tFileHandle f, const char32_t* buffer, int length)
{
	int numWritten = int(fwrite((void*)buffer, 4, length, f));
	return numWritten;
}


int tSystem::tFileTell(tFileHandle handle)
{
	return int(ftell(handle));
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


int tSystem::tGetFileSize(tFileHandle file)
{
	if (!file)
		return 0;

	tFileSeek(file, 0, tSeekOrigin::End);
	int fileSize = tFileTell(file);

	tFileSeek(file, 0, tSeekOrigin::Beginning);			// Go back to beginning.
	return fileSize;
}


int tSystem::tGetFileSize(const tString& filename)
{
	#ifdef PLATFORM_WINDOWS
	if (filename.IsEmpty())
		return 0;

	tString file(filename);
	tPathWin(file);
	uint prevErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	Win32FindData fd;

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 fileUTF16(file);
	WinHandle h = FindFirstFile(fileUTF16.GetLPWSTR(), &fd);
	#else
	WinHandle h = FindFirstFile(file.Chr(), &fd);
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

	tFileHandle fd = tOpenFile(filename, "rb");
	int size = tGetFileSize(fd);
	tCloseFile(fd);

	return size;
	#endif
}


tString tSystem::tGetFileExtension(const tString& filename)																
{
	tString ext = filename.Right('.'); 
	if(ext == filename)
		ext.Clear();

	return ext;
}


// When more than one extension maps to the same filetype (like jpg and jpeg), always put the more common extension
// first in the extensions array.
tSystem::FileTypeExts tSystem::FileTypeExtTable[int(tSystem::tFileType::NumFileTypes)] = //] =
{
//	Extensions							Filetype
	{ "tga" },							// TGA
	{ "bmp" },							// BMP
	{ "png" },							// PNG
	{ "apng" },							// APNG
	{ "gif" },							// GIF
	{ "webp" },							// WEBP
	{ "xpm" },							// XPM
	{ "jpg", "jpeg" },					// JPG
	{ "tif", "tiff" },					// TIFF
	{ "dds" },							// DDS
	{ "hdr", "rgbe" },					// HDR
	{ "exr" },							// EXR
	{ "pcx" },							// PCX
	{ "wbmp" },							// WBMP
	{ "wmf" },							// WMF
	{ "jp2" },							// JP2
	{ "jpc" },							// JPC
	{ "ico" },							// ICO
	{ "tex" },							// TEX
	{ "img" },							// IMG
	{ "cub" },							// CUB
	{ "tac", "tim" },					// TAC
	{ "cfg" },							// CFG
	{ "ini" },							// INI
	// { "too many" }
};


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


const char* tSystem::tGetFileTypeName(tFileType fileType)
{
	if (fileType == tFileType::Unknown)
		return nullptr;

	FileTypeExts& exts = FileTypeExtTable[ int(fileType) ];
	return exts.Ext[0];
}


bool tSystem::tFileExists(const tString& filename)
{
	#if defined(PLATFORM_WINDOWS)
	tString file(filename);
	tPathWin(file);

	int length = file.Length();
	if (file[ length - 1 ] == ':')
		file += "\\*";

	uint prevErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	Win32FindData fd;
	#ifdef TACENT_UTF16_API_CALLS	
	tStringUTF16 fileUTF16(file);
	WinHandle h = FindFirstFile(fileUTF16.GetLPWSTR(), &fd);
	#else
	WinHandle h = FindFirstFile(file.Chr(), &fd);
	#endif
	SetErrorMode(prevErrorMode);
	if (h == INVALID_HANDLE_VALUE)
		return false;

	FindClose(h);
	if (fd.dwFileAttributes & _A_SUBDIR)
		return false;
	
	return true;

	#else
	tString file(filename);
	tPathStd(file);

	struct stat statbuf;
	return stat(file.Chr(), &statbuf) == 0;

	#endif
}


bool tSystem::tDirExists(const tString& dirname)
{
	if (dirname.IsEmpty())
		return false;
		
	tString dir = dirname;
	
	#if defined(PLATFORM_WINDOWS)
	tPathWinFile(dir);

	// Can't quite remember what the * does. Needs testing.
	int length = dir.Length();
	if (dir[ length - 1 ] == ':')
		dir += "\\*";

	uint prevErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	Win32FindData fd;
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 dirUTF16(dir);
	WinHandle h = FindFirstFile(dirUTF16.GetLPWSTR(), &fd);
	#else
	WinHandle h = FindFirstFile(dir.Chr(), &fd);
	#endif

	SetErrorMode(prevErrorMode);
	if (h == INVALID_HANDLE_VALUE)
		return false;

	FindClose(h);
	if (fd.dwFileAttributes & _A_SUBDIR)
		return true;

	return false;

	#else
	tPathStdFile(dir);
	std::filesystem::file_status fstat = std::filesystem::status(dir.Chr());

	return std::filesystem::is_directory(fstat);
	#endif
}


#if defined(PLATFORM_WINDOWS)
bool tSystem::tDriveExists(const tString& driveLetter)
{
	tString drive = driveLetter;
	drive.ToUpper();

	char driveLet = drive[0];
	if ((driveLet > 'Z') || (driveLet < 'A'))
		return false;

	ulong driveBits = GetLogicalDrives();
	if (driveBits & (0x00000001 << (driveLet-'A')))
		return true;

	return false;
}
#endif


bool tSystem::tIsFileNewer(const tString& filenameA, const tString& filenameB)
{
	#if defined(PLATFORM_WINDOWS)
	tString fileA(filenameA);
	tPathWin(fileA);

	tString fileB(filenameB);
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


std::time_t tSystem::tFileTimeToStdTime(std::filesystem::file_time_type tp)
{
	using namespace std::chrono;
	auto sctp = time_point_cast<system_clock::duration>(tp - std::filesystem::file_time_type::clock::now() + system_clock::now());
	return system_clock::to_time_t(sctp);
}


#ifdef PLATFORM_WINDOWS
std::time_t tSystem::tFileTimeToPosixEpoch(FILETIME filetime)
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


#ifdef PLATFORM_WINDOWS
void tSystem::tGetFileInfo(tFileInfo& fileInfo, Win32FindData& fd)
{
	fileInfo.CreationTime = tFileTimeToPosixEpoch(fd.ftCreationTime);
	fileInfo.ModificationTime = tFileTimeToPosixEpoch(fd.ftLastWriteTime);
	fileInfo.AccessTime = tFileTimeToPosixEpoch(fd.ftLastAccessTime);

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


bool tSystem::tGetFileInfo(tFileInfo& fileInfo, const tString& fileName)
{
	fileInfo.Clear();
	fileInfo.FileName = fileName;
	tString file(fileName);

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
	tGetFileInfo(fileInfo, fd);
	FindClose(h);
	return true;

	#else
	tPathStd(file);
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
bool tSystem::tGetFileDetails(tFileDetails& details, const tString& fullFileName)
{
	tString ffn = fullFileName;
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
			#else
			tString title(33);
			StrRetToBuf(&shellDetail.str, localPidl, title.Txt(), 32);
			#endif

			// Get detail.
			#ifdef TACENT_UTF16_API_CALLS
			tStringUTF16 detail(33);
			#else
			tString detail(33);
			#endif
			result = shellFolder2->GetDetailsOf(localPidl, col, &shellDetail);
			if (result == S_OK)
			{
				#ifdef TACENT_UTF16_API_CALLS
				StrRetToBuf(&shellDetail.str, localPidl, detail.GetLPWSTR(), 32);
				#else
				StrRetToBuf(&shellDetail.str, localPidl, detail.Txt(), 32);
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
	tString appName(127);
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 keyString16A(keyString);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, keyString16A.GetLPWSTR(), 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
	#else
	if (RegOpenKeyEx(HKEY_CURRENT_USER, keyString.Chr(), 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
	#endif
	{
		ulong numBytesIO = 127;
		#ifdef TACENT_UTF16_API_CALLS
		RegGetValue(key, LPCWSTR(u""), 0, RRF_RT_REG_SZ | RRF_ZEROONFAILURE, 0, appName.Text(), &numBytesIO);
		#else
		RegGetValue(key, "", 0, RRF_RT_REG_SZ | RRF_ZEROONFAILURE, 0, appName.Text(), &numBytesIO);
		#endif
		RegCloseKey(key);
	}

	if (appName.IsEmpty())
		return tString();

	keyString = "Software\\Classes\\";
	keyString += appName;
	keyString += "\\shell\\open\\command";
	tString exeName(255);
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 keyString16B(keyString);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, keyString16B.GetLPWSTR(), 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
	#else
	if (RegOpenKeyEx(HKEY_CURRENT_USER, keyString.Chr(), 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
	#endif
	{
		ulong numBytesIO = 255;
		#ifdef TACENT_UTF16_API_CALLS
		RegGetValue(key, LPCWSTR(u""), 0, RRF_RT_REG_SZ | RRF_ZEROONFAILURE, 0, exeName.Txt(), &numBytesIO);
		#else
		RegGetValue(key, "", 0, RRF_RT_REG_SZ | RRF_ZEROONFAILURE, 0, exeName.Txt(), &numBytesIO);
		#endif
		RegCloseKey(key);
	}

	return exeName;
}
#endif // PLATFORM_WINDOWS


tString tSystem::tGetSimplifiedPath(const tString& srcPath, bool forceTreatAsDir)
{
	tString path = srcPath;
	tPathStd(path);

	// We do support filenames at the end. However, if the name ends with a "." (or "..") we
	// know it is a folder and so add a trailing "/".
	if (path[path.Length()-1] == '.')
		path += "/";

	if (forceTreatAsDir && (path[path.Length()-1] != '/'))
		path += "/";

	if (tIsDrivePath(path))
	{
		if ((path[0] >= 'a') && (path[0] <= 'z'))
			path[0] = 'A' + (path[0] - 'a');
	}

	// First we'll replace any "../" strings with "|".  Note that pipe indicators are not allowed
	// in filenames so we can safely use them.
	int numUps = path.Replace("../", "|");

	// Now we can remove any "./" strings since all that's left will be up-directory markers.
	path.Remove("./");
	if (!numUps)
		return path;

	// We need to preserve leading '..'s so that paths like ../../Hello/There/ will work.
	int numLeading = path.RemoveLeading("|");
	numUps -= numLeading;
	for (int nl = 0; nl < numLeading; nl++)
		path = "../" + path;

	tString simp;
	for (int i = 0; i < numUps; i++)
	{
		simp += path.ExtractLeft('|');
		simp = tGetUpDir(simp);
	}

	tString res = simp + path;
	return res;
}


bool tSystem::tIsDrivePath(const tString& path)
{
	if ((path.Length() > 1) && (path[1] == ':'))
		return true;

	return false;
}


bool tSystem::tIsAbsolutePath(const tString& path)
{
	if (tIsDrivePath(path))
		return true;

	if ((path.Length() > 0) && ((path[0] == '/') || (path[0] == '\\')))
		return true;

	return false;
}


tString tSystem::tGetFileName(const tString& filename)
{
	tString retStr(filename);
	tPathStd(retStr);
	return retStr.Right('/');
}


tString tSystem::tGetFileBaseName(const tString& filename)
{
	tString r = tGetFileName(filename);
	return r.Left('.');
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
	ret[ lastSlash + 1 ] = '\0';

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
	upPath[ upPath.Length() - 1 ] = '\0';

	for (int i = 0; i < levels; i++)
	{
		int lastSlash = upPath.FindChar('/', true);

		if (isNetLoc && (upPath.CountChar('/') == 1))
			lastSlash = -1;

		if (lastSlash == -1)
			return tString();

		upPath[lastSlash] = '\0';
	}

	upPath += "/";

	if (isNetLoc)
	{
		ret[0] = '/';
		ret[1] = '/';
	}
	return upPath;
}


tString tSystem::tGetRelativePath(const tString& basePath, const tString& path)
{
	#if defined(PLATFORM_WINDOWS)
	tAssert(basePath[ basePath.Length() - 1 ] == '/');
	bool isDir = (path[ path.Length() - 1 ] == '/') ? true : false;

	tString basePathMod = basePath;
	tPathWin(basePathMod);

	tString pathMod = path;
	tPathWin(pathMod);

	#ifdef TACENT_UTF16_API_CALLS	
	tStringUTF16 relLoc16(MAX_PATH);
	tStringUTF16 basePathMod16(basePathMod);
	tStringUTF16 pathMod16(pathMod);
	int success = PathRelativePathTo
	(
		relLoc16.GetLPWSTR(), basePathMod16.GetLPWSTR(), FILE_ATTRIBUTE_DIRECTORY,
		pathMod16.GetLPWSTR(), isDir ? FILE_ATTRIBUTE_DIRECTORY : 0
	);
	#else
	tString relLoc(MAX_PATH);
	int success = PathRelativePathTo
	(
		relLoc.Txt(), basePathMod.Chr(), FILE_ATTRIBUTE_DIRECTORY,
		pathMod.Chr(), isDir ? FILE_ATTRIBUTE_DIRECTORY : 0
	);
	#endif

	if (!success)
		return tString();

	#ifdef TACENT_UTF16_API_CALLS
	tString relLoc(relLoc16);
	#endif

	tPathStd(relLoc);
	if (relLoc[0] == '/')
		return relLoc.Chr() + 1;
	else
		return relLoc;

	#else
	tString refPath(basePath);
	tString absPath(path);
	
	int sizer = refPath.Length()+1;
	int sizea = absPath.Length()+1;
	if (sizea <= 1)
		return tString();
	if (sizer<= 1)
		return absPath;

	// From stackoverflow cuz I don't feel like thinking.
	// https://stackoverflow.com/questions/36173695/how-to-retrieve-filepath-relatively-to-a-given-directory-in-c	
	char relPath[1024];
	relPath[0] = '\0';
	char* pathr = refPath.Txt();
	char* patha = absPath.Txt();
	int inc = 0;

	for (; (inc < sizea) && (inc < sizer); inc += tStd::tStrlen(patha+inc)+1)
	{
		char* tokena = tStd::tStrchr(patha+inc, '/');
		char* tokenr = tStd::tStrchr(pathr+inc, '/');
		
		if (tokena) *tokena = '\0';
		if (tokenr) *tokenr = '\0';
		if (tStd::tStrcmp(patha+inc, pathr+inc) != 0)
			break;
	}

	if (inc < sizea)
		tStd::tStrcat(relPath, absPath.Txt()+inc);

	tString ret(relPath);
	if (ret[ret.Length()-1] != '/')
		ret += '/';
		
	return ret;
	#endif
}


tString tSystem::tGetAbsolutePath(const tString& pth, const tString& basePath)
{
	tString path(pth);
	tPathStd(path);
	if (tIsRelativePath(path))
	{
		if (basePath.IsEmpty())
			path = tGetCurrentDir() + path;
		else
			path = basePath + path;
	}

	return tGetSimplifiedPath(path);
}


tString tSystem::tGetLinuxPath(const tString& pth, const tString& mountPoint)
{
	tString path(pth);
	tPathStd(path);
	if (tIsAbsolutePath(path) && (path.Length() > 1) && (path[1] == ':') && !mountPoint.IsEmpty())
	{
		tString mnt = mountPoint;
		tPathStdDir(mnt);

		char drive = tStd::tChrlwr(path[0]);
		path.ExtractLeft(2);
		path = mnt + tString(drive) + path;
	}
	return path;
}


tString tSystem::tGetFileFullName(const tString& filename)
{
	tString file(filename);
	
	#if defined(PLATFORM_WINDOWS)
	tPathWin(file);
	tString ret(_MAX_PATH + 1);
	_fullpath(ret.Txt(), file.Chr(), _MAX_PATH);
	tPathStd(ret);
	
	#else
	tPathStd(file);
	tString ret(PATH_MAX + 1);
	realpath(file.Chr(), ret.Txt());	
	#endif

	return ret;
}


#if defined(PLATFORM_WINDOWS)
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
		driveInfo.VolumeName = volumeInfoName;
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


tString tSystem::tGetWindowsDir()
{
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 windir16(MAX_PATH);
	GetWindowsDirectory(windir16.GetLPWSTR(), MAX_PATH);
	tString windir(windir16);
	#else
	tString windir(MAX_PATH);
	GetWindowsDirectory(windir.Txt(), MAX_PATH);
	#endif

	tPathStdDir(windir);
	return windir;
}


tString tSystem::tGetSystemDir()
{
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 sysdir16(MAX_PATH);
	GetSystemDirectory(sysdir16.GetLPWSTR(), MAX_PATH);
	tString sysdir(sysdir16);
	#else
	tString sysdir(MAX_PATH);
	GetSystemDirectory(sysdir.Txt(), MAX_PATH);
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
#endif // PLATFORM_WINDOWS


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
	tString result(result16);
	#else
	tString result(MAX_PATH);
	ulong l = GetModuleFileName(0, result.Txt(), MAX_PATH);
	#endif

	tPathStd(result);
	int bi = result.FindChar('/', true);
	tAssert(bi != -1);

	result[bi + 1] = '\0';
	return result;

	#elif defined(PLATFORM_LINUX)
	tString result(PATH_MAX+1);
	readlink("/proc/self/exe", result.Txt(), PATH_MAX);
	
	int bi = result.FindChar('/', true);
	tAssert(bi != -1);
	result[bi + 1] = '\0';
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
	tString result(result16);

	#else
	tString result(MAX_PATH);
	ulong l = GetModuleFileName(0, result.Txt(), MAX_PATH);
	#endif

	tPathStd(result);
	return result;

	#elif defined(PLATFORM_LINUX)
	tString result(PATH_MAX+1);
	readlink("/proc/self/exe", result.Txt(), PATH_MAX);
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
	tString r(r16);

	#else
	tString r(MAX_PATH);
	GetCurrentDirectory(MAX_PATH, r.Txt());
	#endif

	#else
	tString r(PATH_MAX + 1);
	getcwd(r.Txt(), PATH_MAX);

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


bool tSystem::tFindDirs(tList<tStringItem>& foundDirs, const tString& dir, bool includeHidden)
{
	#ifdef PLATFORM_WINDOWS

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
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || includeHidden)
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
					foundDirs.Append(new tStringItem(path + fn + "/"));
			}
		}
	} while (FindNextFile(h, &fd));

	FindClose(h);
	if (GetLastError() != ERROR_NO_MORE_FILES)
		return false;

	#else
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

		if (!entry.is_directory())
			continue;

		tString foundDir((char*)entry.path().u8string().c_str());
		
		// All directories end in a slash in tacent.
		if (foundDir[foundDir.Length()-1] != '/')
			foundDir += "/";
		if (includeHidden || !tIsHidden(foundDir))
			foundDirs.Append(new tStringItem(foundDir));
	}

	#endif
	return true;
}


bool tSystem::tFindDirsRec(tList<tStringItem>& foundDirs, const tString& dir, bool includeHidden)
{
	#ifdef PLATFORM_WINDOWS
	tString pathStr(dir);

	tPathWinDir(pathStr);
	tFindDirs(foundDirs, pathStr, includeHidden);
	Win32FindData fd;
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 pathStrMod16(pathStr + "*.*");
	WinHandle h = FindFirstFile(pathStrMod16.GetLPWSTR(), &fd);
	#else
	WinHandle h = FindFirstFile((pathStr + "*.*").Chr(), &fd);
	#endif
	if (h == INVALID_HANDLE_VALUE)
		return false;

	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// Don't recurse into hidden subdirectories if includeHidden is false.
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || includeHidden)
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
					tFindDirsRec(foundDirs, pathStr + fn + "\\", includeHidden);
			}
		}
	} while (FindNextFile(h, &fd));

	FindClose(h);
	if (GetLastError() != ERROR_NO_MORE_FILES)
		return false;

	#else
	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(dir.Chr()))
	{
		if (!entry.is_directory())
			continue;

		tString foundDir((char*)entry.path().u8string().c_str());

		if (includeHidden || !tIsHidden(foundDir))
			foundDirs.Append(new tStringItem(foundDir));
	}

	#endif
	return true;
}


bool tSystem::tFindDirs(tList<tFileInfo>& foundDirs, const tString& dir)
{
	#ifdef PLATFORM_WINDOWS
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
				tFileInfo* fileInfo = new tFileInfo();
				fileInfo->FileName = tString(path + fn + "/");

				// This is the fast windows-specific tGetFileInfo.
				tGetFileInfo(*fileInfo, fd);
				foundDirs.Append(fileInfo);
			}
		}
	} while (FindNextFile(h, &fd));

	FindClose(h);
	if (GetLastError() != ERROR_NO_MORE_FILES)
		return false;

	#else
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

		if (!entry.is_directory())
			continue;

		tString foundDir((char*)entry.path().u8string().c_str());
		
		// All directories end in a slash in tacent.
		if (foundDir[foundDir.Length()-1] != '/')
			foundDir += "/";

		tFileInfo* fileInfo = new tFileInfo();
		fileInfo->FileName = foundDir;

		#ifdef USE_STD_FILESYSTEM_FOR_FILEINFO
		std::error_code ec;
		std::filesystem::file_status status = entry.status(ec);
		entry.last_write_time().
		fileInfo->ReadOnly = false;
		if (!ec)
		{
			std::filesystem::perms perms = status.permissions();
			bool w = (perms & std::filesystem::perms::owner_write) ? true : false;;
			bool r = (perms & std::filesystem::perms::owner_read);
			fileInfo->ReadOnly = (r && !w);
		}
		#endif

		// @todo Since we already have a std::filesystem entry, we could extract everything from that
		// instead of using stat. Chrono time conversion code would need to be written.
		tGetFileInfo(*fileInfo, foundDir);
		foundDirs.Append(fileInfo);
	}

	#endif
	return true;
}


bool tSystem::tFindDirsRec(tList<tFileInfo>& foundDirs, const tString& dir)
{
	#ifdef PLATFORM_WINDOWS
	tString pathStr(dir);

	tPathWinDir(pathStr);
	tFindDirs(foundDirs, pathStr);
	Win32FindData fd;
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 pathStrMod16(pathStr + "*.*");
	WinHandle h = FindFirstFile(pathStrMod16.GetLPWSTR(), &fd);
	#else
	WinHandle h = FindFirstFile((pathStr + "*.*").Chr(), &fd);
	#endif
	if (h == INVALID_HANDLE_VALUE)
		return false;

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
				tFindDirsRec(foundDirs, pathStr + fn + "\\");
		}
	} while (FindNextFile(h, &fd));

	FindClose(h);
	if (GetLastError() != ERROR_NO_MORE_FILES)
		return false;

	#else
	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(dir.Chr()))
	{
		if (!entry.is_directory())
			continue;

		tString foundDir((char*)entry.path().u8string().c_str());

		// All directories end in a slash in tacent.
		if (foundDir[foundDir.Length()-1] != '/')
			foundDir += "/";

		tFileInfo* fileInfo = new tFileInfo();
		fileInfo->FileName = foundDir;

		// @todo Since we already have a std::filesystem entry, we could extract everything from that
		// instead of using stat. Chrono time conversion code would need to be written.
		tGetFileInfo(*fileInfo, foundDir);
		foundDirs.Append(fileInfo);
	}

	#endif
	return true;
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
		success = tDirExists(dirPath.Chr());

	return success;

	#else
	tPathStdFile(dirPath);
	bool ok = std::filesystem::create_directory(dirPath.Chr());
	if (!ok)
		return tDirExists(dirPath.Chr());

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


bool tSystem::tIsReadOnly(const tString& fileName)
{
	tString file(fileName);

	#if defined(PLATFORM_WINDOWS)
	tPathWinFile(file);

	// The docs for this should be clearer!  GetFileAttributes returns INVALID_FILE_ATTRIBUTES if it
	// fails.  Rather dangerously, and undocumented, INVALID_FILE_ATTRIBUTES has a value of 0xFFFFFFFF.
	// This means that all attribute are apparently true!  This is very lame.  Thank goodness there aren't
	// 32 possible attributes, or there could be real problems.  Too bad it didn't just return 0 on error...
	// especially since they specifically have a FILE_ATTRIBUTES_NORMAL flag that is non-zero!
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 file16(file);
	ulong attribs = GetFileAttributes(file16.GetLPWSTR());
	#else
	ulong attribs = GetFileAttributes(file.Chr());
	#endif
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	return (attribs & FILE_ATTRIBUTE_READONLY) ? true : false;

	#else
	tPathStd(file);

	struct stat st;
	int errCode = stat(file.Chr(), &st);
	if (errCode != 0)
		return false;

	bool w = (st.st_mode & S_IWUSR) ? true : false;
	bool r = (st.st_mode & S_IRUSR) ? true : false;
	return r && !w;

	#endif
}


bool tSystem::tSetReadOnly(const tString& fileName, bool readOnly)
{
	tString file(fileName);
	
	#if defined(PLATFORM_WINDOWS)	
	tPathWinFile(file);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 file16(file);
	ulong attribs = GetFileAttributes(file16.GetLPWSTR());
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	if (!(attribs & FILE_ATTRIBUTE_READONLY) && readOnly)
		SetFileAttributes(file16.GetLPWSTR(), attribs | FILE_ATTRIBUTE_READONLY);
	else if ((attribs & FILE_ATTRIBUTE_READONLY) && !readOnly)
		SetFileAttributes(file16.GetLPWSTR(), attribs & ~FILE_ATTRIBUTE_READONLY);

	attribs = GetFileAttributes(file16.GetLPWSTR());
	#else
	ulong attribs = GetFileAttributes(file.Chr());
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	if (!(attribs & FILE_ATTRIBUTE_READONLY) && readOnly)
		SetFileAttributes(file.Chr(), attribs | FILE_ATTRIBUTE_READONLY);
	else if ((attribs & FILE_ATTRIBUTE_READONLY) && !readOnly)
		SetFileAttributes(file.Chr(), attribs & ~FILE_ATTRIBUTE_READONLY);

	attribs = GetFileAttributes(file.Chr());	
	#endif

	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	if (!!(attribs & FILE_ATTRIBUTE_READONLY) == readOnly)
		return true;

	return false;

	#else
	tPathStd(file);
	
	struct stat st;
	int errCode = stat(file.Chr(), &st);
	if (errCode != 0)
		return false;
	
	uint32 permBits = st.st_mode;

	// Set user R and clear user w. Leave rest unchanged.
	permBits |= S_IRUSR;
	permBits &= ~S_IWUSR;
	errCode = chmod(file.Chr(), permBits);
	
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
		tString fileName = path;
		fileName[fileName.Length()] = '\0';
		fileName = tGetFileName(fileName);
		if ((fileName != ".") && (fileName != "..") && (fileName[0] == '.'))
			return true;
	}
	return false;

	#elif defined(PLATFORM_WINDOWS)
	// In windows it's all based on the file attribute.
	tString file(path);
	tPathWinFile(file);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 file16(file);
	ulong attribs = GetFileAttributes(file16.GetLPWSTR());
	#else
	ulong attribs = GetFileAttributes(file.Chr());
	#endif
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	return (attribs & FILE_ATTRIBUTE_HIDDEN) ? true : false;

	#else
	return false;

	#endif
}


#if defined(PLATFORM_WINDOWS)
bool tSystem::tSetHidden(const tString& fileName, bool hidden)
{
	tString file(fileName);
	tPathWinFile(file);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 file16(file);
	ulong attribs = GetFileAttributes(file16.GetLPWSTR());
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;
	if (!(attribs & FILE_ATTRIBUTE_HIDDEN) && hidden)
		SetFileAttributes(file16.GetLPWSTR(), attribs | FILE_ATTRIBUTE_HIDDEN);
	else if ((attribs & FILE_ATTRIBUTE_HIDDEN) && !hidden)
		SetFileAttributes(file16.GetLPWSTR(), attribs & ~FILE_ATTRIBUTE_HIDDEN);
	attribs = GetFileAttributes(file16.GetLPWSTR());

	#else
	ulong attribs = GetFileAttributes(file.Chr());
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;
	if (!(attribs & FILE_ATTRIBUTE_HIDDEN) && hidden)
		SetFileAttributes(file.Chr(), attribs | FILE_ATTRIBUTE_HIDDEN);
	else if ((attribs & FILE_ATTRIBUTE_HIDDEN) && !hidden)
		SetFileAttributes(file.Chr(), attribs & ~FILE_ATTRIBUTE_HIDDEN);
	attribs = GetFileAttributes(file.Chr());
	#endif

	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	if (!!(attribs & FILE_ATTRIBUTE_HIDDEN) == hidden)
		return true;

	return false;
}


bool tSystem::tIsSystem(const tString& fileName)
{
	tString file(fileName);
	tPathWinFile(file);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 file16(file);
	ulong attribs = GetFileAttributes(file16.GetLPWSTR());
	#else
	ulong attribs = GetFileAttributes(file.Chr());
	#endif

	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	return (attribs & FILE_ATTRIBUTE_SYSTEM) ? true : false;
}


bool tSystem::tSetSystem(const tString& fileName, bool system)
{
	tString file(fileName);
	tPathWinFile(file);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 file16(file);
	ulong attribs = GetFileAttributes(file16.GetLPWSTR());
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;
	if (!(attribs & FILE_ATTRIBUTE_SYSTEM) && system)
		SetFileAttributes(file16.GetLPWSTR(), attribs | FILE_ATTRIBUTE_SYSTEM);
	else if ((attribs & FILE_ATTRIBUTE_SYSTEM) && !system)
		SetFileAttributes(file16.GetLPWSTR(), attribs & ~FILE_ATTRIBUTE_SYSTEM);
	attribs = GetFileAttributes(file16.GetLPWSTR());

	#else
	ulong attribs = GetFileAttributes(file.Chr());
	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;
	if (!(attribs & FILE_ATTRIBUTE_SYSTEM) && system)
		SetFileAttributes(file.Chr(), attribs | FILE_ATTRIBUTE_SYSTEM);
	else if ((attribs & FILE_ATTRIBUTE_SYSTEM) && !system)
		SetFileAttributes(file.Chr(), attribs & ~FILE_ATTRIBUTE_SYSTEM);
	attribs = GetFileAttributes(file.Chr());
	#endif

	if (attribs == INVALID_FILE_ATTRIBUTES)
		return false;

	if (!!(attribs & FILE_ATTRIBUTE_SYSTEM) == system)
		return true;

	return false;
}
#endif // PLATFORM_WINDOWS


bool tSystem::tCopyFile(const tString& dest, const tString& src, bool overWriteReadOnly)
{
	#if defined(PLATFORM_WINDOWS)

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 src16(src);
	tStringUTF16 dest16(dest);
	int success = ::CopyFile(src16.GetLPWSTR(), dest16.GetLPWSTR(), 0);

	#else
	int success = ::CopyFile(src.Chr(), dest.Chr(), 0);
	#endif

	if (!success && overWriteReadOnly)
	{
		tSetReadOnly(dest, false);
		#ifdef TACENT_UTF16_API_CALLS
		success = ::CopyFile(src16.GetLPWSTR(), dest16.GetLPWSTR(), 0);
		#else
		success = ::CopyFile(src.Chr(), dest.Chr(), 0);
		#endif
	}
	return success ? true : false;

	#else
	std::filesystem::path pathFrom(src.Chr());
	std::filesystem::path pathTo(dest.Chr());
	bool success = std::filesystem::copy_file(pathFrom, pathTo);
	if (!success && overWriteReadOnly)
	{
		tSetReadOnly(dest, false);
		success = std::filesystem::copy_file(pathFrom, pathTo);
	}
		
	return success;

	#endif
}


bool tSystem::tRenameFile(const tString& dir, const tString& oldName, const tString& newName)
{
	#if defined(PLATFORM_WINDOWS)
	tString fullOldName = dir + oldName;
	tPathWin(fullOldName);

	tString fullNewName = dir + newName;
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
	tString fullOldName = dir + oldName;
	tPathStd(fullOldName);
	std::filesystem::path oldp(fullOldName.Chr());

	tString fullNewName = dir + newName;
	tPathStd(fullNewName);
	std::filesystem::path newp(fullNewName.Chr());

	std::error_code ec;
	std::filesystem::rename(oldp, newp, ec);
	return !bool(ec);

	#endif
}


bool tSystem::tFindFilesInternal(tList<tStringItem>& foundFiles, const tString& dir, const tExtensions* extensions, bool includeHidden)
{
	if (extensions && extensions->IsEmpty())
		return false;

	// Use current directory if no dirPath supplied.
	tString dirPath(dir);
	if (dirPath.IsEmpty())
		dirPath = (char*)std::filesystem::current_path().u8string().c_str();

	if (dirPath.IsEmpty())
		return false;

	// Even root should look like "/".
	if (dirPath[dirPath.Length() - 1] == '\\')
		dirPath[dirPath.Length() - 1] = '/';

	std::error_code errorCode;
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dirPath.Text(), errorCode))
	{
		if (errorCode || (entry == std::filesystem::directory_entry()))
		{
			errorCode.clear();
			continue;
		}

		if (!entry.is_regular_file())
			continue;

		tString foundFile((char*)entry.path().u8string().c_str());
		tString foundExt = tGetFileExtension(foundFile);

		// If no extension match continue.
		if (extensions && !extensions->Contains(foundExt))
			continue;

		if (includeHidden || !tIsHidden(foundFile))
			foundFiles.Append(new tStringItem(foundFile));
	}

	return true;
}


bool tSystem::tFindFiles(tList<tStringItem>& foundFiles, const tString& dir, const tString& ext, bool includeHidden)
{
	tExtensions extensions;
	if (!ext.IsEmpty())
		extensions.Add(ext);
	return tFindFiles(foundFiles, dir, extensions, includeHidden);
}


bool tSystem::tFindFiles(tList<tStringItem>& foundFiles, const tString& dir, const tExtensions& extensions, bool includeHidden)
{
	return tFindFilesInternal(foundFiles, dir, &extensions, includeHidden);
}


bool tSystem::tFindFiles(tList<tStringItem>& foundFiles, const tString& dir, bool includeHidden)
{
	return tFindFilesInternal(foundFiles, dir, nullptr, includeHidden);
}


bool tSystem::tFindFilesFastInternal(tList<tStringItem>* foundFiles, tList<tFileInfo>* foundInfos, const tString& dir, const tExtensions* extensions, bool includeHidden)
{
	if (extensions && extensions->IsEmpty())
		return false;

	// FindFirstFile etc seem to like backslashes better.
	tString dirStr(dir);
	if (dirStr.IsEmpty())
		dirStr = tGetCurrentDir();

	#ifdef PLATFORM_WINDOWS
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
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || includeHidden)
				{
					#ifdef TACENT_UTF16_API_CALLS
					tString fdFilename((char16_t*)fd.cFileName);
					#else
					tString fdFilename(fd.cFileName);
					#endif
					tString foundName = dirStr + fdFilename;
					tPathStd(foundName);

					tStringItem* newName = foundFiles ? new tStringItem(foundName) : nullptr;
					tFileInfo*   newInfo = foundInfos ? new tFileInfo() : nullptr;
					if (newInfo)
					{
						newInfo->FileName = foundName;
						tGetFileInfo(*newInfo, fd);
					}

					// Holy obscure and annoying FindFirstFile bug! FindFirstFile("*.abc", ...) will also find
					// files like file.abcd. This isn't correct I guess we have to check the extension here.
					// FileMask is required to specify an extension, even if it is ".*"
					if (path[path.Length() - 1] != '*')
					{
						tString foundExtension = tGetFileExtension(fdFilename);
						if (ext.IsEqualCI(foundExtension))
						{
							if (foundFiles)
								foundFiles->Append(newName);
							if (foundInfos)
								foundInfos->Append(newInfo);
						}
					}
					else
					{
						if (foundFiles)
							foundFiles->Append(newName);
						if (foundInfos)
							foundInfos->Append(newInfo);
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

		if (includeHidden || !tIsHidden(foundFile))
		{
			if (foundFiles)
				foundFiles->Append(new tStringItem(foundFile));

			if (foundInfos)
			{
				tFileInfo* newFileInfo = new tFileInfo();
				tGetFileInfo(*newFileInfo, foundFile);
				foundInfos->Append(newFileInfo);
			}
		}
	}
	closedir(dirEnt);
	return true;

	#endif
}


bool tSystem::tFindFilesFast(tList<tStringItem>& foundFiles, const tString& dir, const tString& ext, bool includeHidden)
{
	tExtensions extensions;
	if (!ext.IsEmpty())
		extensions.Add(ext);
	return tFindFilesFast(foundFiles, dir, extensions, includeHidden);
}


bool tSystem::tFindFilesFast(tList<tStringItem>& foundFiles, const tString& dir, const tExtensions& extensions, bool includeHidden)
{
	return tFindFilesFastInternal(&foundFiles, nullptr, dir, &extensions, includeHidden);
}


bool tSystem::tFindFilesFast(tList<tStringItem>& foundFiles, const tString& dir, bool includeHidden)
{
	return tFindFilesFastInternal(&foundFiles, nullptr, dir, nullptr, includeHidden);
}


bool tSystem::tFindFilesFast(tList<tFileInfo>& foundInfos, const tString& dir, const tString& ext, bool includeHidden)
{
	tExtensions extensions;
	if (!ext.IsEmpty())
		extensions.Add(ext);
	return tFindFilesFast(foundInfos, dir, extensions, includeHidden);
}


bool tSystem::tFindFilesFast(tList<tFileInfo>& foundInfos, const tString& dir, const tExtensions& extensions, bool includeHidden)
{
	return tFindFilesFastInternal(nullptr, &foundInfos, dir, &extensions, includeHidden);
}


bool tSystem::tFindFilesFast(tList<tFileInfo>& foundInfos, const tString& dir, bool includeHidden)
{
	return tFindFilesFastInternal(nullptr, &foundInfos, dir, nullptr, includeHidden);
}


bool tSystem::tFindFilesRec(tList<tStringItem>& foundFiles, const tString& dir, const tString& ext, bool includeHidden)
{
	#ifdef PLATFORM_WINDOWS

	// The windows functions seem to like backslashes better.
	tString pathStr(dir);
	tPathWinDir(pathStr);
	if (ext.IsEmpty())
		tFindFiles(foundFiles, dir, includeHidden);
	else
		tFindFiles(foundFiles, dir, ext, includeHidden);
	Win32FindData fd;

	// Look for all directories.
	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 pathStrMod16(pathStr + "*.*");
	WinHandle h = FindFirstFile(pathStrMod16.GetLPWSTR(), &fd);
	#else
	WinHandle h = FindFirstFile((pathStr + "*.*").Chr(), &fd);
	#endif
	if (h == INVALID_HANDLE_VALUE)
		return false;

	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// Don't recurse into hidden subdirectories if includeHidden is false.
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || includeHidden)
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
					tFindFilesRec(foundFiles, pathStr + fn + "\\", ext, includeHidden);
			}
		}
	} while (FindNextFile(h, &fd));

	FindClose(h);
	if (GetLastError() != ERROR_NO_MORE_FILES)
		return false;

	#else
	for (const std::filesystem::directory_entry& entry: std::filesystem::recursive_directory_iterator(dir.Chr()))
	{
		if (!entry.is_regular_file())
			continue;

		tString foundFile((char*)entry.path().u8string().c_str());
		if (!ext.IsEmpty() && (!ext.IsEqualCI(tGetFileExtension(foundFile))))
			continue;

		if (includeHidden || !tIsHidden(foundFile))
			foundFiles.Append(new tStringItem(foundFile));
	}
	#endif

	return true;
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


bool tSystem::tCreateFile(const tString& file)
{
	tFileHandle f = tOpenFile(file.Chr(), "wt");
	if (!f)
		return false;

	tCloseFile(f);
	return true;
}


bool tSystem::tCreateFile(const tString& filename, const tString& contents)
{
	uint32 len = contents.Length();
	return tCreateFile(filename, (uint8*)contents.Chr(), len);
}


bool tSystem::tCreateFile(const tString& filename, uint8* data, int dataLength)
{
	tFileHandle dst = tOpenFile(filename.Chr(), "wb");
	if (!dst)
		return false;

	// Sometimes this needs to be done, for some mysterious reason.
	tFileSeek(dst, 0, tSeekOrigin::Beginning);

	// Write data and close file.
	int numWritten = tWriteFile(dst, data, dataLength);
	tCloseFile(dst);

	// Make sure it was created and an appropriate amount of bytes were written.
	bool verify = tFileExists(filename);
	return verify && (numWritten >= dataLength);
}


bool tSystem::tCreateFile(const tString& filename, char8_t* data, int length, bool writeBOM)
{
	tFileHandle dst = tOpenFile(filename.Chr(), "wb");
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
	bool verify = tFileExists(filename);
	return verify && (numWritten >= length);
}


bool tSystem::tCreateFile(const tString& filename, char16_t* data, int length, bool writeBOM)
{
	tFileHandle dst = tOpenFile(filename.Chr(), "wb");
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
	bool verify = tFileExists(filename);
	return verify && (numWritten >= length);
}


bool tSystem::tCreateFile(const tString& filename, char32_t* data, int length, bool writeBOM)
{
	tFileHandle dst = tOpenFile(filename.Chr(), "wb");
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
	bool verify = tFileExists(filename);
	return verify && (numWritten >= length);
}


bool tSystem::tLoadFile(const tString& filename, tString& dst, char convertZeroesTo)
{
	if (!tFileExists(filename))
	{
		dst.Clear();
		return false;
	}

	int filesize = tGetFileSize(filename);
	if (filesize == 0)
	{
		dst.Clear();
		return true;
	}

	dst.Reserve(filesize);
	uint8* check = tLoadFile(filename, (uint8*)dst.Text());
	if ((check != (uint8*)dst.Text()) || !check)
		return false;

	if (convertZeroesTo != '\0')
	{
		for (int i = 0; i < filesize; i++)
			if (dst[i] == '\0')
			dst[i] = convertZeroesTo;
	}

	return true;
}


uint8* tSystem::tLoadFile(const tString& filename, uint8* buffer, int* fileSize, bool appendEOF)
{
	tFileHandle f = tOpenFile(filename.Chr(), "rb");
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


uint8* tSystem::tLoadFileHead(const tString& fileName, int& bytesToRead, uint8* buffer)
{
	tFileHandle f = tOpenFile(fileName, "rb");
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


bool tSystem::tDeleteFile(const tString& filename, bool deleteReadOnly, bool useRecycleBin)
{
	#ifdef PLATFORM_WINDOWS
	tString file(filename);
	tPathWin(file);

	#ifdef TACENT_UTF16_API_CALLS
	tStringUTF16 file16(file);
	if (deleteReadOnly)
		SetFileAttributes(file16.GetLPWSTR(), FILE_ATTRIBUTE_NORMAL);
	#else
	if (deleteReadOnly)
		SetFileAttributes(file.Chr(), FILE_ATTRIBUTE_NORMAL);
	#endif

	if (!useRecycleBin)
	{
		#ifdef TACENT_UTF16_API_CALLS
		if (DeleteFile(file16.GetLPWSTR()))
		#else
		if (DeleteFile(file.Chr()))
		#endif
			return true;
		else
			return false;
	}
	else
	{
		tString filenamePlusChar = filename + "Z";
		#ifdef TACENT_UTF16_API_CALLS
		tStringUTF16 filenameDoubleNull16(filenamePlusChar);
		*(filenameDoubleNull16.Units() + filenameDoubleNull16.Length() - 1) = 0;
		#else
		tString filenameDoubleNull(filenamePlusChar);
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
	if (!deleteReadOnly && tIsReadOnly(filename))
		return true;

	std::filesystem::path p(filename.Chr());

	if (useRecycleBin)
	{
		tString homeDir = tGetHomeDir();
		tString recycleDir = homeDir + ".local/share/Trash/files/";
		if (tDirExists(recycleDir))
		{
			tString toFile = recycleDir + tGetFileName(filename);
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
