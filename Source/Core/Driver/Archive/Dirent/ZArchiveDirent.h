// ZArchiveDirent -- Basic file system management by dirent and standard c interfaces.
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../Interface/IArchive.h"
#include "../../../Interface/IStreamBase.h"

namespace PaintsNow {
	class ZArchiveDirent final : public IArchive {
	public:
		ZArchiveDirent(IThread& threadApi, const String& root = "./");
		~ZArchiveDirent() override;

		String GetFullPath(const String& path) const override;
		bool Exists(const String& path) const override;
		bool Mount(const String& prefix, IArchive* baseArchive) override;
		bool Unmount(const String& prefix, IArchive* baseArchive) override;

		IStreamBase* Open(const String& path, bool write, uint64_t& length, uint64_t* lastModifiedTime = nullptr) override;
		void Query(const String& uri, const TWrapper<void, const String&>& wrapper) const override;
		bool IsReadOnly() const override;
		bool Delete(const String& uri) override;
		bool Notify(const void* key, const String& path, const TWrapper<void, const String&, size_t>& handler) override;

	private:
		bool MakeDirectoryForFile(const String& path);

	private:
		IThread& threadApi;
		String root;
		std::map<const void*, void*> notificationMap;
	};
}

