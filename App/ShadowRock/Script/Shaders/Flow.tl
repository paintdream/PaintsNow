return {
	Name = "Flow",
	Uniforms = {
		flowSpeed = { 1.0 },
	},
	Textures = {
		baseTexture = {}
	},
	Code = [[
		float4 baseColor = texture(baseTexture, texCoord);
	]]
}