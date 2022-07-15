#include "DynamicObject.h"
#include <sstream>
using namespace PaintsNow;

DynamicUniqueAllocator::DynamicUniqueAllocator() {}

DynamicInfo::Field::Field() : controller(nullptr) {
	reflectable = 0;
	offset = 0;
}

IReflectObject* DynamicInfo::Create() const {
	assert(Math::Alignment(sizeof(DynamicObject)) >= sizeof(size_t));

	size_t s = sizeof(DynamicObject) + size;
	char* buffer = new char[s];
	memset(buffer, 0, s);
	DynamicObject* proxy = new (buffer) DynamicObject(const_cast<DynamicInfo*>(this));
	return proxy;
}

const DynamicInfo::Field* DynamicInfo::operator [] (const String& key) const {
	std::map<String, uint32_t>::const_iterator it = mapNameToField.find(key);
	return it == mapNameToField.end() ? (DynamicInfo::Field*)nullptr : &fields[it->second];
}

DynamicObject::DynamicObject(DynamicInfo* info) : dynamicInfo(info) {
	std::vector<DynamicInfo::Field>& fields = dynamicInfo->fields;
	char* base = (char*)this + sizeof(*this);
	for (size_t i = 0; i < fields.size(); i++) {
		DynamicInfo::Field& field = fields[i];
		char* ptr = base + field.offset;
		if (field.type->GetAllocator() == dynamicInfo->GetAllocator()) {
			// dynamic-created class?
			assert(field.controller == nullptr);
			new (ptr) DynamicObject(dynamicInfo);
		} else if (field.controller != nullptr) {
			field.controller->Creator(ptr);
		}
	}
}

DynamicObject::~DynamicObject() {
	std::vector<DynamicInfo::Field>& fields = dynamicInfo->fields;
	char* base = (char*)this + sizeof(*this);
	for (size_t i = 0; i < fields.size(); i++) {
		DynamicInfo::Field& field = fields[i];
		char* ptr = base + field.offset;
		if (field.type->GetAllocator() == dynamicInfo->GetAllocator()) {
			assert(field.controller == nullptr);
			DynamicObject* proxy = reinterpret_cast<DynamicObject*>(ptr);
			proxy->~DynamicObject();
		} else if (field.controller != nullptr) {
			field.controller->Deletor(ptr);
		}
	}

#ifdef _DEBUG
	dynamicInfo = nullptr;
#endif
}

void DynamicObject::Destroy() {
	// call destructor manually
	this->~DynamicObject();
	delete[] (char*)this;
}

DynamicVector::Iterator::Iterator(DynamicVector* vec) : base(vec), i(0) {}

void DynamicVector::Iterator::Initialize(size_t c) {
	i = 0;
	base->Reinit(base->unique, base->memController, c, base->reflectable);
}

size_t DynamicVector::Iterator::GetTotalCount() const {
	return base->count;
}

bool DynamicVector::Iterator::IsElementBasicObject() const {
	return true;
}

Unique DynamicVector::Iterator::GetElementUnique() const {
	return base->unique;
}

Unique DynamicVector::Iterator::GetElementReferenceUnique() const {
	return base->unique;
}

static IReflectObject dummyObject((int)0);

const IReflectObject& DynamicVector::Iterator::GetElementPrototype() const {
	return base->count == 0 || !base->reflectable ? dummyObject : *reinterpret_cast<const IReflectObject*>(base->buffer);
}

void* DynamicVector::Iterator::Get() {
	return (char*)base->buffer + (i - 1) * base->unique->GetSize();
}

bool DynamicVector::Iterator::Next() {
	if (i >= base->count) {
		return false;
	}

	i++;
	return true;
}

IIterator* DynamicVector::Iterator::New() const {
	return new DynamicVector::Iterator(base);
}

void DynamicVector::Iterator::Attach(void* p) {
	base = reinterpret_cast<DynamicVector*>(p);
	i = 0;
}

bool DynamicVector::Iterator::IsLayoutLinear() const {
	return true;
}

bool DynamicVector::Iterator::IsLayoutPinned() const {
	return false;
}

void* DynamicVector::Iterator::GetHost() const {
	return base;
}

