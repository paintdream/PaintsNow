#include "IWidget.h"
#include "../../../../Utility/LeavesFlute/LeavesFlute.h"
#include "Core/imgui.h"
#include <sstream>

using namespace PaintsNow;

static void FromString(const void* ptr) {
	const String& str = *reinterpret_cast<const String*>(ptr);
	ImGui::Text("\"%s\"", str.c_str());
}

template <class T>
static void PrimitiveString(const void* ptr) {
	const T& value = *reinterpret_cast<const T*>(ptr);
	std::stringstream ss;
	ss << value;
	ImGui::Text("%s", StdToUtf8(ss.str()).c_str());
}

template <class T>
static void VectorString(const void* ptr) {
	const size_t n = T::size;
	const T& vec = *reinterpret_cast<const T*>(ptr);
	std::stringstream ss;
	for (size_t i = 0; i < n; i++) {
		if (i != 0) ss << ", ";
		ss << vec[i];
	}

	if (n == 0) {
		ImGui::Text("%s", StdToUtf8(ss.str()).c_str());
	} else {
		ImGui::Text("(%s)", StdToUtf8(ss.str()).c_str());
	}
}

template <class T>
void Printer::RegisterVectorType() {
	types[UniqueType<T>::Get()] = &VectorString<T>;
}

template <class T>
void Printer::RegisterPrimitiveType() {
	types[UniqueType<T>::Get()] = &PrimitiveString<T>;
}

Printer::Printer() {
	types[UniqueType<String>::Get()] = &FromString;
	RegisterPrimitiveType<bool>();
	RegisterPrimitiveType<void*>();

	RegisterPrimitiveType<float>();
	RegisterVectorType<Float2>();
	RegisterVectorType<Float3>();
	RegisterVectorType<Float4>();

	RegisterPrimitiveType<double>();
	RegisterVectorType<Double2>();
	RegisterVectorType<Double3>();
	RegisterVectorType<Double4>();

	RegisterPrimitiveType<int8_t>();
	RegisterVectorType<Char2>();
	RegisterVectorType<Char3>();
	RegisterVectorType<Char4>();

	RegisterPrimitiveType<uint8_t>();
	RegisterVectorType<UChar2>();
	RegisterVectorType<UChar3>();
	RegisterVectorType<UChar4>();

	RegisterPrimitiveType<int16_t>();
	RegisterVectorType<Short2>();
	RegisterVectorType<Short3>();
	RegisterVectorType<Short4>();

	RegisterPrimitiveType<uint16_t>();
	RegisterVectorType<UShort2>();
	RegisterVectorType<UShort3>();
	RegisterVectorType<UShort4>();

	RegisterPrimitiveType<int32_t>();
	RegisterVectorType<Int2>();
	RegisterVectorType<Int3>();
	RegisterVectorType<Int4>();

	RegisterPrimitiveType<uint32_t>();
	RegisterVectorType<UInt2>();
	RegisterVectorType<UInt3>();
	RegisterVectorType<UInt4>();

	RegisterPrimitiveType<int64_t>();
	RegisterPrimitiveType<uint64_t>();
}

void Printer::AddEnum(Unique unique, size_t value, const char* name) {
	std::vector<String>& s = enums[unique];
	if (value >= s.size()) {
		assert(value == s.size());
		s.emplace_back(name);
	}
}

void Printer::operator () (Unique unique, const void* ptr) const {
	std::unordered_map<UniqueInfo*, void(*)(const void* ptr)>::const_iterator it = types.find(unique);
	if (it != types.end()) {
		it->second(ptr);
	} else {
		std::unordered_map<UniqueInfo*, std::vector<String> >::const_iterator ie = enums.find(unique);
		if (ie != enums.end()) {
			assert(unique->GetSize() <= sizeof(uint32_t));
			uint32_t v = *reinterpret_cast<const uint32_t*>(ptr);
			assert(v < ie->second.size());
			ImGui::Text("%s", ie->second[v].c_str());
		} else {
			ImGui::Text("(0x%p)", ptr);
		}
	}
}

static inline String GetTypeName(Unique unique) {
	return unique == UniqueType<String>::Get() ? "String" : unique == UniqueType<Void>::Get() ? "void" : RemoveNamespace(unique->GetName());
}

IWidget::IWidget() : layerCount(0), IReflect(true, true, true, true), show(true), inited(false) {}

