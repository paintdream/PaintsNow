#include "PassBase.h"
#include <string>
#include "../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

PassBase::PassBase() {}

void PassBase::SetInput(const String& stage, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config) {}
void PassBase::SetCode(const String& stage, const String& code, const std::vector<std::pair<String, String> >& config) {}
void PassBase::SetComplete() {}

TObject<IReflect>& PassBase::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	return *this;
}

class ReflectCollectShader : public IReflect {
public:
	ReflectCollectShader() : IReflect(true, false) {}
	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		singleton Unique shaderTypeUnique = UniqueType<IShader::MetaShader>::Get();
		while (meta != nullptr) {
			const MetaNodeBase* metaNode = meta->GetNode();
			if (metaNode->GetUnique() == shaderTypeUnique) {
				const IShader::MetaShader* metaShader = static_cast<const IShader::MetaShader*>(metaNode);
				assert(!s.IsBasicObject() && s.QueryInterface(UniqueType<IShader>()) != nullptr);
				shaders.emplace_back(std::make_pair(metaShader->shaderType, static_cast<IShader*>(&s)));
				return;
			}

			meta = meta->GetNext();
		}

		assert(s.IsBasicObject() || s.QueryInterface(UniqueType<IShader>()) == nullptr);
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

	typedef std::vector<std::pair<IRender::Resource::ShaderDescription::Stage, IShader*> > ShaderMap;
	ShaderMap shaders;
};

IRender::Resource* PassBase::Compile(IRender& render, IRender::Queue* queue, IRender::Resource* existedShaderResource, const TWrapper<void, IRender::Resource*, IRender::Resource::ShaderDescription&, IRender::Resource::ShaderDescription::Stage, const String&, const String&>& callback, void* context, void* instance) {
	IRender::Resource* shader = existedShaderResource != nullptr ? existedShaderResource : render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_SHADER);

	IRender::Resource::ShaderDescription& shaderDescription = *static_cast<IRender::Resource::ShaderDescription*>(render.MapResource(queue, shader, 0));
	ReflectCollectShader allShaders;
	(*this)(allShaders);

	// concat shader text
	shaderDescription.entries = std::move(allShaders.shaders);
	shaderDescription.compileCallback = callback;
	shaderDescription.context = context;
	shaderDescription.instance = instance;
	shaderDescription.name = ToString();

	// commit
	render.UnmapResource(queue, shader, IRender::MAP_DATA_EXCHANGE);
	return shader;
}

PassBase::Updater::Updater() : textureCount(0), IReflect(true, false, false, false) {}

uint32_t PassBase::Updater::GetBufferCount() const {
	return verify_cast<uint32_t>(buffers.size());
}

uint32_t PassBase::Updater::GetTextureCount() const {
	return textureCount;
}

const std::vector<PassBase::Parameter>& PassBase::Updater::GetParameters() const {
	return parameters;
}

const std::vector<KeyValue<Bytes, uint32_t> >& PassBase::Updater::GetParameterKeys() const {
	return mapParametersKey;
}

const std::vector<KeyValue<uint32_t, uint32_t> >& PassBase::Updater::GetParameterSchemas() const {
	return mapParametersSchema;
}

PassBase::Parameter::Parameter() : internalAddress(nullptr), linearLayout(0), bindBuffer(nullptr) {}

const PassBase::Parameter& PassBase::Updater::operator [] (const Bytes& key) {
	static Parameter defOutput;
	std::vector<KeyValue<Bytes, uint32_t> >::iterator it = BinaryFind(mapParametersKey.begin(), mapParametersKey.end(), key);
	return it != mapParametersKey.end() ? parameters[it->second] : defOutput;
}

const PassBase::Parameter& PassBase::Updater::operator [] (IShader::BindInput::SCHEMA schema) {
	static Parameter defOutput;
	std::vector<KeyValue<uint32_t, uint32_t> >::iterator it = BinaryFind(mapParametersSchema.begin(), mapParametersSchema.end(), schema);
	return it != mapParametersSchema.end() ? parameters[it->second] : defOutput;
}
	
void PassBase::Updater::Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
	Bytes byteName;
	byteName.Assign((const uint8_t*)name, verify_cast<uint32_t>(strlen(name)));
	uint32_t schema = IShader::BindInput::GENERAL;

	Parameter output;
	output.resourceType = IRender::Resource::RESOURCE_UNKNOWN;
	output.type = typeID;
	output.stride = 0;
	output.length = sizeof(IRender::Resource*);
	output.internalAddress = ptr;
