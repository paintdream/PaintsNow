#include "File.h"
#include "../../Core/Interface/IStreamBase.h"

using namespace PaintsNow;

File::File(IStreamBase* s, size_t len, uint64_t modTime) : stream(s), length(len), lastModifiedTime(modTime) {}

File::~File() {
	if (stream != nullptr) {
		Close();
	}
}

TObject<IReflect>& File::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	return *this;
}

IStreamBase* File::GetStream() const {
	return stream;
}

size_t File::GetLength() const {
	return length;
}

uint64_t File::GetLastModifiedTime() const {
	return lastModifiedTime;
}

void File::Close() {
	if (stream != nullptr) {
		stream->Destroy();
		stream = nullptr;
	}
}

IStreamBase* File::Detach() {
	IStreamBase* s = stream;
	stream = nullptr;
	return s;
}