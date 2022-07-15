// HoneyData.h
// PaintDream (paintdream@paintdream.com)
// 2015-12-31
//

#pragma once
#include "../../General/Interface/IDatabase.h"
#include "../../Core/Interface/IScript.h"
#include "../../General/Misc/DynamicObject.h"
#include "../../Core/System/Kernel.h"

namespace PaintsNow {
	class SchemaResolver : public IReflect {
	public:
		SchemaResolver();
		void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override;
		static void SetValueString(IScript::Request& request, char* base);
		static void SetValueText(IScript::Request& request, char* base);
		static void SetValueFloat(IScript::Request& request, char* base);
		static void SetValueInt(IScript::Request& request, char* base);
		static void SetValueNull(IScript::Request& request, char* base);
		void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override;

		typedef void(*Set)(IScript::Request& request, char* base);
		std::vector<std::pair<Set, size_t> > setters;
	};

	class Honey : public TReflected<Honey, WarpTiny> {
	public:
		Honey(IDatabase::MetaData* metaData);
		~Honey() override;
		bool Step();
		void WriteLine(IScript::Request& request);
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		SchemaResolver resolver;
		IDatabase::MetaData* metaData;
	};

	class HoneyData : public IDatabase::MetaData {
	public:
		HoneyData();
		~HoneyData() override;

		IIterator* New() const override;
		void Attach(void* base) override;
		void Initialize(size_t count) override;
		size_t GetTotalCount() const override;
		void* Get() override;
		bool Next() override;
		bool IsLayoutLinear() const override;
		bool IsLayoutPinned() const override;
		void* GetHost() const override;
		bool IsElementBasicObject() const override;
		const IReflectObject& GetElementPrototype() const override;
		Unique GetElementUnique() const override;
		Unique GetElementReferenceUnique() const override;
		virtual const String& GetInternalName() const;

		void Enter();
		void Leave();

		void SetFloat(size_t i);
		void SetString(size_t i);
		void SetInteger(size_t i);

	private:
		DynamicUniqueAllocator uniqueAllocator;
		DynamicObject* dynamicObject;
		IScript::Request* request;
		IScript::Request::Ref tableRef;
		size_t index;
		size_t count;
		typedef void (HoneyData::* Set)(size_t i);
		std::vector<Set> sets;
	};

	IScript::Request& operator >> (IScript::Request& request, HoneyData& honey);
}