#ifdef _DEBUG
	output.name = name;
#endif
	UInt2 subRange(0, ~(uint32_t)0);

	if (s.IsBasicObject() || s.IsIterator()) {
		for (const MetaChainBase* chain = meta; chain != nullptr; chain = chain->GetNext()) {
			const MetaNodeBase* node = chain->GetNode();
			if (node->GetUnique() == UniqueType<IShader::BindEnable>::Get()) {
				const IShader::BindEnable* bindOption = static_cast<const IShader::BindEnable*>(node);
				if (!*bindOption->description) break;
			} else if (node->GetUnique() == UniqueType<IShader::BindInput>::Get()) {
				const IShader::BindInput* bindInput = static_cast<const IShader::BindInput*>(node);
				schema = verify_cast<IShader::BindInput::SCHEMA>(bindInput->description);
				if (bindInput->subRangeQueryer) {
					assert(output.resourceType == IRender::Resource::RESOURCE_UNKNOWN); // BindInput must executed before BindBuffer in this case. i.e. place BindInput AFTER BindBuffer in your reflection handler.
					subRange = bindInput->subRangeQueryer();
				}
			} else if (node->GetUnique() == UniqueType<IShader::BindBuffer>::Get()) {
				const IShader::BindBuffer* bindBuffer = static_cast<const IShader::BindBuffer*>(chain->GetRawNode());
				output.resourceType = verify_cast<uint8_t>(IRender::Resource::RESOURCE_BUFFER);
				output.bindBuffer = const_cast<IShader::BindBuffer*>(bindBuffer);

				std::vector<KeyValue<const IShader::BindBuffer*, std::pair<uint16_t, uint16_t> > >::iterator it = BinaryFind(bufferIDSize.begin(), bufferIDSize.end(), bindBuffer);

				if (it != bufferIDSize.end()) {
					output.slot = verify_cast<uint8_t>(it->second.first);
					output.offset = verify_cast<uint16_t>(it->second.second);
					uint32_t size = 0;
					if (s.IsIterator()) {
						IIterator& iterator = static_cast<IIterator&>(s);
						assert(iterator.IsLayoutLinear());
						uint32_t count = verify_cast<uint32_t>(iterator.GetTotalCount());
						if (count != 0) {
							size = (uint32_t)verify_cast<uint32_t>(iterator.GetElementUnique()->GetSize()) * count;

							iterator.Next();
							output.internalAddress = iterator.Get();

							if (subRange.y() != ~(uint32_t)0) {
								output.internalAddress = (uint8_t*)output.internalAddress + subRange.x();
								assert(subRange.x() + subRange.y() <= size);
								size = subRange.y();
							}
						}
					} else {
						size = verify_cast<uint32_t>(typeID->GetSize());
					}

					output.length = size;
					it->second.second += size;
				} else {
#ifdef _DEBUG
					assert(disabled.count(bindBuffer) != 0);
#endif
					return;
				}
			}

			// Texture binding port is determined by BindTexture property itself
			/*else if (node->GetUnique() == UniqueType<IShader::BindTexture>::Get()) {
				const IShader::BindTexture* bindTexture = static_cast<const IShader::BindTexture*>(chain->GetRawNode());
				output.resourceType = (uint8_t)IRender::Resource::RESOURCE_TEXTURE;
				if (textureID.find(bindTexture) != textureID.end()) {
					output.slot = (uint8_t)textureID[bindTexture];
					output.offset = 0;
				} else {
#ifdef _DEBUG
						assert(disabled.count(bindTexture) != 0);
#endif
						return;
					}
				}*/
		}

		if (output.resourceType != IRender::Resource::RESOURCE_UNKNOWN) {
			BinaryInsert(mapParametersSchema, MakeKeyValue(schema, (uint32_t)verify_cast<uint32_t>(parameters.size())));
			BinaryInsert(mapParametersKey, MakeKeyValue(byteName, (uint32_t)verify_cast<uint32_t>(parameters.size())));
			parameters.emplace_back(output);
		}
	} else {
		// check if disabled
		for (const MetaChainBase* check = meta; check != nullptr; check = check->GetNext()) {
			const MetaNodeBase* node = check->GetNode();
			if (node->GetUnique() == UniqueType<IShader::MetaShader>::Get()) {
				s(*this);
			} else if (node->GetUnique() == UniqueType<IShader::BindInput>::Get()) {
				const IShader::BindInput* bindInput = static_cast<const IShader::BindInput*>(node);
				schema = verify_cast<IShader::BindInput::SCHEMA>(bindInput->description);
			} else if (node->GetUnique() == UniqueType<IShader::BindEnable>::Get()) {
				const IShader::BindEnable* bind = static_cast<const IShader::BindEnable*>(node);
				if (!*bind->description) {
					// disabled skip.
#ifdef _DEBUG
					disabled.insert(&s);
#endif
					return;
				}
			}
		}

		if (typeID == UniqueType<IShader::BindBuffer>::Get()) {
			IShader::BindBuffer* buffer = static_cast<IShader::BindBuffer*>(&s);
			output.resourceType = verify_cast<uint8_t>(IRender::Resource::RESOURCE_BUFFER);
			output.slot = verify_cast<uint8_t>(buffers.size());
			output.offset = 0;
			output.internalAddress = &buffer->resource;
			output.type = UniqueType<IRender::Resource*>::Get();

			BinaryInsert(mapParametersSchema, MakeKeyValue(schema, (uint32_t)verify_cast<uint32_t>(parameters.size())));
			BinaryInsert(mapParametersKey, MakeKeyValue(byteName, (uint32_t)verify_cast<uint32_t>(parameters.size())));
			parameters.emplace_back(output);

			uint32_t id = verify_cast<uint32_t>(bufferIDSize.size());
			BinaryInsert(bufferIDSize, MakeKeyValue((const IShader::BindBuffer*)buffer, std::make_pair((uint16_t)id, (uint16_t)0)));
			buffers.emplace_back(buffer);
		} else if (typeID == UniqueType<IShader::BindTexture>::Get()) {
			IShader::BindTexture* texture = static_cast<IShader::BindTexture*>(&s);
			output.resourceType = verify_cast<uint8_t>(IRender::Resource::RESOURCE_TEXTURE);
			output.slot = verify_cast<uint8_t>(textureCount++);
			output.offset = 0;
			output.internalAddress = &texture->resource;
			output.type = UniqueType<IRender::Resource*>::Get();

			BinaryInsert(mapParametersSchema, MakeKeyValue(schema, (uint32_t)verify_cast<uint32_t>(parameters.size())));
			BinaryInsert(mapParametersKey, MakeKeyValue(byteName, (uint32_t)verify_cast<uint32_t>(parameters.size())));
			parameters.emplace_back(output);
		}
	}
}

