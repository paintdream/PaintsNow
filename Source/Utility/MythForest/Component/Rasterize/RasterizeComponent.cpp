#include "RasterizeComponent.h"
#include "../../Entity.h"

using namespace PaintsNow;

static inline float CrossProduct(const TVector<float, 2>& lhs, const TVector<float, 2>& rhs) {
	return lhs[0] * rhs[1] - lhs[1] * rhs[0];
}

RasterizeComponent::RasterizeComponent(const TShared<TextureResource>& texture) : targetTexture(texture) {}

template <class D, class F>
void RenderMeshEx(UniqueType<D>, F, const TShared<TextureResource>& targetTexture, const TShared<MeshResource>& meshResource, const MatrixFloat4x4& transform) {
	const std::vector<UInt3>& indices = meshResource->meshCollection.indices;
	const std::vector<Float3>& vertices = meshResource->meshCollection.vertices;

	int32_t width = verify_cast<int32_t>(targetTexture->description.dimension.x());
	int32_t height = verify_cast<int32_t>(targetTexture->description.dimension.y());
	IRender::Resource::TextureDescription::Format format = (IRender::Resource::TextureDescription::Format)targetTexture->description.state.format;
	IRender::Resource::TextureDescription::Layout layout = (IRender::Resource::TextureDescription::Layout)targetTexture->description.state.layout;
	int32_t byteDepth = IImage::GetPixelBitDepth(format, layout) / 8;
	if (targetTexture->description.data.Empty()) {
		targetTexture->description.data.Resize(width * height * byteDepth);
	}

	D* buffer = reinterpret_cast<D*>(targetTexture->description.data.GetData());

	for (size_t k = 0; k < indices.size(); k++) {
		const UInt3& index = indices[k];

		Float3 v1 = Math::Transform(transform, vertices[index[0]]);
		Float3 v2 = Math::Transform(transform, vertices[index[1]]);
		Float3 v3 = Math::Transform(transform, vertices[index[2]]);

		// TODO: optimize
		Float3Pair bound(v1, v1);
		Math::Union(bound, v2);
		Math::Union(bound, v3);

		int32_t minX = (int32_t)(bound.first.x() * width + 0.5f), minY = (int32_t)(bound.second.y() * height + 0.5f);
		int32_t maxX = (int32_t)(bound.second.x() * width + 0.5f), maxY = (int32_t)(bound.second.y() * height + 0.5f);

		Float2 q1(v2.x() - v1.x(), v2.y() - v1.y());
		Float2 q2(v3.x() - v2.x(), v3.y() - v2.y());
		Float2 q3(v1.x() - v3.x(), v1.y() - v3.y());
		float hand = CrossProduct(q1, q2);

		for (int32_t j = Math::Max(0, minY); j < Math::Min(maxY + 1, height); j++) {
			for (int32_t i = Math::Max(0, minX); i < Math::Min(maxX + 1, width); i++) {
				Float2 p;
				p.x() = Math::Interpolate(bound.first.x(), bound.second.x(), (float)(i - minX) / (maxX - minX + 1));
				p.y() = Math::Interpolate(bound.first.y(), bound.second.y(), (float)(j - minY) / (maxY - minY + 1));
				Float2 p1 = p - Float2(v1.x(), v1.y());
				Float2 p2 = p - Float2(v2.x(), v2.y());
				Float2 p3 = p - Float2(v3.x(), v3.y());

				if (CrossProduct(q1, p1) * hand < 0)
					continue;
				if (CrossProduct(q2, p2) * hand < 0)
					continue;
				if (CrossProduct(q3, p3) * hand < 0)
					continue;

				float n = Math::DotProduct(p1, q1) / Math::SquareLength(q1);
				float m = Math::DotProduct(p3, q3) / Math::SquareLength(q3);
				float depth = v1.z() + (v2.z() - v1.z()) * n + (v1.z() - v3.z()) * n;

				// ok, shade
				D& target = buffer[i + j * width];
				D source;

				if (getboolean<F>::value) {
					// float ?
					source = D(depth);
				} else {
					source = (D)(-Math::Clamp(depth, 0.0f, 1.0f));
				}

				target = Math::Max(target, source);
			}
		}
	}
}

void RasterizeComponent::RenderMesh(const TShared<MeshResource>& meshResource, const MatrixFloat4x4& transform) {
	IRender::Resource::TextureDescription::Format format = (IRender::Resource::TextureDescription::Format)targetTexture->description.state.format;
	IRender::Resource::TextureDescription::Layout layout = (IRender::Resource::TextureDescription::Layout)targetTexture->description.state.layout;

	assert(layout == IRender::Resource::TextureDescription::R || layout == IRender::Resource::TextureDescription::DEPTH);
	assert(meshResource);

	meshResource->Map();
	switch (format) {
		case IRender::Resource::TextureDescription::UNSIGNED_BYTE:
			RenderMeshEx(UniqueType<uint8_t>(), std::false_type(), targetTexture, meshResource, transform);
			break;
		case IRender::Resource::TextureDescription::UNSIGNED_SHORT:
			RenderMeshEx(UniqueType<uint16_t>(), std::false_type(), targetTexture, meshResource, transform);
			break;
		case IRender::Resource::TextureDescription::UNSIGNED_INT:
			RenderMeshEx(UniqueType<uint32_t>(), std::false_type(), targetTexture, meshResource, transform);
			break;
		case IRender::Resource::TextureDescription::FLOAT:
			RenderMeshEx(UniqueType<float>(), std::true_type(), targetTexture, meshResource, transform);
			break;
		default:
			assert(false); // not supported
			break;
	}

	meshResource->UnMap();
}
