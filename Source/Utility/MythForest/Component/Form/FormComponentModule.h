// FormComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "FormComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class FormComponent;
	class FormComponentModule : public TReflected<FormComponentModule, ModuleImpl<FormComponent> > {
	public:
		FormComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create FormComponent
		/// </summary>
		/// <param name="name"> name </param>
		/// <returns> FormComponent object </returns>
		TShared<FormComponent> RequestNew(IScript::Request& request, String& name);

		/// <summary>
		/// Resize FormComponent slots
		/// </summary>
		/// <param name="formComponent"> the FormComponent </param>
		/// <param name="size"> new size </param>
		void RequestResize(IScript::Request& request, IScript::Delegate<FormComponent> formComponent, int32_t size);

		/// <summary>
		/// Set FormComponent data in slot
		/// </summary>
		/// <param name="formComponent"> the FormComponent </param>
		/// <param name="index"> the slot index </param>
		/// <param name="data"> the data </param>
		void RequestSetData(IScript::Request& request, IScript::Delegate<FormComponent> formComponent, int32_t index, String& data);

		/// <summary>
		/// Get FormMComponent data in slot
		/// </summary>
		/// <param name="formComponent"> the FormComponent</param>
		/// <param name="index"> the slot index </param>
		/// <returns> the data </returns>
		const String& RequestGetData(IScript::Request& request, IScript::Delegate<FormComponent> formComponent, int32_t index);

		/// <summary>
		/// Get name 
		/// </summary>
		/// <param name="formComponent"> the ForComponent </param>
		/// <returns> the name </returns>
		const String& RequestGetName(IScript::Request& request, IScript::Delegate<FormComponent> formComponent);
	};
}

