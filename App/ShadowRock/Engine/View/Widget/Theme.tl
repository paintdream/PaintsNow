local Style = require("Engine/View/Widget/Style")
local Slice = require("Engine/View/Widget/Slice")

local Theme = record
	styles : { string : Style }
	slices : { string : Slice }
end

return Theme