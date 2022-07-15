#include "ZArchiveDirent.h"
#include "../../../../Core/Template/TAlgorithm.h"
#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#endif
#include <cstdarg>
#include <string>

using namespace PaintsNow;

class FileStream : public IStreamBase {
public:
	FileStream(FILE* p, rvalue<String> path) : fp(p), filePath(std::move(path)) {

	}
	~FileStream() override { fclose(fp);}

	IReflectObject* Clone() const override {
		FILE* fp = fopen(filePath.c_str(), "rb");
		if (fp != nullptr) {
			return new FileStream(fp, String(filePath));
		} else {
			return nullptr;
		}
	}

	size_t Printf(const char* format, ...) override {
		va_list va;
		va_start(va, format);
		int n = vfprintf(fp, format, va);
		va_end(va);

		return n < 0 ? 0 : (size_t)n;
	}

	bool Read(void* p, size_t& len) override {
		return (len = fread(p, 1, len, fp)) != 0;
	}

	bool Write(const void* p, size_t& len) override {
		return (len = fwrite(p, 1, len, fp)) != 0;
	}

	bool Seek(SEEK_OPTION option, int64_t offset) override {
		int s = SEEK_CUR;
		switch (option) {
		case BEGIN:
			s = SEEK_SET;
			break;
		case END:
			s = SEEK_END;
			break;
		case CUR:
			s = SEEK_CUR;
			break;
		}

		return fseek(fp, (long)offset, s) == 0;
	}

	bool Truncate(uint64_t length) override {
#ifdef _WIN32
		long size = verify_cast<long>(length);
		return _chsize(_fileno(fp), size) != 0;
#else
		return ftruncate(fileno(fp), length) != 0;
#endif
	}

	bool Transfer(IStreamBase& stream, size_t& len) override {
		const size_t SIZE = 512;
		char buffer[SIZE];
		size_t rl = Math::Min(SIZE, len);
		while (len > 0 && Read(buffer, rl)) {
			size_t wl = Math::Min(SIZE, len);
			stream.Write(buffer, wl);
			len -= SIZE;
			rl = Math::Min(SIZE, len);
		}

		return len == 0;
	}

	bool WriteDummy(size_t& len) override {
		return fseek(fp, (long)len, SEEK_CUR) == 0;
	}

	long GetOffset() const {
		return ftell(fp);
	}

	void Flush() override {
		fflush(fp);
	}

private:
	FILE* fp;
	String filePath;
};

String ZArchiveDirent::GetFullPath(const String& path) const {
	return root + path;
}

bool ZArchiveDirent::Mount(const String& prefix, IArchive* baseArchive) {
	return false;
}

bool ZArchiveDirent::Unmount(const String& prefix, IArchive* baseArchive) {
	return false;
}

ZArchiveDirent::ZArchiveDirent(IThread& thread, const String& r) : threadApi(thread) {
	root = r.length() == 0 ? String("./") : r[r.size() - 1] == '/' ? r : r + '/';
}

bool ZArchiveDirent::IsReadOnly() const {
	return false;
}

#ifdef _WIN32
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

