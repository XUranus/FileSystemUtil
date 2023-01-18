#ifndef __FILE_IO_UTIL_H__
#define __FILE_IO_UTIL_H__

#include <iostream>
#include <iterator>
#include <optional>

#ifdef WIN32
#include <Windows.h>
#endif

#ifdef LINUX
#include <sys/types.h>
#include <dirent.h>
#endif


namespace FileIO {

class StatResult {
public:
#ifdef LINUX
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
	* UNIX fs hardlink file share the same inode, but has no meaning on FAT32/HPFS/NTFS.. fs
	*/
	uint64_t UniqueID() const;

	uint64_t Size() const; // size in bytes
	uint64_t DeviceNmber() const; // no of the disk containning the file
	uint64_t LinksNumber() const; // no of hard links to the file, on WIN32 always set to 1 on non-NTFS fs

	bool IsDirectory() const;

#ifdef WIN32
	bool IsArchive() const;
	bool IsCompressed() const;
	bool IsEncrypted() const;
	bool IsSparseFile() const;
	bool IsHidden() const;
	bool IsOffline() const;
	bool IsReadOnly() const;
	bool IsSystem() const;
	bool IsTemporary() const;
#endif

#ifdef LINUX
	bool IsRegular() const;
	bool IsPipe() const;
	bool IsCharDevice() const;
	bool IsBlockDevice() const;
	bool IsSymLink() const;
	bool IsSocket() const;
#endif

private:
#ifdef LINUX
	struct stat m_stat {};
#endif
#ifdef WIN32
	BY_HANDLE_FILE_INFORMATION m_handleFileInformation{};
#endif
};


class OpenDirEntry
{
public:
	bool Next();
	void Close();

	uint64_t INode();
	bool IsNormal() const;
	bool IsDirectory() const;
	std::string Name() const;
	uint64_t Size() const;
	uint64_t AccessTime() const;
	uint64_t CreationTime() const;
	uint64_t ModifyTime() const;
	bool DoStat();

#ifdef WIN32
	bool IsArchive() const;
	bool IsCompressed() const;
	bool IsEncrypted() const;
	bool IsSparseFile() const;
	bool IsHidden() const;
	bool IsOffline() const;
	bool IsReadOnly() const;
	bool IsSystem() const;
	bool IsTemporary() const;
#endif

#ifdef LINUX
	bool IsUnknown() const;
	bool IsPipe() const;
	bool IsCharDevice() const;
	bool IsBlockDevice() const;
	bool IsSymLink() const;
	bool IsSocket() const;
#endif

	// forbid copy
	OpenDirEntry(const OpenDirEntry&) = delete;
	OpenDirEntry operator = (const OpenDirEntry&) = delete;

private:
	std::string m_dirPath;
	bool m_stated = false;
	bool m_statFailed = false;
#ifdef WIN32
	HANDLE m_fileHandle = nullptr;
	WIN32_FIND_DATA m_findFileData;
#endif

#ifdef LINUX
	struct stat m_stat {};
	DIR* m_dir;
	struct dirent* m_dirent;
#endif
};



std::optional<StatResult> Stat(const std::string& path);
std::optional<OpenDirEntry> OpenDir(const std::string& path);
}

#endif

/*
 reference:
 https://www.cnblogs.com/collectionne/p/6792301.html
 https://learn.microsoft.com/en-us/previous-versions/aa914427(v=msdn.10)
 https://blog.csdn.net/weixin_47679859/article/details/126836727
 https://blog.csdn.net/qq_37858386/article/details/121424783
 https://blog.csdn.net/lj19990824/article/details/120046241
 https://www.bbsmax.com/A/E35poVvgJv/
 https://www.writebug.com/git/Carryme/FileSearch
 https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/stat-functions?view=msvc-170
 https://learn.microsoft.com/en-us/cpp/c-runtime-library/stat-structure-st-mode-field-constants?view=msvc-170
 https://learn.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants
 https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileinformationbyhandle
 https://learn.microsoft.com/en-us/windows/win32/api/fileapi/ns-fileapi-by_handle_file_information
 https://learn.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants
 https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileinformationbyhandle
 */