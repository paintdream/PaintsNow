// ScreenTransformVS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class ScreenTransformVS : public TReflected<ScreenTransformVS, IShader> {
	public:
		ScreenTransformVS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		IShader::BindBuffer vertexBuffer;
		IShader::BindBuffer transformBuffer;

		bool enableVertexTransform;
		bool enableRasterCoord;
		bool reserved[2];

		Float3 vertexPosition;
		Float2 rasterCoord;

		MatrixFloat4x4 worldTransform;

		// Output vars
		Float4 position;
	};
}

