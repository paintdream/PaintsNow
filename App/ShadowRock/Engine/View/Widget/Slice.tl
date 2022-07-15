local Range = record
	x : number
	y : number
	w : number
	h : number
end

local Size = record
	w : number
	h : number
end

local Image = record
	path : string
	width : number
	height : number
end

local Edge = record
	left : number
	top : number
	right : number
	bottom : number
end

local Pattern = record
	repeatable : boolean
	edge : Edge
end

local Slice = record
	image : Image
	pattern : Pattern
	frame : Range
	spriteSourceSize : Range
	rotated : boolean
	trimmed : boolean
	sourceSize : Size
end

return Slice