﻿#ifndef __XURANUS_FILESYSTEM_UTIL_H__
#define __XURANUS_FILESYSTEM_UTIL_H__

#include <string>
#include <iostream>
#include <iterator>
#include <optional>
#include <vector>
#include <cstdint>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef UNICODE
#define UNICODE /* foring using WCHAR on windows */
#endif
/**
 * #define KEEP_WIN32_NATIVE_TIMESTAMP_VALUE
 * uncomment this macro if you need to get native windows microsecond timestamp
 */
#include <Windows.h>
#include <Aclapi.h>
#endif

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#endif

using SparseRangeResult = std::optional<std::vector<std::pair<uint64_t, uint64_t>>>;

#ifdef _WIN32
inline uint64_t CombineDWORD(DWORD low, DWORD high) {
    return (uint64_t)low + ((uint64_t)MAXDWORD + 1) * high;
}

inline uint64_t ConvertWin32Time(DWORD low, DWORD high)
{
    const uint64_t UNIX_TIME_START = 0x019DB1DED53E8000; /* January 1, 1970 (start of Unix epoch) in "ticks" */
    const uint64_t TICKS_PER_SECOND = 10000000; /* a tick is 100ns */
    LARGE_INTEGER li;
    li.LowPart  = low;
    li.HighPart = high;
#ifdef KEEP_WIN32_NATIVE_TIMESTAMP_VALUE
    return li.QuadPart;
#else
    /* Convert ticks since 1/1/1970 into seconds */
    return (li.QuadPart - UNIX_TIME_START) / TICKS_PER_SECOND;
#endif
}
#endif

/**
* wrap some cross-platform filesystem API for Windows/LINUX
*/
namespace FileSystemUtil {

#ifdef _WIN32
/* 
 * transform between UTF-8 and UTF-16
 * any method using _WIN32 API passing WCHAR
 */
std::wstring Utf8ToUtf16(const std::string& str);
std::string Utf16ToUtf8(const std::wstring& wstr);
bool EnablePrivilege();
#endif

/**
* wrap stat for LINUX and GetFileInformationByHandle for windows,
* to provide file meta info query service
*/
class StatResult {
public:
#ifdef __linux__
    StatResult(const std::string& path, const struct stat& statbuff);
#endif

#ifdef _WIN32
    StatResult(const std::wstring& wPath, const BY_HANDLE_FILE_INFORMATION& handleFileInformation);
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
    uint64_t LinksCount() const; /* no of hard links to the file, on _WIN32 always set to 1 on non - NTFS fs */

    bool IsDirectory() const;
    std::string CanonicalPath() const;

#ifdef _WIN32
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
    bool IsReparsePoint() const;
    uint64_t Attribute() const;
    
    /* reparse point related methods */
    DWORD ReparseTag() const;
    bool IsSymbolicLink() const;
    bool IsJunctionPoint() const;
    bool IsMountedDevice() const;
    std::optional<std::wstring> MountedDeviceNameW() const;
    std::optional<std::wstring> JunctionsPointTargetPathW() const;
    std::optional<std::wstring> SymbolicLinkTargetPathW() const;
    bool HasReparseSymbolicLinkTag() const;
    bool HasReparseMountPointTag() const;
    bool HasReparseNfsTag() const;
    bool HasReparseOneDriveTag() const;
    std::optional<std::wstring> FinalPathW() const; 

    std::wstring CanonicalPathW() const;
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
    std::string m_path; /* raw input path */
#endif
#ifdef _WIN32
    BY_HANDLE_FILE_INFORMATION m_handleFileInformation{};
    std::wstring m_wPath; /* raw input path */
#endif
};

std::optional<StatResult> Stat(const std::string& path);
#ifdef _WIN32
std::optional<StatResult> StatW(const std::wstring& wPath);
#endif

class OpenDirEntry
{
public:
#ifdef _WIN32
    OpenDirEntry(
        const std::string&          dirPath,
        const WIN32_FIND_DATAW&     findFileData,
        const HANDLE&               fileHandle);
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
    bool IsReparsePoint() const;
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
#ifdef _WIN32
    std::wstring NameW() const;
    std::wstring FullPathW() const;
#endif
    std::string Name() const;
    std::string FullPath() const;
    bool Next();
    void Close();
    
