// tFile.h
//
// This file contains a class implementation of a file on a disk. It derives from tStream. By passing around tStreams
// any user code can be oblivious to the type of stream. It may be a file on disk, it may be a file in a custom
// filesystem, it may be a pipe, or even a network resource.
//
// A path can refer to either a file or a directory. All paths use forward slashes as the separator. Input paths can
// use backslashes, but consistency in using forward slashes is advised. Directory path specifications always end with
// a trailing slash. Without the trailing separator the path will be interpreted as a file.
//
// Copyright (c) 2004-2006, 2017, 2020-2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <ctime>
#include <Foundation/tHash.h>
#include "System/tThrow.h"
#include "System/tPrint.h"
#include "System/tStream.h"
namespace tSystem
{


// @todo Implement the tFile class.
class tFile : public tStream
{
	tFile(const tString& fileName, tStream::tModes modes)																: tStream(modes) { }
};

int tGetFileSize(tFileHandle);
int tGetFileSize(const tString& fileName);
tFileHandle tOpenFile(const char* filename, const char* mode);
void tCloseFile(tFileHandle);
int tReadFile(tFileHandle, void* buffer, int sizeBytes);
int tWriteFile(tFileHandle, const void* buffer, int sizeBytes);
bool tPutc(char, tFileHandle);
int tGetc(tFileHandle);
int tFileTell(tFileHandle);

enum class tSeekOrigin
{
	Beginning,		// AKA seek_set.
	Current,
	End,
	Set				= Beginning
};
int tFileSeek(tFileHandle, int offsetBytes, tSeekOrigin = tSeekOrigin::Beginning);

// Test if a file exists. Supplied filename should not have a trailing slash. Will return false if you use on
// directories or drives. Use tDirExists for that purpose. Windows Note: tFileExists will not bring up an error box for
// a removable drive without media in it.
bool tFileExists(const tString& fileName);

// Check if a directory or logical drive exists. Valid directory names include "E:/", "C:/Program Files/" etc. Drives
// without media in them are considered non-existent. For example, if "E:/" refers to a CD ROM drive without media in
// it, you'll get a false because you can't actually enter that directory. If the drive doesn't exist on the system at
// all you'll get a false as well. If you want to check if a drive letter exists on windows, use tDriveExists.
bool tDirExists(const tString& dirName);

#if defined(PLATFORM_WINDOWS)
// Drive letter can be of form "C" or "C:" or "C:/" or with lower case for this function.
bool tDriveExists(const tString& driveName);
#endif

// c:/Stuff/Mess.max to max
tString tGetFileExtension(const tString& filename);

// File types are based on file extensions only. If this enum is modified there is an extension mapping table
// in tFile.cpp that needs to be updated as well.
enum class tFileType
{
//	Type			Synonyms								Description
	Unknown = -1,	Invalid = Unknown, EndOfList = Unknown,
	TGA,			Targa = TGA,							// Image. Targa.
	BMP,			Bitmap = BMP,							// Image. Windows bitmap.
	PNG,			Ping = PNG,								// Image. Portable Network Graphics.
	APNG,													// Image. Animated PNG.
	GIF,													// Image. Graphics Interchange Format. Pronounced like the peanut butter.
	WEBP,													// Image. Google Web Image.
	XPM,													// Image. X-Windows Pix Map.
	JPG,			JPeg = JPG,								// Image. Joing Picture Motion Group (or something like that).
	TIFF,													// Image. Tag Interchange File Format.
	DDS,													// Image. Direct Draw Surface. TextureMap/CubeMap.
	HDR,													// Image. Radiance High Dynamic Range.
	EXR,													// Image. OpenEXR High Dynamic Range.
	PCX,													// Image.
	WBMP,													// Image.
	WMF,													// Image.
	JP2,													// Image.
	JPC,													// Image.
	ICO,			Icon = ICO,								// Image. Windows Icon.
	TEX,													// Image. Tacent TextureMap.
	IMG,													// Image. TextureMap.
	CUB,													// Image. Tacent CubeMap.
	TAC,													// Image. Tacent Layered 2D Image.
	CFG,													// Config. Text Config File.
	INI,													// Config. Ini Config File.
	NumFileTypes
};

// The supplied extension should not contain a period. Case insensitive.
tFileType tGetFileTypeFromExtension(const tString& ext);
tFileType tGetFileTypeFromExtension(const char* ext);

// The file does not need to exist for this function to work. This function only uses the extension to determine the
// file type.
tFileType tGetFileType(const tString& file);

// Get all extensions used by a particular filetype. Any existing items in extensions are appended to.
void tGetExtensions(tList<tStringItem>& extensions, tFileType);

// Gets the single most common or default extension for a given filetype. Existing items in extensions are appended to.
void tGetExtension(tList<tStringItem>& extensions, tFileType);
tString tGetExtension(tFileType);

// Currently this returns the same string as the most common (default) extension string.
const char* tGetFileTypeName(tFileType);

struct tFileTypes;

// A little helper type that holds file extension strings. Extensions are lower-case and do not include the dot.
struct tExtensions
{
	tExtensions()																										: Extensions() { }
	tExtensions(const tExtensions& src)																					: Extensions() { Add(src); }
	explicit tExtensions(const char* ext)																				: Extensions() { Add(ext); }
	tExtensions(const tString& ext)																						: Extensions() { Add(ext); }
	tExtensions(tFileType fileType, bool preferredExtensionOnly = false)												: Extensions() { Add(fileType, preferredExtensionOnly); }
	tExtensions(const tFileTypes& fileTypes, bool preferredExtensionsOnly = false)										: Extensions() { Add(fileTypes, preferredExtensionsOnly); }