void PassBase::Updater::Flush() {
	std::vector<uint8_t> fixBufferSlots(buffers.size());

	std::vector<const IShader::BindBuffer*> fixedBuffers;
	for (uint32_t i = 0; i < buffers.size(); i++) {
		// Empty buffer?
		fixBufferSlots[i] = verify_cast<uint32_t>(fixedBuffers.size());
		std::vector<KeyValue<const IShader::BindBuffer*, std::pair<uint16_t, uint16_t> > >::iterator it = BinaryFind(bufferIDSize.begin(), bufferIDSize.end(), buffers[i]);
		if (it->second.second != 0 || it->first->description.state.usage == IRender::Resource::BufferDescription::STORAGE) {
			fixedBuffers.push_back(buffers[i]);
		}
	}

	// generate quick updaters
	quickUpdaters.reserve(parameters.size());

	// compute buffer linearness
	std::vector<bool> bufferLinearness(buffers.size(), true);
	std::vector<const char*> bufferAddresses(buffers.size(), nullptr);

	for (uint32_t k = 0; k < parameters.size(); k++) {
		Parameter& param = parameters[k];
		if (param.resourceType == IRender::Resource::RESOURCE_BUFFER) {
			std::vector<KeyValue<const IShader::BindBuffer*, std::pair<uint16_t, uint16_t> > >::iterator it = BinaryFind(bufferIDSize.begin(), bufferIDSize.end(), buffers[param.slot]);
			param.stride = it->second.second;
			const char* baseAddress = (const char*)param.internalAddress - param.offset;
			if (bufferAddresses[param.slot] == nullptr) {
				bufferAddresses[param.slot] = baseAddress;
			} else if (bufferLinearness[param.slot]) {
				bufferLinearness[param.slot] = bufferAddresses[param.slot] == baseAddress;
			}
		}
	}

	for (uint32_t j = 0; j < parameters.size(); j++) {
		Parameter& param = parameters[j];
		if (param.resourceType == IRender::Resource::RESOURCE_BUFFER) {
			if (bufferLinearness[param.slot]) {
				if (param.offset != 0)
					continue;

				param.linearLayout = 1;
			}
		
			param.slot = fixBufferSlots[param.slot]; // fix slot
		}

		quickUpdaters.emplace_back(j);
	}

	std::swap(buffers, fixedBuffers);
	bufferIDSize.clear();
}

