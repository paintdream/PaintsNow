// RenderPortCameraView.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"
#include "../../Light/LightComponent.h"

namespace PaintsNow {
	class RenderPortCameraView : public TReflected<RenderPortCameraView, RenderPort> {
	public:
		MatrixFloat4x4 projectionMatrix;
		MatrixFloat4x4 inverseProjectionMatrix;
		MatrixFloat4x4 viewMatrix;
		MatrixFloat4x4 inverseViewMatrix;
		MatrixFloat4x4 reprojectionMatrix;
		Float4 projectionParams;
		Float4 inverseProjectionParams;
		Float2 jitterOffset;
		float jitterHistoryRatio;
	};
}