	tExtensions& Add(const tExtensions& src);

	// These Add functions will remove any period and ensure lower-case before adding. They do not check for uniqueness.
	tExtensions& Add(const char* ext);
	tExtensions& Add(const tString& ext);

	// Populates the extension list based on the supplied filetype(s). If preferredExtensionsOnly is false the extensions
	// list will contain _all_ extensions for the supplied filetypes. If preferredExtensionsOnly is true, the list will
	// contain only the preferred extensions. For example, for JPG filetype with preferred true, only the "jpg"
	// extension would be added. With preferred = false, you'd get both "jpg" and "jpeg". Returns ref to self so you can
	// chain the calls.
	tExtensions& Add(tFileType, bool preferredExtensionOnly = false);
	tExtensions& Add(const tFileTypes&, bool preferredExtensionsOnly = false);

	void Clear()																										{ Extensions.Clear(); }
	int Count() const																									{ return Extensions.GetNumItems(); }
	bool IsEmpty() const																								{ return Extensions.IsEmpty(); }

	// Supplied extension must not include period.
	bool Contains(const tString& ext) const;
	tStringItem* First() const																							{ return Extensions.First(); }

	// This list stores the extensions lower-case without the dot.
	// @todo Could use a BST, maybe a balanced AVL BST tree. Would make 'Contains' much faster.
	tList<tStringItem> Extensions;

	// A user specified name for this collection of extensions. Use is optional.
	tString UserName;
};


// Another helper that stores a collection of file-types. Useful if you need, say, a list of file-types you want to
// support in your app. This is preferred over set of supported extensions as it is not always a 1:1 mapping.
struct tFileTypes
{
	tFileTypes()																										: FileTypes() { }
	tFileTypes(const tFileTypes& src)																					: FileTypes() { Add(src); }
	tFileTypes(const char* ext)																							: FileTypes() { Add(ext); }
	tFileTypes(const tString& ext)																						: FileTypes() { Add(ext); }
	tFileTypes(tFileType fileType)																						: FileTypes() { Add(fileType); }
	tFileTypes(const tExtensions& extensions)																			: FileTypes() { Add(extensions); }