void PassBase::Updater::Initialize(PassBase& pass) {
	assert(GetBufferCount() == 0);
	pass(*this);
	Flush();
}

void PassBase::Updater::Capture(IRender::Resource::DrawCallDescription& drawCallDescription, std::vector<Bytes>& bufferData, uint32_t bufferMask) {
	IRender::Resource::DrawCallDescription::BufferRange range;
	memset(&range, 0, sizeof(range));
	drawCallDescription.bufferResources.resize(buffers.size(), range);

	singleton Unique uniqueResource = UniqueType<IRender::Resource*>::Get();

	for (uint32_t k = 0; k < quickUpdaters.size(); k++) {
		const Parameter& parameter = parameters[quickUpdaters[k]];
		if (parameter.resourceType == IRender::Resource::RESOURCE_TEXTURE) {
			assert(parameter.type == uniqueResource);

			if (parameter.slot >= drawCallDescription.textureResources.size()) {
				drawCallDescription.textureResources.resize(parameter.slot + 1, nullptr);
			}

			drawCallDescription.textureResources[parameter.slot] = *reinterpret_cast<IRender::Resource**>(parameter.internalAddress);
		} else if (parameter.resourceType == IRender::Resource::RESOURCE_BUFFER) {
			if (parameter.type != uniqueResource) {
				assert(parameter.slot < buffers.size());
				const IShader::BindBuffer* bindBuffer = buffers[parameter.slot];
				if ((bufferMask & (1 << bindBuffer->description.state.usage)) && bindBuffer->resource == nullptr) {
					if (bufferData.size() <= parameter.slot) {
						bufferData.resize(parameter.slot + 1);
					}

					Bytes& s = bufferData[parameter.slot];
					if (s.Empty()) s.Resize(parameter.stride);
					memcpy(s.GetData() + parameter.offset, parameter.internalAddress, parameter.linearLayout ? parameter.stride : parameter.length);
				}
			}
		}
	}
}

void PassBase::Updater::Update(IRender& render, IRender::Queue* queue, IRender::Resource::DrawCallDescription& drawCallDescription, std::vector<IRender::Resource*>& newBuffers, std::vector<Bytes>& bufferData, uint32_t bufferMask) {
	for (uint32_t i = 0; i < buffers.size(); i++) {
		const IShader::BindBuffer* bindBuffer = buffers[i];
		if ((bufferMask & (1 << bindBuffer->description.state.usage))) {
			assert(i < drawCallDescription.bufferResources.size());
			IRender::Resource*& buffer = drawCallDescription.bufferResources[i].buffer;
			if (bindBuffer->resource != nullptr) {
				buffer = bindBuffer->resource;
			} else {
				if (buffer == nullptr) {
					buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
					newBuffers.emplace_back(buffer);
				}

				IRender::Resource::BufferDescription& desc = *static_cast<IRender::Resource::BufferDescription*>(render.MapResource(queue, buffer, 0));
				desc = bindBuffer->description;
				desc.data = std::move(bufferData[i]);
				assert(desc.data.GetSize() != 0);
				render.UnmapResource(queue, buffer, IRender::MAP_DATA_EXCHANGE);
			}
		}
	}
}

