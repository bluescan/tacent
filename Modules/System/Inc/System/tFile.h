// tFile.h
//
// This file contains a class implementation of a file on a disk. It derives from tStream. By passing around tStreams
// any user code can be oblivious to the type of stream. It may be a file on disk, it may be a file in a custom
// filesystem, it may be a pipe, or even a network resource.
//
// Paths
// * A filesystem path may be represented by a tString or a tPath (not implemented yet but will essentially be a list of
//   strings).
// * Paths when represented by strings use forward slashes as the separator unless it is the beginnging part of a
//   Windows network shar in which case the first two seperators are \\ and \.
//   eg. Posix/Linux path  : "/home/username/work/important.txt
//   eg. Windows file path : "C:/Work/Important.txt"
//   eg. Windows net share : "\\machinename\sharename/Work/Important.txt"
// * A path can refer to either a file or a directory. If used for a directory it _always_ ends in a forward-slash /.
// * Input paths to functions here may use backslashes, but consistency in using forward slashes is advised.
//
// A note on variable naming of paths in this API. If it can be a file or directory, the word 'path' is used. If the
// path must be a directory, the word 'dir' is used. If the path must be a file, the word 'file' is used.
//
// Copyright (c) 2004-2006, 2017, 2020-2023 Tristan Grimmer.
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


// Some file-system calls have the option to use the C++ standard std::filesystem backend. This enum lets you choose
// to use the standard or native APIs. Native is always faster, so that is the usual default, but doing it this way
// (i.e. not forcing native) is a good way to get unit tests going so we can check the correctness of both paths.
enum class Backend
{
	Native,		// eg. 'stat' on Linux. FindFirstFile etc on Windows.
	Stndrd
};


//
// Functions that are file handle based.
//

tFileHandle tOpenFile(const char8_t* file, const char* mode);
tFileHandle tOpenFile(const char* file, const char* mode);
void tCloseFile(tFileHandle);
int tGetFileSize(tFileHandle);
int tReadFile(tFileHandle, void* buffer, int sizeBytes);
int tWriteFile(tFileHandle, const void* buffer, int sizeBytes);
int tWriteFile(tFileHandle, const char8_t* buffer, int length);
int tWriteFile(tFileHandle, const char16_t* buffer, int length);
int tWriteFile(tFileHandle, const char32_t* buffer, int length);
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


//
// Path-based functions work on the syntax of a path but generally do not need to access the filesystem.
//

// Directories are paths that end in a /.
bool tIsDir(const tString& path);

// Files are paths that don't end in a /.
bool tIsFile(const tString& path);

// Uses working dir. Mess.max to c:/Stuff/Mess.max. This function always assumes filename is relative.
tString tGetFileFullName(const tString& file);

// c:/Stuff/Mess.max to Mess.max
tString tGetFileName(const tString& file);

// c:/Stuff/Mess.max to Mess
tString tGetFileBaseName(const tString& file);

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
bool tIsAbsolutePath(const tString& path);
bool tIsRelativePath(const tString& path);

// Converts the path into a simplified absolute path. It will work whether the path was originally absolute or
// relative. If you do not supply a basePath dir, the current working dir will be used. The basePath is only used if
// the supplied path was relative.
tString tGetAbsolutePath(const tString& path, const tString& basePath = tString());

// Returns the relative location of path from basePath. Both these input strings must have a common prefix for this to
// succeed. Returns an empty string if it fails.
tString tGetRelativePath(const tString& basePath, const tString& path);

// Drive paths are DOS/Windows style absolute paths that begin with a drive letter followed by a colon.
// For example, "C:/Hello" would return true, "/mnt/c/Hello" would return false.
bool tIsDrivePath(const tString& path);

// Converts to a Linux-style path. That is, all backslashes become forward slashes, and drive letters get converted to
// mount points. eg. "D:\Stuff\Mess.max" will return "/mnt/d/Stuff/Mess.max"
tString tGetLinuxPath(const tString& path, const tString& mountPoint = "/mnt/");

// Given a path, this function returns the directory portion. If the input was only a filename, it returns the current
// directory string "./". If input is a path specifying a directory, it will return that same path.
// eg. "c:/Stuff/Mess.max" will return "c:/Stuff/"
// eg. "Hello.txt" will return "./"
// eg. "/Only/Path/No/File/" will return "/Only/Path/No/File/"
// Windows network shares retain only required backslashes.
// eg. "\\machine\share/dir/subdir/file.txt" will return "\\machine\share/dir/subdir/"
tString tGetDir(const tString& path);

