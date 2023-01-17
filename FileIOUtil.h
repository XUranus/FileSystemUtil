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


struct OpenDirIterator
{
	void Next();
	bool HasNext();
	void Close();
	OpenDirIterator operator ++();


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


	uint64_t INode() const;
	bool IsNormal() const;
	bool IsDirectory() const;
	std::string Name() const;
	uint64_t Size() const;
	uint64_t AccessTime() const;
	uint64_t CreationTime() const;
	uint64_t ModifyTime() const;
	bool DoStat();

	std::string m_dirPath;
	bool m_stated = false;
	struct stat m_stat {};
#ifdef WIN32
	HANDLE m_fileHandle = nullptr;
	WIN32_FIND_DATA m_findFileData;
#endif

#ifdef LINUX
	DIR* m_dir;
	struct dirent* m_dirent;
#endif

	// forbid copy
	OpenDirIterator(const OpenDirIterator&) = delete;
	OpenDirIterator operator = (const OpenDirIterator&) = delete;
};




OpenDirIterator operator + (int n)
{

}



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
*/