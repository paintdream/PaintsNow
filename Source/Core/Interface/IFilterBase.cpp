#include "IFilterBase.h"

using namespace PaintsNow;

IFilterBase::~IFilterBase() {}

class NoFilterImpl : public IStreamBase {
public:
	NoFilterImpl(IStreamBase& streamBase);
	void Flush() override;
	bool Read(void* p, size_t& len) override;
	bool Write(const void* p, size_t& len) override;
	bool Transfer(IStreamBase& stream, size_t& len) override;
	bool WriteDummy(size_t& len) override;
	bool Seek(SEEK_OPTION option, int64_t offset) override;

private:
	IStreamBase& stream;
};

NoFilterImpl::NoFilterImpl(IStreamBase& streamBase) : stream(streamBase) {}

bool NoFilterImpl::Read(void* p, size_t& len) {
	return stream.Read(p, len);
}

bool NoFilterImpl::Write(const void* p, size_t& len) {
	return stream.Write(p, len);
}

bool NoFilterImpl::Transfer(IStreamBase& s, size_t& len) {
	return stream.Transfer(s, len);
}

bool NoFilterImpl::WriteDummy(size_t& len) {
	return stream.WriteDummy(len);
}

bool NoFilterImpl::Seek(SEEK_OPTION option, int64_t offset) {
	return stream.Seek(option, offset);
}

void NoFilterImpl::Flush() {
	stream.Flush();
}

IStreamBase* NoFilter::CreateFilter(IStreamBase& stream) {
	return new NoFilterImpl(stream);
}