// Given a valid path ending with a slash, this function returns the path n levels higher in the hierarchy. It returns
// the empty string if you go too high or if path was empty.
// eg, "c:/HighDir/MedDir/LowDir/" will return "c:/HighDir/MedDir/" if levels = 1.
// eg. "c:/HighDir/MedDir/LowDir/" will return "c:/HighDir/" if levels = 2.
// eg. "c:/HighDir/MedDir/LowDir/" will return "" if levels = 4.
tString tGetUpDir(const tString& path, int levels = 1);


//
// Path-based functions that access the filesystem.
//

// Test if a file exists. Supplied file name should not have a trailing slash. Will return false if you use on
// directories or drives. Use tDirExists for that purpose. Windows Note: tFileExists will not bring up an error box for
// a removable drive without media in it.
bool tFileExists(const tString& file);

// Check if a directory or logical drive exists. Valid directory names include "E:/", "C:/Program Files/" etc. Drives
// without media in them are considered non-existent. For example, if "E:/" refers to a CD ROM drive without media in
// it, you'll get a false because you can't actually enter that directory. If the drive doesn't exist on the system at
// all you'll get a false as well. If you want to check if a drive letter exists on windows, use tDriveExists.
bool tDirExists(const tString& dir);

// Returns 0 if the file doesn't exist. Also returns 0 if the file exists and its size is actually 0.
int tGetFileSize(const tString& file);

// Works for both files and directories. Returns false if read-only not set or an error occurred like the path not
// existing. For Lixux returns true is user w permission flag not set and r permission flag is set.
bool tIsReadOnly(const tString& path);

// Works for both files and directories. Returns true on success. For Linux, sets the user w permission flag as
// appropriate and the user r permission flag to true. For Windows sets the attribute.
bool tSetReadOnly(const tString& path, bool readOnly = true);

// Works on files and directories. For Linux, checks if first character of file is a dot (and not ".."). For Windows it
// checks the hidden file attribute regardless of whether it starts with a dot or not. If you want a hidden file or
// directory that is hidden on both types of filesystem (fat/ntfs and extN) make your hidden file/dir start with a dot
// (Linux) and set the hidden attribute (Windows).
bool tIsHidden(const tString& path);

#if defined(PLATFORM_WINDOWS)
// These are Windows-only as they access platform-specific attributes or drives. The Set call returns success.
// @todo Make equavalents to set Linux permissions for user, group, other.
bool tSetHidden(const tString& path, bool hidden = true);
bool tIsSystem(const tString& file);
bool tSetSystem(const tString& file, bool system = true);

// Drive letter can be of form "C" or "C:" or "C:/" in either lower or upper case for this function.
bool tDriveExists(const tString& driveName);
#endif

bool tIsFileNewer(const tString& fileA, const tString& fileB);

// If either (or both) file doesn't exist you get false. Entire files will temporarily be read into memory so it's not
// too efficient (only for tool use).
bool tFilesIdentical(const tString& fileA, const tString& fileB);

// Overwrites dest if it exists. Returns true if success. Will return false and not copy if overWriteReadOnly is false
// and the file already exists and is read-only.
bool tCopyFile(const tString& destFile, const tString& srcFile, bool overWriteReadOnly = true);

// Renames the file or directory specified by oldName to the newName. This function can only be used for renaming, not
// moving. Returns true on success. The dir variable should contain the path to where the file or dir you want to rename
// is located.
bool tRenameFile(const tString& dir, const tString& oldPathName, const tString& newPathName);

// Creates an empty file.
bool tCreateFile(const tString& file);
bool tCreateFile(const tString& file, const tString& contents);
bool tCreateFile(const tString& file, uint8* data, int length);

// For easily creating UTF-encoded text files. It is not recommended to write a BOM for UTF-8.
bool tCreateFile(const tString& file, char8_t*  data, int length, bool writeBOM = false);
bool tCreateFile(const tString& file, char16_t* data, int length, bool writeBom = true);
bool tCreateFile(const tString& file, char32_t* data, int length, bool writeBOM = true);

