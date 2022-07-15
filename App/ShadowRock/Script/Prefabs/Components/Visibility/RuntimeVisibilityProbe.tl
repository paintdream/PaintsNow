local RuntimeVisibilityProbe = record end

function RuntimeVisibilityProbe.New() : VisibilityComponent
	local streamComponent = StreamComponentModule.New({ 16, 16, 16 }, 64 )
	local visibilityComponent = VisibilityComponentModule.New(streamComponent)
	visibilityComponent:Setup(500, { 0.2, 0.2, 0.2 }, 10, 1, { 128, 128 })

	return visibilityComponent
end

return RuntimeVisibilityProbe