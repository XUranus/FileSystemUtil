#include "FileIOUtil.h"

using namespace std;

int main()
{
	cout << "Hello CMake." << endl;
	return 0;
}


namespace FileIO {

uint64_t OpenDirIterator::Size() const
{
#ifdef WIN32
	DWORD low = m_findFileData.nFileSizeLow;
	DWORD high = m_findFileData.nFileSizeHigh;
	return low + (MAXDWORD + 1) * high;
#endif
}




#ifdef WIN32
bool OpenDirIterator::IsArchive() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE;
}

bool OpenDirIterator::IsCompressed() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED;
}

bool OpenDirIterator::IsEncrypted() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED;
}

bool OpenDirIterator::IsSparseFile() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED;
}

bool OpenDirIterator::IsHidden() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN;
}

bool OpenDirIterator::IsOffline() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE;
}

bool OpenDirIterator::IsReadOnly() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
}

bool OpenDirIterator::IsSystem() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM;
}

bool OpenDirIterator::IsTemporary() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY;
}
#endif





#ifdef LINUX
bool OpenDirIterator::IsUnknown() const
{
	return (m_dirent->d_type & DT_KNOWN != 0);
}

bool OpenDirIterator::IsPipe() const
{
	return (m_dirent->d_type & DT_FIFO != 0);
}

bool OpenDirIterator::IsCharDevice() const
{
	return (m_dirent->d_type & DT_CHR != 0);
}

bool OpenDirIterator::IsBlockDevice() const
{
	return (m_dirent->d_type & DT_BLK != 0);
}

bool OpenDirIterator::IsSymLink() const
{
	return (m_dirent->d_type & DT_LNK != 0);
}

bool OpenDirIterator::IsSocket() const
{
	return (m_dirent->d_type & DT_SOCK != 0);
}
#endif






uint64_t OpenDirIterator::INode() const
{
#ifdef WIN32
	// TODO:: do stat
#endif
#ifdef LINUX
	return static_cast<uint64_t>(m_dirent->d_ino);
#endif
}

bool OpenDirIterator::IsNormal() const
{
#ifdef WIN32
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL;
#endif
#ifdef LINUX
	return (m_dirent->d_type & DT_REG != 0);
#endif
}

bool OpenDirIterator::IsDirectory() const
{
#ifdef WIN32
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
#endif
#ifdef LINUX
	return m_dirent->d_type & DT_DIR;
#endif
}

std::string OpenDirIterator::Name() const
{
#ifdef WIN32
	return std::string(m_findFileData.cFileName);
#endif
#ifdef LINUX
	return std::string(m_dirent->d_name);
#endif
}

uint64_t OpenDirIterator::AccessTime() const
{
#ifdef WIN32
	DWORD low = m_findFileData.ftLastAccessTime.dwLowDateTime;
	DWORD high = m_findFileData.ftLastAccessTime.dwHighDateTime;
	return low + (MAXDWORD + 1) * high;
#endif
#ifdef LINUX
	if (DoStat()) {
		return std::static_cast<uint64_t>(m_stat.st_atime);
	else {
		return 0;
	}
#endif
}

uint64_t OpenDirIterator::CreationTime() const
{
#ifdef WIN32
	DWORD low = m_findFileData.ftCreationTime.dwLowDateTime;
	DWORD high = m_findFileData.ftCreationTime.dwHighDateTime;
	return low + (MAXDWORD + 1) * high;
#endif
}

uint64_t OpenDirIterator::ModifyTime() const
{
#ifdef WIN32
	DWORD low = m_findFileData.ftLastWriteTime.dwLowDateTime;
	DWORD high = m_findFileData.ftLastWriteTime.dwHighDateTime;
	return low + (MAXDWORD + 1) * high;
#endif
}

bool OpenDirIterator::DoStat()
{
	if (m_stated) {
		return true;
	}
	std::string fullpath = m_dirPath + SEPARATOR + Name();
	if (stat(fullpath.c_str(), &m_stat) < 0) {
		return false;
	}
	m_stated = true;
}






std::optional<OpenDirIterator> OpenDir(const std::string& path)
{
#ifdef WIN32
	OpenDirIterator iterator;
	iterator.m_dirPath = path;
	iterator.m_fileHandle = ::FindFirstFile(path.c_str(), &iterator.m_findFileData);
	if (iterator.m_fileHandle == INVALID_HANDLE_VALUE) {
		return std::nullopt;
	}
	return std::optional(iterator);
#endif

#ifdef LINUX
	OpenDirIterator iterator;
	iterator.m_dirPath = path;
	DIR* dir = ::opendir(path.c_str());
	if (dir == nullptr) {
		return std::nullopt;
	}
	iterator.m_dir = dir;
	iterator.m_dirent = readdir(dir);
	if (iterator.m_dirent == nullptr) {
		return std::nullopt;
	}
	return std::optional(iterator);
#endif
	return std::nullopt;
}

void OpenDirIterator::Next()
{
	memset(&m_stat, 0, sizeof(struct stat));
	m_stated = false;
#ifdef WIN32
	if (m_fileHandle == nullptr || m_fileHandle == INVALID_HANDLE_VALUE) {
		return false;
	}
	if (!::FindNextFile(m_fileHandle, &m_findFileData)) {
		return false;
	}
#endif
#ifdef LINUX
	if (m_dir == nullptr) {
		return false;
	}
	m_dirent = readdir(m_dir);
	if (m_dirent == nullptr) {
		return false;
	}
#endif
}

bool OpenDirIterator::HasNext()
{

}

void OpenDirIterator::Close()
{
#ifdef WIN32
	if (m_fileHandle != nullptr && m_fileHandle != INVALID_HANDLE_VALUE) {
		::FindClose(m_fileHandle);
		m_fileHandle = nullptr;
	}
#endif
}

}