// ScreenFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class ScreenFS : public TReflected<ScreenFS, IShader> {
	public:
		ScreenFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

	public:
		BindTexture inputColorTexture;
		BindTexture inputBloomTexture0;
		BindTexture inputBloomTexture1;
		BindTexture inputBloomTexture2;
		BindBuffer paramBuffer;

	protected:
		Float2 rasterCoord;
		Float4 outputColor;

		Float3 bloomIntensity;
		float invAverageLuminance;
	};
}