class HashExporter : public IReflect {
public:
	Bytes hashValue;
	HashExporter() : IReflect(true, false, false, false) {}
	
	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (s.IsBasicObject()) {
			for (const MetaChainBase* chain = meta; chain != nullptr; chain = chain->GetNext()) {
				const MetaNodeBase* node = chain->GetNode();
				Unique unique = node->GetUnique();
				if (unique == UniqueType<IShader::BindConst<bool> >::Get()
				|| unique == UniqueType<IShader::BindConst<uint32_t> >::Get()
				|| unique == UniqueType<IShader::BindConst<uint16_t> >::Get()
				|| unique == UniqueType<IShader::BindConst<uint8_t> >::Get()) {
					hashValue.Append(reinterpret_cast<uint8_t*>(ptr), verify_cast<uint32_t>(typeID->GetSize()));
					break;
				}
			}
		} else {
			for (const MetaChainBase* check = meta; check != nullptr; check = check->GetNext()) {
				const MetaNodeBase* node = check->GetNode();
				if (node->GetUnique() == UniqueType<IShader::MetaShader>::Get()) {
					s(*this);
				}
			}
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}
};

Bytes PassBase::ExportHash() const {
	HashExporter exporter;
	(const_cast<PassBase&>(*this))(exporter);

	return exporter.hashValue;
}

class ShaderStageMaskExporter : public IReflect {
public:
	uint32_t mask;
	ShaderStageMaskExporter() : IReflect(true, false, false, false), mask(0) {}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (!s.IsBasicObject()) {
			for (const MetaChainBase* check = meta; check != nullptr; check = check->GetNext()) {
				const MetaNodeBase* node = check->GetNode();
				if (node->GetUnique() == UniqueType<IShader::MetaShader>::Get()) {
					const IShader::MetaShader* metaShader = static_cast<const IShader::MetaShader*>(node);
					mask |= 1 << metaShader->shaderType;
				}
			}
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}
};

uint32_t PassBase::ExportShaderStageMask() const {
	ShaderStageMaskExporter exporter;
	(const_cast<PassBase&>(*this))(exporter);

	return exporter.mask;
}

class BindingCleaner : public IReflect {
public:
	BindingCleaner() : IReflect(true, false, false, false) {}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (!s.IsBasicObject()) {
			if (typeID == UniqueType<IShader::BindBuffer>::Get()) {
				IShader::BindBuffer* bindBuffer = static_cast<IShader::BindBuffer*>(&s);
				bindBuffer->resource = nullptr;
			} else if (typeID == UniqueType<IShader::BindTexture>::Get()) {
				IShader::BindTexture* bindTexture = static_cast<IShader::BindTexture*>(&s);
				bindTexture->resource = nullptr;
			} else {
				for (const MetaChainBase* check = meta; check != nullptr; check = check->GetNext()) {
					const MetaNodeBase* node = check->GetNode();
					if (node->GetUnique() == UniqueType<IShader::MetaShader>::Get()) {
						s(*this);
					}
				}
			}
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}
};

void PassBase::ClearBindings() {
	BindingCleaner cleaner;
	(const_cast<PassBase&>(*this))(cleaner);
}

class OptionEvaluator : public IReflect {
public:
	OptionEvaluator() : IReflect(true, false, false, false), next(false) {}
	bool next;

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (s.IsBasicObject()) {
			if (typeID == UniqueType<bool>::Get()) {
				bool& value = *reinterpret_cast<bool*>(ptr);

				for (const MetaChainBase* chain = meta; chain != nullptr; chain = chain->GetNext()) {
					const MetaNodeBase* node = chain->GetNode();
					Unique unique = node->GetUnique();

					if (unique == UniqueType<IShader::BindConst<bool> >::Get()) {
						const IShader::BindConst<bool>* bindConst = static_cast<const IShader::BindConst<bool>*>(node);
						if (value != bindConst->description) {
							value = bindConst->description;
							next = true;
						}
					}
				}
			}
		} else {
			for (const MetaChainBase* check = meta; check != nullptr; check = check->GetNext()) {
				const MetaNodeBase* node = check->GetNode();
				if (node->GetUnique() == UniqueType<IShader::MetaShader>::Get()) {
					s(*this);
				}
			}
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}
};

bool PassBase::FlushOptions() {
	int i = 0;
	while (true) {
		OptionEvaluator evaluator;
		(const_cast<PassBase&>(*this))(evaluator);

		if (!evaluator.next) {
			return i != 0;
		}

		i++;
	}

	return true;
}

PassBase::~PassBase() {}