// Returns true if file existed and was deleted. If tryUseRecycleBin is true and the function can't find the recycle
// bin, it will return false. It is up to you to call it again with tryUseRecycleBin false if you really want the
// file gone.
bool tDeleteFile(const tString& file, bool deleteReadOnly = true, bool tryUseRecycleBin = false);

// Loads entire file into memory. If buffer is nullptr you must free the memory returned at some point by using
// delete[]. If buffer is non-nullptr it must be at least GetFileSize big (+1 if appending EOF). Any problems (file not exist or is
// unreadable etc) and nullptr is returned. Fills in the file size pointer if you supply one (not including optional appened EOF). It is perfectly valid to
// load a file with no data (0 bytes big). In this case LoadFile always returns nullptr even if a non-zero buffer was
// passed in and the fileSize member will be set to 0 (if supplied).
uint8* tLoadFile(const tString& file, uint8* buffer = nullptr, int* fileSize = nullptr, bool appendEOF = false);

// Similar to above, but is best used with a text file. If a binary file is supplied and convertZeroesTo is left at
// default, any null characters '\0' are turned into separators (31). This ensures that the string length will be
// correct. Use convertZeroesTo = '\0' to leave it unmodified, but expect length to be incorrect if a binary file is
// supplied.
bool tLoadFile(const tString& file, tString& dst, char convertZeroesTo = 31);

// Same as LoadFile except only the first bytesToRead bytes are read. Also the actual number read is returned in
// bytesToRead. This will be smaller than the number requested if the file is too small. If there are any problems,
// bytesToRead will contain 0 and if a buffer was supplied it will be returned (perhaps modified). If one wasn't
// supplied and there is a read problem, nullptr will be returned.
uint8* tLoadFileHead(const tString& file, int& bytesToRead, uint8* buffer = nullptr);

// @todo This variant is not implemented yet.
uint8* tLoadFileHead(const tString& file, int bytesToRead, tString& dest);


//
// System path, drive, and network share information.
//
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

#if defined(PLATFORM_WINDOWS)
tString tGetWindowsDir();
tString tGetSystemDir();
tString tGetDesktopDir();

// Gets a list of the drive letters present on the system. The strings returned are in the form "C:". For more
// information on a particular drive, use the DriveInfo functions below. Note that this function may return drive
// letters for drives that are not ready (removable media sometimes acts this way). If you need to determine
// whether a drive is 'ready', the DriveInfo function can do that. It is not done here for efficiency.
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

enum class tDriveState
{
	Unknown,
	Ready,
	NotReady
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
	tDriveState DriveState;
};

// Gets info about a logical drive. Asking for the display name causes a shell call and takes a bit longer, so only
// ask for the info you need. DriveInfo is always filled out if the function succeeds. Returns true if the DriveInfo
// struct was filled out. Returns false if there was a problem like the drive didn't exist. Drive should be in the form
// "C", or "C:", or "C:/", or C:\". It is possible that the name strings end up empty and the function succeeds, so
// check for that. This will happen if the drive exists, but the name is empty or could not be determined. If
// getDisplayName is false, returned DisplayName will be empty. If getStateVolumeSerial is false, VolumeName will be
// empty, SerialNumber will be 0, and DriveState will be Unknown.
bool tGetDriveInfo(tDriveInfo&, const tString& drive, bool getDisplayName = false, bool getStateVolumeSerial = false);

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


//
// File types, extensions, and file details.
//
// File types are based on file extensions only. If this enum is modified there is an extension mapping table in
// tFile.cpp that needs to be updated as well. The compiler will static-assert if the number of entries does not match.
// In order to be clear about the distinction between filetype and extensions, the types here do not have any synonyms.
// A single filetype may map to multiple extensions.
enum class tFileType
{
//	Type						Description
	Unknown		= -1,
	Invalid		= Unknown,
	EndOfList	= Unknown,
	EOL			= Unknown,

