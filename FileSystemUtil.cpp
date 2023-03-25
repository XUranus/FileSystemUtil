#include "FileSystemUtil.h"
#include <cstdint>

#ifdef WIN32
#ifndef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif
#include <locale>
#include <codecvt>
#include <sddl.h>
#include <winioctl.h>
#endif

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <algorithm>

using namespace std;

namespace {
#ifdef WIN32
constexpr auto WIN32_UID = 0;
constexpr auto WIN32_GID = 0;
constexpr auto VOLUME_BUFFER_MAX_LEN = MAX_PATH;
constexpr auto VOLUME_PATH_MAX_LEN = MAX_PATH + 1;
constexpr auto DEVICE_BUFFER_MAX_LEN = MAX_PATH;
constexpr auto DEFAULT_REPARSE_TAG = 0;
const int SYMLINK_FLAG_RELATIVE = 1;
const std::wstring WPATH_PREFIX = LR"(\\?\)";
#endif
}

#ifdef WIN32
/*
 * These structure is used for Interal Windows API. 
 * There's no associated import library to define them, so developers must define them maunally
 */

/* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_reparse_data_buffer?redirectedfrom=MSDN */
typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG  Flags;
            WCHAR  PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR DataBuffer[1];
        } GenericReparseBuffer;
    } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, * PREPARSE_DATA_BUFFER;
#endif

/* Implement Section Begin */
namespace FileSystemUtil {

#ifdef WIN32
std::wstring Utf8ToUtf16(const std::string& str)
{
    using ConvertTypeX = std::codecvt_utf8_utf16<wchar_t>;
    std::wstring_convert<ConvertTypeX> converterX;
    std::wstring wstr = converterX.from_bytes(str);
    return wstr;
}

std::string Utf16ToUtf8(const std::wstring& wstr)
{
    using ConvertTypeX = std::codecvt_utf8_utf16<wchar_t>;
    std::wstring_convert<ConvertTypeX> converterX;
    return converterX.to_bytes(wstr);
}

inline std::wstring ConvertWin32UnicodePath(const std::wstring& wPath)
{
    if (wPath.length() > WPATH_PREFIX.length() && wPath.find(WPATH_PREFIX) == 0) {
        /* already have prefix */
        return wPath;
    }
    return WPATH_PREFIX + wPath;
}

bool EnablePrivilege()
{
    HANDLE hToken;
    char buf[sizeof(TOKEN_PRIVILEGES) * 2];
    TOKEN_PRIVILEGES& tkp = *((TOKEN_PRIVILEGES*) buf);

    if (!::OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &hToken)) {
        return false;
    }

    /* enable SeBackupPrivilege, SeRestorePrivilege */
    if (!::LookupPrivilegeValue(
            nullptr,
            SE_BACKUP_NAME,
            &tkp.Privileges[0].Luid)) { 
        return false;
    }

    if (!::LookupPrivilegeValue(
            nullptr,
            SE_RESTORE_NAME,
            &tkp.Privileges[1].Luid)) {
        return false;
    }

    tkp.PrivilegeCount = 2;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    tkp.Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    ::AdjustTokenPrivileges(hToken, false, &tkp, sizeof(tkp), nullptr, nullptr);
    return true;   
}
#endif


#ifdef __linux__
StatResult::StatResult(const std::string& path, const struct stat& statbuff)
    : m_path(path)
{
    memcpy(&m_stat, &statbuff, sizeof(struct stat));
}
#endif

#ifdef WIN32
StatResult::StatResult(const std::wstring& wPath, const BY_HANDLE_FILE_INFORMATION& handleFileInformation)
    : m_wPath(wPath)
{
    memcpy_s(&m_handleFileInformation, sizeof(BY_HANDLE_FILE_INFORMATION),
        &handleFileInformation, sizeof(BY_HANDLE_FILE_INFORMATION));
}
#endif

uint64_t StatResult::UserID() const
{
#ifdef __linux__
    return static_cast<uint64_t>(m_stat.st_gid);
#endif
#ifdef WIN32
    return WIN32_UID;
#endif
}

uint64_t StatResult::GroupID() const
{
#ifdef __linux__
    return static_cast<uint64_t>(m_stat.st_uid);
#endif
#ifdef WIN32
    return WIN32_GID;
#endif
}

uint64_t StatResult::AccessTime() const
{
#ifdef __linux__
    return static_cast<uint64_t>(m_stat.st_atime);
#endif
#ifdef WIN32
    return ConvertWin32Time(m_handleFileInformation.ftLastAccessTime.dwLowDateTime,
        m_handleFileInformation.ftLastAccessTime.dwHighDateTime);
#endif
}

uint64_t StatResult::CreationTime() const
{
#ifdef __linux__
    return static_cast<uint64_t>(m_stat.st_ctime);
#endif
#ifdef WIN32
    return ConvertWin32Time(m_handleFileInformation.ftCreationTime.dwLowDateTime,
        m_handleFileInformation.ftCreationTime.dwHighDateTime);
#endif
}

uint64_t StatResult::ModifyTime() const
{
#ifdef __linux__
    return static_cast<uint64_t>(m_stat.st_mtime);
#endif
#ifdef WIN32
    return ConvertWin32Time(m_handleFileInformation.ftLastWriteTime.dwLowDateTime,
        m_handleFileInformation.ftLastWriteTime.dwHighDateTime);
#endif
}

uint64_t StatResult::UniqueID() const
{
#ifdef __linux__
    return static_cast<uint64_t>(m_stat.st_ino);
#endif
#ifdef WIN32
    return CombineDWORD(m_handleFileInformation.nFileIndexLow,
        m_handleFileInformation.nFileIndexHigh);
#endif
}

uint64_t StatResult::Size() const
{
#ifdef __linux__
    return static_cast<uint64_t>(m_stat.st_size);
#endif
#ifdef WIN32
    return CombineDWORD(m_handleFileInformation.nFileSizeLow,
        m_handleFileInformation.nFileSizeHigh);
#endif
}

uint64_t StatResult::DeviceID() const
{
#ifdef __linux__
    return static_cast<uint64_t>(m_stat.st_rdev);
#endif
#ifdef WIN32
    return static_cast<uint64_t>(m_handleFileInformation.dwVolumeSerialNumber);
#endif
}

uint64_t StatResult::LinksCount() const
{
#ifdef __linux__
    return static_cast<uint64_t>(m_stat.st_nlink);
#endif
#ifdef WIN32
    return static_cast<uint64_t>(m_handleFileInformation.nNumberOfLinks);
#endif
}

