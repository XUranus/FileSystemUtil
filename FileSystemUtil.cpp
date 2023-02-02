#include "FileSystemUtil.h"

#ifdef WIN32
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1
#include <locale>
#include <codecvt>
#endif

using namespace std;

namespace {
#ifdef WIN32
constexpr auto WIN32_UID = 0;
constexpr auto WIN32_GID = 0;
constexpr auto VOLUME_BUFFER_MAX_LEN = MAX_PATH;
constexpr auto VOLUME_PATH_MAX_LEN = MAX_PATH + 1;
constexpr auto DEVICE_BUFFER_MAX_LEN = MAX_PATH;
#endif
}

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
#endif



#ifdef __linux__
StatResult::StatResult(const struct stat& statbuff)
{
	memcpy(&m_stat, &statbuff, sizeof(struct stat));
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
uint64_t StatResult::Attribute() const { return m_handleFileInformation.dwFileAttributes; }
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
	return std::make_optional<StatResult>(statbuff);
#endif
#ifdef WIN32
	BY_HANDLE_FILE_INFORMATION handleFileInformation{};
	std::wstring wpath = Utf8ToUtf16(path);
	HANDLE hFile = ::CreateFileW(wpath.c_str(), GENERIC_READ,
		FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
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




#ifdef WIN32
OpenDirEntry::OpenDirEntry(const std::string& dirPath, const WIN32_FIND_DATAW& findFileData, const HANDLE& fileHandle)
	:m_dirPath(Utf8ToUtf16(dirPath)), m_findFileData(findFileData), m_fileHandle(fileHandle) {}

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
	std::wstring wpathPattern = Utf8ToUtf16(path);
	if (!wpathPattern.empty() && wpathPattern.back() != L'\\') {
		wpathPattern.push_back(L'\\');
	}
	wpathPattern += L"*.*";
	WIN32_FIND_DATAW findFileData{};
	HANDLE fileHandle = ::FindFirstFileW(wpathPattern.c_str(), &findFileData);
	if (fileHandle == INVALID_HANDLE_VALUE || fileHandle == nullptr) {
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

#ifdef WIN32
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


#endif

}