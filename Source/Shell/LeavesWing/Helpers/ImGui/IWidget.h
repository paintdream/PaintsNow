// IWidget.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-3
//

#pragma once
#include "../../../../Core/Interface/IType.h"
#include "../../../../Core/Interface/IReflect.h"
#include "../../../../Core/Template/TMap.h"

namespace PaintsNow {
	class LeavesFlute;
	inline String RemoveNamespace(const String& s) {
		String t = s;
		String::size_type pos;
		while ((pos = t.find("PaintsNow::")) != String::npos) {
			String m = t.substr(pos + 11);
			std::swap(m, t);
		}

		return t;
	}

	struct Printer {
		Printer();
		void AddEnum(Unique unique, size_t value, const char* name);
		void operator () (Unique unique, const void* ptr) const;
		template <class T>
		void RegisterVectorType();
		template <class T>
		void RegisterPrimitiveType();

		std::unordered_map<UniqueInfo*, void(*)(const void* ptr)> types;
		std::unordered_map<UniqueInfo*, std::vector<String> > enums;
	};

	class IWidget : public IReflect {
	public:
		IWidget();
		virtual void TickRender(LeavesFlute& leavesFlute) = 0;
		virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta);
		virtual void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta);
		virtual void Class(IReflectObject& host, Unique id, const char* name, const char* path, const MetaChainBase* meta);
		virtual void Enum(size_t value, Unique id, const char* name, const MetaChainBase* meta);
		virtual void LeaveMainLoop();

	protected:
		void RenderObject(IReflectObject& object);

	protected:
		std::set<void*> expandedObjects;
		std::set<void*> newExpandedObjects;
		Printer printer;
		uint32_t layerCount;

		bool show;
		bool inited;
		bool reserved[2];
	};
}