	TGA,						// Image. Targa.
	BMP,						// Image. Windows bitmap.
	QOI,						// Image. Quite OK Image Format.
	PNG,						// Image. Portable Network Graphics.
	APNG,						// Image. Animated PNG.
	GIF,						// Image. Graphics Interchange Format. Pronounced like the peanut butter.
	WEBP,						// Image. Google Web Image.
	XPM,						// Image. X-Windows Pix Map.
	JPG,						// Image. Joint Picture Motion Group (or something like that).
	TIFF,						// Image. Tag Interchange File Format.
	DDS,						// Image. Direct Draw Surface. TextureMap/CubeMap.
	KTX,						// Image. Khronos Texture. Similar to a dds file.
	KTX2,						// Image. Khronos Texture V2. Similar to a dds file.
	PVR,						// Image. Imagination Technologies PowerVR format.
	ASTC,						// Image. ARM's Adaptive Scalable Texture Compression format.
	PKM,						// Image. Ericsson ETC1 Image.
	HDR,						// Image. Radiance High Dynamic Range.
	EXR,						// Image. OpenEXR High Dynamic Range.
	PCX,						// Image.
	WBMP,						// Image.
	WMF,						// Image.
	JP2,						// Image.
	JPC,						// Image.
	ICO,						// Image. Windows Icon.
	TAC,						// Image. Tacent Image.
	CFG,						// Config. Text Config File.
	INI,						// Config. Ini Config File.
	TXT,						// Generic. Text File.
	NumFileTypes
};
struct tFileTypes;

// c:/Stuff/Mess.max to max
tString tGetFileExtension(const tString& file);

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

// For the file-type name we use the most common (default) extension string. Essentially this does the same thing as
// tGetExtension. If we need something more descriptive we could add a tGetFileTypeDesc(...)
tString tGetFileTypeName(tFileType);

// This does the reverse. Gets the file-type from the supplied file-type name. Essentially this does the same thing as
// tGetFileTypeFromExtension.
tFileType tGetFileTypeFromName(const tString& name);

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
	// arguments. The last tFileType _must_ be tFileType::EndOfList or tFileType::EOL. Example:
	// tFileTypes gTypes(tFileType::JPG, tFileType::PNG, tFileType::EOL);
	tFileTypes(tFileType, ...);

	// All the add functions check for uniqueness when adding.
	tFileTypes& Add(const tFileTypes& src);
	tFileTypes& Add(const char* ext);
	tFileTypes& Add(const tString& ext);
	tFileTypes& Add(tFileType);
	tFileTypes& Add(const tExtensions&);
	tFileTypes& AddVA(tFileType, ...);
	tFileTypes& AddVA(tFileType, va_list);
	tFileTypes& AddSelected(const tFileTypes& src, bool addAllIfNoneSelected = false);

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

	// Utility functions dealing with selected state.
	void ClearSelected();
	bool AnySelected() const;
	tFileType GetFirstSelectedType() const;
	enum class Separator { Comma, Space, CommaSpace };
	tString GetSelectedString(Separator sepType = Separator::CommaSpace, int maxBeforeEllipsis = -1) const;

	// @todo Could use a BST, maybe a balanced AVL BST tree. Would make 'Contains' much faster, plus it would deal
	tList<tFileTypeItem> FileTypes;

	// A user specified name for this collection of file types. Use is optional. Could be something like "Image" if this
	// collection of types is exclusively comprised of image types.
	tString UserName;
};

// This contains info about a file OR a directory. I guess it's really a tFileOrDirInfo.
struct tFileInfo : public tLink<tFileInfo>
{
	tFileInfo();
	tFileInfo(const tFileInfo& src);
	void Clear();

	tString FileName;
	uint64 FileSize;

	// These are in POSIX Epoch time -- number of seconds that have elapsed since January 1, 1970 (midnight UTC/GMT),
	// For all std::time_t values we interpret -1 as invalid.
	std::time_t CreationTime;
	std::time_t ModificationTime;
	std::time_t AccessTime;
	bool ReadOnly;
	bool Hidden;
	bool Directory;
};

// Returns true if the FileInfo struct was filled out. Returns false if there was a problem like the file didn't exist.
// In this case the struct is left unmodified. This function can be used to get file or directory information.
bool tGetFileInfo(tFileInfo&, const tString& path);

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
bool tGetFileDetails(tFileDetails&, const tString& path);

// Sets the desktop 'open' verb file association. Idempotent. The extensions should NOT include the dot. The specified
// program should be fully qualified (absolute). You may set more than one extension for the same program.
void tSetFileOpenAssoc(const tString& program, const tString& extension, const tString& options = tString());
void tSetFileOpenAssoc(const tString& program, const tList<tStringItem>& extensions, const tString& options = tString());

