// FieldComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "FieldComponent.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class FieldComponent;
	class FieldComponentModule : public TReflected<FieldComponentModule, ModuleImpl<FieldComponent> > {
	public:
		FieldComponentModule(Engine& engine);
		~FieldComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create FieldComponent
		/// </summary>
		/// <returns> FieldComponent object </returns>
		TShared<FieldComponent> RequestNew(IScript::Request& request);

		/// <summary>
		/// Initialize FieldComponent from a simplygon
		/// </summary>
		/// <param name="fieldComponent"> the FieldComponent </param>
		/// <param name="shapeType"> shape type </param>
		/// <param name="range"> field range </param>
		void RequestLoadSimplygon(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, const String& shapeType, const Float3Pair& range);

		/// <summary>
		/// Initialize FieldComponent from a texture
		/// </summary>
		/// <param name="fieldComponent"> the FieldComponent </param>
		/// <param name="textureResource"> the TextureResource </param>
		void RequestLoadTexture(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<TextureResource> textureResource, const Float3Pair& range);

		/// <summary>
		/// Initialize FieldComponent from a mesh
		/// </summary>
		/// <param name="fieldComponent"> the FieldComponent </param>
		/// <param name="meshResource"> the MeshResource </param>
		void RequestLoadMesh(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<MeshResource> meshResource, const Float3Pair& range);

		/// <summary>
		/// Query field value
		/// </summary>
		/// <param name="fieldComponent"> the FieldComponent </param>
		/// <param name="position"> the query position </param>
		/// <returns></returns>
		String RequestQueryData(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, const Float3& position);

		/// <summary>
		/// Query space entities
		/// </summary>
		/// <param name="fieldComponent"> the FieldComponent </param>
		/// <param name="spaceComponent"> the space to query </param>
		/// <returns> list of queried entities </returns>
		std::vector<TShared<Entity> > RequestQuerySpaceEntities(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<SpaceComponent> spaceComponent);

		/// <summary>
		/// Post event to space
		/// </summary>
		/// <param name="fieldComponent"> the FieldComponent </param>
		/// <param name="spaceComponent"> the space to query </param>
		/// <param name="type"> event type, can be 'Update' or 'Custom' </param>
		/// <param name="sender"> event sender </param>
		/// <param name="param"> custom param </param>
		void RequestPostSpaceEvent(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<SpaceComponent> spaceComponent, const String& type, IScript::Delegate<SharedTiny> sender, IScript::Request::Ref param);
	};
}

