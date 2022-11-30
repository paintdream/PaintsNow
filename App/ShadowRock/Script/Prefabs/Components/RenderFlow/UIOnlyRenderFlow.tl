-- UIOnlyRenderFlow.tl
-- 
local UIOnlyRenderFlow = {}
local Debug = require("Script/Util/Debug")

function UIOnlyRenderFlow.New() : RenderFlowComponent
	local renderFlowComponent = RenderFlowComponentModule.New()
	local stageGeometryBuffer : RenderStage = renderFlowComponent:NewRenderStage("GeometryBufferRenderStage", "")
	local stageWidget : RenderStage = renderFlowComponent:NewRenderStage("WidgetRenderStage", "")
	local stageDevice : RenderStage = renderFlowComponent:NewRenderStage("DeviceRenderStage", "")

	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "BaseColorOcclusion", stageWidget, "InputColor")
	renderFlowComponent:LinkRenderStagePort(stageWidget, "OutputColor", stageDevice, "InputColor")

	-- renderFlowComponent:ExportRenderStagePort(stageGeometryBuffer, "Primitives", "Primitives")
	renderFlowComponent:ExportRenderStagePort(stageGeometryBuffer, "CameraView", "CameraView")
	renderFlowComponent:ExportRenderStagePort(stageWidget, "Widgets", "Widgets")
	
	return renderFlowComponent
end

return UIOnlyRenderFlow