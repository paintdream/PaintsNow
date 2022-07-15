#include "Honey.h"
#include <sstream>

using namespace PaintsNow;

namespace PaintsNow {
	IScript::Request& operator << (IScript::Request& request, HoneyData& honey) {
		return request;
	}

	IScript::Request& operator >> (IScript::Request& request, HoneyData& honey) {
		honey.Attach(&request);
		return request;
	}
}

Honey::Honey(IDatabase::MetaData* data) : metaData(data) {
	assert(data != nullptr);
	(data->GetElementPrototype())(resolver); // resolve schema
}

Honey::~Honey() {
	metaData->Destroy();
}

bool Honey::Step() {
	return metaData->Next();
}

TObject<IReflect>& Honey::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

void Honey::WriteLine(IScript::Request& request) {
	request << beginarray;
	for (size_t j = 0; j < resolver.setters.size(); j++) {
		std::pair<SchemaResolver::Set, size_t>& setter = resolver.setters[j];
		setter.first(request, const_cast<char*>(reinterpret_cast<const char*>(metaData->Get())) + setter.second);
	}
	request << endarray;
}

HoneyData::HoneyData() : index(0), count(0), dynamicObject(nullptr), request(nullptr) {

}

const String& HoneyData::GetInternalName() const {
	static String nullName;
	return nullName;
}

HoneyData::~HoneyData() {
	if (request != nullptr) {
		request->Dereference(tableRef);
	}

	if (dynamicObject != nullptr) {
		dynamicObject->Destroy();
	}
}

bool HoneyData::IsLayoutLinear() const {
	return false;
}

void* HoneyData::GetHost() const {
	return nullptr;
}

Unique HoneyData::GetElementUnique() const {
	assert(false);
	return Unique();
}

Unique HoneyData::GetElementReferenceUnique() const {
	assert(false);
	return Unique();
}

IIterator* HoneyData::New() const {
	assert(false);
	return nullptr;
}

SchemaResolver::SchemaResolver() : IReflect(true, false) {}
void SchemaResolver::Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
	size_t offset = (const char*)ptr - (const char*)base;
	if (typeID == UniqueType<int>::Get()) {
		setters.emplace_back(std::make_pair(&SchemaResolver::SetValueInt, offset));
	} else if (typeID == UniqueType<String>::Get()) {
		setters.emplace_back(std::make_pair(&SchemaResolver::SetValueString, offset));
	} else if (typeID == UniqueType<const char*>::Get()) {
		setters.emplace_back(std::make_pair(&SchemaResolver::SetValueText, offset));
	} else if (typeID == UniqueType<double>::Get()) {
		setters.emplace_back(std::make_pair(&SchemaResolver::SetValueFloat, offset));
	} else if (typeID == UniqueType<Void>::Get()) {
		setters.emplace_back(std::make_pair(&SchemaResolver::SetValueNull, offset));
	} else {
		assert(false);
	}
}

void SchemaResolver::SetValueString(IScript::Request& request, char* base) {
	request << *((String*)base);
}

void SchemaResolver::SetValueText(IScript::Request& request, char* base) {
	request << String(*(const char**)base);
}

void SchemaResolver::SetValueFloat(IScript::Request& request, char* base) {
	request << *(double*)base;
}

void SchemaResolver::SetValueInt(IScript::Request& request, char* base) {
	request << *(int*)base;
}

void SchemaResolver::SetValueNull(IScript::Request& request, char* base) {
	request << nil;
}

void SchemaResolver::Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}

static void StringCreator(void* buffer) {
	new (buffer) String();
}

static void StringDeletor(void* buffer) {
	reinterpret_cast<String*>(buffer)->~String();
}

static void StringAssigner(void* dst, const void* src) {
	*reinterpret_cast<String*>(dst) = *reinterpret_cast<const String*>(src);
}

void HoneyData::Attach(void* base) {
	assert(request == nullptr);
	this->request = reinterpret_cast<IScript::Request*>(base);
	IScript::Request& request = *this->request;
	request >> tableRef;

	request.Push();
	request << tableRef;
	IScript::Request::TableStart ts;
	ts.count = 0;
	request >> ts;

	// get schema
	if (ts.count != 0) {
		IScript::Request::TableStart ds;
		ds.count = 0;
		request >> ds;
		std::vector<DynamicInfo::Field> fields(ds.count);

		for (size_t i = 0; i < ds.count; i++) {
			std::stringstream ss;
			ss << "_" << i;
			String key = StdToUtf8(ss.str());
			DynamicInfo::Field& field = fields[i];
			field.name = key;
			static DynamicInfo::MemController mc = {
				StringCreator, StringDeletor, StringAssigner
			};

			int intValue;
			double doubleValue;
			String strValue;
			switch (request.GetCurrentType()) {
				case IScript::Request::INTEGER:
					field.type = UniqueType<int>::Get();
					sets.emplace_back(&HoneyData::SetInteger);
					request >> intValue;
					break;
				case IScript::Request::NUMBER:
					field.type = UniqueType<double>::Get();
					sets.emplace_back(&HoneyData::SetFloat);
					request >> doubleValue;
					break;
				case IScript::Request::STRING:
				default:
					field.type = UniqueType<String>::Get();
					field.controller = &mc;
					sets.emplace_back(&HoneyData::SetString);
					request >> strValue;
					break;
			}
		}
		request << endtable;

		DynamicInfo* info = uniqueAllocator.AllocFromDescriptor("HoneyDataInstance", fields);
		dynamicObject = static_cast<DynamicObject*>(info->Create());
	}

	request << endtable;
	request.Pop();
	count = ts.count;
	index = 0;
}

void HoneyData::SetFloat(size_t i) {
	double db;
	*request >> db;
	dynamicObject->Set(dynamicObject->GetDynamicInfo()->fields[i], &db);
}

void HoneyData::SetString(size_t i) {
	String str;
	*request >> str;
	dynamicObject->Set(dynamicObject->GetDynamicInfo()->fields[i], &str);
}

void HoneyData::SetInteger(size_t i) {
	int v;
	*request >> v;
	dynamicObject->Set(dynamicObject->GetDynamicInfo()->fields[i], &v);
}

void HoneyData::Initialize(size_t count) {
	assert(false);
}

size_t HoneyData::GetTotalCount() const {
	return count;
}

void* HoneyData::Get() {
	return dynamicObject;
}

bool HoneyData::IsElementBasicObject() const {
	return false;
}

const IReflectObject& HoneyData::GetElementPrototype() const {
	return *dynamicObject;
}

bool HoneyData::IsLayoutPinned() const {
	return true;
}

void HoneyData::Enter() {
	request->DoLock();
	request->Push();
	*request << tableRef;
	IScript::Request::TableStart ts;
	ts.count = 0;
	*request >> ts;
	assert(ts.count == count);
}

void HoneyData::Leave() {
	*request << endtable;
	request->Pop();
	request->UnLock();
}

bool HoneyData::Next() {
	if (index < count) {
		*request >> begintable;
		for (size_t n = 0; n < sets.size(); n++) {
			(this->*sets[n])(n);
		}
		*request << endtable;
		index++;
		return true;
	} else {
		return false;
	}
}
