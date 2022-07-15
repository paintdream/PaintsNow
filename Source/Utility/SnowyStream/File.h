// File.h
// PaintDream (paintdream@paintdream.com)
// 2015-1-7
//

#pragma once
#include "../../Core/System/Kernel.h"

namespace PaintsNow {
	class File : public TReflected<File, WarpTiny> {
	public:
		File(IStreamBase* stream, size_t fileSize, uint64_t lastModifiedTime);
		~File() override;

		TObject<IReflect>& operator () (IReflect& reflect) override;
		IStreamBase* GetStream() const;
		IStreamBase* Detach();
		size_t GetLength() const;
		uint64_t GetLastModifiedTime() const;
		void Close();

	private:
		IStreamBase* stream;
		uint64_t lastModifiedTime;
		size_t length;
	};
}