	// This constructor may be used to create a static/global object by simply entering the tFileTypes you want as
	// arguments. The last tFileType _must_ be tFileType::EndOfList. Example:
	// tFileTypes gTypes(tFileType::JPG, tFileType::PNG, tFileType::EndOfList);
	tFileTypes(tFileType, ...);

	// All the add functions check for uniqueness when adding.
	tFileTypes& Add(const tFileTypes& src);
	tFileTypes& Add(const char* ext);
	tFileTypes& Add(const tString& ext);
	tFileTypes& Add(tFileType);
	tFileTypes& Add(const tExtensions&);
	tFileTypes& AddVA(tFileType, ...);
	tFileTypes& AddVA(tFileType, va_list);

	void Clear()																										{ FileTypes.Clear(); }
	int Count() const																									{ return FileTypes.GetNumItems(); }
	bool IsEmpty() const																								{ return FileTypes.IsEmpty(); }
	bool Contains(tFileType) const;

	struct tFileTypeItem : public tLink<tFileTypeItem>
	{
		tFileTypeItem()																									: FileType(tSystem::tFileType::Invalid), Selected(false) { }
		tFileTypeItem(tFileType fileType)																				: FileType(fileType), Selected(false) { }
		tFileTypeItem(const tFileTypeItem& src)																			: FileType(src.FileType), Selected(src.Selected) { }

		tFileType FileType;

		// A user-facing bool that is handy to keep track of selected state.
		bool Selected;
	};
	tFileTypeItem* First() const																						{ return FileTypes.First(); }

	// @todo Could use a BST, maybe a balanced AVL BST tree. Would make 'Contains' much faster, plus it would deal
	tList<tFileTypeItem> FileTypes;

	// A user specified name for this collection of file types. Use is optional. Could be something like "Image Files".
	tString UserName;
};

// Uses working dir. Mess.max to c:/Stuff/Mess.max. This function always assumes filename is relative.
tString tGetFileFullName(const tString& filename);

// c:/Stuff/Mess.max to Mess.max
tString tGetFileName(const tString& filename);

// c:/Stuff/Mess.max to Mess
tString tGetFileBaseName(const tString& filename);

bool tIsFileNewer(const tString& fileA, const tString& fileB);

struct tFileInfo : public tLink<tFileInfo>
{
	tFileInfo();
	void Clear();

	tString FileName;
	uint64 FileSize;
	std::time_t CreationTime;
	std::time_t ModificationTime;
	std::time_t AccessTime;
	bool ReadOnly;
	bool Hidden;
	bool Directory;
};

// Returns true if the FileInfo struct was filled out. Returns false if there was a problem like the file didn't exist.
// In this case the struct is left unmodified. This function can also be used to get directory information.
bool tGetFileInfo(tFileInfo&, const tString& fileName);

#ifdef PLATFORM_WINDOWS
struct tFileDetails
{
	tFileDetails()																										: DetailTitles(), Details() { }
	virtual ~tFileDetails()																								{ DetailTitles.Empty(); Details.Empty(); }

