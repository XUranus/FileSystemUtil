﻿#include <iostream>
#include <optional>
#include <fstream>
#include <chrono>
#include <ctime>

#ifdef _WIN32
#pragma execution_character_set("utf-8")
#endif

#include "FileSystemUtil.h"

using namespace FileSystemUtil;

#ifdef _WIN32
static std::wstring GetLastErrorAsStringW(DWORD errorID)
{
    LPWSTR buffer = nullptr;

    DWORD length = ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorID,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&buffer,
        0,
        nullptr);

    std::wstring errorMessage(buffer, length);
    ::LocalFree(buffer);

    return errorMessage;
}
#endif

static std::string ErrorMessage()
{
#ifdef _WIN32
    return Utf16ToUtf8(GetLastErrorAsStringW(::GetLastError()));
#endif
#ifdef __linux__
    return std::string(strerror(errno)) + "(" + std::to_string(errno) + ")";
#endif
}

static std::string TimestampSecondsToDate(uint64_t timestamp)
{
    auto millsec = std::chrono::seconds(timestamp);
    auto tp = std::chrono::time_point<
    std::chrono::system_clock, std::chrono::seconds>(millsec);
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm* now = std::gmtime(&tt);
    char strtime[100] = "";
    if (std::strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", now) > 0) {
        return std::string(strtime);
    }
    return std::to_string(timestamp);
}

#ifdef _WIN32
std::string Win32FileAttributeFlagsToString(const StatResult& statResult)
{
    std::string ret;
    if (statResult.IsArchive()) { ret += "ARCHIVE | "; }
    if (statResult.IsCompressed()) { ret += "COMPRESSED | "; }
    if (statResult.IsEncrypted()) { ret += "ENCRYPTED | "; }
    if (statResult.IsSparseFile()) { ret += "SPARSE | "; }
    if (statResult.IsHidden()) { ret += "HIDDEN | "; }
    if (statResult.IsOffline()) { ret += "OFFLINE | "; }
    if (statResult.IsReadOnly()) { ret += "READONLY | "; }
    if (statResult.IsSystem()) { ret += "SYSTEM | "; }
    if (statResult.IsTemporary()) { ret += "TEMP | "; }
    if (statResult.IsNormal()) { ret += "NORMAL | "; }
    if (statResult.IsReparsePoint()) { ret += "REPARSE | "; }
    if (!ret.empty() && ret.back() == ' ') {
        ret.pop_back();
        ret.pop_back();
    }
    return ret;
}
#endif

#ifdef __linux__
std::string LinuxFileModeFlagsToString(const StatResult& statResult)
{
    std::string ret;
    if (statResult.IsRegular()) { ret += "REGULAR | "; }
    if (statResult.IsPipe()) { ret += "PIPE | "; }
    if (statResult.IsCharDevice()) { ret += "CHAR | "; }
    if (statResult.IsBlockDevice()) { ret += "BLOCK | "; }
    if (statResult.IsSymLink()) { ret += "SYMLINK | "; }
    if (statResult.IsSocket()) { ret += "SOCKET | "; }
    if (!ret.empty() && ret.back() == ' ') {
        ret.pop_back();
        ret.pop_back();
    }
    return ret;
}
#endif

void PrintHelp()
{
    std::cout << "fsutil  , stat/opendir demo for windows/linux" << std::endl;
    std::cout << "Usage: " << std::endl;
    std::cout << "fsutil -ls <directory path> \t: list subdirectory/file of a directory" << std::endl;
    std::cout << "fsutil -stat <path> \t\t: print the detail info of directory/file" << std::endl;
    std::cout << "fsutil -mkdir <path> \t\t: create directory recursively" << std::endl;
    std::cout << "fsutil -sparse <path> \t\t: query sparse file allocate ranges" << std::endl;
    std::cout << "fsutil -cpsparse <src> <dst> \t: copy sparse file" << std::endl;
#ifdef _WIN32
    std::cout << "fsutil -getsd <path> \t\t: list security descriptor string of _WIN32 path" << std::endl;
    std::cout << "fsutil -copysd <path> \t\t: copy security descriptor from src to target" << std::endl;
    std::cout << "fsutil -mksymlink <link> <target> \t\t: create symbolic link" << std::endl;
    std::cout << "fsutil --drivers \t\t: list drivers" << std::endl;
    std::cout << "fsutil --volumes \t\t: list volumes" << std::endl;
#endif
}

