// ZArchiveVirtual -- 7z archiver
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../Core/Interface/IType.h"
#include "../../../../Core/Interface/IArchive.h"
#include "../../../../Core/Interface/IStreamBase.h"
#include <map>
#include <string>

namespace PaintsNow {
	class ZArchiveVirtual final : public IArchive {
	public:
		ZArchiveVirtual();
		~ZArchiveVirtual() override;

		bool Exists(const String& path) const override;
		String GetFullPath(const String& path) const override;
		bool Mount(const String& basePath, IArchive* baseArchive) override;
		bool Unmount(const String& prefix, IArchive* archive) override;
		IStreamBase* Open(const String& path, bool write, uint64_t& length, uint64_t* lastModifiedTime = nullptr) override;
		void Query(const String& uri, const TWrapper<void, const String&>& wrapper) const override;
		bool IsReadOnly() const override;
		bool Delete(const String& uri) override;
		bool Notify(const void* key, const String& path, const TWrapper<void, const String&, size_t>& handler) override;

	protected:
		class MountInfo {
		public:
			String prefix;
			IArchive* archive;
		};

		std::vector<MountInfo> mountInfos;
	};
}

