// IArchive.h -- Archive format supporting
// PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#pragma once

#include "../PaintsNow.h"
#include "IDevice.h"
#include "IType.h"
#include "../Template/TProxy.h"

namespace PaintsNow {
	class IStreamBase;
	class pure_interface IArchive : public IDevice {
	public:
		~IArchive() override;

		virtual String GetFullPath(const String& path) const = 0;
		virtual bool Mount(const String& prefix, IArchive* baseArchive = nullptr) = 0;
		virtual bool Unmount(const String& prefix, IArchive* baseArchive = nullptr) = 0;
		virtual bool Exists(const String& path) const = 0;
		virtual IStreamBase* Open(const String& path, bool write, uint64_t& length, uint64_t* lastModifiedTime = nullptr) = 0;
		virtual void Query(const String& uri, const TWrapper<void, const String&>& wrapper) const = 0;
		virtual bool IsReadOnly() const = 0;
		virtual bool Delete(const String& uri) = 0;

		enum NOTIFY {
			NONE = 0,
			CREATE = 1 << 0,
			REMOVE = 1 << 1,
			MOVE_OLD = 1 << 2,
			MOVE_NEW = 1 << 3,
			MODIFIED = 1 << 4
		};

		virtual bool Notify(const void* key, const String& path, const TWrapper<void, const String&, size_t>& handler) = 0;
	};
}

