#include "Mount.h"
#include "../../Core/Interface/IStreamBase.h"

using namespace PaintsNow;

Mount::Mount(IArchive& h, const String& pt, IArchive* a, const TShared<File>& f) : hostArchive(h), archive(a), mountPoint(pt), file(f) {
	hostArchive.Mount(mountPoint, archive);
}

Mount::~Mount() {
	Unmount();
}

void Mount::Unmount() {
	if (archive != nullptr) {
		hostArchive.Unmount(mountPoint, archive);
		archive->ReleaseDevice();
		archive = nullptr;
		file = nullptr;
	}
}