// Gets the program and options associated with a particular extension.
tString tGetFileOpenAssoc(const tString& extension);
#endif


//
// File and directory enumeration Functions.
//
// For all following functions when the backend is Stndrd it uses the C++17 std::filesystem calls. While it is clean
// cross-plat API, it can be quite slow... up to 10x slower on Linux. When the backend is Native (the default) then
// platform-specific native file access functions are used (FindFirstFile etc for windows, readdir etc for Linux). They
// are much faster. The order of items returned in files is not defined. In particular, Native and Stndrd may not
// populated the results in the same order. If a platform is not supported and Native is specified, it falls back to
// the Stndrd backend.
//
// Finds sub-directories inside the supplied dir. If the dir to search is empty, the current directory is used. If
// hidden is true, includes hidden directories. The destination list (dirs) is appended to and not cleared so you can
// collect results if necessary. Returns success.
bool tFindDirs   (tList<tStringItem>& dirs, const tString& dir = tString(), bool hidden = true, Backend = Backend::Native);

// This version of tFindDirs can be used if you need additional information along with each directory.
// It is faster to use this than the tStringItem version above in conjunction with tGetFileInfo calls.
bool tFindDirs   (tList<tFileInfo>&   dirs, const tString& dir = tString(), bool hidden = true, Backend = Backend::Native);

// This function finds files in a directory. The files list is always appended to. You must clear it first if that's what you intend. If empty dir
// argument, the contents of the current directory are returned. If false returned files is unmodified. The order of
// items in files is not defined.
bool tFindFiles(tList<tStringItem>& files, const tString& dir, bool hidden = true, Backend = Backend::Native);

// A variant of the above except you can specify only files with a specific extension. Extension can be something like
// "txt" (no dot). On all platforms the extension is not case sensitive. eg. giF will match Gif. Returns false if ext
// empty or no files found. Tacent does not consider files that have no extension special. If you want files that have
// no extension call the previous variant of FindFiles that has no extension argument.
//
// In previous Tacent releases all filetypes were returned if ext was empty, but that was awkward as the semantics
// changed. It is now a separate function.
bool tFindFiles(tList<tStringItem>& files, const tString& dir, const tString& ext, bool hidden = true, Backend = Backend::Native);

// This is similar to the above function but lets you specify more than one extension at a time. This has huge
// performance implications (esp on Linux) if you need to find more than one extension in a directory. If tExtensions
// is empty false will be returned. The order of items in files is not defined. Returns success.
//
// In previous Tacent releases all filetypes were returned if extensions was empty, but that was awkward as the
// semantics changed. It is now a separate function.
bool tFindFiles(tList<tStringItem>& files, const tString& dir, const tExtensions&, bool hidden = true, Backend = Backend::Native);

// If you need the full file info for all the files you are enumerating, call this instead of the tFindFiles above. It
// is much faster than getting the filenames and calling tGetFileInfo on each one -- a lot faster, esp on windows.
bool tFindFiles(tList<tFileInfo>& files, const tString& dir, bool hidden = true, Backend = Backend::Native);
bool tFindFiles(tList<tFileInfo>& files, const tString& dir, const tString& ext, bool hidden = true, Backend = Backend::Native);
bool tFindFiles(tList<tFileInfo>& files, const tString& dir, const tExtensions&, bool hidden = true, Backend = Backend::Native);

// Recursive variants of the functions above. 'files' is appened to. Clear first if desired. See comments above for
// behaviour. Because recursive queries can be dangerous if you are too close to the filesystem root, these are
// separate functions rather than an argument switch.
bool tFindDirsRec(tList<tStringItem>&  dirs,  const tString& dir = tString(), bool hidden = true, Backend = Backend::Native);
bool tFindDirsRec(tList<tFileInfo>&    dirs,  const tString& dir = tString(), bool hidden = true, Backend = Backend::Native);
bool tFindFilesRec(tList<tStringItem>& files, const tString& dir, bool hidden = true, Backend = Backend::Native);
bool tFindFilesRec(tList<tStringItem>& files, const tString& dir, const tString& ext, bool hidden = true, Backend = Backend::Native);
bool tFindFilesRec(tList<tStringItem>& files, const tString& dir, const tExtensions&, bool hidden = true, Backend = Backend::Native);
bool tFindFilesRec(tList<tFileInfo>&   files, const tString& dir, bool hidden = true, Backend = Backend::Native);
bool tFindFilesRec(tList<tFileInfo>&   files, const tString& dir, const tString& ext, bool hidden = true, Backend = Backend::Native);
bool tFindFilesRec(tList<tFileInfo>&   files, const tString& dir, const tExtensions&, bool hidden = true, Backend = Backend::Native);