	// Both lists always have the same number of items. The memory for the strings is managed by this object. The items
	// in the lists will vary for different files. If a particular detail is not present for a certain file, it will
	// not be in the lists.
	tList<tStringItem> DetailTitles;
	tList<tStringItem> Details;
};

// Fills in a FileDetails struct. Details like Artist, Dimensions, etc. will be present in the file details list. This
// call uses the Shell on Windows. It's a little unclear how fast this call is, but it may be a bit of a resource hog.
// This function can also be used for drives and directories. Things like amount of free space, filesystem name, etc
// will be among the details. Returns false if there was a problem.
bool tGetFileDetails(tFileDetails&, const tString& fileName);

// Sets the desktop 'open' verb file association. Idempotent. The extensions should NOT include the dot. The specified
// program should be fully qualified (absolute). You may set more than one extension for the same program.
void tSetFileOpenAssoc(const tString& program, const tString& extension, const tString& options = tString());
void tSetFileOpenAssoc(const tString& program, const tList<tStringItem>& extensions, const tString& options = tString());

// Gets the program and options associated with a particular extension.
tString tGetFileOpenAssoc(const tString& extension);
#endif

// Returns a path or fully qualified filename that is as simple as possible. Mainly this involves removing (and
// resolving) any "." or ".." strings. This is a string manipulation call only -- it does not query the filesystem.
// For example, if the input is:
//
// "E:/Projects/Calamity/Crypto/../../Reign/./Squiggle/"
// the returned string will be
// "E:/Projects/Reign/Squiggle/".
//
// This function also works if a filename is specified at the end. If forceTreatAsDir is false, paths ending with a /
// are treated as directories and paths without a / are treated as files. If force is true, both are treated as dirs
// and the returned path will end with a /.
tString tGetSimplifiedPath(const tString& path, bool forceTreatAsDir = false);
bool tIsRelativePath(const tString& path);
bool tIsAbsolutePath(const tString& path);

// Drive paths are DOS/Windows style absolute paths that begin with a drive letter followed by a colon.
// For example, "C:/Hello" would return true, "/mnt/c/Hello" would return false.
bool tIsDrivePath(const tString& path);

// Returns the relative location of path from basePath. Both these input strings must have a common prefix for this to
// succeed. Returns an empty string if it fails.
tString tGetRelativePath(const tString& basePath, const tString& path);

// Converts the path into a simplified absolute path. It will work whether the path was originally absolute or
// relative. If you do not supply a basePath dir, the current working dir will be used. The basePath is only used if
// the supplied path was relative.
tString tGetAbsolutePath(const tString& path, const tString& basePath = tString());

// Converts to a Linux-style path. That is, all backslashes become forward slashes, and drive letters get converted to
// mount points. eg. "D:\Stuff\Mess.max" will return "/mnt/d/Stuff/Mess.max"
tString tGetLinuxPath(const tString& path, const tString& mountPoint = "/mnt/");

// Given a path, this function returns the directory portion. If the input was only a filename, it returns the current
// directory string "./". If input is a path specifying a directory, it will return that same path.
// eg. "c:/Stuff/Mess.max" will return "c:/Stuff/"
// eg. "Hello.txt" will return "./"
// eg. "/Only/Path/No/File/" will return "/Only/Path/No/File/"
tString tGetDir(const tString& path);

// Given a valid path ending with a slash, this function returns the path n levels higher in the hierarchy. It returns
// the empty string if you go too high or if path was empty.
// eg, "c:/HighDir/MedDir/LowDir/" will return "c:/HighDir/MedDir/" if levels = 1.
// eg. "c:/HighDir/MedDir/LowDir/" will return "c:/HighDir/" if levels = 2.
// eg. "c:/HighDir/MedDir/LowDir/" will return "" if levels = 4.
tString tGetUpDir(const tString& path, int levels = 1);

#if defined(PLATFORM_WINDOWS)

// Gets a list of the drive letters available on a system. The strings returned are in the form "C:". For more
// information on a particular drive, use the DriveInfo functions below.
void tGetDrives(tList<tStringItem>& drives);

enum class tDriveType
{
	Unknown,
	Floppy,
	Removable,
	HardDisk,
	Network,
	Optical,
	RamDisk
};

struct tDriveInfo
{
	tDriveInfo();
	void Clear();

