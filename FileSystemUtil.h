#ifndef __FILE_IO_UTIL_H__
#define __FILE_IO_UTIL_H__

#include <iostream>
#include <iterator>
#include <optional>

#ifdef WIN32
#include <Windows.h>
#endif

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#endif

#ifdef WIN32
inline uint64_t CombineDWORD(DWORD low, DWORD high) {
	return (uint64_t)low + ((uint64_t)MAXDWORD + 1) * high;
}
#endif

/**
* wrap some cross-platform filesystem API for Windows/LINUX
*/
namespace FileSystemUtil {

/**
* wrap stat for LINUX and GetFileInformationByHandle for windows,
* to provide file meta info query service
*/
class StatResult {
public:
#ifdef __linux__
	StatResult(const struct stat& statbuff);
#endif

#ifdef WIN32
	StatResult(const BY_HANDLE_FILE_INFORMATION& handleFileInformation);
#endif
	/**
	* user id and group id of the owner, always set to zero on windows
	*/
	uint64_t UserID() const;
	uint64_t GroupID() const;

	/**
	* AccessTime and CreationTime are invalid on FAT32 drives
	*/
	uint64_t AccessTime() const;
	uint64_t CreationTime() const;
	uint64_t ModifyTime() const;

	/**
	* UNIX fs hardlink file share the same inode, but inode has no meaning on FAT32/HPFS/NTFS.. fs
	* Windows use "file index" to mark a unique id of a file or a directory in a volume
	*/
	uint64_t UniqueID() const;

	uint64_t Size() const; /* size in bytes */
	uint64_t DeviceID() const; /* id of the disk containning the file */
	uint64_t LinksCount() const; /* no of hard links to the file, on WIN32 always set to 1 on non - NTFS fs */

	bool IsDirectory() const;

#ifdef WIN32
	/* based on dwFileAttributes field */
	bool IsArchive() const;
	bool IsCompressed() const;
	bool IsEncrypted() const;
	bool IsSparseFile() const;
	bool IsHidden() const;
	bool IsOffline() const;
	bool IsReadOnly() const;
	bool IsSystem() const;
	bool IsTemporary() const;
	bool IsNormal() const;
	uint64_t Attribute() const;
#endif

#ifdef __linux__
	/* based on st_mode field */
	bool IsRegular() const;
	bool IsPipe() const;
	bool IsCharDevice() const;
	bool IsBlockDevice() const;
	bool IsSymLink() const;
	bool IsSocket() const;
	uint64_t Mode() const;
#endif

private:
#ifdef __linux__
	struct stat m_stat {};
#endif
#ifdef WIN32
	BY_HANDLE_FILE_INFORMATION m_handleFileInformation{};
#endif
};

std::optional<StatResult> Stat(const std::string& path);


class OpenDirEntry
{
public:
#ifdef WIN32
	OpenDirEntry(const std::string& dirPath,
		const WIN32_FIND_DATA& findFileData, const HANDLE& fileHandle);
	bool IsArchive() const;
	bool IsCompressed() const;
	bool IsEncrypted() const;
	bool IsSparseFile() const;
	bool IsHidden() const;
	bool IsOffline() const;
	bool IsReadOnly() const;
	bool IsSystem() const;
	bool IsTemporary() const;
	bool IsNormal() const;
	uint64_t AccessTime() const;
	uint64_t CreationTime() const;
	uint64_t ModifyTime() const;
	uint64_t Size() const;
	uint64_t Attribute() const;
#endif

#ifdef __linux__
	OpenDirEntry(const std::string& dirPath,
		DIR* dirPtr, struct dirent* direntPtr);
	bool IsUnknown() const;
	bool IsPipe() const;
	bool IsCharDevice() const;
	bool IsBlockDevice() const;
	bool IsSymLink() const;
	bool IsSocket() const;
	bool IsRegular() const;
	uint64_t INode() const;
#endif

	bool IsDirectory() const;
	std::string Name() const;
	std::string FullPath() const;
	bool Next();
	void Close();
	
	/* disable copy/assign construct */
	OpenDirEntry(const OpenDirEntry&) = delete;
	OpenDirEntry operator = (const OpenDirEntry&) = delete;
	~OpenDirEntry();

private:
	std::string m_dirPath;
#ifdef WIN32
	HANDLE m_fileHandle = nullptr;
	WIN32_FIND_DATA m_findFileData;
#endif

#ifdef __linux__
	DIR* m_dir = nullptr;
	struct dirent* m_dirent = nullptr;
#endif
};

std::optional<OpenDirEntry> OpenDir(const std::string& path);
}

#endif
