// LayoutComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "LayoutComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class LayoutComponent;
	class LayoutComponentModule : public TReflected<LayoutComponentModule, ModuleImpl<LayoutComponent> > {
	public:
		LayoutComponentModule(Engine& engine);
		~LayoutComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create LayoutComponent
		/// </summary>
		/// <returns> LayoutComponent object </returns>
		TShared<LayoutComponent> RequestNew(IScript::Request& request);

		/// <summary>
		/// Get scroll size of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent</param>
		/// <returns> scroll size </returns>
		Float2 RequestGetScrollSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);

		/// <summary>
		/// Get scroll offset of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent</param>
		/// <returns> scroll offset </returns>
		Float2 RequestGetScrollOffset(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);

		/// <summary>
		/// Set scroll offset of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent</param>
		/// <param name="offset"> the offset </param>
		void RequestSetScrollOffset(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float2& offset);

		/// <summary>
		/// Set layout of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent</param>
		/// <param name="layout"> the layout string </param>
		void RequestSetLayout(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const String& layout);

		/// <summary>
		/// Set weight of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <param name="weight"> weight </param>
		void RequestSetWeight(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, int64_t weight);

		/// <summary>
		/// Get weight of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <returns> weight of LayoutComponent </returns>
		int32_t RequestGetWeight(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);

		/// <summary>
		/// Set rect of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <param name="rect"> the rectange range </param>
		void RequestSetRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float4& rect);

		/// <summary>
		/// Get rect of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <returns> rect </returns>
		Float4 RequestGetRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);

		/// <summary>
		/// Get clipped rect of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <returns> clipped rect </returns>
		Float4 RequestGetClippedRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);

		/// <summary>
		/// Set size of LayoutComponent 
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <param name="size"> target size </param>
		void RequestSetSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float4& size);

		/// <summary>
		/// Get size of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <returns> size of LayoutComponent </returns>
		Float4 RequestGetSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);

		/// <summary>
		/// Set padding of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <param name="padding"> the padding </param>
		void RequestSetPadding(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float4& padding);

		/// <summary>
		/// Get padding of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <returns> padding of LayoutComponent </returns>
		Float4 RequestGetPadding(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);

		/// <summary>
		/// Set margin of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <param name="margin"> margin </param>
		void RequestSetMargin(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float4& margin);

		/// <summary>
		/// Get margin of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <returns> margin of LayoutComponent </returns>
		Float4 RequestGetMargin(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);

		/// <summary>
		/// Enable/Disable content fitting of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <param name="fitContent"> if the sub items fit actual rect </param>
		void RequestSetFitContent(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, bool fitContent);

		/// <summary>
		/// Get content fitting state of LayoutComponent
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <returns> if the sub items fit actual rect </returns>
		bool RequestGetFitContent(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);

		/// <summary>
		/// Set selected index range
		/// </summary>
		/// <param name="layoutComponent"> the LayoutComponent </param>
		/// <param name="start"> start index </param>
		/// <param name="count"> count </param>
		void RequestSetIndexRange(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, int start, int count);
	};
}

