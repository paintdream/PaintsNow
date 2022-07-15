#include "Unit.h"
#include <sstream>

using namespace PaintsNow;

String Unit::GetDescription() const {
	std::stringstream ss;
	ss << GetUnique()->GetBriefName() << "(" << std::hex << (size_t)this << ")";
	return StdToUtf8(ss.str());
}

TObject<IReflect>& Unit::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

MetaUnitIdentifier::MetaUnitIdentifier() {}

TObject<IReflect>& MetaUnitIdentifier::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	return *this;
}

bool MetaUnitIdentifier::Read(IStreamBase& streamBase, void* ptr) const {
	// TODO:
	assert(false);
	return true;
}

bool MetaUnitIdentifier::Write(IStreamBase& streamBase, const void* ptr) const {
	// TODO:
	assert(false);
	return true;
}

String MetaUnitIdentifier::GetUniqueName() const {
	return uniqueName;
}
