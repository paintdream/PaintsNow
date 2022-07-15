#include "WidgetComponent.h"
#include "../../../SnowyStream/Resource/MeshResource.h"

using namespace PaintsNow;

WidgetComponent::WidgetComponent(const TShared<MeshResource>& mesh, const TShared<BatchComponent>& batch) : BaseClass(mesh, batch), texCoordBegin(0, 0, 0, 0), texCoordEnd(1, 1, 1, 1) {
	Flag().fetch_or(RENDERABLECOMPONENT_CAMERAVIEW, std::memory_order_relaxed);
}

void WidgetComponent::SetCoordRect(const Float4& begin, const Float4& end) {
	texCoordBegin = begin;
	texCoordEnd = end;
}

void WidgetComponent::SetMainTexture(const TShared<TextureResource>& texture) {
	mainTexture = texture;
}

uint32_t WidgetComponent::CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) {
	uint32_t start = verify_cast<uint32_t>(outputDrawCalls.size());
	uint32_t count = BaseClass::CollectDrawCalls(outputDrawCalls, inputRenderData, bytesCache, option);
	if (count == ~(uint32_t)0) return count;

	const Bytes texCoordBeginKey = StaticBytes(texCoordBegin);
	const Bytes texCoordEndKey = StaticBytes(texCoordEnd);

	for (uint32_t i = start; i < count; i++) {
		OutputRenderData& renderData = outputDrawCalls[i];
		PassBase::Updater& updater = renderData.shaderResource->GetPassUpdater();
		IRender::Resource::QuickDrawCallDescription& drawCall = renderData.drawCallDescription;
		IRender::Resource::RenderStateDescription& renderState = renderData.renderStateDescription;
		renderState.stencilReplacePass = 0;
		renderState.cull = 1;
		renderState.fill = 1;
		renderState.colorWrite = 1;
		renderState.blend = 1;
		renderState.depthTest = IRender::Resource::RenderStateDescription::DISABLED;
		renderState.depthWrite = 0;
		renderState.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
		renderState.stencilWrite = 0;
		renderState.stencilMask = 0;
		renderState.stencilValue = 0;

		const PassBase::Parameter& paramMainTexture = updater[IShader::BindInput::MAINTEXTURE];
		const PassBase::Parameter& paramTexCoordBegin = updater[texCoordBeginKey];
		const PassBase::Parameter& paramTexCoordEnd = updater[texCoordEndKey];
		assert(paramMainTexture && paramTexCoordBegin && paramTexCoordEnd);

		drawCall.GetTextures()[paramMainTexture.slot] = mainTexture()->GetRenderResource();

		// prepare instance buffer data
		Bytes localInstancedTexCoordBeginData;
		Bytes localInstancedTexCoordEndData;

		Float4* localInstancedTexCoordBeginPtr;
		Float4* localInstancedTexCoordEndPtr;

		size_t localInstancedTexCoordBeginStride;
		size_t localInstancedTexCoordEndStride;

		if (paramTexCoordBegin.slot == paramTexCoordEnd.slot) {
			localInstancedTexCoordBeginData = bytesCache.New(verify_cast<uint32_t>(sizeof(Float4) * 2));
			localInstancedTexCoordBeginPtr = reinterpret_cast<Float4*>(localInstancedTexCoordBeginData.GetData());
			localInstancedTexCoordEndPtr = reinterpret_cast<Float4*>(localInstancedTexCoordBeginData.GetData() + sizeof(Float4));
			localInstancedTexCoordBeginStride = sizeof(Float4) * 2;
			localInstancedTexCoordEndStride = sizeof(Float4) * 2;
		} else {
			localInstancedTexCoordBeginData = bytesCache.New(verify_cast<uint32_t>(sizeof(Float4)));
			localInstancedTexCoordEndData = bytesCache.New(verify_cast<uint32_t>(sizeof(Float4)));
			localInstancedTexCoordBeginPtr = reinterpret_cast<Float4*>(localInstancedTexCoordBeginData.GetData());
			localInstancedTexCoordEndPtr = reinterpret_cast<Float4*>(localInstancedTexCoordEndData.GetData());
			localInstancedTexCoordBeginStride = sizeof(Float4);
			localInstancedTexCoordEndStride = sizeof(Float4);
		}

		*localInstancedTexCoordBeginPtr = texCoordBegin;
		localInstancedTexCoordBeginPtr = reinterpret_cast<Float4*>(reinterpret_cast<uint8_t*>(localInstancedTexCoordBeginPtr) + localInstancedTexCoordBeginStride);
		*localInstancedTexCoordEndPtr = texCoordEnd;
		localInstancedTexCoordEndPtr = reinterpret_cast<Float4*>(reinterpret_cast<uint8_t*>(localInstancedTexCoordEndPtr) + localInstancedTexCoordEndStride);

		for (size_t k = 0; k < renderData.localInstancedData.size(); k++) {
			assert(renderData.localInstancedData[k].first != paramTexCoordEnd.slot); // must not overlapped
		}

		renderData.localInstancedData.emplace_back(std::make_pair(paramTexCoordEnd.slot, Bytes::Null()));
		if (paramTexCoordBegin.slot != paramTexCoordEnd.slot) {
			renderData.localInstancedData.emplace_back(std::make_pair(paramTexCoordBegin.slot, Bytes::Null()));
			renderData.localInstancedData[1].second = std::move(localInstancedTexCoordEndData);
		}

		renderData.localInstancedData[0].second = std::move(localInstancedTexCoordBeginData);
		
		// renderData.localTransforms.emplace_back(MatrixFloat4x4::Identity());
		renderData.drawCallDescription.instanceCount = 1;
	}

	return verify_cast<uint32_t>(outputDrawCalls.size()) - start;
}