template <bool UseBytesCache>
class CachedSnapshotBase {
public:
	CachedSnapshotBase(BytesCache* bytesCache) {}
	std::vector<Bytes> currentBufferData;
};

template <>
class CachedSnapshotBase<true> {
public:
	CachedSnapshotBase(BytesCache* bytesCache) : allocator(bytesCache), currentBufferData(allocator) {}
	typedef TCacheAllocator<Bytes, uint8_t> BytesCacheAllocator;
	BytesCacheAllocator allocator;
	std::vector<Bytes, BytesCacheAllocator> currentBufferData;
};

template <bool UseBytesCache>
class CachedSnapshot : public CachedSnapshotBase<UseBytesCache> {
public:
	using CachedSnapshotBase<UseBytesCache>::currentBufferData;
	CachedSnapshot(BytesCache* bytesCache) : CachedSnapshotBase<UseBytesCache>(bytesCache) {}

	template <class BufferDataVector, class BufferRangesVector, class TextureResourcesVector>
	void Snapshot(const std::vector<PassBase::Parameter>& parameters, BufferDataVector& bufferData, BufferRangesVector& bufferResources, TextureResourcesVector& textureResources, const PassBase::PartialData& partialData, BytesCache* bytesCache) {
		singleton Unique uniqueResource = UniqueType<IRender::Resource*>::Get();
		for (uint32_t i = 0; i < parameters.size(); i++) {
			const PassBase::Parameter& parameter = parameters[i];
			if (parameter.type == uniqueResource) {
				if (parameter.resourceType == IRender::Resource::RESOURCE_BUFFER) {
					const IShader::BindBuffer* buffer = reinterpret_cast<const IShader::BindBuffer*>((const char*)&partialData + (size_t)parameter.internalAddress);
					if (bufferResources.size() <= parameter.slot) {
						IRender::Resource::DrawCallDescription::BufferRange range;
						memset(&range, 0, sizeof(range));
						bufferResources.resize(parameter.slot + 1, range);
					}

					IRender::Resource::DrawCallDescription::BufferRange& range = bufferResources[parameter.slot];
					range.buffer = buffer->resource;
					range.offset = 0;
					range.length = 0;
					range.component = 0;
					range.type = 0;
				} else if (parameter.resourceType == IRender::Resource::RESOURCE_TEXTURE) {
					const IShader::BindTexture* texture = reinterpret_cast<const IShader::BindTexture*>((const char*)&partialData + (size_t)parameter.internalAddress);
					if (textureResources.size() <= parameter.slot) {
						textureResources.resize(parameter.slot + 1);
					}

					textureResources[parameter.slot] = texture->resource;
				} else {
					assert(false);
				}
			} else {
				uint8_t bufferDataSize = verify_cast<uint8_t>(currentBufferData.size());
				if (bufferDataSize <= parameter.slot) {
					currentBufferData.resize(parameter.slot + 1);
				}

				assert(parameter.slot < currentBufferData.size());
				Bytes& buffer = currentBufferData[parameter.slot];
				const PassBase::Parameter& p = parameters[i];
				if (buffer.Empty()) {
					if (UseBytesCache) {
						buffer = bytesCache->New(p.stride);
					} else {
						buffer.Resize(p.stride);
					}
				}

				if (UseBytesCache) {
					buffer.Import(p.offset, (const uint8_t*)&partialData + (size_t)p.internalAddress, p.type->GetSize());
				} else {
					memcpy(buffer.GetData() + p.offset, (const uint8_t*)&partialData + (size_t)p.internalAddress, p.type->GetSize());
				}
			}
		}

		if (currentBufferData.size() >= bufferData.size()) {
			bufferData.resize(currentBufferData.size());
		}

		for (size_t n = 0; n < currentBufferData.size(); n++) {
			if (UseBytesCache) {
				bytesCache->Link(bufferData[n], currentBufferData[n]);
			} else {
				bufferData[n].Append(currentBufferData[n]);
			}
		}
	}
};

void PassBase::PartialUpdater::Snapshot(std::vector<Bytes>& bufferData, std::vector<IRender::Resource::DrawCallDescription::BufferRange>& bufferResources, std::vector<IRender::Resource*>& textureResources, const PassBase::PartialData& partialData, BytesCache* bytesCache) const {
	OPTICK_EVENT();
	if (bytesCache != nullptr) {
		CachedSnapshot<true> snapshot(bytesCache);
		snapshot.Snapshot(parameters, bufferData, bufferResources, textureResources, partialData, bytesCache);
	} else {
		CachedSnapshot<false> snapshot(bytesCache);
		snapshot.Snapshot(parameters, bufferData, bufferResources, textureResources, partialData, bytesCache);
	}
}

