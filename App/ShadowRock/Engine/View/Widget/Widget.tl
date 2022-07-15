-- Widget.tl
-- PaintDream (paintdream@paintdream.com)
--
local Theme = require("Engine/View/Widget/Theme")
local Style = require("Engine/View/Widget/Style")
local Slice = require("Engine/View/Widget/Slice")
local ResourceCache = require("Engine/View/ResourceCache")

local Widget = record
	name : string
	weight : number
	entity : Entity
	textViewComponent : TextViewComponent
	spaceComponent : SpaceComponent
	layoutComponent : LayoutComponent
	widgetComponent : WidgetComponent
	style : string
	slice : string
	theme : Theme
	childs : { any : Widget }
	__index : any
end

Widget.__index = Widget

local loader = require
local function LoadTheme(themePath : string) : Theme
	return loader(themePath) as Theme
end

Widget.theme = LoadTheme("Engine/Widget/Theme/PurpleLove")

local fontCache = ResourceCache.New("FontResource")
local textureCache = ResourceCache.New("TextureResource")

function Widget:GetStyle() : Style
	return self.theme.styles[self.style]
end

function Widget:GetSlice() : Slice
	if not self.slice then return nil as Slice end

	return self.theme.slices[self.slice]
end

function Widget:SetWeight(weight : number)
	self.weight = weight
	self.layoutComponent:SetWeight(self.weight)
end

function Widget:GetWeight() : number
	return self.weight
end

function Widget:SetText(text : string)
	local textViewComponent = self.textViewComponent
	if not textViewComponent then
		-- get font from theme
		local style = self:GetStyle()
		textViewComponent = TextViewComponentModule.New(fontCache:Load(style.font) as FontResource, nil as MeshResource, nil as MaterialResource, nil as BatchComponent)
		self.entity:AddEntityComponent(textViewComponent as Component)
		self.textViewComponent = textViewComponent
	end

	textViewComponent:SetText(text)
end

function Widget:AddChild(child : Widget)
	table.insert(self.childs as { Widget }, child)

	if child.name ~= "" then
		assert(not self.childs[child.name])
		self.childs[child.name] = child
	end

	if not self.spaceComponent then
		self.spaceComponent = SpaceComponentModule.New(-1, false)
	end

	self.spaceComponent:InsertEntity(child.entity)
end

function Widget:RemoveChild(child : Widget)
	local childs = self.childs as { Widget }
	for k, v in ipairs(childs) do
		if v == child then
			table.remove(childs, k)
			break
		end
	end

	if child.name ~= "" then
		self.childs[child.name] = nil as Widget
	end

	assert(self.spaceComponent)
	self.spaceComponent:RemoveEntity(child.entity)
end

function Widget:GetChild(...: any) : Widget
	local lists = { ... } as { any }
	local p : Widget = self
	for _, v in ipairs(lists) do
		p = p.childs[v]
		if p == nil as Widget then return nil as Widget end	
	end

	return p
end

function Widget:Update()
	local style = self:GetStyle()
	local layoutComponent = self.layoutComponent

	layoutComponent:SetPadding(style.padding)
	layoutComponent:SetMargin(style.margin)
	layoutComponent:SetSize(style.size)
	layoutComponent:SetWeight(self.weight)

	local slice = self:GetSlice()
	if slice then
		local widgetComponent = self.widgetComponent
		if not widgetComponent then
			widgetComponent = WidgetComponentModule.New(nil as BatchComponent, nil as MaterialResource, nil as TextureResource)
			self.widgetComponent = widgetComponent
		end

		local image = slice.image
		local frame = slice.frame
		local pattern = slice.pattern
		local edge = pattern.edge
		local texture = textureCache:Load(image.path) as TextureResource
		local w = image.width
		local h = image.height
		local outTexCoord = { frame.x / w, frame.y / h, (frame.x + frame.w) / w, (frame.y + frame.h) / h }
		local inTexCoord = { (frame.x + edge.left) / w, (frame.y - edge.bottom) / h, (frame.x + frame.w - edge.right) / w, (frame.y + frame.h - edge.top) / h }

		widgetComponent:SetWidgetTexture(texture)
		widgetComponent:SetWidgetCoord(inTexCoord, outTexCoord)
		widgetComponent:SetWidgetRepeatMode(pattern.repeatable)
	end
end

function Widget.New(style : string, slice : string, text : string) : Widget
	local widget = {
		entity = MythForest.NewEntity(-1)
		style = style,
		slice = slice,
		childs = {},
		layoutComponent = LayoutComponentModule.New()
	}

	local ret = setmetatable(widget, Widget as METATABLE) as Widget
	ret:Update()

	if text then
		ret:SetText(text)
	end

	return ret
end

function Widget.NewFromDocument(document : { any : any }) : Widget
	return {} as Widget
end

return Widget