bool DirectoryExists(LPCWSTR szPath) {
	DWORD dwAttrib = GetFileAttributesW(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool MakeDirectoryRecursive(const wchar_t* path) {
	wchar_t fullPath[MAX_PATH * 4];
	wcsncpy(fullPath, path, MAX_PATH * 4);
	if (!PathRemoveFileSpecW(fullPath)) return false;
	if (!DirectoryExists(fullPath)) {
		if (path[0] == '\0') return false;
		
		if (MakeDirectoryRecursive(fullPath)) {
			return ::CreateDirectoryW(fullPath, nullptr) != FALSE;
		} else {
			return false;
		}
	} else {
		return true;
	}
}

#else
bool MakeDirectoryRecursive(const String& filePath) {
	String path = filePath.substr(0, filePath.find_last_of('/'));
	int state = mkdir(path.c_str(), 0775);
	if (state == -1) {
		if (errno == ENOENT) {
			if (MakeDirectoryRecursive(path)) {
				// recreate
				state = mkdir(path.c_str(), 0775);
			}
		} else if (errno == EEXIST) {
			state = true;
		}
	} else {
		state = true;
	}

	return state;
}

#endif

bool ZArchiveDirent::MakeDirectoryForFile(const String& filePath) {
	#ifdef _WIN32
	WCHAR fullPath[MAX_PATH * 4];
	::GetFullPathNameW((wchar_t*)filePath.c_str(), MAX_PATH * 4, fullPath, nullptr);
	return MakeDirectoryRecursive(fullPath);
	#else
	return MakeDirectoryRecursive(filePath);
	#endif
}

bool ZArchiveDirent::Exists(const String& path) const {
	if (path.empty()) return false;
#ifdef _WIN32
	String wpath = Utf8ToSystem(root + path);
	if (path[path.size() - 1] == '/' || path[path.size() - 1] == '\\') {
		return DirectoryExists((LPCWSTR)wpath.c_str());
	} else {
		return GetFileAttributesW((LPCWSTR)wpath.c_str()) != 0xFFFFFFFF;
	}
#else
	return access((root + path).c_str(), F_OK) == 0;
#endif
}

IStreamBase* ZArchiveDirent::Open(const String& uri, bool write, uint64_t& length, uint64_t* lastModifiedTime) {
	String path = root + uri;

#ifdef _WIN32
	String wpath = Utf8ToSystem(path);
	if (write) {
		if (!MakeDirectoryForFile(wpath)) {
			return nullptr;
		}
	}

	FILE* fp = _wfopen((const wchar_t*)wpath.c_str(), write ? L"wb" : L"rb");
	if (fp == nullptr) {
		DWORD err = GetLastError();
		return nullptr;
	}

	int fd = _fileno(fp);
	HANDLE hFile = (HANDLE)_get_osfhandle(fd);
	if (hFile != INVALID_HANDLE_VALUE) {
		if (lastModifiedTime != nullptr) {
			FILETIME ftLastModified;
			if (::GetFileTime(hFile, nullptr, nullptr, &ftLastModified)) {
				uint64_t m = ((uint64_t)ftLastModified.dwHighDateTime << 32) + ftLastModified.dwLowDateTime;
				*lastModifiedTime = (m - 116444736000000000) / 10000000;
			}
		}
		
		DWORD high;
		DWORD low = ::GetFileSize(hFile, &high);
		
		
#ifndef _WIN64
		if (high != 0) {
			fprintf(stderr, "File too large\n");
			fclose(fp);
			return nullptr;
		}
		length = low;
#else
		length = ((size_t)high << 32) | low;
#endif
	} else {
		fprintf(stderr, "Error on getting handle of file\n");
	}

#else
	if (write) {
		if (!MakeDirectoryForFile(path)) {
			return nullptr;
		}
	}

	FILE* fp = fopen(path.c_str(), write ? "wb" : "rb");
	if (fp != nullptr) {
		struct stat buf;
		int fd = fileno(fp);
		fstat(fd, &buf);
		if ((buf.st_mode & S_IFDIR) == S_IFDIR) {
			fclose(fp);
			return nullptr;
		}
		
		if (lastModifiedTime != nullptr) {
			*lastModifiedTime = buf.st_mtime;
		}

		length = buf.st_size;
	} else {
		return nullptr;
	}
#endif

	return new FileStream(fp, std::move(path));
}

void ZArchiveDirent::Query(const String& uri, const TWrapper<void, const String&>& wrapper) const {
	String path = root + uri;

#ifdef _WIN32
	String searchPath = path + "/*.*";
	String wpath = Utf8ToSystem(searchPath);
	WIN32_FIND_DATAW findData;
	HANDLE hFind = ::FindFirstFileW((WCHAR*)wpath.data(), &findData);
	if (hFind != INVALID_HANDLE_VALUE) {
		BOOL hasNext = TRUE;
		do {
			if (findData.cFileName[0] != L'.') {
				String file = SystemToUtf8(String((const char*)findData.cFileName, wcslen(findData.cFileName) * 2));
				if (!!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					file += '/';
				}

				wrapper(file);
			}
		} while (::FindNextFileW(hFind, &findData));
		::FindClose(hFind);
	}

#else
	String wpath = Utf8ToSystem(path);
	DIR* dir = opendir(wpath.c_str());

	if (dir != nullptr) {
		dirent* dp;
		while ((dp = readdir(dir)) != nullptr) {
			if (dp->d_name[0] == '.') continue;
			// convert to locale
			String name = SystemToUtf8(String(dp->d_name));
			if (dp->d_type == DT_DIR) name += '/';

			wrapper(name);
		}

		closedir(dir);
	}
#endif
}

bool ZArchiveDirent::Delete(const String& uri) {
	String path = root + uri;
#ifdef _WIN32
	String wpath = Utf8ToSystem(path);
	return ::DeleteFileW((WCHAR*)wpath.c_str()) != 0;
#else
	return remove(path.c_str()) == 0;
#endif
}

#ifdef _WIN32

class NotifyInfo {
public:
	bool ThreadProc(IThread::Thread* thread, size_t index) {
		OVERLAPPED overlapped;
		memset(&overlapped, 0, sizeof(overlapped));
		overlapped.hEvent = event;
		const size_t BUFFER_SIZE = 4096;
		const size_t MASK = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;
		char buffer[BUFFER_SIZE];
		DWORD returned = 0;

		while (::ReadDirectoryChangesW(directoryHandle, buffer, sizeof(buffer), true, MASK, &returned, &overlapped, nullptr)) {
			DWORD object = ::WaitForSingleObject(event, INFINITE);
			if (terminated.load(std::memory_order_acquire) != 0)
				break;

			if (object == WAIT_OBJECT_0) {
				DWORD size = 0;
				::GetOverlappedResult(directoryHandle, &overlapped, &size, false);

				const char* p = buffer;
				while (p < buffer + size) {
					const FILE_NOTIFY_INFORMATION* info = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(p);
					uint32_t notify = 0;
					if (info->Action & FILE_ACTION_ADDED) {
						notify |= IArchive::CREATE;
					}

					if (info->Action & FILE_ACTION_REMOVED) {
						notify |= IArchive::REMOVE;
					}

					if (info->Action & FILE_ACTION_MODIFIED) {
						notify |= IArchive::MODIFIED;
					}

					if (info->Action & FILE_ACTION_RENAMED_OLD_NAME) {
						notify |= IArchive::MOVE_OLD;
					}

					if (info->Action & FILE_ACTION_RENAMED_NEW_NAME) {
						notify |= IArchive::MOVE_NEW;
					}

					handler(SystemToUtf8(String((const char*)info->FileName, info->FileNameLength)), notify);
					p += info->NextEntryOffset;
				}
			}
		}

		while (terminated.load(std::memory_order_acquire) == 0) {
			::WaitForSingleObject(event, INFINITE);
		}

		::CloseHandle(directoryHandle);
		::CloseHandle(event);
		delete this;

		return true; // delete self
	}

	String path;
	HANDLE directoryHandle;
	HANDLE event;
	TWrapper<void, const String&, size_t> handler;
	std::atomic<size_t> terminated;
};

#endif

bool ZArchiveDirent::Notify(const void* key, const String& path, const TWrapper<void, const String&, size_t>& handler) {
#ifdef _WIN32
	if (handler) {
		std::map<const void*, void*>::iterator it = notificationMap.find(key);
		if (it != notificationMap.end())
			return false;

		HANDLE directoryHandle = ::CreateFileW((WCHAR*)Utf8ToSystem(path).c_str(), FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

		if (directoryHandle == INVALID_HANDLE_VALUE)
			return false;

		NotifyInfo* info = new NotifyInfo();
		info->directoryHandle = directoryHandle;
		info->event = ::CreateEventA(nullptr, false, false, nullptr);
		info->path = path;
		info->handler = handler;
		info->terminated.store(0, std::memory_order_release);

		threadApi.NewThread(Wrap(info, &NotifyInfo::ThreadProc), 0);
		notificationMap[key] = info;
		
		return true;
	} else {
		std::map<const void*, void*>::iterator it = notificationMap.find(key);
		if (it != notificationMap.end()) {
			NotifyInfo* info = reinterpret_cast<NotifyInfo*>(notificationMap[key]);
			info->terminated.store(1, std::memory_order_release);
			::SetEvent(info->event);
			notificationMap[key] = nullptr;
			return true;
		} else {
			return false;
		}
	}
#else
	return false;
#endif
}

ZArchiveDirent::~ZArchiveDirent() {
#ifdef _WIN32
	if (!notificationMap.empty()) {
		for (std::map<const void*, void*>::iterator it = notificationMap.begin(); it != notificationMap.end(); ++it) {
			NotifyInfo* info = reinterpret_cast<NotifyInfo*>((*it).second);
			fprintf(stderr, "Notification not released! %p - %s\n", (*it).first, info->path.c_str());
			info->terminated.store(1, std::memory_order_release);
		}
	}
#endif
}