void IWidget::Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
	bool expanded = false;
	if (layerCount != 0)
		ImGui::Indent(8.0f * layerCount);

	void* address = ptr;
	if (!s.IsBasicObject() || refTypeID != typeID) {
		expanded = expandedObjects.count(address) != 0;
		ImGui::Text(expanded ? "-" : "+");
		if (ImGui::IsItemClicked(0)) {
			expanded = !expanded;
		}

		if (expanded) {
			newExpandedObjects.insert(address);
		}
	} else {
		ImGui::Text(" ");
	}

	ImGui::SameLine();
	ImGui::Text("%s", name);
	if (layerCount != 0)
		ImGui::Unindent(8.0f * layerCount);
	ImGui::NextColumn();

	if (s.IsBasicObject() && typeID == refTypeID) {
		printer(typeID, ptr);
	} else {
		ImGui::Text("%s", RemoveNamespace(s.ToString()).c_str());
	}

	ImGui::NextColumn();
	if (refTypeID == typeID) {
		ImGui::Text("%s", GetTypeName(typeID).c_str());
	} else {
		IReflectObject* object = *reinterpret_cast<IReflectObject**>(ptr);
		if (object == nullptr) {
			ImGui::Text("%s", GetTypeName(typeID).c_str());
		} else {
			ImGui::Text("%s", (GetTypeName(object->GetUnique()) + " * ").c_str());
		}
	}
	ImGui::NextColumn();
	ImGui::NextColumn();

	if (expanded) {
		++layerCount;
		if (refTypeID != typeID) {
			IReflectObject* object = *reinterpret_cast<IReflectObject**>(ptr);
			if (object != nullptr) {
				(*object)(*this);
			}
		} else if (s.IsIterator()) {
			IIterator& iterator = static_cast<IIterator&>(s);
			Unique subUnique = iterator.GetElementUnique();
			Unique refUnique = iterator.GetElementReferenceUnique();
			char name[64];
			int i = 0;
			while (iterator.Next()) {
				sprintf(name, "[%d]", i++);
				void* subptr = iterator.Get();
				assert(subptr != nullptr);
				static IReflectObject dummy;
				subptr = subUnique == refUnique ? subptr : *reinterpret_cast<void**>(subptr);
				Property(iterator.IsElementBasicObject() && subUnique == refUnique ? dummy : *reinterpret_cast<IReflectObject*>(subptr), refUnique, refUnique, name, nullptr, subptr, nullptr);
			}
		} else {
			s(*this);
		}
		--layerCount;
	}
}

void IWidget::Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {
	if (layerCount != 0)
		ImGui::Indent(8.0f * layerCount);

	bool isScriptMethod = strncmp(name, "Request", 7) == 0;
	ImGui::Text(" ");
	ImGui::SameLine();
	ImGui::Text("%s", isScriptMethod ? name + 7 : name);
	ImGui::NextColumn();
	const TMethod<false, Void, Void>& method = *static_cast<const TMethod<false, Void, Void>*>(p);
	ImGui::Text("0x%p", *(void**)&method.p);
	ImGui::NextColumn();
	ImGui::Text("%s", isScriptMethod ? "ScriptMethod" : "Method");
	ImGui::NextColumn();
	// make signature
	std::stringstream ss;
	ss << GetTypeName(retValue) << " (";
	size_t startIndex = isScriptMethod ? 1 : 0;
	for (size_t k = startIndex; k < params.size(); k++) {
		if (k != startIndex) ss << ", ";
		ss << GetTypeName(params[k].type);
	}

	ss << ")";
	ImGui::Text("%s", StdToUtf8(ss.str()).c_str());
	ImGui::NextColumn();

	if (layerCount != 0)
		ImGui::Unindent(8.0f * layerCount);
}

void IWidget::Class(IReflectObject& host, Unique id, const char* name, const char* path, const MetaChainBase* meta) {
	while (meta != nullptr) {
		MetaNodeBase* node = const_cast<MetaNodeBase*>(meta->GetNode());
		if (node->GetUnique() != UniqueType<Void>::Get()) {
			(*node)(*this);
		}

		meta = meta->GetNext();
	}
}

void IWidget::Enum(size_t value, Unique id, const char* name, const MetaChainBase* meta) {
	// Do Not record enum?
	printer.AddEnum(id, value, name);
}

void IWidget::RenderObject(IReflectObject& object) {
	std::swap(newExpandedObjects, expandedObjects);
	newExpandedObjects.clear();

	ImGui::Columns(4);
	if (!inited) {
		ImGui::SetColumnWidth(0, 150.0f);
		ImGui::SetColumnWidth(1, 100.0f);
		ImGui::SetColumnWidth(2, 150.0f);
		ImGui::SetColumnWidth(3, 350.0f);
		inited = true;
	}

	ImGui::TextColored(ImVec4(1, 1, 0, 1), "Name");
	ImGui::NextColumn();
	ImGui::TextColored(ImVec4(1, 1, 0, 1), "Value");
	ImGui::NextColumn();
	ImGui::TextColored(ImVec4(1, 1, 0, 1), "Type");
	ImGui::NextColumn();
	ImGui::TextColored(ImVec4(1, 1, 0, 1), "Note");
	ImGui::NextColumn();
	object(*this);
	ImGui::Columns(1);
}

void IWidget::LeaveMainLoop() {}