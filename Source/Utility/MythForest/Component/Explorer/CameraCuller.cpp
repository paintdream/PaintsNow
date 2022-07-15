#include "CameraCuller.h"

#include <cmath>
using namespace PaintsNow;

const double PI = 3.14159265358979323846;
PerspectiveCamera::PerspectiveCamera() : fov((float)PI / 2.0f), aspect(1.0f), nearPlane(0.05f), farPlane(1000.0f) {}

static inline Float4 BuildPlane(const Float3& a, const Float3& b, const Float3& c) {
	Float3 u = a - b;
	Float3 v = c - b;
	Float3 n = Math::Normalize(Math::CrossProduct(u, v));
	return Float4(n.x(), n.y(), n.z(), -Math::DotProduct(a, n));
}

bool FrustrumCuller::operator () (const Float3Pair& box) const {
	// check visibility
	const Float4 half(0.5f, 0.5f, 0.5f, 0.5f);
	Float4 begin = Float4::Load(box.first);
	Float4 end = Float4::Load(box.second);
	Float4 size = end - begin;
	Float4 center = begin + size * half;

	for (size_t i = 0; i < sizeof(planes) / sizeof(planes[0]); i++) {
		const Float4& plane = planes[i];
		float r = Math::DotProduct(Math::Abs(size * plane), half) + plane.w();

		if (Math::DotProduct(plane, center) + r < -1e-4) {
			return false;
		}
	}

	return true;
}

Float3 FrustrumCuller::GetPosition() const {
	return Float3(viewTransform(3, 0), viewTransform(3, 1), viewTransform(3, 2));
}

Float3 FrustrumCuller::GetDirection() const {
	return Float3(-viewTransform(2, 0), -viewTransform(2, 1), -viewTransform(2, 2));
}

void PerspectiveCamera::UpdateCaptureData(FrustrumCuller& captureData, const MatrixFloat4x4& cameraWorldMatrix) const {
	Float3 position(cameraWorldMatrix(3, 0), cameraWorldMatrix(3, 1), cameraWorldMatrix(3, 2));
	Float3 up(cameraWorldMatrix(1, 0), cameraWorldMatrix(1, 1), cameraWorldMatrix(1, 2));
	Float3 direction(-cameraWorldMatrix(2, 0), -cameraWorldMatrix(2, 1), -cameraWorldMatrix(2, 2));
	direction = Math::Normalize(direction);

	captureData.viewTransform = cameraWorldMatrix;

	// update planes ...
	float tanHalfFov = (float)tan(fov / 2.0f);
	Float3 basisX = Math::Normalize(Math::CrossProduct(direction, up));
	Float3 basisY = Math::CrossProduct(basisX, direction);
	float nearStepY = nearPlane * tanHalfFov;
	float nearStepX = nearStepY * aspect;
	float farStepY = farPlane * tanHalfFov;
	float farStepX = farStepY * aspect;

	Float3 nearCenter = position + direction * nearPlane;
	Float3 farCenter = position + direction * farPlane;

	Float3 nearLeftBottom = nearCenter - basisX * nearStepX - basisY * nearStepY;
	Float3 nearRightBottom = nearCenter + basisX * nearStepX - basisY * nearStepY;
	Float3 nearLeftTop = nearCenter - basisX * nearStepX + basisY * nearStepY;
	Float3 nearRightTop = nearCenter + basisX * nearStepX + basisY * nearStepY;

	Float3 farLeftBottom = farCenter - basisX * farStepX - basisY * farStepY;
	Float3 farRightBottom = farCenter + basisX * farStepX - basisY * farStepY;
	Float3 farLeftTop = farCenter - basisX * farStepX + basisY * farStepY;
	Float3 farRightTop = farCenter + basisX * farStepX + basisY * farStepY;

	captureData.planes[0] = BuildPlane(nearLeftTop, nearRightTop, farLeftTop);				// Top
	captureData.planes[1] = BuildPlane(nearLeftBottom, farLeftBottom, nearRightBottom);		// Bottom
	captureData.planes[2] = BuildPlane(nearLeftTop, farLeftTop, farLeftBottom);				// Left
	captureData.planes[3] = BuildPlane(nearRightTop, farRightBottom, farRightTop);			// Right
	captureData.planes[4] = BuildPlane(nearLeftTop, nearLeftBottom, nearRightBottom);		// Front
	captureData.planes[5] = BuildPlane(farLeftTop, farRightBottom, farLeftBottom);			// Back
}

void OrthoCamera::UpdateCaptureData(FrustrumCuller& captureData, const MatrixFloat4x4& cameraWorldMatrix) {
	Float3 position(cameraWorldMatrix(3, 0), cameraWorldMatrix(3, 1), cameraWorldMatrix(3, 2));
	Float3 right(cameraWorldMatrix(0, 0), cameraWorldMatrix(0, 1), cameraWorldMatrix(0, 2));
	Float3 up(cameraWorldMatrix(1, 0), cameraWorldMatrix(1, 1), cameraWorldMatrix(1, 2));
	Float3 direction(-cameraWorldMatrix(2, 0), -cameraWorldMatrix(2, 1), -cameraWorldMatrix(2, 2));

	captureData.viewTransform = cameraWorldMatrix;

	Float3 nearCenter = position - direction;
	Float3 farCenter = position + direction;

	Float3 nearLeftBottom = nearCenter - right - up;
	Float3 nearRightBottom = nearCenter + right - up;
	Float3 nearLeftTop = nearCenter - right + up;
	Float3 nearRightTop = nearCenter + right + up;

	Float3 farLeftBottom = farCenter - right - up;
	Float3 farRightBottom = farCenter + right - up;
	Float3 farLeftTop = farCenter - right + up;
	Float3 farRightTop = farCenter + right + up;

	captureData.planes[0] = BuildPlane(nearLeftTop, nearRightTop, farLeftTop);				// Top
	captureData.planes[1] = BuildPlane(nearLeftBottom, farLeftBottom, nearRightBottom);		// Bottom
	captureData.planes[2] = BuildPlane(nearLeftTop, farLeftTop, farLeftBottom);				// Left
	captureData.planes[3] = BuildPlane(nearRightTop, farRightBottom, farRightTop);			// Right
	captureData.planes[4] = BuildPlane(nearLeftTop, nearLeftBottom, nearRightBottom);		// Front
	captureData.planes[5] = BuildPlane(farLeftTop, farRightBottom, farLeftBottom);			// Back
}
