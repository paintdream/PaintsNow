// Mount.h
// PaintDream (paintdream@paintdream.com)
// 2018-8-4
//

#pragma once
#include "../../Core/System/Kernel.h"
#include "../../Core/Interface/IArchive.h"
#include "File.h"

namespace PaintsNow {
	class Mount : public TReflected<Mount, WarpTiny> {
	public:
		Mount(IArchive& hostArchive, const String& mountPoint, IArchive* archive, const TShared<File>& file);
		~Mount() override;

		void Unmount();

	protected:
		IArchive& hostArchive;
		IArchive* archive;
		String mountPoint;
		TShared<File> file;
	};
}

