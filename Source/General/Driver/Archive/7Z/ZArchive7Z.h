// ZArchive7Z -- 7z archiver
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../Core/Interface/IType.h"
#include "../../../../Core/Interface/IArchive.h"
#include "../../../../Core/Interface/IStreamBase.h"
#include <map>
#include <string>

extern "C" {
	#include "../../Filter/LZMA/Core/7z.h"
	#include "../../Filter/LZMA/Core/7zFile.h"
	#include "../../Filter/LZMA/Core/7zCrc.h"
	#include "../../Filter/LZMA/Core/7zAlloc.h"
	#include "../../Filter/LZMA/Core/7zBuf.h"
}

namespace PaintsNow {
	class ZArchive7Z final : public IArchive {
	public:
		ZArchive7Z(IStreamBase& stream, size_t len);
		~ZArchive7Z() override;

		bool Exists(const String& path) const override;
		String GetFullPath(const String& path) const override;
		bool Mount(const String& prefix, IArchive* baseArchive) override;
		bool Unmount(const String& prefix, IArchive* baseArchive) override;
		IStreamBase* Open(const String& uri, bool write, uint64_t& length, uint64_t* lastModifiedTime = nullptr) override;
		void Query(const String& uri, const TWrapper<void, const String&>& wrapper) const override;
		bool IsReadOnly() const override;
		bool Delete(const String& uri) override;
		bool Notify(const void* key, const String& path, const TWrapper<void, const String&, size_t>& handler) override;

		static int main(int argc, char* argv[]);

		IStreamBase& GetStream();
		int64_t GetPos() const;
		void SetPos(int64_t s);
		int64_t GetLength() const;

	private:
		bool Open();

	private:
		IStreamBase& stream;
		CFileInStream archiveStream;
		CLookToRead lookStream;
		CSzArEx db;
		ISzAlloc allocImp;
		ISzAlloc allocTempImp;
		int64_t pos;
		int64_t size;
		bool opened;

		std::unordered_map<String, uint32_t> mapPathToID;
	};
}