int DoStatCommand(const std::string& path)
{
    std::optional<StatResult> statResult = Stat(path);
    if (!statResult) {
        std::cout << "stat failed, error: " << ErrorMessage() << std::endl;
        return 1;
    }
    std::cout << "Name: \t\t" << statResult->CanonicalPath() << std::endl;
    std::cout << "Type: \t\t" << (statResult->IsDirectory() ? "Directory" : "File") << std::endl;
    std::cout << "UniqueID: \t" << statResult->UniqueID() << std::endl;
    std::cout << "Size: \t\t" << statResult->Size() << std::endl;
    std::cout << "Device: \t" << statResult->DeviceID() << std::endl;
    std::cout << "Links: \t\t" << statResult->LinksCount() << std::endl;
    std::cout << "Atime: \t\t" << TimestampSecondsToDate(statResult->AccessTime())  << std::endl;
    std::cout << "CTime: \t\t" << TimestampSecondsToDate(statResult->CreationTime()) << std::endl;
    std::cout << "MTime: \t\t" << TimestampSecondsToDate(statResult->ModifyTime()) << std::endl;
#ifdef _WIN32
    std::cout << "Attr: \t\t" << statResult->Attribute() << std::endl;
    std::cout << "Flags: \t\t" << Win32FileAttributeFlagsToString(statResult.value()) << std::endl;
    if (statResult->IsReparsePoint()) {
        if (statResult->HasReparseMountPointTag()) { std::cout << "Reparse: \tMountPoint" << std::endl; }
        if (statResult->HasReparseNfsTag()) { std::cout << "Reparse: \tNFS" << std::endl; }
        if (statResult->HasReparseOneDriveTag()) { std::cout << "Reparse: \tOnedrive" << std::endl; }
        if (statResult->HasReparseSymbolicLinkTag()) { std::cout << "Reparse: \tSymlink" << std::endl; }
        if (statResult->IsMountedDevice()) {
            std::wcout << L"Device: \t" << statResult->MountedDeviceNameW().value() << std::endl;
        }
        if (statResult->IsJunctionPoint()) {
            std::wcout << L"Junction: \t" << statResult->JunctionsPointTargetPathW().value() << std::endl;
        }
        if (statResult->IsSymbolicLink()) {
            std::wcout << L"Symlink: \t" << statResult->SymbolicLinkTargetPathW().value() << std::endl;
        }
        if (statResult->FinalPathW()) {
            std::wcout << L"FinalPath: \t" << statResult->FinalPathW().value() << std::endl;
        }
    }
    /* check ADS file */
    std::optional<AlternateDataStreamEntry> adsEntry = OpenAlternateDataStreamW(Utf8ToUtf16(path));
    if (!adsEntry) {
        if (::GetLastError() != ERROR_HANDLE_EOF) {
            std::cout << "open ADS stream failed, error: " << ErrorMessage() << std::endl;
            return -1;
        } else {
            /* no other data stream */
            return 0;
        }
    }
    int adsIndex = 1;
    do {
        std::wstring wStreamName = adsEntry->StreamNameW();
        if (wStreamName == L"::$DATA") {
            continue; // skip main data stream
        }
        std::wcout << L"Stream[" << adsIndex++ << L"] " << wStreamName << std::endl;
    } while (adsEntry->Next());
#endif
#ifdef __linux__
    std::cout << "Mode: \t\t" << statResult->Mode() << std::endl;
    std::cout << "Flags: \t\t" << LinuxFileModeFlagsToString(statResult.value()) << std::endl;
#endif
    return 0;
}

int DoListCommand(const std::string& path)
{
    int total = 0;
    std::optional<OpenDirEntry> openDirEntry = OpenDir(path);
    if (!openDirEntry) {
#ifdef _WIN32
        std::cout << "open dir failed, error: " << ErrorMessage() << std::endl;
#endif
#ifdef __linux__
        std::cout << "open dir failed, error: " << ErrorMessage() << std::endl;
#endif
        return 1;
    }
    else {
        do {
            if (openDirEntry->Name() == "." || openDirEntry->Name() == "..") {
                continue;
            }
            std::optional<StatResult> subStatResult = Stat(openDirEntry->FullPath());
            if (subStatResult) {
                std::string type = subStatResult->IsDirectory() ? "Directory" : "File";

#ifdef _WIN32
                if (subStatResult->IsReparsePoint()) {
                    if (subStatResult->SymbolicLinkTargetPathW()) {
                        type = "Symbolic";
                    } else if (subStatResult->JunctionsPointTargetPathW()) {
                        type = "Junction";
                    } else if (subStatResult->MountedDeviceNameW()) {
                        type = "MountDev";
                    } else {
                        type = "Invalid";
                    }
                }
#endif
                std::cout
                    << "UniqueID: " << subStatResult->UniqueID() << "\t"
#ifdef _WIN32
                    << "Attribute: " << subStatResult->Attribute() << "\t"
#endif
                    << "Type: " << type << "\t"
                    << "Path: " << openDirEntry->FullPath()
                    << std::endl;
                total++;
            }
            else {
                std::cout << "Stat " << openDirEntry->FullPath() << " Failed, error: " << ErrorMessage() << std::endl;
            }
        } while (openDirEntry->Next());
    }
    std::cout << "Total SubItems = " << total << std::endl;
    return 0;
}

