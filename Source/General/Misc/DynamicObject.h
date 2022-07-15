// DynamicObject.h
// 2015-6-21
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../Core/Interface/IReflect.h"

namespace PaintsNow {
	class DynamicObject;
	class_aligned(CPU_CACHELINE_SIZE) DynamicInfo : public UniqueInfo {
	public:
		struct MemController {
			void (*Creator)(void*);
			void (*Deletor)(void*);
			void (*Assigner)(void*, const void*);
		};

		class Field {
		public:
			Field();
			String name;
			Unique type;
			MemController* controller;

			struct {
				uint32_t reflectable : 1;
				uint32_t offset : 31;
			};

			bool operator < (const Field& rhs) const {
				return offset < rhs.offset;
			}
		};

		const Field* operator [] (const String& key) const;

		IReflectObject* Create() const;
		void SetAllocator(UniqueAllocator* alloc) { allocator = alloc; }
		void SetName(const String& name) { typeName = name; }
		void SetSize(size_t s) { size = s; }

		std::vector<Field> fields;
		std::map<String, uint32_t> mapNameToField;
	};

	class DynamicUniqueAllocator : protected UniqueAllocator {
	public:
		DynamicUniqueAllocator();

		DynamicInfo* Create(const String& name, size_t size);
		DynamicInfo* Get(const String& name);

		DynamicInfo* AllocFromDescriptor(const String& name, const std::vector<DynamicInfo::Field>& descriptors);
		std::map<String, DynamicInfo> mapType;
	};

	class DynamicObject : public TReflected<DynamicObject, IReflectObjectComplex> {
	public:
		DynamicInfo* GetDynamicInfo() const;
		void Set(const DynamicInfo::Field& field, const void* value);
		void* Get(const DynamicInfo::Field& field) const;
		void Destroy() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		friend class DynamicInfo;
		DynamicObject(DynamicInfo* info);
		~DynamicObject() override;
		DynamicObject& operator = (const DynamicObject& rhs);

		DynamicInfo* dynamicInfo;
	};

	class DynamicVector : public TReflected<DynamicVector, IReflectObjectComplex> {
	public:
		static DynamicInfo::MemController& GetVectorController();
		void Reinit(Unique unique, DynamicInfo::MemController* mc, size_t n, bool reflectable);
		void Set(size_t i, const void* value);
		void* Get(size_t i) const;
		class Iterator : public IIterator {
		public:
			Iterator(DynamicVector* vec);
			IIterator* New() const override;
			void Attach(void* base) override;
			void* GetHost() const override;
			void Initialize(size_t count) override;
			size_t GetTotalCount() const override;
			void* Get() override;
			bool IsElementBasicObject() const override;
			Unique GetElementUnique() const override;
			Unique GetElementReferenceUnique() const override;
			const IReflectObject& GetElementPrototype() const override;
			bool IsLayoutLinear() const override;
			bool IsLayoutPinned() const override;
			bool Next() override;

		private:
			DynamicVector* base;
			uint32_t i;
		};

	protected:
		static void VectorCreator(void* buffer);
		static void VectorDeletor(void* buffer);
		static void VectorAssigner(void* dst, const void* src);

		friend class DynamicObject;
		friend class Iterator;
		DynamicVector(Unique unique, DynamicInfo::MemController*, size_t c, bool reflectable);
		~DynamicVector() override;
		DynamicVector& operator = (const DynamicVector&);
		void Init();
		void Cleanup();

		Unique unique;
		DynamicInfo::MemController* memController;
		struct {
			uint32_t reflectable : 1;
			uint32_t count : 31;
		};
		void* buffer;

	private:
		DynamicVector(const DynamicVector&);
	};

	class DynamicObjectWrapper : public TReflected<DynamicObjectWrapper, IReflectObjectComplex> {
	public:
		DynamicObjectWrapper(DynamicUniqueAllocator& allocator);
		~DynamicObjectWrapper() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		DynamicUniqueAllocator& uniqueAllocator;
		DynamicObject* dynamicObject;
	};
}