// Creates a directory. It can also handle creating all the directories in a path. Calling with a string like
// "C:/DirA/DirB/" will ensure that DirA and DirB exist. Returns true if successful.
bool tCreateDir(const tString& dir);

// A relentless delete. Doesn't care about read-only unless deleteReadOnly is false. This call does a recursive delete.
// If a file has an open handle, however, this fn will fail. If the directory didn't exist before the call then this function silently returns. Returns true if dir existed and was deleted.
bool tDeleteDir(const tString& directory, bool deleteReadOnly = true);

// @todo Implement the tFile class. Right now we're basically just reserving the class name.
class tFile : public tStream { tFile(const tString& file, tStream::tModes modes)																: tStream(modes) { } };

// File hash functions using tHash standard hash algorithms.
uint32 tHashFileFast32(  const tString& filename, uint32         iv = tHash::HashIV32);
uint32 tHashFile32(      const tString& filename, uint32         iv = tHash::HashIV32);
uint64 tHashFile64(      const tString& filename, uint64         iv = tHash::HashIV64);
tuint128 tHashFile128(   const tString& filename, tuint128       iv = tHash::HashIV128);
tuint256 tHashFile256(   const tString& filename, const tuint256 iv = tHash::HashIV256);
tuint128 tHashFileMD5(   const tString& filename, tuint128       iv = tHash::HashIVMD5);
tuint256 tHashFileSHA256(const tString& filename, const tuint256 iv = tHash::HashIVSHA256);


};


// The following file system error objects may be thrown by functions in tFile.
struct tFileError : public tError
{
	tFileError(const char* format, ...)																					: tError("tFile Module. ") { va_list marker; va_start(marker, format); Message += tsrvPrintf(format, marker); }
	tFileError(const tString& m)																						: tError("tFile Module. ") { Message += m; }
	tFileError()																										: tError("tfile Module.") { }
};


// Implementation below this line.


inline tFileHandle tSystem::tOpenFile(const char8_t* file, const char* mode)
{
	return fopen((const char*)file, mode);
}


inline tFileHandle tSystem::tOpenFile(const char* file, const char* mode)
{
	return fopen(file, mode);
}


inline void tSystem::tCloseFile(tFileHandle f)
{
	if (!f)
		return;

	fclose(f);
}


inline int tSystem::tReadFile(tFileHandle handle, void* buffer, int sizeBytes)
{
	// Load the entire thing into memory.
	int numRead = int(fread((char*)buffer, 1, sizeBytes, handle));
	return numRead;
}


inline int tSystem::tWriteFile(tFileHandle handle, const void* buffer, int sizeBytes)
{
	int numWritten = int(fwrite((void*)buffer, 1, sizeBytes, handle));
	return numWritten;
}


inline int tSystem::tWriteFile(tFileHandle handle, const char8_t* buffer, int length)
{
	int numWritten = int(fwrite((void*)buffer, 1, length, handle));
	return numWritten;
}


inline int tSystem::tWriteFile(tFileHandle handle, const char16_t* buffer, int length)
{
	int numWritten = int(fwrite((void*)buffer, 2, length, handle));
	return numWritten;
}


