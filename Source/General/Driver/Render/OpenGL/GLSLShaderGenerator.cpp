#include "GLSLShaderGenerator.h"
#include <sstream>
using namespace PaintsNow;

// Shader Compilation
static const String frameCode = "#define PI 3.1415926 \n\
#define GAMMA 2.2 \n\
#define clip(f) if (f < 0) discard; \n\
#define constexpr \n\
#define uint2 uvec2 \n\
#define uint3 uvec3 \n\
#define uint4 uvec4 \n\
#define int2 ivec2 \n\
#define int3 ivec3 \n\
#define int4 ivec4 \n\
#define float2 vec2 \n\
#define float3 vec3 \n\
#define float4 vec4 \n\
#define float3x3 mat3 \n\
#define float4x4 mat4 \n\
#define make_float3x3(a11, a12, a13, a21, a22, a23, a31, a32, a33) float3x3(a11, a12, a13, a21, a22, a23, a31, a32, a33) \n\
#define make_float4x4(a11, a12, a13, a14, a21, a22, a23, a24, a31, a32, a33, a34, a41, a42, a43, a44) float4x4(a11, a12, a13, a14, a21, a22, a23, a24, a31, a32, a33, a34, a41, a42, a43, a44) \n\
#define lerp(a, b, v) mix(a, b, v) \n\
#define ddx dFdx \n\
#define ddy dFdy \n\
#define WorkGroupSize gl_WorkGroupSize \n\
#define NumWorkGroups gl_NumWorkGroups \n\
#define LocalInvocationID gl_LocalInvocationID \n\
#define WorkGroupID gl_WorkGroupID \n\
#define GlobalInvocationID gl_GlobalInvocationID \n\
#define LocalInvocationIndex gl_LocalInvocationIndex \n\
#define textureShadow texture \n\
float3 unprojection(float4 unproj, float3 v) { \n\
    float nz = unproj.w / (unproj.z + v.z); \n\
	float2 uv = unproj.xy * v.xy * nz; \n\
	return float3(uv.x, uv.y, -nz); \n\
} \n\
float4 projection(float4 proj, float3 v) { return float4(proj.x * v.x, proj.y * v.y, proj.z * v.z + proj.w, -v.z); } \n\
float saturate(float x) { return clamp(x, 0.0, 1.0); } \n\
float2 saturate(float2 x) { return clamp(x, float2(0.0, 0.0), float2(1.0, 1.0)); } \n\
float3 saturate(float3 x) { return clamp(x, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0)); } \n\
float4 saturate(float4 x) { return clamp(x, float4(0.0, 0.0, 0.0, 0.0), float4(1.0, 1.0, 1.0, 1.0)); } \n\
#define mult_mat(a, b) (a * b) \n\
#define mult_vec(m, v) (m * v) \n";

static String GetMemorySpec(IShader::MEMORY_SPEC spec) {
	switch (spec) {
		case IShader::MEMORY_DEFAULT:
			return "";
		case IShader::MEMORY_COHERENT:
			return "coherent ";
		case IShader::MEMORY_VOLATILE:
			return "volatile ";
		case IShader::MEMORY_RESTRICT:
			return "restrict ";
		case IShader::MEMORY_READONLY:
			return "readonly ";
		case IShader::MEMORY_WRITEONLY:
			return "writeonly ";
		default:
			return "";
	}
}

GLSLShaderGenerator::GLSLShaderGenerator(IRender::Resource::ShaderDescription::Stage s, uint32_t& pinputIndex, uint32_t& poutputIndex, uint32_t& pbindingIndex)
	: IReflect(true, true), stage(s), debugVertexBufferIndex(0), inputIndex(pinputIndex), outputIndex(poutputIndex), bindingIndex(pbindingIndex), propertyLevel(0),
	forceLayout(false), vulkanNDC(false) {}

const String& GLSLShaderGenerator::GetFrameCode() {
	return frameCode;
}

String GLSLShaderGenerator::FormatCode(const String& input) {
	String result;
	result.reserve(input.size() * 12 / 10);

	for (size_t i = 0; i < input.size(); i++) {
		char ch = input[i];
		result += ch;
		if ((ch == '{' || ch == ';') && input[i + 1] != '\n') {
			result += '\n';
		}
	}

	return result;
}


