local CustomShader = record end

global ShaderInput = record
	Name : string
	Type : string
	Stages : { string }
	Value : any
	Binding : string
end

global ShaderDescription = record
	Template : string -- [Runtime]/ShaderResource/CustomMaterialPass
	Name : string
	Priority : number
	Shaders : { string : string }
	Inputs : { ShaderInput }
	States : { string : string }
end

local function FromValue(value : any) : (string, string)
	local t : string = type(value)
	if t == "string" then
		return "texture", value as string
	elseif t == "boolean" then
		return "bool", tostring(value)
	elseif t == "table" then
		local count = #(value as { number })
		local v = table.concat(value as { string }, " ")
		if count == 9 then return "float3x3", v
		elseif count == 16 then return "float4x4", v
		else return "float" .. count, v  end -- vector
	else
		return "float", tostring(value)
	end
end

function CustomShader.New(description : ShaderDescription) : ShaderComponent
	local shaderResource : ShaderResource = SnowyStream.NewResource(description.Template, "ShaderResource", false) as ShaderResource
	assert(shaderResource)

	local shaderComponent : ShaderComponent = ShaderComponentModule.New(shaderResource, description.Name)
	for _, value in ipairs(description.Inputs) do
		for _, stage in ipairs(value.Stages) do
			local t, v = FromValue(value.Value)
			shaderComponent:SetInput(stage, t, value.Name, v, value.Binding, {})
		end
	end

	for stage, text in pairs(description.Shaders) do
		shaderComponent:SetCode(stage, text, {})
	end

	return shaderComponent
end

return CustomShader