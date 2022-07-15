// DataComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "DataComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class DataComponent;
	class DataComponentModule : public TReflected<DataComponentModule, ModuleImpl<DataComponent> > {
	public:
		DataComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create a new data component
		/// </summary>
		/// <param name="maxObjectCount"> max object count</param>
		/// <returns></returns>
		TShared<DataComponent> RequestNew(IScript::Request& request, size_t maxObjectCount);

		/// <summary>
		/// Set (by now we only support add) a property for a data component.
		/// </summary>
		/// <param name="dataComponent"> data component </param>
		/// <param name="name"> property name </param>
		/// <param name="sizeInBytes"> property size (in bytes) </param>
		/// <returns> property index </returns>
		size_t RequestSetProperty(IScript::Request& request, IScript::Delegate<DataComponent> dataComponent, const String& name, size_t sizeInBytes);

		/// <summary>
		/// Get property index of a data component
		/// </summary>
		/// <param name="dataComponent"> data component </param>
		/// <param name="name"> property name to get </param>
		/// <returns> property index, nil for missing </returns>
		void RequestGetProperty(IScript::Request& request, IScript::Delegate<DataComponent> dataComponent, const String& name);

		/// <summary>
		/// Set property data of specified object
		/// </summary>
		/// <param name="dataComponent"> data component </param>
		/// <param name="objectIndex"> object index, must be lower than max object index </param>
		/// <param name="propertyIndex"> property index </param>
		/// <param name="data"> new data </param>
		void RequestSetPropertyData(IScript::Request& request, IScript::Delegate<DataComponent> dataComponent, size_t objectIndex, size_t propertyIndex, const StringView& data);

		/// <summary>
		/// Get property data of specified object
		/// </summary>
		/// <param name="dataComponent"> data component </param>
		/// <param name="objectIndex"> object index, must be lower than max object index </param>
		/// <param name="propertyIndex"> property index </param>
		/// <returns> requested data </returns>
		String RequestGetPropertyData(IScript::Request& request, IScript::Delegate<DataComponent> dataComponent, size_t objectIndex, size_t propertyIndex);
	};
}