static inline String ToString(uint32_t value) {
	std::stringstream ss;
	ss << value;
	return StdToUtf8(ss.str());
}

void GLSLShaderGenerator::Complete() {
	for (size_t i = 0; i < bufferBindings.size(); i++) {
		const IShader::BindBuffer* buffer = bufferBindings[i].pointer;
		uint32_t binding = bufferBindings[i].binding;
		const std::pair<String, String>& info = mapBufferDeclaration[buffer];
		switch (buffer->description.state.usage) {
			case IRender::Resource::BufferDescription::VERTEX:
			case IRender::Resource::BufferDescription::INSTANCED:
				declaration += info.second;
				break;
			case IRender::Resource::BufferDescription::UNIFORM:
				if (!info.second.empty()) {
					declaration += "layout (std140";
					if (forceLayout) {
						declaration += ", binding = ";
						declaration += ToString(binding);
					}

					declaration += String(") uniform _") + info.first + " {\n" + info.second + "} " + info.first + "; \n";
				}
				break;
			case IRender::Resource::BufferDescription::STORAGE:
				if (!info.second.empty()) {
					declaration += GetMemorySpec(buffer->memorySpec);
					declaration += "layout (std430";
					if (forceLayout) {
						declaration += ", binding = ";
						declaration += ToString(binding);
					}

					declaration += String(") buffer _") + info.first + " {\n" + info.second + "} " + info.first + ";\n";
				}
				break;
		}
	}
}

struct DeclareMap {
	DeclareMap() {
		mapTypeNames[UniqueType<int>::Get()] = "int";
		mapTypeNames[UniqueType<float>::Get()] = "float";
		mapTypeNames[UniqueType<Float2>::Get()] = "float2";
		mapTypeNames[UniqueType<Float3>::Get()] = "float3";
		mapTypeNames[UniqueType<Float4>::Get()] = "float4";
		mapTypeNames[UniqueType<uint32_t>::Get()] = "uint";
		mapTypeNames[UniqueType<UInt2>::Get()] = "uvec2";
		mapTypeNames[UniqueType<UInt3>::Get()] = "uvec3";
		mapTypeNames[UniqueType<UInt4>::Get()] = "uvec4";
		mapTypeNames[UniqueType<MatrixFloat3x3>::Get()] = "float3x3";
		mapTypeNames[UniqueType<MatrixFloat4x4>::Get()] = "float4x4";
	}

	String operator [] (Unique id) {
		std::map<Unique, String>::iterator it = mapTypeNames.find(id);
		if (it != mapTypeNames.end()) {
			return it->second;
		} else {
			return id->GetBriefName();
		}
	}

	std::map<Unique, String> mapTypeNames;

	static DeclareMap& GetInstance() {
		return TSingleton<DeclareMap>::Get();
	}
};

static String GetTextureFormatString(IRender::Resource::TextureDescription::State state) {
	const char* prefixes[] = { "r", "rg", "rgb", "rgba" };
	String p;
	String bit = "32";
	String f = "";
	switch (state.format) {
		case IRender::Resource::Description::UNSIGNED_BYTE:
			bit = "8";
			break;
		case IRender::Resource::Description::UNSIGNED_SHORT:
			bit = "16";
			break;
		case IRender::Resource::Description::HALF:
			bit = "16";
			f = "f";
			break;
		case IRender::Resource::Description::UNSIGNED_INT:
			bit = "32";
			break;
		case IRender::Resource::Description::FLOAT:
			bit = "32";
			f = "f";
			break;
	}

	if (state.layout <= IRender::Resource::TextureDescription::RGBA) {
		p = prefixes[state.layout];
		return String(p) + bit + f;
	} else {
		if (state.layout == IRender::Resource::TextureDescription::RGB10PACK) {
			return f.empty() ? "rgb10_a2" : "r11f_g11f_b10f";
		} else {
			return String("r") + bit + f;
		}
	}
}