TObject<IReflect>& DynamicObject::operator () (IReflect& reflect) {
	reflect.Class(*this, dynamicInfo, dynamicInfo->GetName().c_str(), "PaintsNow", nullptr);

	if (reflect.IsReflectProperty()) {
		char* base = (char*)this + sizeof(*this);
		std::vector<DynamicInfo::Field>& fields = dynamicInfo->fields;
		for (size_t i = 0; i < fields.size(); i++) {
			DynamicInfo::Field& field = fields[i];
			char* ptr = base + field.offset;
			IReflectObject* reflectObject = reinterpret_cast<IReflectObject*>(ptr);
			if (field.type == UniqueType<DynamicVector>::Get()) {
				DynamicVector::Iterator iterator(static_cast<DynamicVector*>(reflectObject));
				reflect.Property(iterator, field.type, field.type, field.name.c_str(), (char*)this, ptr, nullptr);
			} else {
				reflect.Property(field.reflectable ? dummyObject : *reflectObject, field.type, field.type, field.name.c_str(), (char*)(this), ptr, nullptr);
			}
		}
	}

	return *this;
}

DynamicInfo* DynamicUniqueAllocator::Create(const String& name, size_t size) {
	assert(false);
	return nullptr;
}

DynamicInfo* DynamicUniqueAllocator::Get(const String& name) {
	std::map<String, DynamicInfo>::iterator it = mapType.find(name);
	return it == mapType.end() ? nullptr : &it->second;
}

DynamicInfo* DynamicUniqueAllocator::AllocFromDescriptor(const String& name, const std::vector<DynamicInfo::Field>& descriptors) {
	// Compose new name
	String desc;
	String allNames;
	size_t maxSize = (size_t)-1;
	std::vector<DynamicInfo::Field> fields = descriptors;
	size_t lastOffset = 0;
	size_t maxAlignment = sizeof(DynamicObject);

	for (size_t k = 0; k < fields.size(); k++) {
		if (k != 0) desc += ", ";
		DynamicInfo::Field& p = fields[k];
		Unique info = p.type;
		maxSize = Math::Min(maxSize, info->GetSize());

		desc += p.name + ": " + info->GetName();
		allNames += info->GetName();
		size_t s = Math::Alignment(info->GetSize());
		maxAlignment = Math::Max(maxAlignment, s);
		while (lastOffset != 0 && Math::Alignment(lastOffset) < s) {
			lastOffset += Math::Alignment(lastOffset);
		}

		p.offset = lastOffset;
		lastOffset += info->GetSize();
	}

	std::stringstream ss;
	ss << name.c_str() << "{" << descriptors.size() << "-" << (size_t)HashBuffer(allNames.c_str(), allNames.size()) << "-" << (size_t)HashBuffer(desc.c_str(), desc.size()) << "}";

	String newName = StdToUtf8(ss.str());
	std::map<String, DynamicInfo>::iterator it = mapType.find(newName);
	if (it != mapType.end()) {
		// assert(false); // Performance warning: should check it before calling me!
		return &it->second;
	} else {
		DynamicInfo& info = mapType[newName];
		info.SetAllocator(this);
		info.SetName(newName);
		std::sort(fields.begin(), fields.end());
		for (size_t i = 0; i < fields.size(); i++) {
			info.mapNameToField[fields[i].name] = verify_cast<uint32_t>(i);
		}

		while (lastOffset != 0 && Math::Alignment(lastOffset) < maxAlignment) {
			lastOffset += Math::Alignment(lastOffset);
		}

		info.SetSize(fields.empty() ? sizeof(size_t) : lastOffset);
		std::swap(info.fields, fields);

		return &info;
	}
}

DynamicInfo* DynamicObject::GetDynamicInfo() const {
	return dynamicInfo;
}

DynamicObject& DynamicObject::operator = (const DynamicObject& rhs) {
	if (dynamicInfo == rhs.dynamicInfo && this != &rhs) {
		std::vector<DynamicInfo::Field>& fields = dynamicInfo->fields;
		const char* rbase = (const char*)&rhs + sizeof(*this);
		for (size_t i = 0; i < fields.size(); i++) {
			DynamicInfo::Field& field = fields[i];
			Set(field, rbase);
		}
	}

	return *this;
}

