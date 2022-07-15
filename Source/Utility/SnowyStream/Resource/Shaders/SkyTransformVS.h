// SkyTransformVS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class SkyTransformVS : public TReflected<SkyTransformVS, IShader> {
	public:
		SkyTransformVS();

		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		IShader::BindBuffer instanceBuffer;
		IShader::BindBuffer globalBuffer;
		IShader::BindBuffer vertexPositionBuffer;
		MatrixFloat4x4 worldMatrix;
		MatrixFloat4x4 viewProjectionMatrix;

		// Input
		Float3 vertexPosition;

		// Output
		Float4 rasterPosition;
		Float3 worldPosition;
	};
}
