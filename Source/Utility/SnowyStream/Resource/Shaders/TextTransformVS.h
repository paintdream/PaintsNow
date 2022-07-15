// TextTransformVS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class TextTransformVS : public TReflected<TextTransformVS, IShader> {
	public:
		TextTransformVS();

		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		IShader::BindBuffer positionBuffer;
		IShader::BindBuffer instanceBuffer;
		IShader::BindBuffer texCoordRectBuffer;

		// Instanced
		MatrixFloat4x4 worldMatrix;
		Float4 texCoordRect;
		Float4 instanceColor;

		// Input
		Float3 unitTexCoord;

		// Output
		Float4 rasterPosition;
		Float4 color;
		Float2 texCoord;
	};
}
