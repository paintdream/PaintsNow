#include "SkeletonResource.h"
#include "../../../Core/Template/TAlgorithm.h"
#include <cassert>
#include <cmath>
#include "../../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

const double EPSILON = 1e-7;

using namespace PaintsNow;

SkeletonResource::SkeletonResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {}

TObject<IReflect>& SkeletonResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(boneAnimation);
	}

	return *this;
}

void SkeletonResource::Attach(IRender& render, void* deviceContext) {
	offsetMatrices.resize(boneAnimation.joints.size());
	offsetMatricesInv.resize(boneAnimation.joints.size());
	for (size_t i = 0; i < boneAnimation.joints.size(); i++) {
		PrepareOffsetTransform(i);
	}
}

void SkeletonResource::Detach(IRender& render, void* deviceContext) {}
void SkeletonResource::Upload(IRender& render, void* deviceContext) {}
void SkeletonResource::Download(IRender& render, void* deviceContext) {}

void SkeletonResource::PrepareTransform(std::vector<MatrixFloat4x4>& transforms, size_t k) {
	const Joint& joint = boneAnimation.joints[k];
	int parent = joint.parent;
	if (parent != -1) {
		transforms[k] = transforms[k] * transforms[parent];
	}
}

void SkeletonResource::PrepareOffsetTransform(size_t k) {
	const Joint& joint = boneAnimation.joints[k];
	int parent = joint.parent;

	offsetMatrices[k] = joint.offsetMatrix;

	// mult all parents
	if (parent != -1) {
		offsetMatrices[k] = offsetMatrices[k] * offsetMatrices[parent];
	}

	offsetMatricesInv[k] = Math::QuickInverse(offsetMatrices[k]);
}

static uint32_t LocateFrame(const std::vector<float>& frames, float time) {
	assert(!frames.empty());
	size_t frame = std::lower_bound(frames.begin(), frames.end(), time) - frames.begin();
	return verify_cast<uint32_t>(Math::Min(frame, frames.size() - 1));
}

const IAsset::BoneAnimation& SkeletonResource::GetBoneAnimation() const {
	return boneAnimation;
}

const std::vector<MatrixFloat4x4>& SkeletonResource::GetOffsetTransforms() const {
	return offsetMatrices;
}

void SkeletonResource::UpdateBoneMatrixBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer, const std::vector<MatrixFloat4x4>& data) {
	UpdateBuffer(render, queue, buffer, const_cast<std::vector<MatrixFloat4x4>&>(data), IRender::Resource::BufferDescription::UNIFORM, "BoneMatrix");
}

void SkeletonResource::ClearBoneMatrixBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer) {
	ClearBuffer(render, queue, buffer);
}

