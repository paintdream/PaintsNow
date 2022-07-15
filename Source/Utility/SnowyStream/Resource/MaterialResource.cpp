#include "MaterialResource.h"
#include "../ResourceManager.h"
#include "../../../Core/Driver/Profiler/Optick/optick.h"
#include <sstream>

using namespace PaintsNow;

MaterialResource::MaterialResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {}

uint64_t MaterialResource::ComputeHashValue() const {
	uint64_t hash = 0;
	for (size_t i = 0; i < materialParams.variables.size(); i++) {
		const IAsset::Material::Variable& var = materialParams.variables[i];
		hash = HashBuffer(var.key.GetData(), var.key.GetSize(), hash);
		hash = HashBuffer(var.value.GetData(), var.value.GetSize(), hash);
	}

	return hash;
}

void MaterialResource::ApplyMaterial(const TShared<ShaderResource>& original) {
	PassBase& pass = original->GetPass();
	PassBase::Updater& updater = original->GetPassUpdater();

	// Apply material
	for (size_t i = 0; i < materialParams.variables.size(); i++) {
		const IAsset::Material::Variable& var = materialParams.variables[i];
		PassBase::Parameter& parameter = const_cast<PassBase::Parameter&>(updater[var.key]);
		if (parameter) {
			if (var.type == IAsset::TYPE_TEXTURE) {
				// Lookup texture 
				IAsset::TextureIndex textureIndex = var.Parse(UniqueType<IAsset::TextureIndex>());
				IRender::Resource* texture = nullptr;
				assert(textureIndex.value < textureResources.size());
				if (textureIndex.value < textureResources.size()) {
					TShared<TextureResource>& res = textureResources[textureIndex.value];
					assert(res);
					if (res) {
						texture = res->GetRenderResource();
					}
				}

				parameter = texture;
			} else if (var.type == IAsset::TYPE_TEXTURE_RUNTIME || var.type == IAsset::TYPE_BUFFER_RUNTIME) {
				assert(var.value.GetSize() == sizeof(IRender::Resource*));
				IRender::Resource* renderResource = *reinterpret_cast<IRender::Resource* const*>(var.value.GetData());
				parameter = renderResource;
			} else {
				parameter = var.value;
			}
		}
	}
}

void MaterialResource::Export(IRender& render, IRender::Queue* queue, const TShared<ShaderResource>& original) {
	Bytes templateHash = original->GetHashValue();
	PassBase& pass = original->GetPass();
	ApplyMaterial(original);

	// get shader hash
	pass.FlushOptions();
	Bytes shaderHash = pass.ExportHash();

	if (!(templateHash == shaderHash)) {
		original->Compile(render, queue, &shaderHash);
	}
}

