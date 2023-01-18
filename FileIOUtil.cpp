#include "FileIOUtil.h"

using namespace std;

namespace {
constexpr auto WIN32_UID = 0;
constexpr auto WIN32_GID = 0;
}

namespace FileIO {

#ifdef LINUX
StatResult::StatResult(const struct stat& statbuff)
{
	memcpy_s(&m_stat, sizeof(struct stat), &statbuff, sizeof(struct stat));
}
#endif

#ifdef WIN32
StatResult::StatResult(const BY_HANDLE_FILE_INFORMATION& handleFileInformation)
{
	memcpy_s(&m_handleFileInformation, sizeof(BY_HANDLE_FILE_INFORMATION),
		&handleFileInformation, sizeof(BY_HANDLE_FILE_INFORMATION));
}
#endif

uint64_t StatResult::UserID() const
{
#ifdef LINUX
	return static_cast<uint64_t>(m_stat.st_gid);
#endif
#ifdef WIN32
	return WIN32_UID;
#endif
}

uint64_t StatResult::GroupID() const
{
#ifdef LINUX
	return static_cast<uint64_t>(m_stat.st_uid);
#endif
#ifdef WIN32
	return WIN32_GID;
#endif
}

uint64_t StatResult::AccessTime() const
{
#ifdef LINUX
	return static_cast<uint64_t>(m_stat.st_atime);
#endif
#ifdef WIN32
	DWORD low = m_handleFileInformation.ftLastAccessTime.dwLowDateTime;
	DWORD high = m_handleFileInformation.ftLastAccessTime.dwHighDateTime;
	return low + (MAXDWORD + 1) * high;
#endif
}

uint64_t StatResult::CreationTime() const
{
#ifdef LINUX
	return static_cast<uint64_t>(m_stat.st_ctime);
#endif
#ifdef WIN32
	DWORD low = m_handleFileInformation.ftCreationTime.dwLowDateTime;
	DWORD high = m_handleFileInformation.ftCreationTime.dwHighDateTime;
	return low + (MAXDWORD + 1) * high;
#endif
}

uint64_t StatResult::ModifyTime() const
{
#ifdef LINUX
	return static_cast<uint64_t>(m_stat.st_mtime);
#endif
#ifdef WIN32
	DWORD low = m_handleFileInformation.ftLastWriteTime.dwLowDateTime;
	DWORD high = m_handleFileInformation.ftLastWriteTime.dwHighDateTime;
	return low + (MAXDWORD + 1) * high;
#endif
}

uint64_t StatResult::UniqueID() const
{
#ifdef LINUX
	return static_cast<uint64_t>(m_stat.st_ino);
#endif
#ifdef WIN32
	DWORD low = m_handleFileInformation.nFileIndexLow;
	DWORD high = m_handleFileInformation.nFileIndexHigh;
	return low + (MAXDWORD + 1) * high;
#endif
}

uint64_t StatResult::Size() const
{
#ifdef LINUX
	return static_cast<uint64_t>(m_stat.st_size);
#endif
#ifdef WIN32
	DWORD low = m_handleFileInformation.nFileSizeLow;
	DWORD high = m_handleFileInformation.nFileSizeHigh;
	return low + (MAXDWORD + 1) * high;
#endif
}

uint64_t StatResult::DeviceNmber() const
{
#ifdef LINUX
	return static_cast<uint64_t>(m_stat.st_rdev);
#endif
#ifdef WIN32
	return static_cast<uint64_t>(m_handleFileInformation.dwVolumeSerialNumber);
#endif
}

uint64_t StatResult::LinksNumber() const
{
#ifdef LINUX
	return static_cast<uint64_t>(m_stat.st_rdev);
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
#ifdef LINUX
	return (m_stat.st_mode & S_IFDIR) != 0;
#endif
}

#ifdef WIN32
bool StatResult::IsArchive() const { return m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE; }
bool StatResult::IsCompressed() const { return m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED; }
bool StatResult::IsEncrypted() const { return m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED; }
bool StatResult::IsSparseFile() const { return m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE; }
bool StatResult::IsHidden() const { return m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN; }
bool StatResult::IsOffline() const { return m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE; }
bool StatResult::IsReadOnly() const { return m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY; }
bool StatResult::IsSystem() const { return m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM; }
bool StatResult::IsTemporary() const { return m_handleFileInformation.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY; }
#endif

#ifdef LINUX
bool StatResult::IsRegular() const { return (m_stat.m_mode & S_IFREG) != 0; }
bool StatResult::IsPipe() const { return (m_stat.m_mode & S_IFFIFO) != 0; }
bool StatResult::IsCharDevice() const { return (m_stat.m_mode & S_IFCHR) != 0; }
bool StatResult::IsBlockDevice() const { return (m_stat.m_mode & S_IFBLK) != 0; }
bool StatResult::IsSymLink() const { return (m_stat.m_mode & S_IFLNK) != 0; }
bool StatResult::IsSocket() const { return (m_stat.m_mode & S_IFSOCK) != 0; }
#endif



uint64_t OpenDirEntry::Size() const
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
bool OpenDirEntry::IsArchive() const {	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE; }
bool OpenDirEntry::IsCompressed() const { return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED; }
bool OpenDirEntry::IsEncrypted() const { return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED; }
bool OpenDirEntry::IsSparseFile() const { return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE; }
bool OpenDirEntry::IsHidden() const { return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN; }
bool OpenDirEntry::IsOffline() const {	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE; }
bool OpenDirEntry::IsReadOnly() const { return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY; }
bool OpenDirEntry::IsSystem() const { return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM; }
bool OpenDirEntry::IsTemporary() const { return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY; }
#endif





#ifdef LINUX
bool OpenDirEntry::IsUnknown() const
{
	return (m_dirent->d_type & DT_KNOWN != 0);
}

bool OpenDirEntry::IsPipe() const
{
	return (m_dirent->d_type & DT_FIFO != 0);
}

bool OpenDirEntry::IsCharDevice() const
{
	return (m_dirent->d_type & DT_CHR != 0);
}

bool OpenDirEntry::IsBlockDevice() const
{
	return (m_dirent->d_type & DT_BLK != 0);
}

bool OpenDirEntry::IsSymLink() const
{
	return (m_dirent->d_type & DT_LNK != 0);
}

bool OpenDirEntry::IsSocket() const
{
	return (m_dirent->d_type & DT_SOCK != 0);
}
#endif






uint64_t OpenDirEntry::INode()
{
#ifdef WIN32
	if (DoStat()) {
		return static_cast<uint64_t>(m_stat.st_ino);
	} else {
		return 0;
	}
#endif
#ifdef LINUX
	return static_cast<uint64_t>(m_dirent->d_ino);
#endif
}

bool OpenDirEntry::IsNormal() const
{
#ifdef WIN32
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL;
#endif
#ifdef LINUX
	return (m_dirent->d_type & DT_REG != 0);
#endif
}

bool OpenDirEntry::IsDirectory() const
{
#ifdef WIN32
	return m_findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
#endif
#ifdef LINUX
	return m_dirent->d_type & DT_DIR;
#endif
}

std::string OpenDirEntry::Name() const
{
#ifdef WIN32
	return std::string(m_findFileData.cFileName);
#endif
#ifdef LINUX
	return std::string(m_dirent->d_name);
#endif
}

uint64_t OpenDirEntry::AccessTime() const
{
#ifdef WIN32
	DWORD low = m_findFileData.ftLastAccessTime.dwLowDateTime;
	DWORD high = m_findFileData.ftLastAccessTime.dwHighDateTime;
	return low + (MAXDWORD + 1) * high;
#endif
#ifdef LINUX
	if (DoStat()) {
		return static_cast<uint64_t>(m_stat.st_atime);
	else {
		return 0;
	}
#endif
}

uint64_t OpenDirEntry::CreationTime() const
{
#ifdef WIN32
	DWORD low = m_findFileData.ftCreationTime.dwLowDateTime;
	DWORD high = m_findFileData.ftCreationTime.dwHighDateTime;
	return low + (MAXDWORD + 1) * high;
#endif
#ifdef LINUX
	if (DoStat()) {
		return static_cast<uint64_t>(m_stat.st_ctime);
	else {
		return 0;
	}
#endif
}

uint64_t OpenDirEntry::ModifyTime() const
{
#ifdef WIN32
	DWORD low = m_findFileData.ftLastWriteTime.dwLowDateTime;
	DWORD high = m_findFileData.ftLastWriteTime.dwHighDateTime;
	return low + (MAXDWORD + 1) * high;
#endif
#ifdef LINUX
	if (DoStat()) {
		return static_cast<uint64_t>(m_stat.st_mtime);
	else {
		return 0;
	}
#endif
}

bool OpenDirEntry::DoStat()
{
	if (m_stated) {
		return true;
	}
#ifdef LINUX
	const std::string separator = "/";
	std::string fullpath = m_dirPath + separator + Name();
	if (stat(fullpath.c_str(), &m_stat) < 0) {
		m_statFailed = true;
		return false;
	}
	m_stated = true;
#endif
#ifdef WIN32

#endif
}








bool OpenDirEntry::Next()
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


void OpenDirEntry::Close()
{
#ifdef WIN32
	if (m_fileHandle != nullptr && m_fileHandle != INVALID_HANDLE_VALUE) {
		::FindClose(m_fileHandle);
		m_fileHandle = nullptr;
	}
#endif
}








std::optional<StatResult> Stat(const std::string& path)
{
#ifdef LINUX
	struct stat statbuff {};
	if (::stat(path.c_str(), &statbuff) < 0) {
		return std::nullopt;
	}
	return std::make_optional<StatResult>(statbuff);
#endif
#ifdef WIN32
	BY_HANDLE_FILE_INFORMATION handleFileInformation{};
	OFSTRUCT ofStruct;
	HANDLE hFile = ::CreateFile(path.c_str(), GENERIC_READ,
		FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, 0);
	if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
		return std::nullopt;
	}
	if (::GetFileInformationByHandle(hFile, &handleFileInformation) == 0) {
		::CloseHandle(hFile);
		return std::nullopt;
	}
	::CloseHandle(hFile);
	return std::make_optional<StatResult>(handleFileInformation);
#endif
}

std::optional<OpenDirEntry> OpenDir(const std::string& path)
{
#ifdef WIN32
	OpenDirEntry iterator;
	iterator.m_dirPath = path;
	iterator.m_fileHandle = ::FindFirstFile(path.c_str(), &iterator.m_findFileData);
	if (iterator.m_fileHandle == INVALID_HANDLE_VALUE) {
		return std::nullopt;
	}
	return std::make_optional(iterator);
#endif

#ifdef LINUX
	OpenDirEntry iterator;
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

}