bool StatResult::IsDirectory() const
{
#ifdef WIN32
    return (m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#endif
#ifdef __linux__
    return (m_stat.st_mode & S_IFDIR) != 0;
#endif
}

std::string StatResult::CanonicalPath() const
{
#ifdef WIN32
    return Utf16ToUtf8(CanonicalPathW());
#endif
#ifdef __linux__
    char* posixPathPtr = ::realpath(m_path.c_str(), nullptr);
    if (posixPathPtr == nullptr) {
        return "";
    }
    std::string posixPath(posixPathPtr);
    ::free(posixPathPtr);
    return posixPath;
#endif
}

#ifdef WIN32
bool StatResult::IsArchive() const { return (m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0; }
bool StatResult::IsCompressed() const { return (m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) != 0; }
bool StatResult::IsEncrypted() const { return (m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) != 0; }
bool StatResult::IsSparseFile() const { return (m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) != 0; }
bool StatResult::IsHidden() const { return (m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0; }
bool StatResult::IsOffline() const { return (m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) != 0; }
bool StatResult::IsReadOnly() const { return (m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0; }
bool StatResult::IsSystem() const { return (m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0; }
bool StatResult::IsTemporary() const { return (m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) != 0; }
bool StatResult::IsNormal() const { return (m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) != 0; }
bool StatResult::IsReparsePoint() const { return (m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0; }
uint64_t StatResult::Attribute() const { return m_handleFileInformation.dwFileAttributes; }


/* 
 * return pointer to REPARSE_DATA_BUFFER which store target info of file with reparse attribute,
 * need to free memory if return non-nullptr value
 */
static REPARSE_DATA_BUFFER* GetReparseDataBufferW(const std::wstring& wPath)
{
    REPARSE_DATA_BUFFER* pReparseBuffer = nullptr;
    DWORD dwSize;
    /* Open the file for read */
    std::wstring unicodePath = ConvertWin32UnicodePath(wPath);
    HANDLE hFile = ::CreateFileW(
        unicodePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
        0);
    if (hFile == INVALID_HANDLE_VALUE) {
        /* failed */
        return nullptr;
    }
    /* Allocated areas info */
    pReparseBuffer = (REPARSE_DATA_BUFFER*)::malloc(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (pReparseBuffer == nullptr) {
        /* malloc failed */
        ::CloseHandle(hFile);
        return nullptr;
    }
    bool ret = ::DeviceIoControl(
        hFile,
        FSCTL_GET_REPARSE_POINT,
        nullptr,
        0,
        pReparseBuffer,
        MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
        &dwSize,
        nullptr);
    if (!ret) {
        /* failed */
        ::free(pReparseBuffer);
        ::CloseHandle(hFile);
        return nullptr;
    }
    ::CloseHandle(hFile);
    return pReparseBuffer;
}

DWORD StatResult::ReparseTag() const
{
    if (!IsReparsePoint()) {
        return DEFAULT_REPARSE_TAG;
    }
    WIN32_FIND_DATAW findFileData{};
    std::wstring wCanonicalPath = CanonicalPathW();
    std::wstring unicodePath = ConvertWin32UnicodePath(wCanonicalPath);
    HANDLE fileHandle = ::FindFirstFileW(unicodePath.c_str(), &findFileData);
    if (fileHandle == INVALID_HANDLE_VALUE || fileHandle == nullptr) {
        return DEFAULT_REPARSE_TAG;
    }
    ::FindClose(fileHandle);
    fileHandle = nullptr;
    return findFileData.dwReserved0;
}


bool StatResult::IsSymbolicLink() const { return HasReparseSymbolicLinkTag();}

bool StatResult::IsJunctionPoint() const
{
    std::optional<std::wstring> wPath = JunctionsPointTargetPathW();
    if (!wPath) {
        return false;
    }
    return true;
}

bool StatResult::IsMountedDevice() const {
    std::optional<std::wstring> wDeviceName = MountedDeviceNameW();
    if (!wDeviceName) {
        return false;
    }
    return true;
}

/*
 * If this sparse point has IO_REPARSE_TAG_MOUNT_POINT tag, it maybe a device mount point or a junction link
 * if it's device   point, GetVolumeNameForVolumeMountPointW can accquire device name
 * if it's junction link, GetVolumeNameForVolumeMountPointW will return false
 */
std::optional<std::wstring> StatResult::MountedDeviceNameW() const
{
    if (!HasReparseMountPointTag()) {
        return std::nullopt;
    }
    WCHAR deviceNameBuff[DEVICE_BUFFER_MAX_LEN] = L"";
    std::wstring wCanonicalPath = CanonicalPathW();
    /* GetVolumeNameForVolumeMountPointW require input path to end with backslash */
    if (wCanonicalPath.back() != L'\\') {
        wCanonicalPath.push_back(L'\\');
    }
    if (::GetVolumeNameForVolumeMountPointW(wCanonicalPath.c_str(), deviceNameBuff, DEVICE_BUFFER_MAX_LEN)) {
        return std::make_optional<std::wstring>(deviceNameBuff);
    }
    return std::nullopt;
}

std::optional<std::wstring> StatResult::JunctionsPointTargetPathW() const
{
    REPARSE_DATA_BUFFER* pReparseBuffer = GetReparseDataBufferW(m_wPath);
    if (pReparseBuffer == nullptr) {
        /* failed */
        return std::nullopt;
    }
    if (pReparseBuffer->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT) {
        /* not a symbolic link */
        ::free(pReparseBuffer);
        return std::nullopt;
    }

    std::wstring wTarget;
    std::wstring wPrintName;

    USHORT targetNameIndex = pReparseBuffer->MountPointReparseBuffer.SubstituteNameOffset / sizeof(WCHAR);
    USHORT targetNameLength = pReparseBuffer->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
    USHORT displayNameIndex = pReparseBuffer->MountPointReparseBuffer.PrintNameOffset / sizeof(WCHAR);
    USHORT displayNameLength = pReparseBuffer->MountPointReparseBuffer.PrintNameLength / sizeof(WCHAR);
    WCHAR* targetName = &pReparseBuffer->MountPointReparseBuffer.PathBuffer[targetNameIndex];
    WCHAR* displayName = &pReparseBuffer->MountPointReparseBuffer.PathBuffer[displayNameIndex];
    wTarget.assign(targetName, targetName + targetNameLength);
    wPrintName.assign(displayName, displayName + displayNameLength);
    ::free(pReparseBuffer);
    
    /*
    * Due to lack to offical api to determine if a reparse point with IO_REPARSE_TAG_MOUNT_POINT tag is a junction point,
    * (both directory junction and device mount point has IO_REPARSE_TAG_MOUNT_POINT tag)
    * if this directory is a mounted device, wPrintName and wTarget name will also be meaningful (returned as to device ame),
    * To make sure this method return a target of directory junction, this method will check if it can obtain device name.
    * If it's a device mount point, return std::nullopt
    */
    std::optional<std::wstring> wDeviceName = MountedDeviceNameW();
    if (wDeviceName) {
        return std::nullopt;
    }
    return std::make_optional<std::wstring>(wPrintName);
}

std::optional<std::wstring> StatResult::SymbolicLinkTargetPathW() const
{
    REPARSE_DATA_BUFFER* pReparseBuffer = GetReparseDataBufferW(m_wPath);
    if (pReparseBuffer == nullptr) {
        /* failed */
        return std::nullopt;
    }
    if (pReparseBuffer->ReparseTag != IO_REPARSE_TAG_SYMLINK) {
        /* not a symbolic link */
        ::free(pReparseBuffer);
        return std::nullopt;
    }

    std::wstring wTarget;
    std::wstring wPrintName;

    USHORT targetNameIndex = pReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameOffset >> 1;
    USHORT targetNameLength = pReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameLength >> 1;
    USHORT displayNameIndex = pReparseBuffer->SymbolicLinkReparseBuffer.PrintNameOffset >> 1;
    USHORT displayNameLength = pReparseBuffer->SymbolicLinkReparseBuffer.PrintNameLength >> 1;
    WCHAR* targetName = &pReparseBuffer->SymbolicLinkReparseBuffer.PathBuffer[targetNameIndex];
    WCHAR* displayName = &pReparseBuffer->SymbolicLinkReparseBuffer.PathBuffer[displayNameIndex];
    wTarget.assign(targetName, targetName + targetNameLength);
    wPrintName.assign(displayName, displayName + displayNameLength);

    std::wcout << L"target: " << wTarget << std::endl;
    std::wcout << L"printName: " << wPrintName << std::endl;
    std::wcout << L"flags: " << pReparseBuffer->SymbolicLinkReparseBuffer.Flags << std::endl;

    ::free(pReparseBuffer);
    return std::make_optional<std::wstring>(wPrintName);
}

// https://learn.microsoft.com/en-us/windows/win32/fileio/reparse-point-tags
bool StatResult::HasReparseSymbolicLinkTag() const { return ReparseTag() == IO_REPARSE_TAG_SYMLINK; }
bool StatResult::HasReparseMountPointTag() const { return ReparseTag() == IO_REPARSE_TAG_MOUNT_POINT; }
bool StatResult::HasReparseNfsTag() const { return ReparseTag() == IO_REPARSE_TAG_NFS; }
bool StatResult::HasReparseOneDriveTag() const { return ReparseTag() == IO_REPARSE_TAG_ONEDRIVE; }

/* return final path of reparse target */
std::optional<std::wstring> StatResult::FinalPathW() const
{
    BY_HANDLE_FILE_INFORMATION handleFileInformation{};
    HANDLE hFile = ::CreateFileW(
        m_wPath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        0);
    if (hFile == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }
    WCHAR wPathBuff[MAX_PATH] = L"";
    DWORD length = ::GetFinalPathNameByHandleW(hFile, wPathBuff, MAX_PATH, FILE_NAME_NORMALIZED);
    if (length == 0) {
        /* failed */
        ::CloseHandle(hFile);
        return std::nullopt;
    }
    if (length >= MAX_PATH) {
        DWORD extendLength = length + 1;
        WCHAR* wPathExtendBuff = new WCHAR[extendLength];
        if (::GetFinalPathNameByHandleW(hFile, wPathExtendBuff, extendLength, FILE_NAME_NORMALIZED) == 0) {
            /* failed */
            delete[] wPathExtendBuff;
            ::CloseHandle(hFile);
            return std::nullopt;
        }
        /* succeed */
        ::CloseHandle(hFile);
        std::wstring wTargetPath(wPathExtendBuff);
        delete[] wPathExtendBuff;
        return NormalizeWin32PathW(wTargetPath);
    }
    /* succeed */
    ::CloseHandle(hFile);
    std::wstring wTargetPath(wPathBuff);
    return NormalizeWin32PathW(wTargetPath);
}

std::wstring StatResult::CanonicalPathW() const
{
    WCHAR wPathBuff[MAX_PATH] = L"";
    DWORD length = ::GetFullPathNameW(m_wPath.c_str(), MAX_PATH, wPathBuff, nullptr);
    if (length == 0) {
        /* failed */
        return L"";
    }
    if (length >= MAX_PATH) {
        DWORD extendLength = length + 1;
        WCHAR* wPathExtendBuff = new WCHAR[extendLength];
        if (::GetFullPathNameW(m_wPath.c_str(), extendLength, wPathExtendBuff, nullptr) == 0) {
            /* failed */
            delete[] wPathExtendBuff;
            return L"";
        }
        /* succeed */
        std::wstring wCanonicalPath(wPathExtendBuff);
        delete[] wPathExtendBuff;
        return NormalizeWin32PathW(wCanonicalPath);
    }
    /* succeed */
    std::wstring wCanonicalPath(wPathBuff);
    return NormalizeWin32PathW(wCanonicalPath);
}
#endif

#ifdef __linux__
bool StatResult::IsRegular() const { return (m_stat.st_mode & S_IFREG) != 0; }
bool StatResult::IsPipe() const { return (m_stat.st_mode & S_IFIFO) != 0; }
bool StatResult::IsCharDevice() const { return (m_stat.st_mode & S_IFCHR) != 0; }
bool StatResult::IsBlockDevice() const { return (m_stat.st_mode & S_IFBLK) != 0; }
bool StatResult::IsSymLink() const { return (m_stat.st_mode & S_IFLNK) != 0; }
bool StatResult::IsSocket() const { return (m_stat.st_mode & S_IFSOCK) != 0; }
uint64_t StatResult::Mode() const { return m_stat.st_mode; }
#endif

std::optional<StatResult> Stat(const std::string& path)
{
#ifdef __linux__
    struct stat statbuff {};
    if (stat(path.c_str(), &statbuff) < 0) {
        return std::nullopt;
    }
    return std::make_optional<StatResult>(path, statbuff);
#endif
#ifdef WIN32
    return StatW(Utf8ToUtf16(path));
#endif
}

#ifdef WIN32
std::optional<StatResult> StatW(const std::wstring& wPath)
{
    std::wstring unicodePath = ConvertWin32UnicodePath(wPath);
    BY_HANDLE_FILE_INFORMATION handleFileInformation{};
    HANDLE hFile = ::CreateFileW(
        unicodePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
        0);
    if (hFile == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }
    if (::GetFileInformationByHandle(hFile, &handleFileInformation) == 0) {
        ::CloseHandle(hFile);
        return std::nullopt;
    }
    ::CloseHandle(hFile);
    DWORD attribute = ::GetFileAttributesW(unicodePath.c_str()); /* to detect REPARSE_POINT flag */
    if (attribute != INVALID_FILE_ATTRIBUTES) {
        handleFileInformation.dwFileAttributes = attribute;
    }
    return std::make_optional<StatResult>(wPath, handleFileInformation);
}
#endif

#ifdef WIN32
OpenDirEntry::OpenDirEntry(
    const std::string&      dirPath,
    const WIN32_FIND_DATAW& findFileData,
    const HANDLE&           fileHandle)
    : m_dirPath(Utf8ToUtf16(dirPath)), m_findFileData(findFileData), m_fileHandle(fileHandle) {}

bool OpenDirEntry::IsArchive() const { return (m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0; }
bool OpenDirEntry::IsCompressed() const { return (m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) != 0; }
bool OpenDirEntry::IsEncrypted() const { return (m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) != 0; }
bool OpenDirEntry::IsSparseFile() const { return (m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) != 0; }
bool OpenDirEntry::IsHidden() const { return (m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0; }
bool OpenDirEntry::IsOffline() const { return (m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) != 0; }
bool OpenDirEntry::IsReadOnly() const { return (m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0; }
bool OpenDirEntry::IsSystem() const { return (m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0; }
bool OpenDirEntry::IsTemporary() const { return (m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) != 0; }
bool OpenDirEntry::IsNormal() const { return (m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) != 0; }
bool OpenDirEntry::IsReparsePoint() const { return (m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0; }
uint64_t OpenDirEntry::Attribute() const { return m_findFileData.dwFileAttributes; }

uint64_t OpenDirEntry::AccessTime() const
{
    return ConvertWin32Time(m_findFileData.ftLastAccessTime.dwLowDateTime,
        m_findFileData.ftLastAccessTime.dwHighDateTime);
}

uint64_t OpenDirEntry::CreationTime() const
{
    return ConvertWin32Time(m_findFileData.ftCreationTime.dwLowDateTime,
        m_findFileData.ftCreationTime.dwHighDateTime);
}

uint64_t OpenDirEntry::ModifyTime() const
{
    return ConvertWin32Time(m_findFileData.ftLastWriteTime.dwLowDateTime,
        m_findFileData.ftLastWriteTime.dwHighDateTime);
}

uint64_t OpenDirEntry::Size() const
{
    return CombineDWORD(m_findFileData.nFileSizeLow, m_findFileData.nFileSizeHigh);
}
#endif

#ifdef __linux__
OpenDirEntry::OpenDirEntry(const std::string& dirPath, DIR* dirPtr, struct dirent* direntPtr)
    :m_dirPath(dirPath), m_dir(dirPtr), m_dirent(direntPtr) {}

bool OpenDirEntry::IsUnknown() const { return (m_dirent->d_type & DT_UNKNOWN != 0); }
bool OpenDirEntry::IsPipe() const { return (m_dirent->d_type & DT_FIFO != 0); }
bool OpenDirEntry::IsCharDevice() const { return (m_dirent->d_type & DT_CHR != 0); }
bool OpenDirEntry::IsBlockDevice() const { return (m_dirent->d_type & DT_BLK != 0); }
bool OpenDirEntry::IsSymLink() const { return (m_dirent->d_type & DT_LNK != 0); }
bool OpenDirEntry::IsSocket() const { return (m_dirent->d_type & DT_SOCK != 0); }
bool OpenDirEntry::IsRegular() const { return (m_dirent->d_type & DT_REG != 0); }
uint64_t OpenDirEntry::INode() const { return static_cast<uint64_t>(m_dirent->d_ino); }
#endif


bool OpenDirEntry::IsDirectory() const
{
#ifdef WIN32
    return (m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#endif
#ifdef __linux__
    return (m_dirent->d_type & DT_DIR) != 0;
#endif
}

std::string OpenDirEntry::Name() const
{
#ifdef WIN32
    return Utf16ToUtf8(std::wstring(m_findFileData.cFileName));
#endif
#ifdef __linux__
    return std::string(m_dirent->d_name);
#endif
}

std::string OpenDirEntry::FullPath() const
{
#ifdef __linux__
    const std::string separator = "/";
    if (!m_dirPath.empty() && m_dirPath.back() == separator[0]) {
        return m_dirPath + Name();
    }
    else {
        return m_dirPath + separator + Name();
    }
#endif
#ifdef WIN32
    std::wstring wfullpath = FullPathW();
    return Utf16ToUtf8(wfullpath);
#endif
}

#ifdef WIN32

std::wstring OpenDirEntry::NameW() const
{
    return std::wstring(m_findFileData.cFileName);
}

std::wstring OpenDirEntry::FullPathW() const
{
    const std::wstring separator = L"\\";
    std::wstring wfullpath;
    if (!m_dirPath.empty() && m_dirPath.back() == separator[0]) {
        wfullpath = m_dirPath + NameW();
    }
    else {
        wfullpath = m_dirPath + separator + NameW();
    }
    return wfullpath;
}

#endif

bool OpenDirEntry::Next()
{
#ifdef WIN32
    if (m_fileHandle == nullptr || m_fileHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    if (!::FindNextFileW(m_fileHandle, &m_findFileData)) {
        m_fileHandle = nullptr;
        return false;
    }
    return true;
#endif
#ifdef __linux__
    if (m_dir == nullptr) {
        return false;
    }
    m_dirent = readdir(m_dir);
    if (m_dirent == nullptr) {
        return false;
    }
    return true;
#endif
}

void OpenDirEntry::Close()
{
#ifdef WIN32
    if (m_fileHandle != nullptr && m_fileHandle != INVALID_HANDLE_VALUE) {
        ::FindClose(m_fileHandle);
        m_fileHandle = nullptr;
    }
#endif
#ifdef __linux__
    if (m_dir != nullptr) {
        ::closedir(m_dir);
        m_dir = nullptr;
        m_dirent = nullptr;
    }
#endif
}

std::optional<OpenDirEntry> OpenDir(const std::string& path)
{
#ifdef WIN32
    std::wstring wpathPattern = ConvertWin32UnicodePath(Utf8ToUtf16(path));
    if (!wpathPattern.empty() && wpathPattern.back() != L'\\') {
        wpathPattern.push_back(L'\\');
    }
    wpathPattern += L"*.*";
    WIN32_FIND_DATAW findFileData{};
    HANDLE fileHandle = ::FindFirstFileW(wpathPattern.c_str(), &findFileData);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }
    return std::make_optional<OpenDirEntry>(path, findFileData, fileHandle);
#endif
#ifdef __linux__
    DIR* dirPtr = ::opendir(path.c_str());
    if (dirPtr == nullptr) {
        return std::nullopt;
    }
    struct dirent* direntPtr = readdir(dirPtr);
    if (direntPtr == nullptr) {
        return std::nullopt;
    }
    return std::make_optional<OpenDirEntry>(path, dirPtr, direntPtr);
#endif
}

OpenDirEntry::~OpenDirEntry() {
    Close();
}


SparseRangeResult QuerySparseAllocateRanges(const std::string& path)
{
    std::optional<StatResult> statResult = Stat(path);
    /* check if is a file */
    if (!statResult || statResult->IsDirectory()) {
        return std::nullopt;
    }
#ifdef WIN32
    if (!statResult->IsSparseFile()) {
        return std::nullopt;
    }
    return QuerySparseWin32AllocateRangesW(Utf8ToUtf16(path));
#endif
#ifdef __linux__
    return QuerySparsePosixAllocateRanges(path);
#endif
}

bool CopySparseFile(const std::string& srcPath, const std::string& dstPath,
    const std::vector<std::pair<uint64_t, uint64_t>>& ranges)
{
    std::optional<StatResult> srcResult = Stat(srcPath);
    std::optional<StatResult> dstResult = Stat(dstPath);
    /* check if is a file */
    if (!srcResult || dstResult || srcResult->IsDirectory()) {
        return false;
    }
#ifdef WIN32
    if (!srcResult->IsSparseFile()) {
        return false;
    }
    return CopySparseFileWin32W(Utf8ToUtf16(srcPath), Utf8ToUtf16(dstPath), ranges);
#endif
#ifdef __linux__
    return CopySparseFilePosix(srcPath, dstPath, ranges);
#endif
}

#ifdef WIN32
/*
 * Invoke Stat() and check if it's sparse file
 * Represent the range using [<offset, length>] in bytes
 * Exmple from:
 * https://github.com/microsoft/cpprestsdk/blob/master/Release/tests/functional/streams/CppSparseFile.cpp
 */
SparseRangeResult QuerySparseWin32AllocateRangesW(const std::wstring& wPath)
{
    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    /* Open the file for read */
    std::wstring unicodePath = ConvertWin32UnicodePath(wPath);
    HANDLE hFile = ::CreateFileW(
                        unicodePath.c_str(),
                        GENERIC_READ,
                        0,
                        nullptr,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }
    LARGE_INTEGER liFileSize;
    GetFileSizeEx(hFile, &liFileSize);
    /* Range to be examined (the whole file) */
    FILE_ALLOCATED_RANGE_BUFFER queryRange;
    queryRange.FileOffset.QuadPart = 0;
    queryRange.Length = liFileSize;
    /* Allocated areas info */
    FILE_ALLOCATED_RANGE_BUFFER allocRanges[1024];
    DWORD nbytes;
    bool fFinished;
    do
    {
        fFinished = DeviceIoControl(
                        hFile,
                        FSCTL_QUERY_ALLOCATED_RANGES,
                        &queryRange,
                        sizeof(queryRange),
                        allocRanges,
                        sizeof(allocRanges),
                        &nbytes,
                        nullptr);
        if (!fFinished) {
            DWORD dwError = GetLastError();
            /* ERROR_MORE_DATA is the only error that is normal */
            if (dwError != ERROR_MORE_DATA) {
                ::CloseHandle(hFile);
                return std::nullopt;
            }
        }
        /* Calculate the number of records returned */
        DWORD dwAllocRangeCount = nbytes / sizeof(FILE_ALLOCATED_RANGE_BUFFER);
        /* Print each allocated range */
        for (int i = 0; i < dwAllocRangeCount; i++) {
            ranges.emplace_back(allocRanges[i].FileOffset.QuadPart, allocRanges[i].Length.QuadPart);
        }
        // Set starting address and size for the next query
        if (!fFinished && dwAllocRangeCount > 0)
        {
            queryRange.FileOffset.QuadPart = 
                allocRanges[dwAllocRangeCount - 1].FileOffset.QuadPart +
                allocRanges[dwAllocRangeCount - 1].Length.QuadPart;
            queryRange.Length.QuadPart = liFileSize.QuadPart - queryRange.FileOffset.QuadPart;
        }
    } while (!fFinished);
    ::CloseHandle(hFile);
    return std::make_optional(ranges);
}

bool CopySparseFileWin32W(const std::wstring& wSrcPath, const std::wstring& wDstPath,
    const std::vector<std::pair<uint64_t, uint64_t>>& ranges)
{
    const int DEFAULT_BUFF_SIZE = 1024;
    char buff[DEFAULT_BUFF_SIZE] = "\0";
    /* open file for read */
    std::wstring wSrcUnicodePath = ConvertWin32UnicodePath(wSrcPath);
    HANDLE hInFile = ::CreateFileW(
        wSrcUnicodePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);
    if (hInFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    /* open file for write */
    std::wstring wDstUnicodePath = ConvertWin32UnicodePath(wDstPath);
    HANDLE hOutFile = ::CreateFileW(
        wDstUnicodePath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        CREATE_NEW,
        0,
        nullptr);
    if (hOutFile == INVALID_HANDLE_VALUE) {
        ::CloseHandle(hInFile);
        return false;
    }
    /* set taget file sparse */
    DWORD dwTemp; /* store the setted attribute */
    ::DeviceIoControl(hOutFile,
        FSCTL_SET_SPARSE,
        nullptr,
        0,
        nullptr,
        0,
        &dwTemp,
        nullptr);
    /* truncate target file at first */
    LARGE_INTEGER sizeEx;
    if (!::GetFileSizeEx(hInFile, &sizeEx)) {
        ::CloseHandle(hInFile);
        return false;
    }
    ::SetFilePointerEx(hOutFile, sizeEx, nullptr, FILE_BEGIN);
    ::SetEndOfFile(hOutFile);
    /* write allocated range */
    for (const std::pair<uint64_t, uint64_t>& range : ranges) {
        uint64_t offset = range.first;
        uint64_t len = range.second;
        sizeEx.QuadPart = offset;
        do {
            int nbytes = 0; /* n bytes to copy in this batch */
            ::SetFilePointerEx(hInFile, sizeEx, nullptr, FILE_BEGIN); /* set fd to the beginning of the range */
            ::SetFilePointerEx(hOutFile, sizeEx, nullptr, FILE_BEGIN);
            nbytes = len < sizeof(buff) ? len: sizeof(buff); /* if the range can be copied in this batch */
            if (!::ReadFile(hInFile, buff, nbytes, nullptr, nullptr)) {
                /* read failed */
                ::CloseHandle(hInFile);
                ::CloseHandle(hOutFile);
                return false;
            }
            DWORD nWritten = 0;
            if (!::WriteFile(hOutFile, buff, nbytes, &nWritten, nullptr)) {
                /* write failed */
                ::CloseHandle(hInFile);
                ::CloseHandle(hOutFile);
                return false;
            }
            offset = offset + nbytes; /* reset offset and length */
            len = len - nbytes;
        } while (len != 0);
    }
    /* copy success */
    ::CloseHandle(hInFile);
    ::CloseHandle(hOutFile);
    return true;
}
#endif

#ifdef __linux__
SparseRangeResult QuerySparsePosixAllocateRanges(const std::string& path)
{
#ifdef SEEK_HOLE
    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    int fd = ::open(path.c_str() , O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        return std::nullopt;
    }
    off_t end = ::lseek(fd, 0, SEEK_END);
    off_t hole = ::lseek(fd, 0, SEEK_HOLE);
    off_t cur = 0, offset = 0 , len = 0;
    while (cur < end) {
        cur = ::lseek(fd, cur, SEEK_DATA);
        if (cur == -1 && errno == ENXIO) {
            cur = end;
            continue;
        }
        if (cur < 0) {
            return std::nullopt; /* query failed */
        }
        offset = cur;
        cur = ::lseek(fd, cur, SEEK_HOLE);
        if (cur < 0) {
            return std::nullopt; /* query failed */
        }
        len = cur - offset;
        ranges.emplace_back(offset, len);
    }
    return std::make_optional(ranges);
#else
    /* kernel not support sparse file */
    return std::nullopt;
#endif
}

bool CopySparseFilePosix(const std::string& srcPath, const std::string& dstPath,
    const std::vector<std::pair<uint64_t, uint64_t>>& ranges)
{
    const int DEFAULT_BUFF_SIZE = 1024;
    char buff[DEFAULT_BUFF_SIZE] = "\0";
    int inFd = ::open(srcPath.c_str(), O_RDONLY);
    if (inFd < 0) {
        return false;
    }
    int outFd = ::open(dstPath.c_str() ,O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (outFd < 0) {
        ::close(inFd);
        return false;
    }
    /* truncate target file at first */
    off_t srcSize = ::lseek(inFd, 0, SEEK_END);
    if (::ftruncate(outFd, srcSize) < 0) {
        /* truncate failed */
        ::close(inFd);
        ::close(outFd);
        return false;
    }
    /* write allocated range */
    for (const std::pair<uint64_t, uint64_t>& range: ranges) {
        uint64_t offset = range.first;
        uint64_t len = range.second;
        do {
            int nbytes = 0; /* n bytes to copy in this batch */
            ::lseek(inFd, offset, SEEK_SET); /* set fd to the beginning of the range */
            ::lseek(outFd, offset, SEEK_SET);
            nbytes = min(len, sizeof(buff)); /* if the range can be copied in this batch */
            int n = ::read(inFd, buff, nbytes);
            if (n != nbytes) {
                /* read failed */
                ::close(inFd);
                ::close(outFd);
                return false;
            }
            n = ::write(outFd, buff, nbytes);
            if (n != nbytes) {
                /* write failed */
                ::close(inFd);
                ::close(outFd);
                return false;
            }
            offset = offset + nbytes; /* reset offset and length */
            len = len - nbytes;
        } while (len != 0);
    }
    /* copy success */
    ::close(inFd);
    ::close(outFd);
    return true;
}
#endif

#ifdef WIN32
/* Win32 Volumes related API */
std::vector<std::wstring> GetWin32DriverListW()
{
    std::vector<std::wstring> wdrivers;
    DWORD dwLen = ::GetLogicalDriveStrings(0, nullptr); /* the length of volumes str */
    if (dwLen <= 0) {
        return wdrivers;
    }
    wchar_t* pszDriver = new wchar_t[dwLen];
    ::GetLogicalDriveStringsW(dwLen, pszDriver);
    wchar_t* pDriver = pszDriver;
    while (*pDriver != '\0') {
        std::wstring wDriver = std::wstring(pDriver);
        wdrivers.push_back(wDriver);
        pDriver += wDriver.length() + 1;
    }
    delete[] pszDriver;
    pszDriver = nullptr;
    pDriver = nullptr;
    return wdrivers;
}

std::vector<std::string> GetWin32DriverList()
{
    std::vector<std::string> drivers;
    std::vector<std::wstring> wdrivers = GetWin32DriverListW();
    for (const std::wstring& wDriver : wdrivers) {
        drivers.push_back(Utf16ToUtf8(wDriver));
    }
    return drivers;
}

/* member methods implementation for Win32VolumeDetail */
Win32VolumesDetail::Win32VolumesDetail(const std::wstring& wVolumeName) : m_wVolumeName(wVolumeName) {}

std::wstring Win32VolumesDetail::VolumeNameW() const { return m_wVolumeName; }

std::string Win32VolumesDetail::VolumeName() const { return Utf16ToUtf8(m_wVolumeName); }

std::optional<std::wstring> Win32VolumesDetail::GetVolumeDeviceNameW()
{
    if (m_wVolumeName.size() < 4 ||
        m_wVolumeName[0] != L'\\' ||
        m_wVolumeName[1] != L'\\' ||
        m_wVolumeName[2] != L'?' ||
        m_wVolumeName[3] != L'\\' ||
        m_wVolumeName.back() != L'\\') { /* illegal volume name */
        return std::nullopt;
    }
    std::wstring wVolumeParam = m_wVolumeName;
    wVolumeParam.pop_back(); /* QueryDosDeviceW does not allow a trailing backslash */
    wVolumeParam = wVolumeParam.substr(4);
    WCHAR deviceNameBuf[DEVICE_BUFFER_MAX_LEN] = L"";
    DWORD charCount = ::QueryDosDeviceW(wVolumeParam.c_str(), deviceNameBuf, ARRAYSIZE(deviceNameBuf));
    if (charCount == 0) {
        return std::nullopt;
    }
    return std::make_optional<std::wstring>(deviceNameBuf);
}

std::optional<std::string> Win32VolumesDetail::GetVolumeDeviceName()
{
    std::optional<std::wstring> wDeviceName = GetVolumeDeviceNameW();
    if (!wDeviceName) {
        return std::nullopt;
    }
    return std::make_optional<std::string>(Utf16ToUtf8(wDeviceName.value()));
}

std::optional<std::vector<std::wstring>> Win32VolumesDetail::GetVolumePathListW()
{
    /* https://learn.microsoft.com/en-us/windows/win32/fileio/displaying-volume-paths */
    if (m_wVolumeName.size() < 4 ||
        m_wVolumeName[0] != L'\\' ||
        m_wVolumeName[1] != L'\\' ||
        m_wVolumeName[2] != L'?' ||
        m_wVolumeName[3] != L'\\' ||
        m_wVolumeName.back() != L'\\') { /* illegal volume name */
        return std::nullopt;
    }
    std::vector<std::wstring> wPathList;
    PWCHAR devicePathNames = nullptr;
    DWORD charCount = MAX_PATH + 1;
    bool success = false;
    while (true) {
        devicePathNames = (PWCHAR) new BYTE[charCount * sizeof(WCHAR)];
        if (!devicePathNames) { /* failed to malloc on heap */
            return std::nullopt;
        }
        success = ::GetVolumePathNamesForVolumeNameW(
            m_wVolumeName.c_str(),
            devicePathNames,
            charCount,
            &charCount
        );
        if (success || ::GetLastError() != ERROR_MORE_DATA) {
            break;
        }
        delete[] devicePathNames;
        devicePathNames = nullptr;
    }
    if (success) {
        for (PWCHAR nameIdx = devicePathNames;
            nameIdx[0] != L'\0';
            nameIdx += ::wcslen(nameIdx) + 1) {
            wPathList.push_back(std::wstring(nameIdx));
        }
    }
    if (devicePathNames != nullptr) {
        delete[] devicePathNames;
        devicePathNames = nullptr;
    }
    return std::make_optional<std::vector<std::wstring>>(wPathList);
}

std::optional<std::vector<std::string>> Win32VolumesDetail::GetVolumePathList()
{
    std::optional<std::vector<std::wstring>> wPathList = GetVolumePathListW();
    if (!wPathList) {
        return std::nullopt;
    }
    std::vector<std::string> pathList;
    for (const std::wstring& wPath : wPathList.value()) {
        pathList.push_back(Utf16ToUtf8(wPath));
    }
    return std::make_optional<std::vector<std::string>>(pathList);
}

std::optional<std::vector<Win32VolumesDetail>> GetWin32VolumeList()
{
    std::vector<std::wstring> wVolumes;
    std::vector<Win32VolumesDetail> volumeDetails;
    WCHAR wVolumeNameBuffer[VOLUME_BUFFER_MAX_LEN] = L"";
    HANDLE handle = ::FindFirstVolumeW(wVolumeNameBuffer, VOLUME_BUFFER_MAX_LEN);
    if (handle == INVALID_HANDLE_VALUE) {
        ::FindVolumeClose(handle);
        return std::nullopt;
    }
    wVolumes.push_back(std::wstring(wVolumeNameBuffer));
    while (::FindNextVolumeW(handle, wVolumeNameBuffer, VOLUME_BUFFER_MAX_LEN)) {
        wVolumes.push_back(std::wstring(wVolumeNameBuffer));
    }
    ::FindVolumeClose(handle);
    handle = INVALID_HANDLE_VALUE;
    for (const std::wstring& wVolumeName : wVolumes) {
        Win32VolumesDetail volumeDetail(wVolumeName);
        volumeDetails.push_back(volumeDetail);
    }
    return volumeDetails;
}

/* Win32 Security Descriptor related API */

/**
* https://learn.microsoft.com/en-us/windows/win32/api/aclapi/nf-aclapi-getnamedsecurityinfoa?redirectedfrom=MSDN
* https://learn.microsoft.com/en-us/previous-versions/windows/hardware/device-stage/drivers/ff556610(v=vs.85)
* https://learn.microsoft.com/en-us/windows/win32/secauthz/security-information
* https://learn.microsoft.com/en-us/windows/win32/api/accctrl/ne-accctrl-se_object_type
* https://learn.microsoft.com/en-us/windows/win32/api/aclapi/nf-aclapi-getnamedsecurityinfoa
* https://blog.csdn.net/eggfly178/article/details/41773601
* https://learn.microsoft.com/en-us/windows/win32/secauthz/security-descriptor-string-format
*/
std::optional<std::wstring> GetSecurityDescriptorW(const std::wstring& wPath)
{
    PACL dAcl = nullptr;
    PSECURITY_DESCRIPTOR psd = nullptr;
    DWORD result = 0;
    LPWSTR wSddlStr = nullptr;
    std::wstring wPathUnicode = ConvertWin32UnicodePath(wPath);
    try
    {
        result = ::GetNamedSecurityInfoW(
            wPathUnicode.c_str(),
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            NULL,
            NULL,
            &dAcl,
            NULL,
            &psd);
    } catch (const std::exception& e) {
        return std::nullopt;
    }
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    }
    bool ret = ::ConvertSecurityDescriptorToStringSecurityDescriptorW(
        psd,
        SDDL_REVISION_1,
        DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
        &wSddlStr,
        nullptr);
    ::LocalFree(psd);
    psd = nullptr;
    if (!ret) {
        return std::nullopt;
    }
    std::wstring res(wSddlStr);
    ::LocalFree(wSddlStr);
    return std::make_optional<std::wstring>(res);
}

std::optional<std::string> GetSecurityDescriptor(const std::string& path)
{
    std::optional<std::wstring> result = GetSecurityDescriptorW(Utf8ToUtf16(path));
    if (!result) {
        return std::nullopt;
    }
    return std::make_optional<std::string>(Utf16ToUtf8(result.value()));
}

std::optional<std::wstring> GetDACLW(const std::wstring& wPath)
{
    PACL dAcl = nullptr;
    PSECURITY_DESCRIPTOR psd = nullptr;
    DWORD result = 0;
    LPWSTR wSddlStr = nullptr;
    std::wstring wPathUnicode = ConvertWin32UnicodePath(wPath);
    try
    {
        result = ::GetNamedSecurityInfoW(
            wPathUnicode.c_str(),
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            NULL,
            NULL,
            &dAcl,
            NULL,
            &psd);
    } catch (const std::exception& e) {
        return std::nullopt;
    }
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    }
    bool ret = ::ConvertSecurityDescriptorToStringSecurityDescriptorW(
        psd,
        SDDL_REVISION_1,
        DACL_SECURITY_INFORMATION,
        &wSddlStr,
        nullptr);
    ::LocalFree(psd);
    psd = nullptr;
    if (!ret) {
        return std::nullopt;
    }
    std::wstring res(wSddlStr);
    ::LocalFree(wSddlStr);
    return std::make_optional<std::wstring>(res);
}

std::optional<std::string> GetDACL(const std::string& path)
{
    std::optional<std::wstring> result = GetDACLW(Utf8ToUtf16(path));
    if (!result) {
        return std::nullopt;
    }
    return std::make_optional<std::string>(Utf16ToUtf8(result.value()));
}

std::optional<std::wstring> GetSACLW(const std::wstring& wPath)
{
    PACL dAcl = nullptr;
    PSECURITY_DESCRIPTOR psd = nullptr;
    DWORD result = 0;
    LPWSTR wSddlStr = nullptr;
    std::wstring wPathUnicode = ConvertWin32UnicodePath(wPath);
    try
    {
        result = ::GetNamedSecurityInfoW(
            wPathUnicode.c_str(),
            SE_FILE_OBJECT,
            SACL_SECURITY_INFORMATION,
            NULL,
            NULL,
            &dAcl,
            NULL,
            &psd);
    }
    catch (const std::exception& e) {
        return std::nullopt;
    }
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    }
    bool ret = ::ConvertSecurityDescriptorToStringSecurityDescriptorW(
        psd,
        SDDL_REVISION_1,
        SACL_SECURITY_INFORMATION,
        &wSddlStr,
        nullptr);
    ::LocalFree(psd);
    psd = nullptr;
    if (!ret) {
        return std::nullopt;
    }
    std::wstring res(wSddlStr);
    return std::make_optional<std::wstring>(res);
}

std::optional<std::string> GetSACL(const std::string& path)
{
    std::optional<std::wstring> result = GetSACLW(Utf8ToUtf16(path));
    if (!result) {
        return std::nullopt;
    }
    return std::make_optional<std::string>(Utf16ToUtf8(result.value()));
}

std::wstring NormalizeWin32PathW(std::wstring& wPath)
{
    std::wstring wNormalizedPath = wPath;
    /* example: \\?\C:\Test\Dir1 => C:\Test\Dir1 */
    if (wNormalizedPath.find(WPATH_PREFIX) == 0) {
        wNormalizedPath = wNormalizedPath.substr(WPATH_PREFIX.length());
    }
    while (!wNormalizedPath.empty() && wNormalizedPath.back() == L'\\') {
        wNormalizedPath.pop_back();
    }
    /* example: C: */
    if (wNormalizedPath.length() == 2 && wNormalizedPath[1] == L':') {
        wNormalizedPath.push_back(L'\\');
    }
    return wNormalizedPath;
}

/*
 * reparse point related methods 
 * https://github.com/googleprojectzero/symboliclink-testing-tools/blob/main/CommonUtils/ReparsePoint.cpp
 */
bool CreateSymbolicLinkW(
    const std::wstring& wLinkFilePath,
    const std::wstring& wTagetPath,
    bool isDirectory,
    bool isRelative)
{
    std::wstring wLinkFilePathFinal = ConvertWin32UnicodePath(wLinkFilePath);
    std::wstring wTagetPathFinal = ConvertWin32UnicodePath(wTagetPath);;

    /* check if target exist */
    if (Exists(Utf16ToUtf8(wLinkFilePath))) {
        return false;
    }

    return ::CreateSymbolicLinkW(
        wLinkFilePathFinal.c_str(),
        wTagetPathFinal.c_str(),
        isDirectory ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0);
}

bool CreateJunctionPointW(const std::wstring& wSrcPath, const std::wstring& wTargetPath)
{
    // TODO
    return CreateSymbolicLinkW(wSrcPath, wTargetPath, true, true);
    return false;
}


/* ADS releated API */
std::optional<AlternateDataStreamEntry> OpenAlternateDataStreamW(const std::wstring& wPath)
{
    if (!EnablePrivilege()) {
        /* enable privilege failed */
        return std::nullopt;
    }
    std::wstring wPathUnicode = ConvertWin32UnicodePath(wPath);
    HANDLE hFile = ::CreateFileW(
        wPathUnicode.c_str(),
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
        nullptr 
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        /* failed to obtain file handle */
        return std::nullopt;
    }
    return std::make_optional<AlternateDataStreamEntry>(hFile);
}

bool IsAlternateDataStreamW(const std::wstring& wPath)
{
    std::optional<AlternateDataStreamEntry> adsEntry = OpenAlternateDataStreamW(wPath);
    std::optional<std::wstring> wStreamName;
    if (adsEntry->NextStreamNameW()) {
        return true;
    }
    return false;
}

AlternateDataStreamEntry::AlternateDataStreamEntry(HANDLE hFile)
{
    m_hFile = hFile;
    m_numReaded = 0;
    m_numToSkip = 0;
	m_context = nullptr;
}

std::optional<std::wstring> AlternateDataStreamEntry::NextStreamNameW()
{
    if (m_hFile == INVALID_HANDLE_VALUE || m_hFile == nullptr) {
        return std::nullopt;
    }
	WIN32_STREAM_ID& wsi = *((WIN32_STREAM_ID*)m_buff);
    std::wstring wStreamName;
	while (wStreamName.empty()) {
		/* we are at the start of a stream header. read it */
		if (!::BackupRead(m_hFile, m_buff, 20, &m_numReaded, FALSE, TRUE, &m_context)) {
            /* read backup data failed */
            CloseRead();
			return std::nullopt;
        }
		if (m_numReaded == 0) {
            /* read completed */
            CloseRead();
			return std::nullopt;
        }
		if (wsi.dwStreamNameSize > 0)
		{
			if (!::BackupRead(m_hFile, m_buff + 20, wsi.dwStreamNameSize, &m_numReaded, FALSE, TRUE, &m_context)) {
                /* read backup data failed */
                CloseRead();
				return std::nullopt;
            }
			if (m_numReaded != wsi.dwStreamNameSize) {
                CloseRead();
				return std::nullopt;
            }
		}
        /* check wsi struct and pick whose dwStreamId is BACKUP_ALTERNATE_DATA */
        if (wsi.dwStreamId == BACKUP_ALTERNATE_DATA && wsi.dwStreamNameSize != 0) {
            wStreamName = wsi.cStreamName;
        }
		/* skip stream data */
		if (wsi.Size.QuadPart > 0) {
			DWORD lo, hi;
			::BackupSeek(m_hFile, 0xffffffffL, 0x7fffffffL, &lo, &hi, &m_context);
		}
        if (!wStreamName.empty()) {
            break;
        }
	}
    if (!wStreamName.empty()) {
        return std::make_optional<std::wstring>(wStreamName);
    }
    return std::nullopt;
}


void AlternateDataStreamEntry::CloseRead()
{
    /* make NT release the context */
    ::BackupRead(m_hFile, m_buff, 0, &m_numReaded, TRUE, FALSE, &m_context);
	::CloseHandle(m_hFile);
    m_hFile = INVALID_HANDLE_VALUE;
    m_context = nullptr;
}

AlternateDataStreamEntry::~AlternateDataStreamEntry()
{
    if (m_hFile != INVALID_HANDLE_VALUE || m_hFile != nullptr) {
        ::CloseHandle(m_hFile);
    }
    m_hFile = INVALID_HANDLE_VALUE;
    m_context = nullptr;
}

#endif


bool IsDirectory(const std::string& path)
{
    std::optional<StatResult> result = Stat(path);
    if (!result) {
        return false;
    }
    return result->IsDirectory();
}

bool IsEmptyDirectory(const std::string& path)
{
    std::optional<OpenDirEntry> entry = OpenDir(path);
    if (!entry) {
        return false;
    }
    do {
        if (entry->Name() != "." || entry->Name() != "..") {
            return false;
        }
    } while (entry->Next());
    return true;
}

bool Exists(const std::string& path)
{
    std::optional<StatResult> result = Stat(path);
    if (!result) {
        return false;
    }
    return true;
}

bool Mkdir(const std::string& path)
{
#ifdef WIN32
    std::wstring wPathUnicode = ConvertWin32UnicodePath(Utf8ToUtf16(path));
    return CreateDirectoryW(wPathUnicode.c_str(), nullptr);
#endif
#ifdef __linux__
    int ret = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    return ret == 0;
#endif
}

bool MkdirRecursive(const std::string& path)
{
    if (!Exists(ParentDirectoryPath(path))) {
        bool success = MkdirRecursive(ParentDirectoryPath(path));
        if (!success) {
            return false;
        }
    }
    return Mkdir(path);
}

std::string ParentDirectoryPath(const std::string& path)
{
    const std::string currendDirPath = ".";
#ifdef WIN32
    std::string win32Path = path;
    std::replace(win32Path.begin(), win32Path.end(), '/', '\\');
    if (!win32Path.empty() && win32Path.back() != '\\') {
        win32Path.pop_back();
    }
    size_t pos = win32Path.find_last_of('\\');
    if (pos == std::string::npos) {
        return currendDirPath;
    }
    return win32Path.substr(0, pos);
#endif
#ifdef __linux__
    std::string posixPath = path;
    std::replace(posixPath.begin(), posixPath.end(), '\\', '/');
    if (!posixPath.empty() && posixPath.back() != '/') {
        posixPath.pop_back();
    }
    size_t pos = posixPath.find_last_of('/');
    if (pos == std::string::npos) {
        return currendDirPath;
    }
    return posixPath.substr(0, pos);
#endif
}

}