	tString Letter;						// A two character drive letter string like "C:"
	tString DisplayName;				// The drive name like in the shell (windows explorer).
	tString VolumeName;
	uint32 SerialNumber;				// Seems to more or less uniquely identify a disc. Handy.
	tDriveType DriveType;
};

// Gets info about a logical drive. Asking for the display name causes a shell call and takes a bit longer, so only
// ask for the info you need. DriveInfo is always filled out if the function succeeds. Returns true if the DriveInfo
// struct was filled out. Returns false if there was a problem like the drive didn't exist. Drive should be in the form
// "C", or "C:", or "C:/", or C:\". It is possible that the name strings end up empty and the function succeeds, so
// check for that. This will happen if the drive exists, but the name is empty or could not be determined.
bool tGetDriveInfo(tDriveInfo&, const tString& drive, bool getDisplayName = false, bool getVolumeAndSerial = false);

// Sets the volume name of the specified drive. The drive string may take the format "C", "C:", "C:/", or "C:\". In
// some cases the name cannot be set. Read-only volumes or strange volume names will cause this function to return
// false (failure).
bool tSetVolumeName(const tString& drive, const tString& newVolumeName);

// Windows network shares.
struct tNetworkShareResult
{
	void Clear()					{ RequestComplete = false; NumSharesFound = 0; ShareNames.Empty(); }
	bool RequestComplete			= false;
	int NumSharesFound				= 0;
	tsList<tStringItem> ShareNames;
};

// This function blocks and takes quite a bit of time to run. However, the result struct places the shares in a
// thread-safe list (tsList) so you can spin up a thread to make this call, For now it does not signal so you would
// need to poll RequestComplete, but signalling intermediate results and complete could be added in the future.
// You can also treat the ShareNames ln the results as a message queue, taking the names off as they come in.
// The ShareNames take the format "\\MACHINENAME\ShareName". If retrieveMachinesWithNoShares this function will return
// all the machines it can find even if they don't have any shared folder. If false, only entries with valid shares
// will be returned. That is, with true you may get results like "\\MACHINENAME" as well.
int tGetNetworkShares(tNetworkShareResult&, bool retrieveMachinesWithNoShares = true);

// This is a convenience function to parse a single share name like "\\MACHINENAME\ShareName" into a list of strings.
// For example, "\\MACHINENAME\ShareName" turns into a list of 2 strings: "MACHINENAME" and "ShareName".
// If retrieveMachinesWithNoShares with true you will also get results like "\\MACHINENAME" which explode
// to a single string "MOUNTAINVIEW".
void tExplodeShareName(tList<tStringItem>& exploded, const tString& shareName);

#endif // PLATFORM_WINDOWS

// The following set and get functions work equally well on both files and directories. The "Set" calls return true on
// success. The "Is" calls return true if the attribute is set, and false if it isn't or an error occurred (like the
// object didn't exist).
bool tIsReadOnly(const tString& fileName);							// For Lixux returns true is user w flag not set and r flag is set.
bool tSetReadOnly(const tString& fileName, bool readOnly = true);	// For Linux, sets the user w flag as appropriate and the r flag to true.

// For Linux, checks if first character of filename is a dot (and not ".."). For Windows it checks the hidden
// fileattribute regardless of whether it starts with a dot or not. If you want hidden files in a way that is
// platform agnostic, make your hidden file start with a dot (Linux) , and set the hidden attribute (Windows).
bool tIsHidden(const tString& fileName);

#if defined(PLATFORM_WINDOWS)
bool tSetHidden(const tString& fileName, bool hidden = true);
bool tIsSystem(const tString& fileName);
bool tSetSystem(const tString& fileName, bool system = true);
tString tGetWindowsDir();
tString tGetSystemDir();
tString tGetDesktopDir();
#endif

// Gets the home directory. On Linux usually something like "/home/username/". On windows usually something like "C:/Users/UserName/".
tString tGetHomeDir();

// Gets the directory that the current process is being run from.
tString tGetProgramDir();

// Gets the full directory and executable name that the current process is being run from.
tString tGetProgramPath();

// Includes the trailing slash. Gets the current directory.
tString tGetCurrentDir();

// Set the current directory. Returns true if successful. For example, SetCurrentDir("C:/"); Will set it to the root of
// the c drive as will "C:" by itself. SetCurrentDir(".."); will move the current dir up a directory.
bool tSetCurrentDir(const tString& dir);

// Overwrites dest if it exists. Returns true if success. Will return false and not copy if overWriteReadOnly is false
// and the file already exists and is read-only.
bool tCopyFile(const tString& destFile, const tString& srcFile, bool overWriteReadOnly = true);

// Renames the file or directory specified by oldName to the newName. This function can only be used for renaming, not
// moving. Returns true on success. The dir variable should contain the path to where the file or dir you want to
// rename is located.
bool tRenameFile(const tString& dir, const tString& oldName, const tString& newName);

// The foundfiles list is always appended to. You must clear it first if that's what you intend. If empty second
// argument, the contents of the current directory are returned. Extension can be something like "txt" (no dot).
// On all platforms the extension is not case sensitive. eg. giF will match Gif. If ext is empty, all filetypes
// are included. The order of items in foundFiles is not defined. Returns success.
bool tFindFiles(tList<tStringItem>& foundFiles, const tString& dir, const tString& ext = tString(), bool includeHidden = true);

// This is similar to the above function but lets you specify more than one extension at a time. This has huge
// performance implications (esp on Linux) if you need to find more than one extension in a directory. If extensions
// is empty, all filetypes are included. The order of items in foundFiles is not defined. Returns success.
bool tFindFiles(tList<tStringItem>& foundFiles, const tString& dir, const tExtensions& extensions, bool includeHidden = true);

// The above two functions are based on the generic C++17 std::filesystem interface. While a clean cross-plat API,
// it really is quite slow... up to 10x slower on Linux. The following two functions do the same thing, but by using
// the native C file access functions (FindFirstFile etc for windows, readdir etc for Linux). They are, as the name
// indicates, much faster. The order of items in foundFiles is not defined. In particular, it may not match the non-
// fast versions of tFindFiles. Returns success.
bool tFindFilesFast(tList<tStringItem>& foundFiles, const tString& dir, const tString& ext = tString(), bool includeHidden = true);
bool tFindFilesFast(tList<tStringItem>& foundFiles, const tString& dir, const tExtensions& extensions, bool includeHidden = true);

// If you need the full file info for all the files you are enumerating, call this instead of the tFindFilesFast above.
// It is much faster than getting the filenames and calling tGetFileInfo on each one -- a lot faster, esp on windows.
bool tFindFilesFast(tList<tFileInfo>& foundFiles, const tString& dir, const tString& ext = tString(), bool includeHidden = true);
bool tFindFilesFast(tList<tFileInfo>& foundFiles, const tString& dir, const tExtensions& extensions, bool includeHidden = true);

// foundFiles is appened to. Clear first if desired. Extension can be something like "txt" (no dot).
bool tFindFilesRecursive(tList<tStringItem>& foundFiles, const tString& dir, const tString& ext = tString(), bool includeHidden = true);
bool tFindDirsRecursive(tList<tStringItem>& foundDirs, const tString& dir, bool includeHidden = true);

// If the dirPath to search is empty, the current dir is used. Returns success.
bool tFindDirs(tList<tStringItem>& foundDirs, const tString& dirPath = tString(), bool includeHidden = false);

// A relentless delete. Doesn't care about read-only unless deleteReadOnly is false. This call does a recursive delete.
// If a file has an open handle, however, this fn will fail. If the directory didn't exist before the call then this function silently returns. Returns true if dir existed and was deleted.
bool tDeleteDir(const tString& directory, bool deleteReadOnly = true);

// Creates a directory. It can also handle creating all the directories in a path. Calling with a string like
// "C:/DirA/DirB/" will ensure that DirA and DirB exist. Returns true if successful.
bool tCreateDir(const tString& dir);

// Returns true if file existed and was deleted. If tryUseRecycleBin is true and the function can't find the recycle
// bin, it will return false. It is up to you to call it again with tryUseRecycleBin false if you really want the
// file gone.
bool tDeleteFile(const tString& filename, bool deleteReadOnly = true, bool tryUseRecycleBin = false);

// If either (or both) file doesn't exist you get false. Entire files will temporarily be read into memory so it's not
// too efficient (only for tool use).
bool tFilesIdentical(const tString& fileA, const tString& fileB);

// Loads entire file into memory. If buffer is nullptr you must free the memory returned at some point by using
// delete[]. If buffer is non-nullptr it must be at least GetFileSize big (+1 if appending EOF). Any problems (file not exist or is
// unreadable etc) and nullptr is returned. Fills in the file size pointer if you supply one (not including optional appened EOF). It is perfectly valid to
// load a file with no data (0 bytes big). In this case LoadFile always returns nullptr even if a non-zero buffer was
// passed in and the fileSize member will be set to 0 (if supplied).
uint8* tLoadFile(const tString& filename, uint8* buffer = nullptr, int* fileSize = nullptr, bool appendEOF = false);

// Similar to above, but is best used with a text file. If a binary file is supplied and convertZeroesTo is left at
// default, any null characters '\0' are turned into separators (31). This ensures that the string length will be
// correct. Use convertZeroesTo = '\0' to leave it unmodified, but expect length to be incorrect if a binary file is
// supplied.
bool tLoadFile(const tString& filename, tString& dst, char convertZeroesTo = 31);

// Same as LoadFile except only the first bytesToRead bytes are read. Also the actual number read is returned in
// bytesToRead. This will be smaller than the number requested if the file is too small. If there are any problems,
// bytesToRead will contain 0 and if a buffer was supplied it will be returned (perhaps modified). If one wasn't
// supplied and there is a read problem, nullptr will be returned.
uint8* tLoadFileHead(const tString& filename, int& bytesToRead, uint8* buffer = nullptr);
uint8* tLoadFileHead(const tString& filename, int bytesToRead, tString& dest);
bool tCreateFile(const tString& filename);					// Creates an empty file.
bool tCreateFile(const tString& filename, const tString& contents);
bool tCreateFile(const tString& filename, uint8* data, int dataLength);

// File hash functions using tHash standard hash algorithms.
uint32 tHashFileFast32(const tString& filename, uint32 iv = tHash::HashIV32);
uint32 tHashFile32(const tString& filename, uint32 iv = tHash::HashIV32);
uint64 tHashFile64(const tString& filename, uint64 iv = tHash::HashIV64);
tuint128 tHashFile128(const tString& filename, tuint128 iv = tHash::HashIV128);
tuint256 tHashFile256(const tString& filename, const tuint256 iv = tHash::HashIV256);
tuint128 tHashFileMD5(const tString& filename, tuint128 iv = tHash::HashIVMD5);
tuint256 tHashFileSHA256(const tString& filename, const tuint256 iv = tHash::HashIVSHA256);


};


