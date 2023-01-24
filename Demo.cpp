#include <iostream>
#include <optional>
#include "FileSystemUtil.h"

using namespace FileSystemUtil;

#ifdef WIN32
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
	std::cout << "fsutil -l <directory path> \t: list subdirectory/file of a directory" << std::endl;
	std::cout << "fsutil -s <path> \t\t: print the detail info of directory/file" << std::endl;
}

int DoStatCommand(const std::string& path)
{
	std::optional<StatResult> statResult = Stat(path);
	if (!statResult) {
#ifdef WIN32
		std::cout << "stat failed, errcode = " << ::GetLastError() << std::endl;
#endif
#ifdef __linux__
		std::cout << "stat failed, errcode = " << errno << std::endl;
#endif
		return 1;
	}
	std::cout << "Name: \t\t" << path << std::endl;
	std::cout << "Type: \t\t" << (statResult->IsDirectory() ? "Directory" : "File") << std::endl;
	std::cout << "UniqueID: \t" << statResult->UniqueID() << std::endl;
	std::cout << "Size: \t\t" << statResult->Size() << std::endl;
	std::cout << "Device: \t" << statResult->DeviceID() << std::endl;
	std::cout << "Links: \t\t" << statResult->LinksCount() << std::endl;
	std::cout << "Atime: \t\t" << statResult->AccessTime() << std::endl;
	std::cout << "CTime: \t\t" << statResult->CreationTime() << std::endl;
	std::cout << "MTime: \t\t" << statResult->ModifyTime() << std::endl;
#ifdef WIN32
	std::cout << "Attr: \t\t" << statResult->Attribute() << std::endl;
	std::cout << "Flags: \t\t" << Win32FileAttributeFlagsToString(statResult.value()) << std::endl;
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
#ifdef WIN32
		std::cout << "open dir failed, errcode = " << ::GetLastError() << std::endl;
#endif
#ifdef __linux__
		std::cout << "open dir failed, errcode = " << errno << std::endl;
#endif
		return 1;
	}
	else {
		do {
			std::optional<StatResult> subStatResult = Stat(openDirEntry->FullPath());
			if (subStatResult) {
				std::cout
					<< "UniqueID: " << subStatResult->UniqueID() << "\t"
					<< "Type: " << (subStatResult->IsDirectory() ? "Directory" : "File") << "\t"
					<< "Path: " << openDirEntry->FullPath()
					<< std::endl;
				total++;
			}
			else {
				std::cout << "Stat " << openDirEntry->FullPath() << " Failed" << std::endl;
			}
		} while (openDirEntry->Next());
	}
	std::cout << "Total SubItems = " << total << std::endl;
	return 0;
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		PrintHelp();
		std::cout << "insufficient paramaters" << std::endl;
		return 1;
	}
	std::string path;
	bool commandExecuted = false;
	for (int i = 1; i < argc; ++i) {
		if (std::string(argv[i]) == "-l" && i + 1 < argc) {
			return DoListCommand(std::string(argv[i + 1]));
		} else if (std::string(argv[i]) == "-s" && i + 1 < argc) {
			return DoStatCommand(std::string(argv[i + 1]));
		} else {
			return DoStatCommand(std::string(argv[i]));
		}
	}
	PrintHelp();
	std::cout << "invalid parameters";
	return 1;
}