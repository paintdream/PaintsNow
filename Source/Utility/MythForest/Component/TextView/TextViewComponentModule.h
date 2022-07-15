// TextViewComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "TextViewComponent.h"
#include "../Batch/BatchComponentModule.h"
#include "../Renderable/RenderableComponentModule.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class TextViewComponent;
	class TextViewComponentModule : public TReflected<TextViewComponentModule, TRenderableComponentModule<TextViewComponent> > {
	public:
		TextViewComponentModule(Engine& engine);
		~TextViewComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Initialize() override;

		/// <summary>
		/// Create TextViewComponent
		/// </summary>
		/// <param name="fontResource"> the FontResource</param>
		/// <param name="meshResource"> the TextView mesh, can be empty (quad mesh default) </param>
		/// <param name="materialResource"> the TextView material, can be empty </param>
		/// <param name="batchComponent"> the BatchComponent</param>
		/// <returns> TextViewComponent object </returns>
		TShared<TextViewComponent> RequestNew(IScript::Request& request, IScript::Delegate<FontResource> fontResource, IScript::Delegate<MeshResource> meshResource, IScript::Delegate<MaterialResource> materialResource, IScript::Delegate<BatchComponent> batchComponent);

		/// <summary>
		/// Set font size of TextViewComponent
		/// </summary>
		/// <param name="textViewComponent"> the TextViewComponent </param>
		/// <param name="fontSize"> the font size</param>
		void RequestSetFontSize(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent, uint32_t fontSize);

		/// <summary>
		/// Get text of TextViewComponent
		/// </summary>
		/// <param name="textViewComponent"> the TextViewComponent </param>
		/// <returns> text of TextViewComponent </returns>
		String RequestGetText(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent);

		/// <summary>
		/// Set text of TextViewComponent
		/// </summary>
		/// <param name="textViewComponent"> the TextViewComponent </param>
		/// <param name="text"> text to display, support simple text marks </param>
		void RequestSetText(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent, const String& text);

		/// <summary>
		/// Locate text position based on raster offset
		/// </summary>
		/// <param name="textViewComponent"> the TextViewComponent </param>
		/// <param name="offset"> raster offset </param>
		/// <param name="isRowCol"> is row or column </param>
		/// <returns> text position </returns>
		Short3 RequestLocateText(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent, const Short2& offset, bool isRowCol);

	protected:
		TShared<MaterialResource> defaultTextMaterial;
		TShared<MeshResource> defaultTextMesh;
		BatchComponentModule* batchComponentModule;
	};
}

