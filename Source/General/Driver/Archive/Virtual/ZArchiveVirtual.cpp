#include "ZArchiveVirtual.h"
#include "../../../../Core/System/MemoryStream.h"

using namespace PaintsNow;

bool ZArchiveVirtual::Exists(const String& uri) const {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		const MountInfo& info = mountInfos[i];
		if (info.prefix.length() == 0 || info.prefix.compare(0, info.prefix.length(), uri) == 0) {
			if (info.archive->Exists(uri)) {
				return true;
			}
		}
	}

	return false;
}

String ZArchiveVirtual::GetFullPath(const String& uri) const {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		const MountInfo& info = mountInfos[i];
		if (info.prefix.length() == 0 || info.prefix.compare(0, info.prefix.length(), uri) == 0) {
			String path = info.archive->GetFullPath(uri);
			if (!path.empty()) {
				return path;
			}
		}
	}

	return String();
}

bool ZArchiveVirtual::Mount(const String& prefix, IArchive* archive) {
	assert(archive != nullptr);
	// check if exists?
	// allow multiple mounts!
	/*
	for (size_t i = 0; i < mountInfos.size(); i++) {
		MountInfo& info = mountInfos[i];
		if (info.prefix == prefix) {
			info.archive = archive;

			// always success
			return true;
		}
	}*/

	MountInfo info;
	info.archive = archive;
	info.prefix = prefix;

	mountInfos.emplace_back(std::move(info));
	return true;
}

bool ZArchiveVirtual::Unmount(const String& prefix, IArchive* archive) {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		MountInfo& info = mountInfos[i];
		if (info.archive == archive && info.prefix == prefix) {
			mountInfos.erase(mountInfos.begin() + i);
			return true;
		}
	}

	return false;
}

ZArchiveVirtual::ZArchiveVirtual() {}

ZArchiveVirtual::~ZArchiveVirtual() {}

IStreamBase* ZArchiveVirtual::Open(const String& uri, bool write, uint64_t& length, uint64_t* lastModifiedTime) {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		MountInfo& info = mountInfos[i];
		if (info.prefix.length() == 0 || info.prefix.compare(0, info.prefix.length(), uri) == 0) {
			IStreamBase* stream = info.archive->Open(uri.substr(info.prefix.length()), write, length, lastModifiedTime);
			if (stream != nullptr) {
				return stream;
			}
		}
	}

	return nullptr;
}

void ZArchiveVirtual::Query(const String& uri, const TWrapper<void, const String&>& wrapper) const {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		const MountInfo& info = mountInfos[i];
		if (info.prefix.length() == 0 || info.prefix.compare(0, info.prefix.length(), uri) == 0) {
			info.archive->Query(uri.substr(info.prefix.length()), wrapper);
		}
	}
}

bool ZArchiveVirtual::IsReadOnly() const {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		const MountInfo& info = mountInfos[i];
		if (!info.archive->IsReadOnly())
			return false;
	}

	return true;
}

bool ZArchiveVirtual::Delete(const String& uri) {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		const MountInfo& info = mountInfos[i];
		if (info.prefix.compare(0, info.prefix.length(), uri) == 0) {
			if (info.archive->Delete(uri.substr(info.prefix.length())))
				return true;
		}
	}

	return false;
}

bool ZArchiveVirtual::Notify(const void* key, const String& path, const TWrapper<void, const String&, size_t>& handler) {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		const MountInfo& info = mountInfos[i];
		if (info.prefix.compare(0, info.prefix.length(), path) == 0) {
			if (info.archive->Notify(key, path.substr(info.prefix.length()), handler))
				return true;
		}
	}

	return false;
}