float SkeletonResource::Snapshot(std::vector<MatrixFloat4x4>& transforms, uint32_t clipIndex, float time, bool repeat) {
	OPTICK_EVENT();
	assert(transforms.size() == boneAnimation.joints.size());
	if (boneAnimation.clips.size() == 0) {
		return time;
	}

	const IAsset::BoneAnimation::Clip& clip = boneAnimation.clips[clipIndex];
	if (time > clip.duration) {
		if (repeat) {
			time = fmod(time, clip.duration);
		} else {
			time = clip.duration;
		}
	}

	for (size_t c = 0; c < clip.channels.size(); c++) {
		const IAsset::BoneAnimation::Channel& channel = clip.channels[c];
		int a = channel.jointIndex;
		const Joint* joint = &boneAnimation.joints[a];

		// process positions
		Float3 presentPosition;

		if (channel.transSequence.frames.size() != 0) {
			// interpolate between this frame and the next one.
			uint32_t frame = LocateFrame(channel.transSequence.timestamps, time);
			uint32_t nextFrame = (frame + 1) % (int)channel.transSequence.frames.size();
			const Float3& key = channel.transSequence.frames[frame];
			const Float3& nextKey = channel.transSequence.frames[nextFrame];
			float keyTime = channel.transSequence.timestamps[frame];
			float diffTime = channel.transSequence.timestamps[nextFrame] - keyTime;

			if (diffTime < 0)
				diffTime += clip.duration;

			if (diffTime > 0) {
				float factor = ((float)time - keyTime) / diffTime;
				// linear interpolation
				float factor1;
				float factor2;
				float factor3;
				float factor4;
				float factorTimesTwo;
				float inverseFactorTimesTwo;

				float inverseFactor = 1.0f - factor;

				switch (channel.transSequence.interpolate) {
					case IAsset::INTERPOLATE_NONE:
						presentPosition = key;
						break;
					case IAsset::INTERPOLATE_LINEAR:
						presentPosition = key + (nextKey - key) * factor;
						break;
					case IAsset::INTERPOLATE_HERMITE:
					{
						factorTimesTwo = factor * factor;

						factor1 = factorTimesTwo * (2.0f * factor - 3.0f) + 1;
						factor2 = factorTimesTwo * (factor - 2.0f) + factor;
						factor3 = factorTimesTwo * (factor - 1.0f);
						factor4 = factorTimesTwo * (3.0f - 2.0f * factor);
						const std::pair<Float3, Float3>& tangents = channel.transSequence.frameTangents[frame];
						const std::pair<Float3, Float3>& nextTangents = channel.transSequence.frameTangents[nextFrame];
						presentPosition = key * factor1 + tangents.second * factor2 + nextKey * factor3 + nextTangents.first * factor4;
						break;
					}
					case IAsset::INTERPOLATE_BEZIER:
					{
						factorTimesTwo = factor * factor;
						inverseFactorTimesTwo = inverseFactor * inverseFactor;

						factor1 = inverseFactorTimesTwo * inverseFactor;
						factor2 = 3.0f * factor * inverseFactorTimesTwo;
						factor3 = 3.0f * factorTimesTwo * inverseFactor;
						factor4 = factorTimesTwo * factor;

						const std::pair<Float3, Float3>& tangents = channel.transSequence.frameTangents[frame];
						const std::pair<Float3, Float3>& nextTangents = channel.transSequence.frameTangents[nextFrame];
						presentPosition = key * factor1 + tangents.second * factor2 + nextKey * factor3 + nextTangents.first * factor4;
						break;
					}
				}
			} else {
				presentPosition = key;
			}
		}

		// process rotation
		QuaternionFloat presentRotation;

		if (channel.rotSequence.frames.size() != 0) {
			// interpolate between this frame and the next one.
			uint32_t frame = LocateFrame(channel.rotSequence.timestamps, time);
			uint32_t nextFrame = (frame + 1) % (int)channel.rotSequence.frames.size();
			const Float4& key = channel.rotSequence.frames[frame];
			const Float4& nextKey = channel.rotSequence.frames[nextFrame];
			float keyTime = channel.rotSequence.timestamps[frame];
			float diffTime = channel.rotSequence.timestamps[nextFrame] - keyTime;

			if (diffTime < 0)
				diffTime += clip.duration;

			if (diffTime > EPSILON) {
				float factor = ((float)time - keyTime) / diffTime;
				// linear interpolation
				switch (channel.rotSequence.interpolate) {
					case IAsset::INTERPOLATE_NONE:
						presentRotation = key;
						break;
					case IAsset::INTERPOLATE_LINEAR:
						QuaternionFloat::Interpolate(presentRotation, QuaternionFloat(key), QuaternionFloat(nextKey), factor);
						break;
					case IAsset::INTERPOLATE_HERMITE:
					case IAsset::INTERPOLATE_BEZIER:
					{
						const std::pair<Float4, Float4>& tangents = channel.rotSequence.frameTangents[frame];
						const std::pair<Float4, Float4>& nextTangents = channel.rotSequence.frameTangents[nextFrame];
						QuaternionFloat::InterpolateSquad(presentRotation, QuaternionFloat(key), QuaternionFloat(tangents.second), QuaternionFloat(nextKey), QuaternionFloat(nextTangents.first), factor);
						break;
					}
				}
			} else {
				presentRotation = key;
			}
		}

		// process scaling
		Float3 presentScaling(1, 1, 1);

		if (channel.scalingSequence.frames.size() != 0) {
			uint32_t frame = LocateFrame(channel.scalingSequence.timestamps, time);
			uint32_t nextFrame = (frame + 1) % (int)channel.scalingSequence.frames.size();
			const Float3& key = channel.scalingSequence.frames[frame];
			const Float3& nextKey = channel.scalingSequence.frames[nextFrame];
			float keyTime = channel.scalingSequence.timestamps[frame];
			float diffTime = channel.scalingSequence.timestamps[nextFrame] - keyTime;

			if (diffTime < 0)
				diffTime += clip.duration;

			if (diffTime > EPSILON) {
				float factor = ((float)time - keyTime) / diffTime;
				float factor1;
				float factor2;
				float factor3;
				float factor4;
				float factorTimesTwo;
				float inverseFactorTimesTwo;

				float inverseFactor = 1.0f - factor;

				switch (channel.scalingSequence.interpolate) {
					case IAsset::INTERPOLATE_NONE:
						presentScaling = key;
						break;
					case IAsset::INTERPOLATE_LINEAR:
						presentScaling = key + (nextKey - key) * factor;
						break;
					case IAsset::INTERPOLATE_HERMITE:
					{
						factorTimesTwo = factor * factor;

						factor1 = factorTimesTwo * (2.0f * factor - 3.0f) + 1;
						factor2 = factorTimesTwo * (factor - 2.0f) + factor;
						factor3 = factorTimesTwo * (factor - 1.0f);
						factor4 = factorTimesTwo * (3.0f - 2.0f * factor);
						const std::pair<Float3, Float3>& tangents = channel.scalingSequence.frameTangents[frame];
						const std::pair<Float3, Float3>& nextTangents = channel.scalingSequence.frameTangents[nextFrame];
						presentScaling = key * factor1 + tangents.second * factor2 + nextKey * factor3 + nextTangents.first * factor4;
						break;
					}
					case IAsset::INTERPOLATE_BEZIER:
					{
						factorTimesTwo = factor * factor;
						inverseFactorTimesTwo = inverseFactor * inverseFactor;

						factor1 = inverseFactorTimesTwo * inverseFactor;
						factor2 = 3.0f * factor * inverseFactorTimesTwo;
						factor3 = 3.0f * factorTimesTwo * inverseFactor;
						factor4 = factorTimesTwo * factor;

						const std::pair<Float3, Float3>& tangents = channel.scalingSequence.frameTangents[frame];
						const std::pair<Float3, Float3>& nextTangents = channel.scalingSequence.frameTangents[nextFrame];
						presentScaling = key * factor1 + tangents.second * factor2 + nextKey * factor3 + nextTangents.first * factor4;
						break;
					}
				}
			} else {
				presentScaling = key;
			}
		}

		MatrixFloat4x4 mat;
		presentRotation.WriteMatrix(mat);
		mat = mat.Transpose();

		for (int i = 0; i < 3; i++)
			mat(0, i) *= presentScaling.x();
		for (int j = 0; j < 3; j++)
			mat(1, j) *= presentScaling.y();
		for (int k = 0; k < 3; k++)
			mat(2, k) *= presentScaling.z();

		mat(3, 0) = presentPosition.x();
		mat(3, 1) = presentPosition.y();
		mat(3, 2) = presentPosition.z();

		transforms[a] = mat * offsetMatrices[a];
	}

	// make transforms from top to end
	for (size_t k = 0; k < boneAnimation.joints.size(); k++) {
		PrepareTransform(transforms, k);
	}

	for (size_t d = 0; d < boneAnimation.joints.size(); d++) {
		transforms[d] = offsetMatricesInv[d] * transforms[d];
	}

	return time;
}