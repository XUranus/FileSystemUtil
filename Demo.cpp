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

	if (!statResult) {
#ifdef WIN32
		std::cout << "stat failed, errcode = " << ::GetLastError() << std::endl;
#endif
#ifdef __linux__
		std::cout << "stat failed, errcode = " << errno << std::endl;
#endif
		return 1;
	}
	std::cout << "Type: " << (statResult->IsDirectory() ? "Directory" : "File") << std::endl;
	std::cout << "UniqueID: " << statResult->UniqueID() << std::endl;
	std::optional<OpenDirEntry> openDirEntry = OpenDir(path);
	if (statResult->IsDirectory()) {
		int total = 0;
		if (!openDirEntry) {
#ifdef WIN32
			std::cout << "open dir failed, errcode = " << ::GetLastError() << std::endl;
#endif
#ifdef __linux__
			std::cout << "open dir failed, errcode = " << errno << std::endl;
#endif
			return 1;
		} else {
			do {
				std::optional<StatResult> subStatResult = Stat(openDirEntry->FullPath());
				if (subStatResult) {
					std::cout
						<< "UniqueID: " << Stat(openDirEntry->FullPath())->UniqueID() << "\t"
						<< "Path: " << openDirEntry->FullPath()
						<< std::endl;
					total++;
				} else {
					std::cout << "Stat " << openDirEntry->FullPath() << " Failed" << std::endl;
				}
			} while (openDirEntry->Next());
		}
		std::cout << "Total SubItems = " << total << std::endl;
	}
	return 0;
}