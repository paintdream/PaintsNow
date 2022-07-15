// StandardParameterFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class StandardParameterFS : public TReflected<StandardParameterFS, IShader> {
	public:
		StandardParameterFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		String GetShaderText() override;

	protected:
		BindTexture baseColorTexture;
		BindTexture normalTexture;
		BindTexture mixtureTexture;

		/*
		BindBuffer paramBuffer;
		Float2 invScreenSize;
		float timestamp;
		float reserved;*/

		Float4 texCoord;
		Float3 viewNormal;
		Float3 viewTangent;
		Float3 viewBinormal;

		Float4 tintColor;
		Float3 outputColor;
		Float3 outputNormal;
		float alpha;
		float metallic;
		float roughness;
		float occlusion;

	public:
		bool enableBaseColorTint;
		bool enableBaseColorTexture;
		bool enableNormalTexture;
		bool enableMixtureTexture;
		bool enableAlphaTest;
	};
}
