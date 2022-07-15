// CameraCuller.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-24
//

#pragma once
#include "../../../../Core/Interface/IType.h"
#include "../../../../Core/Template/TBuffer.h"

namespace PaintsNow {
	struct FrustrumCuller {
		bool operator () (const Float3Pair& box) const;
		Float3 GetPosition() const;
		Float3 GetDirection() const;

		MatrixFloat4x4 viewTransform;
		Float4 planes[6];
	};

	class PerspectiveCamera {
	public:
		PerspectiveCamera();
		void UpdateCaptureData(FrustrumCuller& captureData, const MatrixFloat4x4& cameraWorldMatrix) const;
		float nearPlane;
		float farPlane;
		float fov;
		float aspect;
	};

	struct OrthoCamera {
		static void UpdateCaptureData(FrustrumCuller& captureData, const MatrixFloat4x4& cameraWorldMatrix);
	};
}

