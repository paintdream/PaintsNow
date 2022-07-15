local GUI = record
	spaceComponent : SpaceComponent
	rootEntity : Entity
	rayTraceWidget : WidgetComponent
	textView : TextViewComponent
	__index : any
end

GUI.__index = GUI

function GUI.New() : GUI
	local meta = {} as GUI
	local rootEntity = MythForest.NewEntity(-1)
	local spaceComponent = SpaceComponentModule.New(-1, true)
	rootEntity:AddEntityComponent(spaceComponent as Component)
	meta.spaceComponent = spaceComponent
	meta.rootEntity = rootEntity

	local renderPolicy = TextViewComponentModule.NewRenderPolicy("Widgets", 0, 1)

	local widgetEntity = MythForest.NewEntity(-1)
	local fontResource = SnowyStream.NewResource("/Packed/Font/Cantarell-Regular", "FontResource", false)
	local textViewComponent = TextViewComponentModule.New(fontResource as FontResource, nil as MeshResource, nil as MaterialResource, nil as BatchComponent)
	textViewComponent:AddRenderPolicy(renderPolicy)
	textViewComponent:SetText("Hello, world!")
	meta.textView = textViewComponent
	widgetEntity:AddEntityComponent(textViewComponent as Component)

	local widgetComponent = WidgetComponentModule.New(nil as BatchComponent, nil as MaterialResource, nil as TextureResource)
	widgetComponent:SetVisible(false)
	widgetComponent:AddRenderPolicy(renderPolicy)
	meta.rayTraceWidget = widgetComponent
	widgetEntity:AddEntityComponent(widgetComponent as Component)

	spaceComponent:InsertEntity(widgetEntity)

	return setmetatable(meta, GUI as METATABLE) as GUI
end

return GUI