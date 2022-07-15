#include "ZFilterJson.h"
#define JSON_HAS_INT64
#include "Core/json.h"
#include "../../../../General/Misc/DynamicObject.h"

using namespace PaintsNow;
using namespace Json;

class FilterJsonImpl : public IStreamBase {
public:
	FilterJsonImpl(IStreamBase& streamBase);

	void Flush() override;
	bool Read(void* p, size_t& len) override;
	bool Write(const void* p, size_t& len) override;
	bool Transfer(IStreamBase& stream, size_t& len) override;
	bool WriteDummy(size_t& len) override;
	bool Seek(IStreamBase::SEEK_OPTION option, int64_t offset) override;

	// object writing/reading routine
	bool Write(IReflectObject& s, Unique unique, void* ptr, size_t length) override;
	bool Read(IReflectObject& s, Unique unique, void* ptr, size_t length) override;

protected:
	IStreamBase& stream;
};

void FilterJsonImpl::Flush() {
	stream.Flush();
}

bool FilterJsonImpl::Read(void* p, size_t& len) {
	assert(false);
	return stream.Read(p, len);
}

bool FilterJsonImpl::Write(const void* p, size_t& len) {
	assert(false);
	return stream.Write(p, len);
}

bool FilterJsonImpl::Transfer(IStreamBase& s, size_t& len) {
	assert(false);
	return stream.Transfer(s, len);
}

bool FilterJsonImpl::Seek(SEEK_OPTION option, int64_t offset) {
	assert(false);
	return stream.Seek(option, offset);
}

template <bool read>
class Exchanger : public IReflect {
public:
	Exchanger(Json::Value& v) : IReflect(true, false), root(v) {}
	// IReflect
	void OnValue(IReflectObject& s, Unique typeID, Json::Value& v, void* base) {
		singleton Unique strType = UniqueType<String>::Get();
		singleton Unique floatType = UniqueType<float>::Get();
		singleton Unique doubleType = UniqueType<double>::Get();
		singleton Unique int32Type = UniqueType<int32_t>::Get();
		singleton Unique uint32Type = UniqueType<uint32_t>::Get();
		singleton Unique int64Type = UniqueType<int64_t>::Get();
		singleton Unique uint64Type = UniqueType<uint64_t>::Get();
		singleton Unique boolType = UniqueType<bool>::Get();

		if (!s.IsBasicObject()) {
			if (s.IsIterator()) {
				IIterator& it = static_cast<IIterator&>(s);
				if (read) {
					if (v.isArray()) {
						it.Initialize(v.size());
					}
				} else {
					v = Json::Value(Json::arrayValue);
				}

				if (v.isArray()) {
					size_t index = 0;
					Unique id = it.GetElementUnique();

					if (!it.IsElementBasicObject()) {
						IReflectObject& prototype = const_cast<IReflectObject&>(it.GetElementPrototype());
						while (it.Next()) {
							OnValue(prototype, id, v[(Json::ArrayIndex)index++], it.Get());
						}
					} else {
						IReflectObject basicObject;
						while (it.Next()) {
							void* ptr = it.Get();
							OnValue(basicObject, id, v[(Json::ArrayIndex)index++], ptr);
						}
					}
				}
			} else {
				Exchanger sub(v);
				s(sub);
			}
		} else if (typeID == strType) {
			String& target = *reinterpret_cast<String*>(base);
			if (read) {
				target = v.isString() ? StdToUtf8(v.asString()) : "";
			} else {
				v = std::string(target.c_str(), target.length());
			}
		} else if (typeID == floatType) {
			float& target = *reinterpret_cast<float*>(base);
			if (read) {
				target = v.isDouble() ? (float)v.asDouble() : 0;
			} else {
				v = target;
			}
		} else if (typeID == doubleType) {
			double& target = *reinterpret_cast<double*>(base);
			if (read) {
				target = v.isDouble() ? v.asDouble() : 0;
			} else {
				v = target;
			}
		} else if (typeID == int32Type || typeID == uint32Type) {
			int32_t& target = *reinterpret_cast<int32_t*>(base);
			if (read) {
				target = v.isInt() ? v.asInt() : 0;
			} else {
				v = target;
			}
		} else if (typeID == int64Type || typeID == uint64Type) {
			int64_t& target = *reinterpret_cast<int64_t*>(base);
			if (read) {
				target = v.isInt64() ? v.asInt64() : 0;
			} else {
				v = (Json::Value::Int64)target;
			}
		} else if (typeID == boolType) {
			bool& target = *reinterpret_cast<bool*>(base);
			if (read) {
				target = v.isBool() ? v.asBool() : false;
			} else {
				v = target;
			}
		}
	}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (read && !root.isMember(name)) {
			return;
		}

		singleton Unique metaRuntime = UniqueType<MetaRuntime>::Get();
		const MetaStreamPersist* customPersist = nullptr;

		// check serialization chain
		while (meta != nullptr) {
			// runtime data can not be serialized
			if (meta->GetNode()->GetUnique() == metaRuntime) {
				return;
			} else {
				const MetaStreamPersist* persistHelper = meta->GetNode()->QueryInterface(UniqueType<MetaStreamPersist>());
				if (persistHelper != nullptr) {
					// assert(false);
					return; // not supported!
				}
			}

			meta = meta->GetNext();
		}

		Json::Value& v = root[name];
		OnValue(s, typeID, v, ptr);
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

private:
	Json::Value& root;
};

