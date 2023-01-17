#include "FileIOUtil.h"

using namespace std;

int main()
{
	cout << "Hello CMake." << endl;
	return 0;
}


namespace FileIO {

uint64_t OpenDirResult::Size() const
{
#ifdef WIN32
	DWORD low = m_findFileData.nFileSizeLow;
	DWORD high = m_findFileData.nFileSizeHigh;
	return low + (MAXDWORD + 1) * high;
#endif
#ifdef LINUX
	if (DoStat()) {
		return m_stat.m_size;
	} else {
		return 0;
	}
#endif
}




#ifdef WIN32
bool OpenDirResult::IsArchive() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE;
}

bool OpenDirResult::IsCompressed() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED;
}

bool OpenDirResult::IsEncrypted() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED;
}

bool OpenDirResult::IsSparseFile() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED;
}

bool OpenDirResult::IsHidden() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN;
}

bool OpenDirResult::IsOffline() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE;
}

bool OpenDirResult::IsReadOnly() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
}

bool OpenDirResult::IsSystem() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM;
}

bool OpenDirResult::IsTemporary() const
{
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY;
}
#endif





#ifdef LINUX
bool OpenDirResult::IsUnknown() const
{
	return (m_dirent->d_type & DT_KNOWN != 0);
}

bool OpenDirResult::IsPipe() const
{
	return (m_dirent->d_type & DT_FIFO != 0);
}

bool OpenDirResult::IsCharDevice() const
{
	return (m_dirent->d_type & DT_CHR != 0);
}

bool OpenDirResult::IsBlockDevice() const
{
	return (m_dirent->d_type & DT_BLK != 0);
}

bool OpenDirResult::IsSymLink() const
{
	return (m_dirent->d_type & DT_LNK != 0);
}

bool OpenDirResult::IsSocket() const
{
	return (m_dirent->d_type & DT_SOCK != 0);
}
#endif






uint64_t OpenDirResult::INode()
{
#ifdef WIN32
	if (DoStat()) {
		return std::static_cast<uint64_t>(m_stat.st_ino);
	} else {
		return 0;
	}
#endif
#ifdef LINUX
	return static_cast<uint64_t>(m_dirent->d_ino);
#endif
}

bool OpenDirResult::IsNormal() const
{
#ifdef WIN32
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL;
#endif
#ifdef LINUX
	return (m_dirent->d_type & DT_REG != 0);
#endif
}

bool OpenDirResult::IsDirectory() const
{
#ifdef WIN32
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
#endif
#ifdef LINUX
	return m_dirent->d_type & DT_DIR;
#endif
}

std::string OpenDirResult::Name() const
{
#ifdef WIN32
	return std::string(m_findFileData.cFileName);
#endif
#ifdef LINUX
	return std::string(m_dirent->d_name);
#endif
}

uint64_t OpenDirResult::AccessTime() const
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

uint64_t OpenDirResult::CreationTime() const
{
#ifdef WIN32
	DWORD low = m_findFileData.ftCreationTime.dwLowDateTime;
	DWORD high = m_findFileData.ftCreationTime.dwHighDateTime;
	return low + (MAXDWORD + 1) * high;
#endif
#ifdef LINUX
	if (DoStat()) {
		return std::static_cast<uint64_t>(m_stat.st_ctime);
	else {
		return 0;
	}
#endif
}

uint64_t OpenDirResult::ModifyTime() const
{
#ifdef WIN32
	DWORD low = m_findFileData.ftLastWriteTime.dwLowDateTime;
	DWORD high = m_findFileData.ftLastWriteTime.dwHighDateTime;
	return low + (MAXDWORD + 1) * high;
#endif
#ifdef LINUX
	if (DoStat()) {
		return std::static_cast<uint64_t>(m_stat.st_mtime);
	else {
		return 0;
	}
#endif
}

bool OpenDirResult::DoStat()
{
	if (m_stated) {
		return true;
	}
#ifdef WIN32
	const std::string separator = "\\";
#else
	const std::string separator = "/";
#endif
	std::string fullpath = m_dirPath + separator + Name();
	if (stat(fullpath.c_str(), &m_stat) < 0) {
		m_statFailed = true;
		return false;
	}
	m_stated = true;
}






std::optional<OpenDirResult> OpenDir(const std::string& path)
{
#ifdef WIN32
	OpenDirResult iterator;
	iterator.m_dirPath = path;
	iterator.m_fileHandle = ::FindFirstFile(path.c_str(), &iterator.m_findFileData);
	if (iterator.m_fileHandle == INVALID_HANDLE_VALUE) {
		return std::nullopt;
	}
	return std::optional(iterator);
#endif

#ifdef LINUX
	OpenDirResult iterator;
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

bool OpenDirResult::Next()
{
	memset(&m_stat, 0, sizeof(struct stat));
	m_stated = false;
	m_statFailed = false;
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


void OpenDirResult::Close()
{
#ifdef WIN32
	if (m_fileHandle != nullptr && m_fileHandle != INVALID_HANDLE_VALUE) {
		::FindClose(m_fileHandle);
		m_fileHandle = nullptr;
	}
#endif
}

}