int DoMkdirCommand(const std::string& path)
{
    if (MkdirRecursive(path)) {
        std::cout << "Success" << std::endl;
        return 0;
    } else {
        std::cout << "Failed" << std::endl;
        return -1;
    }
}

int DoQuerySparseCommand(const std::string& path)
{
    std::optional<StatResult> statResult = Stat(path);
    if (!statResult) {
        std::cout << "File Not Exist" << std::endl;
        return -1;
    }
    SparseRangeResult result = QuerySparseAllocateRanges(path);
    if (!result) {
        std::cout << "file is not a sparse file" << std::endl;
        return -1;
    }
    std::cout << "Logical Size: " << statResult->Size() << std::endl;
    std::cout << "Sparse Allocate Range:" << std::endl;
    uint64_t physicAllocTotal = 0;
    for (const std::pair<uint64_t, uint64_t>& range: result.value()) {
        physicAllocTotal += range.second;
        std::cout << "offset = " << range.first << " , length = " << range.second << std::endl;
    }
    std::cout << "Physic Allocate Size: " << physicAllocTotal << std::endl;
    std::cout << "Hole Size: " << statResult->Size() - physicAllocTotal << std::endl;
    return 0;
}

int DoCopySparseCommand(const std::string& srcPath, const std::string& dstPath)
{
    std::optional<StatResult> statResult = Stat(srcPath);
    if (!statResult) {
        std::cout << "Source File Not Exist" << std::endl;
        return -1;
    }
    SparseRangeResult result = QuerySparseAllocateRanges(srcPath);
    if (!result) {
        std::cout << "Source file is not a sparse file" << std::endl;
        return -1;
    }
    if (!CopySparseFile(srcPath, dstPath, result.value())) {
        std::cout << "Copy Failed" << std::endl;
        return -1;
    }
    std::cout << "Copy Succeed" << std::endl;
    return 0;
}

#ifdef _WIN32
int DoGetSecurityDescriptorWCommand(const std::wstring& wPath)
{
    DWORD retCode = 0;
    std::optional<std::wstring> wSd = GetSecurityDescriptorW(wPath, retCode);
    if (wSd) {
        std::wcout << L"SecurityDescriptor:\n" << wSd.value() << std::endl;
        return 0;
    }
    std::wcout << L"error: " << retCode << std::endl;
    return -1;
}

int DoCopySecurityDescriptorWCommand(const std::wstring& wPathSrc, const std::wstring wPathTarget)
{
    DWORD retCode = 0;
    std::optional<std::wstring> wSd = GetSecurityDescriptorW(wPathSrc, retCode);
    std::wcout << wPathSrc << L" ==> " << wPathTarget << std::endl;
    if (!wSd) {
        std::wcerr << L"get security descriptor failed, error: " << retCode << std::endl;
        return -1;
    }
    std::wcout << L"SecurityDescriptor:\n" << wSd.value() << std::endl;
    if (!SetSecurityDescriptorW(wPathTarget, wSd.value(), retCode)) {
        std::wcerr << L"set security descriptor failed, error: " << retCode << std::endl;
        return -1;
    }
    std::wcout << "Success" << std::endl;
}

void ListWin32Drivers()
{
    std::vector<std::wstring> wDrivers = GetWin32DriverListW();
    for (const std::wstring& wDriver : wDrivers) {
        std::wcout << wDriver << std::endl;
    }
    return;
}

void ListWin32Volumes()
{
    std::optional<std::vector<Win32VolumesDetail>> wVolumes = GetWin32VolumeList();
    if (!wVolumes) {
        std::wcout << L"failed to list volumes, error: " << Utf8ToUtf16(ErrorMessage()) << std::endl;
        return;
    }
    for (Win32VolumesDetail& volumeDetail: wVolumes.value()) {
        std::wcout << L"Name: \t\t" << volumeDetail.VolumeNameW() << std::endl;
        if (volumeDetail.GetVolumeDeviceNameW()) {
            std::wcout << L"Device: \t" << volumeDetail.GetVolumeDeviceNameW().value() << std::endl;
        }
        if (volumeDetail.GetVolumePathListW()) {
            int index = 0;
            std::vector<std::wstring> wPathList = volumeDetail.GetVolumePathListW().value();
            for (const std::wstring& wPath : wPathList) {
                std::wcout << L"Path" << ++index << L": \t\t" << wPath << std::endl;
            }
        }
        std::wcout << std::endl;
    }
    return;
}

