local StandardPhase = record end

function StandardPhase.New(renderFlowComponent : RenderFlowComponent) : PhaseComponent
	local phaseComponent = PhaseComponentModule.New(renderFlowComponent, "PhaseView")
	return phaseComponent
end

return StandardPhase