static String GetSamplerTypeDeclaration(IRender::Resource::ShaderDescription::Stage stage, const IShader::BindTexture* bindTexture) {
	static const char* samplerTypes[] = {
		"sampler1D", "sampler2D", "samplerCube", "sampler3D"
	};

	// TODO: support integer textures
	static const char* samplerTypesCS[] = {
		"image1D", "image2D", "imageCube", "image3D"
	};

	String declaration = (stage == IRender::Resource::ShaderDescription::COMPUTE ? samplerTypesCS : samplerTypes)[bindTexture->description.state.type];
	declaration += (bindTexture->description.dimension.z() != 0 && bindTexture->description.state.type != IRender::Resource::TextureDescription::TEXTURE_3D ? "Array" : "");
	declaration += (bindTexture->description.state.pcf ? "Shadow " : " ");

	return declaration;
}

void GLSLShaderGenerator::Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
	// singleton Unique uniqueBindOffset = UniqueType<IShader::BindOffset>::Get();
	singleton Unique uniqueBindInput = UniqueType<IShader::BindInput>::Get();
	singleton Unique uniqueBindOutput = UniqueType<IShader::BindOutput>::Get();
	singleton Unique uniqueBindTexture = UniqueType<IShader::BindTexture>::Get();
	singleton Unique uniqueBindBuffer = UniqueType<IShader::BindBuffer>::Get();
	singleton Unique uniqueBindConstBool = UniqueType<IShader::BindConst<bool> >::Get();
	singleton Unique uniqueBindConstInt = UniqueType<IShader::BindConst<int> >::Get();
	singleton Unique uniqueBindConstFloat = UniqueType<IShader::BindConst<float> >::Get();
	DeclareMap& declareMap = DeclareMap::GetInstance();

	if (s.IsBasicObject() || s.IsIterator()) {
		String statement;
		const IShader::BindBuffer* bindBuffer = nullptr;
		String arr;
		String arrDef;
		singleton Unique uniqueBindOption = UniqueType<IShader::BindEnable>::Get();
		singleton Unique uniqueBool = UniqueType<bool>::Get();
		bool isBoolean = typeID == uniqueBool;

		for (const MetaChainBase* pre = meta; pre != nullptr; pre = pre->GetNext()) {
			const MetaNodeBase* node = pre->GetNode();
			Unique uniqueNode = node->GetUnique();
			if (!isBoolean && uniqueNode == uniqueBindOption) {
				const IShader::BindEnable* bindOption = static_cast<const IShader::BindEnable*>(pre->GetNode());
				if (!*bindOption->description) {
					// defines as local
					if (s.IsIterator()) {
						IIterator& iterator = static_cast<IIterator&>(s);
						initialization += String("\t") + declareMap[iterator.GetElementUnique()] + " " + name + "[1];\n";
					} else {
						initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
					}

					return;
				}
			}

			// Bind directly
			if (uniqueNode == uniqueBindBuffer) {
				bindBuffer = static_cast<const IShader::BindBuffer*>(pre->GetRawNode());
			}
		}

		if (s.IsIterator()) {
			IIterator& iterator = static_cast<IIterator&>(s);
			int count = (int)iterator.GetTotalCount();
			if (count == 0) {
				arr = "[]";
			} else {
				arr = String("[") + ToString(count) + "]";
			}
			arrDef = "[]";

			typeID = iterator.GetElementUnique();
		}

		for (const MetaChainBase* chain = meta; chain != nullptr; chain = chain->GetNext()) {
			const MetaNodeBase* node = chain->GetNode();
			Unique uniqueNode = node->GetUnique();

			// Bind directly
			if (uniqueNode == uniqueBindInput) {
				const IShader::BindInput* bindInput = static_cast<const IShader::BindInput*>(node);
				if (bindInput->description == IShader::BindInput::COMPUTE_GROUP) {
					assert(bindBuffer == nullptr);
					// get dimension data
					assert(typeID == UniqueType<UInt3>::Get());
					if (typeID == UniqueType<UInt3>::Get()) {
						const UInt3& data = *reinterpret_cast<UInt3*>(ptr);
						std::stringstream ss;
						ss << "layout (local_size_x = " << data.x() << ", local_size_y = " << data.y() << ", local_size_z = " << data.z() << ") in;\n";
						declaration += StdToUtf8(ss.str());
					}
				} else if (bindInput->description == IShader::BindInput::LOCAL) {
					// Do not declare it here
					// initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
				} else if (bindInput->description == IShader::BindInput::RASTERCOORD) {
					initialization += String("\t") + declareMap[typeID] + " " + name + " = gl_FragCoord;\n";
				} else {
					if (bindBuffer == nullptr || mapBufferEnabled[bindBuffer]) {
						if (stage == IRender::Resource::ShaderDescription::VERTEX) {
							assert(bindBuffer != nullptr);
#ifdef _DEBUG
							if (bindBuffer->description.state.usage) {
								while (debugVertexBufferIndex < bufferBindings.size()) {
									if (bufferBindings[debugVertexBufferIndex].pointer == bindBuffer) {
										break;
									}

									debugVertexBufferIndex++;
								}

								// IShader::BindBuffer* must be with the same order as input varyings
								assert(debugVertexBufferIndex < bufferBindings.size());
							}
#endif

							switch (bindBuffer->description.state.usage) {
								case IRender::Resource::BufferDescription::VERTEX:
									assert(typeID->GetSize() <= 4 * sizeof(float));
									statement += String("layout (location = ") + ToString(inputIndex++) + ") in " + declareMap[typeID] + " " + name + ";\n";
									break;
								case IRender::Resource::BufferDescription::INSTANCED:
									assert(typeID->GetSize() < 4 * sizeof(float) || typeID->GetSize() % (4 * sizeof(float)) == 0);
									statement += String("layout (location = ") + ToString(inputIndex) + ") in " + declareMap[typeID] + " " + name + ";\n";
									inputIndex += ((uint32_t)verify_cast<uint32_t>(typeID->GetSize()) + sizeof(float) * 3) / (sizeof(float) * 4u);
									break;
							}
						} else {
							if (forceLayout) {
								statement += String("layout (location = ") + ToString(inputIndex) + ") ";
							}

							statement += "in " + declareMap[typeID] + " " + name + ";\n";
							inputIndex++;
						}
					} else {
						// Not enabled, fallback to local
						assert(bindBuffer->description.state.usage != IRender::Resource::BufferDescription::UNIFORM);
						initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
					}
				}
			} else if (uniqueNode == uniqueBindOutput) {
				const IShader::BindOutput* bindOutput = static_cast<const IShader::BindOutput*>(node);
				if (bindOutput->description == IShader::BindOutput::LOCAL) {
					initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
				} else if (bindOutput->description == IShader::BindOutput::HPOSITION) {
					assert(arr.empty());
					initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
					if (vulkanNDC) {
						finalization += String("\tvec4 vkPosition = ") + name + ";\n";
						finalization += String("\tgl_Position = vec4(vkPosition.x, -vkPosition.y, (vkPosition.z + vkPosition.w) / 2.0, vkPosition.w);\n");
					} else {
						finalization += String("\tgl_Position = ") + name + ";\n";
					}
				} else {
					if (stage == IRender::Resource::ShaderDescription::FRAGMENT || forceLayout) {
						statement += String("layout (location = ") + ToString(outputIndex++) + ") out " + declareMap[typeID] + " " + name + ";\n";
					} else {
						outputIndex++;
						statement += "out " + declareMap[typeID] + " " + name + ";\n";
					}
				}
			} else if (uniqueNode == uniqueBindConstBool) {
				const IShader::BindConst<bool>* bindOption = static_cast<const IShader::BindConst<bool>*>(node);
				declaration += String("#define ") + name + " " + (bindOption->description ? "true" : "false") + "\n";
				constants.emplace_back(IAsset::Material::Variable(name, bindOption->description));
			} else if (uniqueNode == uniqueBindConstInt) {
				const IShader::BindConst<int>* bindOption = static_cast<const IShader::BindConst<int>*>(node);
				std::stringstream ss;
				ss << bindOption->description;
				declaration += String("#define ") + name + " " + StdToUtf8(ss.str()) + "\n";
				constants.emplace_back(IAsset::Material::Variable(name, (uint32_t)bindOption->description));
			} else if (uniqueNode == uniqueBindConstFloat) {
				const IShader::BindConst<float>* bindOption = static_cast<const IShader::BindConst<float>*>(node);
				std::stringstream ss;
				ss << bindOption->description;
				declaration += String("#define ") + name + " " + StdToUtf8(ss.str()) + "\n";
				constants.emplace_back(IAsset::Material::Variable(name, bindOption->description));
			}
		}

		if (bindBuffer != nullptr && mapBufferEnabled[bindBuffer]) {
			std::map<const IShader::BindBuffer*, std::pair<String, String> >::iterator it = mapBufferDeclaration.find(bindBuffer);
			assert(it != mapBufferDeclaration.end());
			if (it != mapBufferDeclaration.end()) {
				switch (bindBuffer->description.state.usage) {
					case IRender::Resource::BufferDescription::VERTEX:
					case IRender::Resource::BufferDescription::INSTANCED:
						assert(!statement.empty());
						it->second.second += statement;
						break;
					case IRender::Resource::BufferDescription::UNIFORM:
					case IRender::Resource::BufferDescription::STORAGE:
						it->second.second += String("\t") + declareMap[typeID] + " _" + name + arr + ";\n";
						initialization += String("#define ") + name + " " + it->second.first + "." + "_" + name + "\n";
						break;
				}
			} else {
				// these buffer usage do not support structure-mapped layout.
				assert(bindBuffer->description.state.usage != IRender::Resource::BufferDescription::VERTEX);
				assert(bindBuffer->description.state.usage != IRender::Resource::BufferDescription::INSTANCED);
				declaration += statement;
			}
		} else {
			if (propertyLevel != 0) {
				declaration += String("\t") + declareMap[typeID] + " " + name + ";\n";
			} else {
				declaration += statement;
			}
		}
	} else {
		bool enabled = true;
		const IShader::BindBuffer* bindBuffer = nullptr;
		for (const MetaChainBase* check = meta; check != nullptr; check = check->GetNext()) {
			const MetaNodeBase* node = check->GetNode();
			if (node->GetUnique() == UniqueType<IShader::BindEnable>::Get()) {
				const IShader::BindEnable* bind = static_cast<const IShader::BindEnable*>(node);
				if (!*bind->description) {
					enabled = false;
				}
			} else if (node->GetUnique() == uniqueBindBuffer) {
				bindBuffer = static_cast<const IShader::BindBuffer*>(check->GetRawNode());
			}
		}

		if (typeID == uniqueBindBuffer) {
			const IShader::BindBuffer* bindBuffer = static_cast<const IShader::BindBuffer*>(&s);
			if (bindBuffer->description.state.usage != IRender::Resource::BufferDescription::UNIFORM || enabled) {
				mapBufferDeclaration[bindBuffer] = std::make_pair(String(name), String(""));
				mapBufferEnabled[bindBuffer] = enabled;
				if ((bindBuffer->description.state.usage == IRender::Resource::BufferDescription::UNIFORM) || (bindBuffer->description.state.usage == IRender::Resource::BufferDescription::STORAGE)) {
					bufferBindings.emplace_back(Binding<IShader::BindBuffer>(bindBuffer, bindingIndex, String("_") + name));
				} else {
					bufferBindings.emplace_back(Binding<IShader::BindBuffer>(bindBuffer, bindingIndex, name));
				}

				bindingIndex++;
			}
		} else if (typeID == uniqueBindTexture) {
			assert(enabled);
			const IShader::BindTexture* bindTexture = static_cast<const IShader::BindTexture*>(&s);

			if (stage == IRender::Resource::ShaderDescription::COMPUTE) {
				declaration += "layout (";
				declaration += GetTextureFormatString(bindTexture->description.state);
				if (forceLayout) {
					declaration += ", binding = ";
					declaration += ToString(bindingIndex);
				}

				declaration += ") ";
				declaration += GetMemorySpec(bindTexture->memorySpec);
			} else if (forceLayout) {
				declaration += "layout (binding = ";
				declaration += ToString(bindingIndex);
				declaration += ") ";
			}

			declaration += "uniform ";
			declaration += GetSamplerTypeDeclaration(stage, bindTexture);
			declaration += name;
			declaration += ";\n";

			textureBindings.emplace_back(Binding<IShader::BindTexture>(bindTexture, bindingIndex, name));
			bindingIndex++;
		} else {
			String typeName = typeID->GetBriefName();
			std::map<String, String>::iterator it = mapStructureDefinition.find(typeName);
			if (it == mapStructureDefinition.end()) {
				// custom structure?
				propertyLevel++;

				// save current state
				String structDeclaration;
				String structInitialization;
				String structFinalization;

				std::swap(structDeclaration, declaration);
				std::swap(structInitialization, initialization);
				std::swap(structFinalization, finalization);

				s(*this);

				std::swap(structDeclaration, declaration);
				std::swap(structInitialization, initialization);
				std::swap(structFinalization, finalization);

				propertyLevel--;

				// merge declaration
				// add a struct
				String def = "struct ";
				def += typeName;
				def += " {\n";
				def += structDeclaration;
				def += "};\n\n";

				mapStructureDefinition[typeName] = std::move(def);
				structures.emplace_back(typeName);
			}

			if (propertyLevel == 0) {
				assert(bindBuffer != nullptr);
				std::map<const IShader::BindBuffer*, std::pair<String, String> >::iterator it = mapBufferDeclaration.find(bindBuffer);
				assert(it != mapBufferDeclaration.end());
				if (it != mapBufferDeclaration.end()) {
					switch (bindBuffer->description.state.usage) {
						case IRender::Resource::BufferDescription::VERTEX:
						case IRender::Resource::BufferDescription::INSTANCED:
							assert(false); // not supported
							break;
						case IRender::Resource::BufferDescription::UNIFORM:
						case IRender::Resource::BufferDescription::STORAGE:
							it->second.second += String("\t") + declareMap[typeID] + " _" + name + ";\n";
							initialization += String("#define ") + name + " " + it->second.first + "." + "_" + name + "\n";
							break;
					}
				}
			} else {
				declaration += String("\t") + typeName + " " + name + ";\n";
			}
		}
	}
}