// The following file system error objects may be thrown by functions in tFile.
struct tFileError : public tError
{
	tFileError(const char* format, ...)																					: tError("tFile Module. ") { va_list marker; va_start(marker, format); Message += tvsPrintf(format, marker); }
	tFileError(const tString& m)																						: tError("tFile Module. ") { Message += m; }
	tFileError()																										: tError("tfile Module.") { }
};


// Implementation below this line.


inline tSystem::tExtensions& tSystem::tExtensions::Add(const tExtensions& src)
{
	for (tStringItem* ext = src.Extensions.First(); ext; ext = ext->Next())
		Extensions.Append(new tStringItem(*ext));

	return *this;
}


inline tSystem::tExtensions& tSystem::tExtensions::Add(const char* ext)
{
	if (!ext)
		return *this;

	tStringItem* item = new tStringItem(ext);
	item->ToLower();
	item->Remove('.');
	Extensions.Append(item);

	return *this;
}


inline tSystem::tExtensions& tSystem::tExtensions::Add(const tString& ext)
{
	if (ext.IsEmpty())
		return *this;

	return Add(ext.ConstText());
}


inline tSystem::tExtensions& tSystem::tExtensions::Add(tFileType fileType, bool preferredExtensionOnly)
{
	if (preferredExtensionOnly)
		tGetExtension(Extensions, fileType);
	else
		tGetExtensions(Extensions, fileType);

	return *this;
}


