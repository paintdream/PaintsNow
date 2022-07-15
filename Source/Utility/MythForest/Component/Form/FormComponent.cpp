#include "FormComponent.h"

using namespace PaintsNow;

FormComponent::FormComponent(const String& n) : name(n) {}

TShared<SharedTiny> FormComponent::GetCookie(void* key) const {
	std::vector<KeyValue<void*, TShared<SharedTiny> > >::const_iterator it = BinaryFind(cookies.begin(), cookies.end(), key);
	return it == cookies.end() ? TShared<SharedTiny>(nullptr) : it->second;
}

void FormComponent::SetCookie(void* key, const TShared<SharedTiny>& tiny) {
	if (tiny) {
		BinaryInsert(cookies, MakeKeyValue(key, tiny));
	} else {
		BinaryErase(cookies, key);
	}
}

void FormComponent::ClearCookies() {
	cookies.clear();
}

void FormComponent::SetName(const String& n) {
	name = n;
}

void FormComponent::SetName(rvalue<String> n) {
	name = std::move(n);
}

const String& FormComponent::GetName() const {
	return name;
}

std::vector<String>& FormComponent::GetValues() {
	return values;
}

const std::vector<String>& FormComponent::GetValues() const {
	return values;
}