void GLSLShaderGenerator::Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {
	std::vector<String> names;
	std::vector<void*> protos;
	String codeString;

	while (meta != nullptr) {
		MetaNodeBase* node = const_cast<MetaNodeBase*>(meta->GetNode());
		MetaParameter* parameter = node->QueryInterface(UniqueType<MetaParameter>());
		if (parameter != nullptr) {
			names.emplace_back(parameter->value);
			protos.emplace_back(parameter->prototype);
		} else {
			IShader::BindFunction* bindFunction = node->QueryInterface(UniqueType<IShader::BindFunction>());
			if (bindFunction != nullptr) {
				codeString = bindFunction->codeString;
			}
		}

		meta = meta->GetNext();
	}

	if (!codeString.empty()) {
		DeclareMap& declareMap = DeclareMap::GetInstance();

		String functionDeclaration;
		functionDeclaration += "void ";
		functionDeclaration += name;
		functionDeclaration += "(";

		assert(names.size() == params.size()); // must provide all names!

		for (size_t i = 0; i < params.size(); i++) {
			const Param& param = params[i];
			if (i != 0) functionDeclaration += ", ";

			if (param.decayType == UniqueType<IShader::BindTexture>::Get()) {
				IShader::BindTexture* bindTexture = reinterpret_cast<IShader::BindTexture*>(protos[protos.size() - 1 - i]);
				assert(bindTexture != nullptr);
				functionDeclaration += GetSamplerTypeDeclaration(stage, bindTexture) + names[params.size() - 1 - i];
			} else {
				functionDeclaration += (param.isReference && !param.isConst ? String("inout ") : String("")) + declareMap[param.decayType] + " " + names[params.size() - 1 - i];
			}
		}

		functionDeclaration += ") {\n";
		functionDeclaration += codeString;
		functionDeclaration += "\n}\n\n";

		declaration += functionDeclaration;
	}
}