void PassBase::PartialUpdater::Snapshot(std::vector<Bytes, TCacheAllocator<Bytes> >& bufferData, std::vector<IRender::Resource::DrawCallDescription::BufferRange, TCacheAllocator<IRender::Resource::DrawCallDescription::BufferRange> >& bufferResources, std::vector<IRender::Resource*, TCacheAllocator<IRender::Resource*> >& textureResources, const PartialData& partialData, BytesCache* bytesCache) {
	OPTICK_EVENT();
	CachedSnapshot<true> snapshot(bytesCache);
	snapshot.Snapshot(parameters, bufferData, bufferResources, textureResources, partialData, bytesCache);
}

Bytes PassBase::PartialUpdater::ComputeHash() const {
	if (parameters.empty()) return Bytes::Null();

	Bytes buffer(verify_cast<uint32_t>(parameters.size() * sizeof(uint16_t)));
	uint16_t* p = (uint16_t*)buffer.GetData();
	for (uint32_t i = 0; i < parameters.size(); i++) {
		const PassBase::Parameter& output = parameters[i];
		static_assert(sizeof(output.offset) == sizeof(uint16_t), "Size check");
		p[i] = output.offset;
	}

	return buffer;
}

class ReflectPartial : public IReflect {
public:
	ReflectPartial(PassBase::Updater& u) : IReflect(true), updater(u) {}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		singleton Unique uniqueBindBuffer = UniqueType<IShader::BindBuffer>::Get();
		singleton Unique uniqueBindTexture = UniqueType<IShader::BindTexture>::Get();
		if (s.IsBasicObject() || typeID == uniqueBindBuffer || typeID == uniqueBindTexture) {
			const PassBase::Parameter* m = nullptr;
			for (const MetaChainBase* p = meta; p != nullptr; p = p->GetNext()) {
				const MetaNodeBase* node = p->GetNode();
				if (node->GetUnique() == UniqueType<IShader::BindInput>::Get()) {
					const IShader::BindInput* bindInput = static_cast<const IShader::BindInput*>(node);
					if (bindInput->description != IShader::BindInput::GENERAL) {
						m = &updater[IShader::BindInput::SCHEMA(bindInput->description)];
					}
				}
			}

			// search for same 
			Bytes byteName;
			byteName.Assign((const uint8_t*)name, verify_cast<uint32_t>(strlen(name)));
			const PassBase::Parameter& parameter = m != nullptr && *m ? *m : updater[byteName];

			if (parameter) {
				const_cast<PassBase::Parameter&>(parameter).internalAddress = (void*)((size_t)ptr - (size_t)base);
				outputs.emplace_back(parameter);
			}
		}
	}

	std::vector<PassBase::Parameter> outputs;
	PassBase::Updater& updater;
};

void PassBase::PartialData::Export(PartialUpdater& particalUpdater, PassBase::Updater& updater) const {
	OPTICK_EVENT();
	ReflectPartial reflector(updater);
	(*const_cast<PassBase::PartialData*>(this))(reflector);

#ifdef _DEBUG
	if (!reflector.outputs.empty()) {
		std::map<IShader::BindBuffer*, std::pair<size_t, size_t> > sizeMap;
		for (uint32_t i = 0; i < reflector.outputs.size(); i++) {
			Parameter& parameter = reflector.outputs[i];
			if (parameter.type != UniqueType<IRender::Resource*>::Get()) {
				std::pair<size_t, size_t>& p = sizeMap[parameter.bindBuffer];
				p.first = i;
				p.second += (uint32_t)verify_cast<uint32_t>(parameter.type->GetSize());
			}
		}

		// check completeness
		for (std::map<IShader::BindBuffer*, std::pair<size_t, size_t> >::iterator it = sizeMap.begin(); it != sizeMap.end(); ++it) {
			assert((*it).second.second == reflector.outputs[(*it).second.first].stride); // must provide all segments by now
		}
	}
#endif

	particalUpdater.parameters = std::move(reflector.outputs);
}