bool FilterJsonImpl::Write(IReflectObject& s, Unique unique, void* ptr, size_t length) {
	assert(!s.IsBasicObject());
	Json::Value v;
	Exchanger<false> exchanger(v);
	s(exchanger);

	stream << v.toStyledString();
	return true;
}

static void StringCreator(void* buffer) {
	new (buffer) String();
}

static void StringDeletor(void* buffer) {
	reinterpret_cast<String*>(buffer)->~String();
}

static void StringAssigner(void* dst, const void* src) {
	*reinterpret_cast<String*>(dst) = *reinterpret_cast<const String*>(src);
}

static DynamicInfo::MemController mcString = {
	StringCreator, StringDeletor, StringAssigner
};

static DynamicInfo* ParseUniqueObject(DynamicObjectWrapper& wrapper, const Value& value);
static std::pair<Unique, DynamicInfo::MemController*> ParseUniqueValue(DynamicObjectWrapper& wrapper, const Value& value) {
	std::pair<Unique, DynamicInfo::MemController*> res;
	res.second = nullptr;

	if (value.isBool()) {
		res.first = UniqueType<bool>::Get();
	} else if (value.isInt()) {
		res.first = UniqueType<int>::Get();
	} else if (value.isDouble()) {
		res.first = UniqueType<double>::Get();
	} else if (value.isString()) {
		res.first = UniqueType<String>::Get();
		res.second = &mcString;
	} else if (value.isArray()) {
		res.first = UniqueType<DynamicVector>::Get();
	} else if (value.isObject()) {
		res.first = ParseUniqueObject(wrapper, value);
	}

	return res;
}

static DynamicInfo* ParseUniqueObject(DynamicObjectWrapper& wrapper, const Value& value) {
	// enumerate attributes ...
	assert(value.isObject());
	std::vector<DynamicInfo::Field> fields;
	size_t count = 0;
	for (Value::const_iterator it = value.begin(); it != value.end(); ++it) {
		DynamicInfo::Field& field = fields[count++];

		field.name = StdToUtf8(it.name());
		std::pair<Unique, DynamicInfo::MemController*> info = ParseUniqueValue(wrapper, *it);
		field.type = info.first;
		field.controller = info.second;
		field.reflectable = !!(it->isObject() || it->isArray());
	}

	return wrapper.uniqueAllocator.AllocFromDescriptor("JSON", fields);
}

static void ParseDynamicObject(DynamicObjectWrapper& wrapper, const Value& value, DynamicObject* object);

template <class T, class F>
void ParseDynamicValue(DynamicObjectWrapper& wrapper, const Value& v, T* object, const F& field) {
	if (v.isBool()) {
		bool data = v.asBool();
		object->Set(field, &data);
	} if (v.isInt()) {
		int data = v.asInt();
		object->Set(field, &data);
	} else if (v.isDouble()) {
		double data = v.asDouble();
		object->Set(field, &data);
	} else if (v.isString()) {
		String data = v.asCString();
		object->Set(field, &data);
	} else if (v.isArray()) {
		DynamicVector* vec = reinterpret_cast<DynamicVector*>(object->Get(field));
		size_t size = v.size();
		// set by first element type
		if (size != 0) {
			const Value& f = v[0];
			std::pair<Unique, DynamicInfo::MemController*> res = ParseUniqueValue(wrapper, f);
			vec->Reinit(res.first, res.second, size, f.isArray() || f.isObject());
		}
	} else if (v.isObject()) {
		ParseDynamicObject(wrapper, v, reinterpret_cast<DynamicObject*>(object->Get(field)));
	}
}

static void ParseDynamicObject(DynamicObjectWrapper& wrapper, const Value& value, DynamicObject* object) {
	Unique unique = object->GetDynamicInfo();
	const std::vector<DynamicInfo::Field>& fields = object->GetDynamicInfo()->fields;

	size_t i = 0;
	for (Value::const_iterator it = value.begin(); it != value.end(); ++it) {
		ParseDynamicValue(wrapper, *it, object, fields[i++]);
	}
}

bool FilterJsonImpl::Read(IReflectObject& s, Unique unique, void* ptr, size_t length) {
	// check dynamic object
	assert(!s.IsBasicObject());
	String str;
	if (stream >> str) {
		Reader reader;
		Value document;
		reader.parse(str.c_str(), str.c_str() + str.length(), document, false);

		if (document.isObject()) {
			if (s.GetUnique() == UniqueType<DynamicObjectWrapper>::Get()) {
				DynamicObjectWrapper& wrapper = static_cast<DynamicObjectWrapper&>(s);
				assert(wrapper.dynamicObject == nullptr);
				Unique unique = ParseUniqueObject(wrapper, document);
				DynamicObject* object = static_cast<DynamicObject*>(unique->Create());
				ParseDynamicObject(wrapper, document, object);
				wrapper.dynamicObject = object;
			} else {
				Exchanger<true> exchanger(document);
				s(exchanger);
			}

			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

bool FilterJsonImpl::WriteDummy(size_t& len) {
	return stream.WriteDummy(len);
}

FilterJsonImpl::FilterJsonImpl(IStreamBase& streamBase) : stream(streamBase) {}

IStreamBase* ZFilterJson::CreateFilter(IStreamBase& streamBase) {
	return new FilterJsonImpl(streamBase);
}