inline int tSystem::tWriteFile(tFileHandle handle, const char32_t* buffer, int length)
{
	int numWritten = int(fwrite((void*)buffer, 4, length, handle));
	return numWritten;
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


inline int tSystem::tFileTell(tFileHandle handle)
{
	return int(ftell(handle));
}


inline bool tSystem::tIsDir(const tString& path)
{
	if (path.IsEmpty())
		return false;

	return (path[path.Length()-1] == '/');
}


inline bool tSystem::tIsFile(const tString& path)
{
	if (path.IsEmpty())
		return false;

	return (path[path.Length()-1] != '/');
}


inline bool tSystem::tIsAbsolutePath(const tString& path)
{
	if (tIsDrivePath(path))
		return true;

	if ((path.Length() > 0) && ((path[0] == '/') || (path[0] == '\\')))
		return true;

	return false;
}


inline bool tSystem::tIsRelativePath(const tString& path)
{
	return !tIsAbsolutePath(path);
}


inline bool tSystem::tIsDrivePath(const tString& path)
{
	if ((path.Length() > 1) && (path[1] == ':'))
		return true;

	return false;
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
	DriveState = tDriveState::Unknown;
}
#endif


inline tString tSystem::tGetFileTypeName(tFileType fileType)
{
	return tGetExtension(fileType);
}


inline tSystem::tFileType tSystem::tGetFileTypeFromName(const tString& name)
{
	return tGetFileTypeFromExtension(name);
}


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

	return Add(ext.Chr());
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


inline tSystem::tFileTypes& tSystem::tFileTypes::AddSelected(const tFileTypes& src, bool addAllIfNoneSelected)
{
	if (addAllIfNoneSelected && !src.AnySelected())
	{
		for (tFileTypeItem* typeItem = src.First(); typeItem; typeItem = typeItem->Next())
			Add(typeItem->FileType);
		return *this;
	}
	for (tFileTypeItem* typeItem = src.First(); typeItem; typeItem = typeItem->Next())
		if (typeItem->Selected)
			Add(typeItem->FileType);

	return *this;
}


inline bool tSystem::tFileTypes::Contains(tFileType fileType) const
{
	for (tFileTypeItem* fti = FileTypes.First(); fti; fti = fti->Next())
		if (fileType == fti->FileType)
			return true;

	return false;
}


inline void tSystem::tFileTypes::ClearSelected()
{
	for (tFileTypeItem* typeItem = First(); typeItem; typeItem = typeItem->Next())
		typeItem->Selected = false;
}


inline bool tSystem::tFileTypes::AnySelected() const
{
	for (tFileTypeItem* typeItem = First(); typeItem; typeItem = typeItem->Next())
		if (typeItem->Selected)
			return true;
	return false;
}


inline tSystem::tFileType tSystem::tFileTypes::GetFirstSelectedType() const
{
	for (tFileTypeItem* typeItem = First(); typeItem; typeItem = typeItem->Next())
	if (typeItem->Selected)
		return typeItem->FileType;
	return tFileType::Invalid;
}


inline tString tSystem::tFileTypes::GetSelectedString(Separator sepType, int maxBeforeEllipsis) const
{
	tString str;
	bool isFirst = true;
	int numAdded = 0;
	const char* sep = ",";
	if (sepType == Separator::Space)
		sep = " ";
	if (sepType == Separator::CommaSpace)
		sep = ", ";
	
	for (tFileTypeItem* typeItem = First(); typeItem; typeItem = typeItem->Next())
	{
		if (typeItem->Selected)
		{
			if ((maxBeforeEllipsis > -1) && (numAdded >= maxBeforeEllipsis))
			{
				str += " ...";
				return str;
			}
			if (!isFirst)
				str += sep;
			else
				isFirst = false;
			str += tString(tSystem::tGetFileTypeName(typeItem->FileType));
			numAdded++;
		}
	}

	return str;
}


inline tSystem::tFileInfo::tFileInfo() :
	FileName(),
	FileSize(0),
	CreationTime(-1),
	ModificationTime(-1),
	AccessTime(-1),
	ReadOnly(false),
	Hidden(false),
	Directory(false)
{
}


inline tSystem::tFileInfo::tFileInfo(const tFileInfo& src) :
	FileName(src.FileName),
	FileSize(src.FileSize),
	CreationTime(src.CreationTime),
	ModificationTime(src.ModificationTime),
	AccessTime(src.AccessTime),
	ReadOnly(src.ReadOnly),
	Hidden(src.Hidden),
	Directory(src.Directory)
{
}


inline void tSystem::tFileInfo::Clear()
{
	FileName.Clear();
	FileSize = 0;
	CreationTime = -1;
	ModificationTime = -1;
	AccessTime = -1;
	ReadOnly = false;
	Hidden = false;
	Directory = false;
}