int DoMakeSymlinkCommand(const std::wstring& wLinkFilePath, const std::wstring& wTargetPath)
{
    if (CreateSymbolicLinkW(wLinkFilePath, wTargetPath, true, true)) {
        std::wcout << wLinkFilePath << L" ===> " << wTargetPath << std::endl;
        return 0;
    } else {
        std::wcout << L"create symbolic link failed" << std::endl;
        return -1;
    }
}

void TryRequiringPrivilege()
{
    // Set ACL
    if (!EnablePrivilegeW(SE_RESTORE_NAME)) {
        std::cerr << "== Warning: Error enabling SeRestorePrivilege! == " << std::endl;
    }
}

int wmain(int argc, WCHAR** argv)
{
    ::SetConsoleOutputCP(65001); // forcing cmd to use UTF-8 output encoding
    TryRequiringPrivilege();
    if (argc < 2) {
        PrintHelp();
        std::wcout << L"insufficient paramaters" << std::endl;
        return 1;
    }
    bool commandExecuted = false;
    for (int i = 1; i < argc; ++i) {
        if (std::wstring(argv[i]) == L"-ls" && i + 1 < argc) {
            return DoListCommand(Utf16ToUtf8(std::wstring(argv[i + 1])));
        } else if (std::wstring(argv[i]) == L"-stat" && i + 1 < argc) {
            return DoStatCommand(Utf16ToUtf8(std::wstring(argv[i + 1])));
        } else if (std::wstring(argv[i]) == L"-mkdir" && i + 1 < argc) {
            return DoMkdirCommand(Utf16ToUtf8(std::wstring(argv[i + 1])));
        } else if (std::wstring(argv[i]) == L"-sparse" && i + 1 < argc) {
            return DoQuerySparseCommand(Utf16ToUtf8(std::wstring(argv[i + 1])));
        } else if (std::wstring(argv[i]) == L"-cpsparse" && i + 2 < argc) {
            return DoCopySparseCommand(Utf16ToUtf8(std::wstring(argv[i + 1])), Utf16ToUtf8(std::wstring(argv[i + 2])));
        } else if (std::wstring(argv[i]) == L"-getsd" && i + 1 < argc) {
            return DoGetSecurityDescriptorWCommand(std::wstring(argv[i + 1]));
        } else if (std::wstring(argv[i]) == L"-copysd" && i + 2 < argc) {
            return DoCopySecurityDescriptorWCommand(std::wstring(argv[i + 1]), std::wstring(argv[i + 2]));
        } else if (std::wstring(argv[i]) == L"-mksymlink" && i + 2 < argc) {
            return DoMakeSymlinkCommand(std::wstring(argv[i + 1]), std::wstring(argv[i + 2]));
        } else if (std::wstring(argv[i]) == L"--drivers") {
            ListWin32Drivers();
            return 0;
        } else if (std::wstring(argv[i]) == L"--volumes") {
            ListWin32Volumes();
            return 0;
        } else {
            return DoStatCommand(Utf16ToUtf8(std::wstring(argv[i])));
        }
    }
    PrintHelp();
    std::cout << "invalid parameters";
    return 1;



}

#else

int main(int argc, char** argv)
{
    if (argc < 2) {
        PrintHelp();
        std::cout << "insufficient paramaters" << std::endl;
        return 1;
    }
    bool commandExecuted = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-ls" && i + 1 < argc) {
            return DoListCommand(std::string(argv[i + 1]));
        } else if (std::string(argv[i]) == "-stat" && i + 1 < argc) {
            return DoStatCommand(std::string(argv[i + 1]));
        } else if (std::string(argv[i]) == "-mkdir" && i + 1 < argc) {
            return DoMkdirCommand(std::string(argv[i + 1]));
        } else if (std::string(argv[i]) == "-sparse" && i + 1 < argc) {
            return DoQuerySparseCommand(std::string(argv[i + 1]));
        } else if (std::string(argv[i]) == "-cpsparse" && i + 2 < argc) {
            return DoCopySparseCommand(std::string(argv[i + 1]), std::string(argv[i + 2]));
        } else {
            return DoStatCommand(std::string(argv[i]));
        }
    }
    PrintHelp();
    std::cout << "invalid parameters";
    return 1;
}

#endif
