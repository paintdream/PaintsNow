// RasterizeComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "RasterizeComponent.h"
#include "../../Module.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/MeshResource.h"

namespace PaintsNow {
	class RasterizeComponent;
	class RasterizeComponentModule : public TReflected<RasterizeComponentModule, ModuleImpl<RasterizeComponent> > {
	public:
		RasterizeComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create RasterizeComponent
		/// </summary>
		/// <param name="textureResource"> the Target Texture Resource </param>
		/// <returns> RasterizeComponent object </returns>
		TShared<RasterizeComponent> RequestNew(IScript::Request& request, IScript::Delegate<TextureResource> texture);

		/// <summary>
		/// Rasterize a mesh, write depth to texture
		/// </summary>
		/// <param name="rasterizeComponent"> the RasterizeComponent </param>
		/// <param name="mesh"> mesh </param>
		/// <param name="transform"> transform </param>
		void RequestRenderMesh(IScript::Request& request, IScript::Delegate<RasterizeComponent> rasterizeComponent, IScript::Delegate<MeshResource> meshResource, const MatrixFloat4x4& transform);
	};
}
