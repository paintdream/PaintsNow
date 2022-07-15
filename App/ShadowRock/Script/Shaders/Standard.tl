return {
	Template = "[Runtime]/ShaderResource/CustomMaterialPass",
	Name = "PBR",
	Inputs = {
		-- VertexInstances
		{
			Name = "WorldMatrix",
			Stages = { "VertexInstance" },
			Value = {
				1.0, 0.0, 0.0, 0.0
				0.0, 1.0, 0.0, 0.0
				0.0, 0.0, 1.0, 0.0
				0.0, 0.0, 0.0, 1.0
			},
			Binding = "TRANSFORM_WORLD"
		},
		-- VertexInputs
		{
			Name = "Position"
			Stages = { "VertexInput" },
			Value = { 0.0, 0.0, 0.0 }, -- float3
			Binding = "POSITION",
		},
		{
			Name = "Color",
			Stages = { "VertexInput" },
			Value = { 1.0, 1.0, 1.0, 1.0 }, -- float4
			Binding = "COLOR",
		},
		{
			Name = "Normal",
			Stages = { "VertexInput" },
			Value = { 0.0, 0.0, 1.0, 0.0 }, -- float4
			Binding = "NORMAL",
		},
		{
			Name = "Tangent",
			Stages = { "VertexInput" },
			Value = { 1.0, 0.0, 0.0, 0.0 },
			Binding = "TANGENT"
		},
		{
			Name = "TexCoord",
			Stages = { "VertexInput" },
			Value = { 0.0, 0.0, 0.0, 0.0 } -- float4
			Binding = "TEXCOORD0"
		},

		-- Uniforms
		{
			Name = "ViewMatrix",
			Stages = { "VertexUniform" },
			Value = {
				1.0, 0.0, 0.0, 0.0,
				0.0, 1.0, 0.0, 0.0,
				0.0, 0.0, 1.0, 0.0,
				0.0, 0.0, 0.0, 1.0
			},
			Binding = "TRANSFORM_VIEW"
		},
		{
			Name = "ViewProjectionMatrix",
			Stages = { "VertexUniform" },
			Value = {
				1.0, 0.0, 0.0, 0.0,
				0.0, 1.0, 0.0, 0.0,
				0.0, 0.0, 1.0, 0.0,
				0.0, 0.0, 0.0, 1.0
			},
			Binding = "TRANSFORM_VIEWPROJECTION"
		},

		-- Options
		{
			Name = "EnableVertexNormal"
			Stages = { "VertexOptions", "FragmentOptions" },
			Value = true,
			Binding = "Normal",
		},
		{
			Name = "EnableVertexTangent"
			Stages = { "VertexOptions", "FragmentOptions" },
			Value = true,
			Binding = "Tangent"
		},
		{
			Name = "EnableVertexColor"
			Stages = { "VertexOptions", "FragmentOptions" },
			Value = true,
			Binding = "Color"
		},

		-- Varyings
		{
			Name = "OutputPosition",
			Stages = { "VertexOutput" },
			Value = { 0.0, 0.0, 0.0, 1.0 },
			Binding = "POSITION"
		},
		{
			Name = "VaryingTexCoord",
			Stages = { "VertexVarying" },
			Value = { 0.0, 0.0, 0.0, 0.0 },
			Binding = ""
		},
		{
			Name = "VaryingNormal",
			Stages = { "VertexVarying" },
			Value = { 0.0, 0.0, 1.0 },
			Binding = "EnableVertexNormal"
		},
		{
			Name = "VaryingTangent",
			Stages = { "VertexVarying" },
			Value = { 0.0, 0.0, 1.0 },
			Binding = "EnableVertexTangent"
		},
		{
			Name = "VaryingBinormal",
			Stages = { "VertexVarying" },
			Value = { 0.0, 0.0, 1.0 },
			Binding = "EnableVertexTangent"
		},
		{
			Name = "VaryingColor",
			Stages = { "VertexVarying" },
			Value = { 1.0, 1.0, 1.0, 1.0 },
			Binding = "EnableVertexColor"
		},

		-- Textures
		{
			Name = "baseColorTexture",
			Stages = { "FragmentUniform" },
			Value = "[Runtime]/TextureResource/MissingBaseColor",
			Binding = ""
		},
		{
			Name = "normalTexture",
			Stages = { "FragmentUniform" },
			Value = "[Runtime]/TextureResource/MissingNormal",
			Binding = ""
		},
		{
			Name = "mixtureTexture",
			Stages = { "FragmentUniform" },
			Value = "[Runtime]/TextureResource/MissingMaterial",
			Binding = ""
		},

		-- Options
		{
			Name = "EnableBaseColorTexture",
			Stages = { "FragmentOptions" },
			Value = true,
			Binding = "baseColorTexture",
		},
		{
			Name = "EnableNormalTexture",
			Stages = { "FragmentOptions" },
			Value = true,
			Binding = "normalTexture"
		},
		{
			Name = "EnableMixtureTexture",
			Stages = { "FragmentOptions" },
			Value = true,
			Binding = "mixtureTexture"
		},
	},
	
	Shaders = {
		Vertex = [[
	float4 pos;
	pos.xyz = Position;
	pos.w = 1;

	pos = mult_vec(WorldMatrix, pos);
	OutputPosition = mult_vec(ViewProjectionMatrix, pos);
	VaryingTexCoord = TexCoord;

	if (EnableVertexColor) {
		VaryingColor = Color;
	}

	if (EnableVertexNormal) {
		float4 normal = Normal * 2.0 / 255.0 - float4(1.0, 1.0, 1.0, 1.0);
		float4x4 viewWorldMatrix = mult_mat(ViewMatrix, WorldMatrix);
		if (EnableVertexTangent) {
			float4 tangent = Tangent * 2.0 / 255.0 - float4(1.0, 1.0, 1.0, 1.0);
			VaryingTangent = mult_vec(float3x3(viewWorldMatrix), tangent.xyz);
			VaryingBinormal = mult_vec(float3x3(viewWorldMatrix), normal.xyz);
			VaryingNormal = cross(VaryingBinormal, VaryingTangent);
			VaryingBinormal *= tangent.w;
		}
		
		if (!EnableVertexTangent) {
			VaryingNormal = mult_vec(float3x3(viewWorldMatrix), normal.xyz);
		}
	}
		]],

		Fragment = [[
			if (EnableBaseColorTexture) {
				float4 color = texture(baseColorTexture, VaryingTexCoord.xy);
				if (EnableVertexColor) {
					outputColor = VaryingColor.xyz * color.xyz;
				} else {
					outputColor = color.xyz;
				}
				
				alpha = color.w;
			}

			if (EnableVertexTangent) {
				float4 bump = texture(normalTexture, VaryingTexCoord.xy);
				bump.xyz = bump.xyz * 2.0 - float3(1.0, 1.0, 1.0);
				outputNormal = normalize(VaryingTangent.xyz * bump.x + VaryingBinormal.xyz * bump.y + VaryingNormal.xyz * bump.z);
			}
			
			if (!EnableVertexTangent) {
				outputNormal = VaryingNormal.xyz;
			}

			if (EnableMixtureTexture) {
				float4 material = texture(mixtureTexture, VaryingTexCoord.xy);
				occlusion = material.x;
				roughness = material.y;
				metallic = material.z;
			}
		]]
	}
}