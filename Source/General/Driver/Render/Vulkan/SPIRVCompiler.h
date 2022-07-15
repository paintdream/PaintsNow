// SPRIVCompiler.h
// PaintDream (paintdream@paintdream.com)
// 2021-1-3
//

#pragma once
#include "../../../../Core/PaintsNow.h"
#include "../../../../Core/Interface/IType.h"
#include "../../../../General/Interface/IRender.h"

namespace PaintsNow {
	class SPIRVCompiler {
	public:
		static String Compile(IRender::Resource::ShaderDescription::Stage stage, const String& inputSource);
	};
}

