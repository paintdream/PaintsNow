// DataComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../../Core/Interface/IType.h"

namespace PaintsNow {
	class DataComponent : public TAllocatedTiny<DataComponent, Component> {
	public:
		DataComponent(size_t initMaxObjectCount);
		enum {
			DATACOMPONENT_FIX_COUNT = COMPONENT_CUSTOM_BEGIN,
			DATACOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
		};

		void SetProperty(const IReflectObject& prototype);
		size_t SetProperty(const String& name, size_t size);
		size_t GetProperty(const String& name) const;
		uint8_t* GetPropertyData(size_t objectIndex, size_t propertyIndex);
		const uint8_t* GetPropertyData(size_t objectIndex, size_t propertyIndex) const;
		size_t GetPropertySize(size_t propertyIndex) const;

		size_t AllocateObject();
		void DeallocateObject(size_t objectIndex);

	protected:
		class Property : public SharedTiny {
		public:
			Property(size_t size, size_t count);
			String name;
			Bytes data;
			size_t size;
		};

		bool IsObjectIndexValid(size_t index) const;

	protected:
		size_t maxObjectCount;
		std::vector<TShared<Property> > properties;
		std::vector<size_t> bitmap;
	};
}