inline tSystem::tExtensions& tSystem::tExtensions::Add(const tFileTypes& types, bool preferredExtensionsOnly)
{
	for (tFileTypes::tFileTypeItem* typeItem = types.FileTypes.First(); typeItem; typeItem = typeItem->Next())
		Add(typeItem->FileType, preferredExtensionsOnly);

	return *this;
}


inline bool tSystem::tExtensions::Contains(const tString& searchExt) const
{
	for (tStringItem* ext = Extensions.First(); ext; ext = ext->Next())
	{
		if (ext->IsEqualCI(searchExt))
			return true;
	}

	return false;
}


inline tSystem::tFileTypes::tFileTypes(tFileType fileType, ...)
{
	Add(fileType);
	va_list valist;
	va_start(valist, fileType);
	AddVA(fileType, valist);
	va_end(valist);
}


inline tSystem::tFileTypes& tSystem::tFileTypes::Add(const tFileTypes& src)
{
	for (tFileTypeItem* fti = src.FileTypes.First(); fti; fti = fti->Next())
		Add(fti->FileType);

	return *this;
}


inline tSystem::tFileTypes& tSystem::tFileTypes::Add(const char* ext)
{
	return Add(tGetFileTypeFromExtension(ext));
}


inline tSystem::tFileTypes& tSystem::tFileTypes::Add(const tString& ext)
{
	return Add(tGetFileTypeFromExtension(ext));
}