    /* disable copy/assign construct */
    OpenDirEntry(const OpenDirEntry&) = delete;
    OpenDirEntry operator = (const OpenDirEntry&) = delete;
    ~OpenDirEntry();

private:
#ifdef _WIN32
    std::wstring m_dirPath;
    HANDLE m_fileHandle = nullptr;
    WIN32_FIND_DATAW m_findFileData;
#endif

#ifdef __linux__
    std::string m_dirPath;
    DIR* m_dir = nullptr;
    struct dirent* m_dirent = nullptr;
#endif
};

std::optional<OpenDirEntry> OpenDir(const std::string& path);

/* Sparse File allocate range API */
SparseRangeResult QuerySparseAllocateRanges(const std::string& path);
bool CopySparseFile(
    const std::string& srcPath,
    const std::string& dstPath,
    const std::vector<std::pair<uint64_t, uint64_t>>& ranges);
#ifdef _WIN32
SparseRangeResult QuerySparseWin32AllocateRangesW(const std::wstring& wPath);
bool CopySparseFileWin32W(
    const std::wstring& wSrcPath,
    const std::wstring& wDstPath,
    const std::vector<std::pair<uint64_t, uint64_t>>& ranges);
#endif
#ifdef __linux__
SparseRangeResult QuerySparsePosixAllocateRanges(const std::string& path);
bool CopySparseFilePosix(
    const std::string& srcPath,
    const std::string& dstPath,
    const std::vector<std::pair<uint64_t, uint64_t>>& ranges);
#endif

#ifdef _WIN32
/* _WIN32 Volumes related API */
std::vector<std::wstring> GetWin32DriverListW();
std::vector<std::string> GetWin32DriverList();

class Win32VolumesDetail {
public:
    Win32VolumesDetail(const std::wstring& wVolumeName);
    std::wstring VolumeNameW() const;
    std::string VolumeName() const;
    std::optional<std::wstring> GetVolumeDeviceNameW();
    std::optional<std::string> GetVolumeDeviceName();
    std::optional<std::vector<std::wstring>> GetVolumePathListW();
    std::optional<std::vector<std::string>> GetVolumePathList();
private:
    std::wstring m_wVolumeName;
};

std::optional<std::vector<Win32VolumesDetail>> GetWin32VolumeList();

bool EnablePrivilegeW(const wchar_t* wPrivilegeName);

/* _WIN32 Security Descriptor related API */
std::optional<std::wstring> GetSecurityDescriptorW(const std::wstring& wPath, DWORD& errCode);
std::optional<std::string> GetSecurityDescriptor(const std::string& path, DWORD& errCode);
bool SetSecurityDescriptorW(const std::wstring& wPath, const std::wstring& wSddlStr, DWORD& errCode);
bool SetSecurityDescriptor(const std::string& path, const std::string& sddlStr, DWORD& errCode);

/*
 * Normalize windows path, convert the path to the form like C:\Dir1\Dir2
 * Except root path like C:\, D:\, path won't ends with backslash
 */
std::wstring NormalizeWin32PathW(std::wstring& wPath);

/* reparse point related methods */
bool CreateSymbolicLinkW(
    const std::wstring& wLinkFilePath,
    const std::wstring& wTargetPath,
    bool isDirectory = false,
    bool isRelative = true);
bool CreateJunctionPointW(const std::wstring& wSrcPath, const std::wstring& wDstPath);


/* ADS releated API
 * https://github.com/chenchao266/mfc/blob/b58abf9eb4c6d90ef01b9f1203b174471293dfba/wfc_sample/Sample/dump_ntfs_streams.cpp
 */

const int WIN32_STREAM_ID_BUFFER_LENGTH_MAX = 4096;

class AlternateDataStreamEntry {
public:
    AlternateDataStreamEntry(HANDLE hFile, WIN32_FIND_STREAM_DATA lpFindStreamData);
    std::wstring StreamNameW();
    bool Next();
    DWORD Error() const;
    bool Eof() const;
    ~AlternateDataStreamEntry();

private:
    HANDLE m_hStream = INVALID_HANDLE_VALUE;
    WIN32_FIND_STREAM_DATA m_findStreamData;
    DWORD m_error = 0;
    bool m_eof = false;
};

/* ADS releated API */
std::optional<AlternateDataStreamEntry> OpenAlternateDataStreamW(const std::wstring& wPath);
#endif

/* Common cross-platform API */
bool IsDirectory(const std::string& path);
bool IsEmptyDirectory(const std::string& path);
bool Exists(const std::string& path);
bool Mkdir(const std::string& path);
bool MkdirRecursive(const std::string& path);
std::string ParentDirectoryPath(const std::string& path);
}

#endif