TShared<ShaderResource> MaterialResource::Instantiate(const TShared<MeshResource>& mesh, IRender::Resource::DrawCallDescription& drawCallTemplate, std::vector<Bytes>& bufferData) {
	OPTICK_EVENT();
	assert(mesh);

	TShared<ShaderResource> shaderTemplateResource = originalShaderResource;
	Bytes templateHash = shaderTemplateResource->GetHashValue();

	while (true) {
		TShared<ShaderResource> mutationShaderResource;
		mutationShaderResource.Reset(static_cast<ShaderResource*>(shaderTemplateResource->Clone()));
		assert(mutationShaderResource->GetPassUpdater().GetBufferCount() != 0);
		PassBase& pass = mutationShaderResource->GetPass();
		PassBase::Updater& updater = mutationShaderResource->GetPassUpdater();
		mutationShaderResource->GetPass().ClearBindings();

		ApplyMaterial(mutationShaderResource);

		// apply buffers
		std::vector<PassBase::Parameter> descs;
		std::vector<std::pair<uint32_t, uint32_t> > offsets;
		mesh->bufferCollection.GetDescription(descs, offsets, updater);
		std::vector<IRender::Resource*> data;
		mesh->bufferCollection.UpdateData(data);
		assert(data.size() == descs.size());

		for (size_t k = 0; k < descs.size(); k++) {
			IShader::BindBuffer* bindBuffer = descs[k].bindBuffer;
			if (bindBuffer != nullptr) {
				bindBuffer->resource = data[k];
			}
		}

		// get shader hash
		pass.FlushOptions();
		Bytes shaderHash = pass.ExportHash();

		if (templateHash == shaderHash) { // matched!
			updater.Capture(drawCallTemplate, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
			for (size_t k = 0; k < descs.size(); k++) {
				assert(descs[k].slot < drawCallTemplate.bufferResources.size());
			}

			drawCallTemplate.shaderResource = shaderTemplateResource->GetShaderResource();
			assert(drawCallTemplate.shaderResource != nullptr);
			assert(shaderTemplateResource->GetPassUpdater().GetBufferCount() != 0);
			return shaderTemplateResource;
		} else {
			templateHash = shaderHash;
			// create new shader mutation
			String location = originalShaderResource->GetLocation();
			location += "/";
			for (size_t i = 0; i < shaderHash.GetSize(); i++) {
				shaderHash[i] += (uint8_t)'0';
			}

			location.append((const char*)shaderHash.GetData(), shaderHash.GetSize());

			TShared<ShaderResource> cached = static_cast<ShaderResource*>(resourceManager.LoadExistSafe(location)());

			if (cached) {
				// use cache
				mutationShaderResource = cached;
			} else {
				mutationShaderResource.Reset(static_cast<ShaderResource*>(mutationShaderResource->Clone()));
			}

			// cached?
			resourceManager.DoLockWriter();
			cached = static_cast<ShaderResource*>(resourceManager.LoadExist(location)());

			if (cached) {
				// use cache
				resourceManager.UnLockWriter();
				mutationShaderResource = cached;
			} else {
				mutationShaderResource->SetLocation(location);
				assert(!(mutationShaderResource->Flag().load(std::memory_order_acquire) & RESOURCE_UPLOADED));
				// printf("Variant Shader Created: %s\n", mutationShaderResource->GetLocation().c_str());
				resourceManager.Insert(mutationShaderResource());
				resourceManager.UnLockWriter();
			}

			shaderTemplateResource = mutationShaderResource;
		}
	}
}

void MaterialResource::Attach(IRender& render, void* deviceContext) {}

void MaterialResource::Detach(IRender& render, void* deviceContext) {}

void MaterialResource::Upload(IRender& render, void* deviceContext) {
	Flag().fetch_or(RESOURCE_UPLOADED, std::memory_order_release);
}

void MaterialResource::Download(IRender& render, void* deviceContext) {}

size_t MaterialResource::ReportDeviceMemoryUsage() const {
	size_t size = 0;
	for (size_t i = 0; i < textureResources.size(); i++) {
		const TShared<TextureResource>& texture = textureResources[i];
		size += texture->ReportDeviceMemoryUsage();
	}

	return size;
}

TShared<MaterialResource> MaterialResource::CloneWithOverrideShader(const TShared<ShaderResource>& overrideShaderResource) {
	if (overrideShaderResource == originalShaderResource) {
		return this;
	} else {
		OPTICK_EVENT();
		assert(overrideShaderResource);
		// create overrider
		TShared<MaterialResource> clone;
		if (uniqueLocation.size() != 0) {
			String overrideLocation = uniqueLocation + "@(" + overrideShaderResource->GetLocation() + ")";
			clone = static_cast<MaterialResource*>(resourceManager.LoadExistSafe(overrideLocation)());

			if (!clone) {
				clone = TShared<MaterialResource>::From(new MaterialResource(resourceManager, overrideLocation));
				clone->materialParams = materialParams;
				clone->textureResources = textureResources;
				clone->originalShaderResource = overrideShaderResource;
				clone->SetLocation(overrideLocation);

				SharedLockGuardWriter guard(resourceManager.GetThreadApi(), resourceManager.GetLock());
				TShared<ResourceBase> other = resourceManager.LoadExist(overrideLocation);
				if (!other) {
					resourceManager.Insert(clone());
				} else {
					clone.StaticCast(other);
				}
			}
		} else {
			// Orphan
			clone = TShared<MaterialResource>::From(new MaterialResource(resourceManager, ""));
			clone->materialParams = materialParams;
			clone->textureResources = textureResources;
			clone->originalShaderResource = overrideShaderResource;
		}

		return clone;
	}
}

template <class T>
struct PushValue {
	static void Push(IScript::Request& request, const String& name, std::vector<IAsset::Material::Variable>& variables) {
		T value;
		request >> value;
		variables.emplace_back(IAsset::Material::Variable(name, value));
	}

	static void PushFloats(IScript::Request& request, const String& name, std::vector<IAsset::Material::Variable>& variables) {
		T value;
		float* v = (float*)&value;
		for (size_t i = 0; i < sizeof(value) / sizeof(float); i++) {
			request >> v[i];
		}
		variables.emplace_back(IAsset::Material::Variable(name, value));
	}
};

void MaterialResource::ScriptModify(IScript::Request& request, const String& action, IScript::Request::Arguments args) {
	// TODO: thread safety ...
	if (action == "Set") {
		std::vector<IAsset::Material::Variable> variables;
		IScript::Request::ArrayStart as;
		as.count = 0;
		request.DoLock();
		request >> as;
		for (size_t i = 0; i < as.count / 2; i++) {
			String name;
			request >> name;
			if (name.size() == 0) continue;

			if (name[0] == '#') { // unlikely
				uint32_t value;
				request >> value;
				const uint32_t MASK = ~(uint32_t)0;

				if (name == "#stencilReplacePass") {
					materialParams.state.stencilReplacePass = value;
					materialParams.stateMask.stencilReplacePass = MASK;
				} else if (name == "#stencilReplaceFail") {
					materialParams.state.stencilReplaceFail = value;
					materialParams.stateMask.stencilReplaceFail = MASK;
				} else if (name == "#stencilReplaceZFail") {
					materialParams.state.stencilReplaceZFail = value;
					materialParams.stateMask.stencilReplaceZFail = MASK;
				} else if (name == "#cullFrontFace") {
					materialParams.state.cullFrontFace = value;
					materialParams.stateMask.cullFrontFace = MASK;
				} else if (name == "#cull") {
					materialParams.state.cull = value;
					materialParams.stateMask.cull = MASK;
				} else if (name == "#fill") {
					materialParams.state.fill = value;
					materialParams.stateMask.fill = MASK;
				} else if (name == "#blend") {
					materialParams.state.blend = value;
					materialParams.stateMask.blend = MASK;
				} else if (name == "#colorWrite") {
					materialParams.state.colorWrite = value;
					materialParams.stateMask.colorWrite = MASK;
				} else if (name == "#depthTest") {
					materialParams.state.depthTest = value;
					materialParams.stateMask.depthTest = MASK;
				} else if (name == "#depthWrite") {
					materialParams.state.depthWrite = value;
					materialParams.stateMask.depthWrite = MASK;
				} else if (name == "#stencilTest") {
					materialParams.state.stencilTest = value;
					materialParams.stateMask.stencilTest = MASK;
				} else if (name == "#stencilWrtie") {
					materialParams.state.stencilWrite = value;
					materialParams.stateMask.stencilWrite = MASK;
				} else if (name == "#stencilMask") {
					materialParams.state.stencilMask = value;
					materialParams.stateMask.stencilMask = MASK;
				} else if (name == "#stencilValue") {
					materialParams.state.stencilMask = value;
					materialParams.stateMask.stencilMask = MASK;
				}
			} else {
				switch (request.GetCurrentType()) {
					case IScript::Request::NIL:
					case IScript::Request::BOOLEAN:
						PushValue<bool>::Push(request, name, variables);
						break;
					case IScript::Request::INTEGER:
						PushValue<uint32_t>::Push(request, name, variables);
						break;
					case IScript::Request::STRING:
					{
						String value;
						request >> value;
						size_t k;
						for (k = 0; k < textureResources.size(); k++) {
							if (value == textureResources[k]->GetLocation()) {
								variables.emplace_back(IAsset::Material::Variable(name, IAsset::TextureIndex(verify_cast<uint32_t>(k))));
							}
						}

						if (k == textureResources.size()) {
							TShared<ResourceBase> resource = resourceManager.LoadExistSafe(value);
							if (resource && resource->QueryInterface(UniqueType<TextureResource>()) != nullptr) {
								textureResources.emplace_back(static_cast<TextureResource*>(resource()));
								variables.emplace_back(IAsset::Material::Variable(name, IAsset::TextureIndex(verify_cast<uint32_t>(k))));
							}
						}

						break;
					}
					case IScript::Request::ARRAY:
					case IScript::Request::TABLE:
					{
						IScript::Request::ArrayStart vs;
						vs.count = 0;
						request >> vs;
						switch (vs.count) {
							case 2:
								PushValue<Float2>::PushFloats(request, name, variables);
								break;
							case 3:
								PushValue<Float3>::PushFloats(request, name, variables);
								break;
							case 4:
								PushValue<Float4>::PushFloats(request, name, variables);
								break;
							case 9:
								PushValue<MatrixFloat3x3>::PushFloats(request, name, variables);
								break;
							case 16:
								PushValue<MatrixFloat4x4>::PushFloats(request, name, variables);
								break;
						}
						request << endarray;
						break;
					}
					default:
						PushValue<float>::Push(request, name, variables);
						break;
				}
			}
		}
		request << endarray;
		request.UnLock();

		MergeParameters(variables);
	} else if (action == "Clear") {
		materialParams.variables.clear();
	}

	Flag().fetch_or(TINY_MODIFIED, std::memory_order_release);
}

void MaterialResource::MergeParameters(std::vector<IAsset::Material::Variable>& variables) {
	if (materialParams.variables.empty()) {
		std::swap(materialParams.variables, variables);
	} else {
		// merge parameters
		for (size_t i = 0; i < variables.size(); i++) {
			IAsset::Material::Variable& variable = variables[i];

			size_t k;
			for (k = 0; k < materialParams.variables.size(); k++) {
				IAsset::Material::Variable& target = materialParams.variables[k];
				if (target.key == variable.key) {
					target.type = variable.type;
					target.value = std::move(variable.value);
					break;
				}
			}

			if (k == materialParams.variables.size()) {
				// add new one
				materialParams.variables.emplace_back(std::move(variable));
			}
		}
	}
}

void MaterialResource::Clear() {
	materialParams = IAsset::Material();
	textureResources.clear();
}

void MaterialResource::Import(const TShared<ShaderResource>& shaderResource) {
	assert(Flag().load(std::memory_order_acquire) & ResourceBase::RESOURCE_ORPHAN);
	assert(shaderResource);
	PassBase::Updater& updater = shaderResource->GetPassUpdater();
	const std::vector<PassBase::Parameter>& parameters = updater.GetParameters();
	std::vector<IAsset::Material::Variable> variables;
	const std::vector<KeyValue<Bytes, uint32_t> >& keys = updater.GetParameterKeys();

	for (size_t i = 0; i < keys.size(); i++) {
		String name((const char*)keys[i].first.GetData(), keys[i].first.GetSize());
		const PassBase::Parameter& parameter = parameters[keys[i].second];

		if (parameter.type == UniqueType<IRender::Resource*>::Get()) {
			if (parameter.resourceType == IRender::Resource::RESOURCE_TEXTURE) {
				variables.emplace_back(IAsset::Material::Variable(name, IAsset::TextureRuntime(*reinterpret_cast<IRender::Resource* const*>(parameter.internalAddress))));
			} else if (parameter.resourceType == IRender::Resource::RESOURCE_BUFFER) {
				// TODO: track buffers
			}
		} else {
			if (parameter.type == UniqueType<float>::Get()) {
				variables.emplace_back(IAsset::Material::Variable(name, *reinterpret_cast<float*>(parameter.internalAddress)));
			} else if (parameter.type == UniqueType<Float2>::Get()) {
				variables.emplace_back(IAsset::Material::Variable(name, *reinterpret_cast<Float2*>(parameter.internalAddress)));
			} else if (parameter.type == UniqueType<Float3>::Get()) {
				variables.emplace_back(IAsset::Material::Variable(name, *reinterpret_cast<Float3*>(parameter.internalAddress)));
			} else if (parameter.type == UniqueType<Float4>::Get()) {
				variables.emplace_back(IAsset::Material::Variable(name, *reinterpret_cast<Float4*>(parameter.internalAddress)));
			} else if (parameter.type == UniqueType<MatrixFloat3x3>::Get()) {
				variables.emplace_back(IAsset::Material::Variable(name, *reinterpret_cast<MatrixFloat3x3*>(parameter.internalAddress)));
			} else if (parameter.type == UniqueType<MatrixFloat4x4>::Get()) {
				variables.emplace_back(IAsset::Material::Variable(name, *reinterpret_cast<MatrixFloat4x4*>(parameter.internalAddress)));
			} else if (parameter.type == UniqueType<bool>::Get()) {
				variables.emplace_back(IAsset::Material::Variable(name, *reinterpret_cast<bool*>(parameter.internalAddress)));
			} else if (parameter.type == UniqueType<uint8_t>::Get()) {
				variables.emplace_back(IAsset::Material::Variable(name, *reinterpret_cast<uint8_t*>(parameter.internalAddress)));
			} else if (parameter.type == UniqueType<uint16_t>::Get()) {
				variables.emplace_back(IAsset::Material::Variable(name, *reinterpret_cast<uint16_t*>(parameter.internalAddress)));
			} else if (parameter.type == UniqueType<uint32_t>::Get()) {
				variables.emplace_back(IAsset::Material::Variable(name, *reinterpret_cast<uint32_t*>(parameter.internalAddress)));
			}
		}
	}

	MergeParameters(variables);
}

TObject<IReflect>& MaterialResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(materialParams);
		ReflectProperty(originalShaderResource)[MetaResourceInternalPersist(resourceManager)];
		ReflectProperty(textureResources)[MetaResourceInternalPersist(resourceManager)];
	}

	return *this;
}