inline tSystem::tFileTypes& tSystem::tFileTypes::Add(tFileType fileType)
{
	if (fileType == tFileType::Invalid)
		return *this;

	if (Contains(fileType))
		return *this;

	FileTypes.Append(new tFileTypeItem(fileType));
	return *this;
}


inline tSystem::tFileTypes& tSystem::tFileTypes::Add(const tExtensions& extensions)
{
	for (tStringItem* ext = extensions.First(); ext; ext = ext->Next())
		Add(tGetFileTypeFromExtension(*ext));

	return *this;
}


inline tSystem::tFileTypes& tSystem::tFileTypes::AddVA(tFileType fileType, ...)
{
	Add(fileType);
	va_list valist;
	va_start(valist, fileType);
	AddVA(fileType, valist);
	va_end(valist);
	return *this;
}


inline tSystem::tFileTypes& tSystem::tFileTypes::AddVA(tFileType fileType, va_list valist)
{
	if (fileType == tFileType::EndOfList)
		return *this;

	tFileType ftype = va_arg(valist, tFileType);
	while (ftype != tFileType::EndOfList)
	{
		Add(ftype);
		ftype = va_arg(valist, tFileType);
	}

	return *this;
}


inline bool tSystem::tFileTypes::Contains(tFileType fileType) const
{
	for (tFileTypeItem* fti = FileTypes.First(); fti; fti = fti->Next())
		if (fileType == fti->FileType)
			return true;

	return false;
}


inline bool tSystem::tPutc(char ch, tFileHandle file)
{
	int ret = putc(int(ch), file);
	if (ret == EOF)
		return false;
	return true;
}


inline int tSystem::tGetc(tFileHandle file)
{
	return fgetc(file);
}


inline tSystem::tFileInfo::tFileInfo() :
	FileName(),
	FileSize(0),
	CreationTime(0),
	ModificationTime(0),
	ReadOnly(false),
	Hidden(false),
	Directory(false)
{
}


inline void tSystem::tFileInfo::Clear()
{
	FileName.Clear();
	FileSize = 0;
	CreationTime = 0;
	ModificationTime = 0;
	ReadOnly = false;
	Hidden = false;
	Directory = false;
}


inline bool tSystem::tIsRelativePath(const tString& path)
{
	return !tIsAbsolutePath(path);
}


#if defined(PLATFORM_WINDOWS)
inline tSystem::tDriveInfo::tDriveInfo() :
	Letter(),
	DisplayName(),
	VolumeName(),
	SerialNumber(0),
	DriveType(tDriveType::Unknown)
{
}


inline void tSystem::tDriveInfo::Clear()
{
	Letter.Clear();
	DisplayName.Clear();
	VolumeName.Clear();
	SerialNumber = 0;
	DriveType = tDriveType::Unknown;
}
#endif
