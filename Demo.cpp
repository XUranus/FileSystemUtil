#include <iostream>
#include <optional>
#include "FileSystemUtil.h"

using namespace FileSystemUtil;

int main(int argc, char** argv)
{
	if (argc < 2) {
		std::cout << "insuffcient paramaters" << std::endl;
	}
	std::string path = std::string(argv[1]);
	std::optional<StatResult> statResult = Stat(path);
#ifdef WIN32
	if (!statResult) {
		std::cout << "stat failed, errcode = " << ::GetLastError() << std::endl;
		return 1;
	} else {
		std::cout << "UniqueID: " << statResult->UniqueID() << std::endl;
	}
	std::optional<OpenDirEntry> openDirEntry = OpenDir(path);
	if (!openDirEntry) {
		std::cout << "open dir failed, errcode = " << ::GetLastError() << std::endl;
		return 1;
	} else {
		do {
			std::cout << "Path: " << openDirEntry->FullPath() << std::endl;
		} while (openDirEntry->Next());
	}
#endif

#ifdef __linux__
	if (!statResult) {
		std::cout << "stat failed, errcode = " << errno << std::endl;
		return 1;
	}
	else {
		std::cout << "UniqueID: " << statResult->UniqueID() << std::endl;
	}
	std::optional<OpenDirEntry> openDirEntry = OpenDir(path);
	if (!openDirEntry) {
		std::cout << "open dir failed, errcode = " << errno << std::endl;
		return 1;
	}
	else {
		do {
			std::cout << "Path: " << openDirEntry->FullPath() << std::endl;
		} while (openDirEntry->Next());
	}
#endif
	return 0;
}