-- StandardRenderFlow.tl
-- 
local StandardRenderFlow = {}

function StandardRenderFlow.New(bloomLayerCount : number) : RenderFlowComponent
	local renderFlowComponent = RenderFlowComponentModule.New()
	bloomLayerCount = math.max(bloomLayerCount, 1) -- at least one bloom layer
	local strBloomLayer = tostring(bloomLayerCount)
	local computePass = SnowyStream.GetRenderProfile("ComputeShader") ~= 0
	if not computePass then
		print("StandardRenderFlow: ComputeShader not supported by this device, fallback to texture encoded light.")
	end

	local debugSSR = false

	-- Step 1: Setup Geometry stage
	local stageGeometryBuffer : RenderStage = renderFlowComponent:NewRenderStage("GeometryBufferRenderStage", "")
	local stageDeferredLighting : RenderStage = renderFlowComponent:NewRenderStage(computePass and "DeferredLightingBufferEncodedRenderStage" or "DeferredLightingTextureEncodedRenderStage", "")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "BaseColorOcclusion", stageDeferredLighting, "BaseColorOcclusion")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "NormalRoughnessMetallic", stageDeferredLighting, "NormalRoughnessMetallic")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "Depth", stageDeferredLighting, "Depth")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "Depth", stageDeferredLighting, "LoadDepth")

	-- Step 2: Setup SSR
	local stageScreenSpaceTrace : RenderStage = renderFlowComponent:NewRenderStage("ScreenSpaceTraceRenderStage", "")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "CameraView", stageScreenSpaceTrace, "CameraView")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "Depth", stageScreenSpaceTrace, "LoadDepth")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "Depth", stageScreenSpaceTrace, "Depth")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "NormalRoughnessMetallic", stageScreenSpaceTrace, "Normal")

	-- Step 3: Set Light
	local tileLevel = 3
	local stageDepthBoundingSetup : RenderStage = renderFlowComponent:NewRenderStage("DepthBoundingSetupRenderStage", "1")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "Depth", stageDepthBoundingSetup, "InputDepth")

	local lastDepthStage = stageDepthBoundingSetup
	for i = 2, tileLevel do
		local stageDepthBounding : RenderStage = renderFlowComponent:NewRenderStage("DepthBoundingRenderStage", tostring(i))
		renderFlowComponent:LinkRenderStagePort(lastDepthStage, "OutputDepth", stageDepthBounding, "InputDepth")
		lastDepthStage = stageDepthBounding
	end

	local stageColorBarrier : RenderStage = renderFlowComponent:NewRenderStage("FrameBarrierRenderStage", "")
	local stageLightBuffer : RenderStage = renderFlowComponent:NewRenderStage(computePass and "LightBufferEncodeRenderStage" or "LightTextureEncodeRenderStage", tostring(tileLevel))
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "CameraView", stageLightBuffer, "CameraView")
	renderFlowComponent:LinkRenderStagePort(lastDepthStage, "OutputDepth", stageLightBuffer, "InputDepth")
	renderFlowComponent:LinkRenderStagePort(stageLightBuffer, computePass and "LightBuffer" or "LightTexture", stageDeferredLighting, computePass and "LightBuffer" or "LightTexture")
	renderFlowComponent:LinkRenderStagePort(stageLightBuffer, "LightSource", stageDeferredLighting, "LightSource")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "CameraView", stageDeferredLighting, "CameraView")
	renderFlowComponent:LinkRenderStagePort(stageScreenSpaceTrace, "ScreenCoord", stageDeferredLighting, "ReflectCoord")
	renderFlowComponent:LinkRenderStagePort(stageColorBarrier, "Next", stageDeferredLighting, "LastInputColor")
	renderFlowComponent:LinkRenderStagePort(stageLightBuffer, "LightSource", stageScreenSpaceTrace, "LightSource")

	local lastShadowMask : RenderStage = nil as RenderStage
	for i = 1, 3 do
		local stageShadowMask : RenderStage = renderFlowComponent:NewRenderStage("ShadowMaskRenderStage", tostring(i - 1))
		renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "CameraView", stageShadowMask, "CameraView")
		renderFlowComponent:LinkRenderStagePort(stageLightBuffer, "LightSource", stageShadowMask, "LightSource")
		renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "Depth", stageShadowMask, "InputDepth")

		if lastShadowMask then
			renderFlowComponent:LinkRenderStagePort(lastShadowMask, "OutputMask", stageShadowMask, "InputMask")
			renderFlowComponent:LinkRenderStagePort(lastShadowMask, "MoveDepth", stageShadowMask, "LoadDepth")
		else
			renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "Depth", stageShadowMask, "LoadDepth")
		end

		if i == 3 then
			renderFlowComponent:LinkRenderStagePort(stageShadowMask, "OutputMask", stageDeferredLighting, "ShadowTexture")
		end

		lastShadowMask = stageShadowMask
	end

	-- Step 4: Forward shading
	local stageForwardLighting = renderFlowComponent:NewRenderStage("ForwardLightingRenderStage", "")
	renderFlowComponent:LinkRenderStagePort(stageDeferredLighting, "OutputColor", stageForwardLighting, "InputColor")
	renderFlowComponent:LinkRenderStagePort(lastShadowMask, "MoveDepth", stageForwardLighting, "InputDepth")

	-- Step 5: Anti-Aliasing
	local stageAntiAliasing : RenderStage = renderFlowComponent:NewRenderStage("AntiAliasingRenderStage", "")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "CameraView", stageAntiAliasing, "CameraView")
	if not debugSSR then
		renderFlowComponent:LinkRenderStagePort(stageForwardLighting, "OutputColor", stageAntiAliasing, "InputColor")
	end

	renderFlowComponent:LinkRenderStagePort(stageAntiAliasing, "OutputColor", stageColorBarrier, "Front") -- chain
	renderFlowComponent:LinkRenderStagePort(stageColorBarrier, "Next", stageAntiAliasing, "LastInputColor")
	renderFlowComponent:LinkRenderStagePort(stageForwardLighting, "OutputDepth", stageAntiAliasing, "Depth")

	-- Step 6: Apply HDR effects
	local stageScreen : RenderStage = renderFlowComponent:NewRenderStage("ScreenRenderStage", strBloomLayer)
	renderFlowComponent:LinkRenderStagePort(stageAntiAliasing, "OutputColor", stageScreen, "InputColor")
	local screenMaterial = SnowyStream.NewResource("", "MaterialResource", true)
	SnowyStream.ModifyResource(screenMaterial, "Set", {
		"invAverageLuminance", 1.0,
		"bloomIntensity", { 0.2, 0.3, 0.3 }
	})
	renderFlowComponent:OverrideRenderStageMaterial(stageScreen, screenMaterial as MaterialResource)

	-- Step 7: Prepare Bloom
	local lastStage = stageAntiAliasing
	for i = 0, bloomLayerCount - 1 do
		local stageBloom : RenderStage = renderFlowComponent:NewRenderStage("BloomRenderStage", tostring(i + 1))
		renderFlowComponent:LinkRenderStagePort(lastStage, "OutputColor", stageBloom, "InputColor")

		if i == 0 then
			renderFlowComponent:ExportRenderStagePort(stageBloom, "OutputColor", "PrevSceneColor")
		end

		renderFlowComponent:LinkRenderStagePort(stageBloom, "OutputColor", stageScreen, "BloomLayers[" .. tostring(i) .. "]")
		lastStage = stageBloom
	end

	-- Step 8: Adding UIs
	local stageWidget : RenderStage = renderFlowComponent:NewRenderStage("WidgetRenderStage", "")
	renderFlowComponent:LinkRenderStagePort(stageScreen, "OutputColor", stageWidget, "InputColor")

	-- Step 9: Write to output device
	local stageDevice : RenderStage = renderFlowComponent:NewRenderStage("DeviceRenderStage", "")
	renderFlowComponent:LinkRenderStagePort(stageWidget, "OutputColor", stageDevice, "InputColor")

	-- Step 10: Export symbols, notice that we only care abount scenes
	renderFlowComponent:ExportRenderStagePort(stageLightBuffer, "LightSource", "LightSource")
	renderFlowComponent:ExportRenderStagePort(stageGeometryBuffer, "Primitives", "Primitives")
	renderFlowComponent:ExportRenderStagePort(stageGeometryBuffer, "CameraView", "CameraView")
	renderFlowComponent:ExportRenderStagePort(stageForwardLighting, "Primitives", "DirectPrimitives")
	renderFlowComponent:ExportRenderStagePort(stageScreen, "OutputColor", "SceneColor")
	renderFlowComponent:ExportRenderStagePort(stageWidget, "Widgets", "Widgets")

	-- Debug: test screen space trace
	if debugSSR then
		renderFlowComponent:LinkRenderStagePort(stageScreenSpaceTrace, "ScreenCoord", stageAntiAliasing, "InputColor")
	end
	
	return renderFlowComponent
end

return StandardRenderFlow