void DynamicObject::Set(const DynamicInfo::Field& field, const void* value) {
	char* buffer = (char*)this + sizeof(*this) + field.offset;
	if (buffer == value) return;

	if (field.type->GetAllocator() == dynamicInfo->GetAllocator()) {
		const DynamicObject* src = reinterpret_cast<const DynamicObject*>(value);
		DynamicObject* dst = reinterpret_cast<DynamicObject*>(buffer);
		*dst = *src;
	} else if (field.controller != nullptr) {
		field.controller->Assigner(buffer, value);
	} else {
		memcpy(buffer, value, field.type->GetSize());
	}
}

void* DynamicObject::Get(const DynamicInfo::Field& field) const {
	return (char*)this + sizeof(*this) + field.offset;
}

DynamicObjectWrapper::DynamicObjectWrapper(DynamicUniqueAllocator& allocator) : uniqueAllocator(allocator), dynamicObject(nullptr) {}

DynamicObjectWrapper::~DynamicObjectWrapper() {
	if (dynamicObject != nullptr) {
		dynamicObject->Destroy();
	}
}

TObject<IReflect>& DynamicObjectWrapper::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	return *this;
}

DynamicVector::DynamicVector(Unique type, DynamicInfo::MemController* mc, size_t c, bool r) : unique(type), memController(mc), reflectable(r), count(verify_cast<uint32_t>(c)), buffer(nullptr) {
	Init();
}

void DynamicVector::Init() {
	size_t size = unique->GetSize();
	if (count != 0) {
		assert(buffer == nullptr);
		buffer = new char[size * count];

		// initialize them
		if (memController != nullptr) {
			char* base = reinterpret_cast<char*>(buffer);
			for (size_t k = 0; k < count; k++) {
				memController->Creator(base + k * size);
			}
		}
	}
}

DynamicVector::~DynamicVector() {
	Cleanup();
}

void DynamicVector::Reinit(Unique u, DynamicInfo::MemController* mc, size_t n, bool r) {
	Cleanup();
	reflectable = r;
	unique = u;
	memController = mc;	
	count = n;
	Init();
}

void DynamicVector::Set(size_t i , const void* value) {
	memController->Assigner(reinterpret_cast<char*>(buffer) + i * unique->GetSize(), value);
}

void* DynamicVector::Get(size_t i) const {
	return reinterpret_cast<char*>(buffer) + i * unique->GetSize();
}

void DynamicVector::Cleanup() {
	if (buffer != nullptr) {
		if (memController != nullptr) {
			size_t size = unique->GetSize();
			char* base = reinterpret_cast<char*>(buffer);
			for (size_t k = 0; k < count; k++) {
				memController->Deletor(base + k * size);
			}
		}

		delete[] reinterpret_cast<char*>(buffer);
		buffer = nullptr;
	}
}

DynamicVector& DynamicVector::operator = (const DynamicVector& vec) {
	if (this != &vec) {
		Cleanup();
		// copy info
		assert(buffer == nullptr);
		unique = vec.unique;
		memController = vec.memController;
		count = vec.count;

		size_t size = unique->GetSize();
		if (count != 0) {
			buffer = new char[size * count];

			// initialize them
			if (memController != nullptr) {
				char* base = reinterpret_cast<char*>(buffer);
				const char* src = reinterpret_cast<const char*>(vec.buffer);
				for (size_t k = 0; k < count; k++) {
					memController->Creator(base + k * size);
					memController->Assigner(base + k * size, src + k * size);
				}
			}
		}
	}

	return *this;
}

void DynamicVector::VectorCreator(void* buffer) {
	new (buffer) DynamicVector(UniqueType<int>::Get(), nullptr, 0, false);
}

void DynamicVector::VectorDeletor(void* buffer) {
	reinterpret_cast<DynamicVector*>(buffer)->~DynamicVector();
}

void DynamicVector::VectorAssigner(void* dst, const void* src) {
	*reinterpret_cast<DynamicVector*>(dst) = *reinterpret_cast<const DynamicVector*>(src);
}

DynamicInfo::MemController& DynamicVector::GetVectorController() {
	static DynamicInfo::MemController controller = {
		&DynamicVector::VectorCreator,
		&DynamicVector::VectorDeletor,
		&DynamicVector::VectorAssigner,
	};

	return controller;
}