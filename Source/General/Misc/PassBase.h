// PassBase -- Render Pass
// PaintDream (paintdream@paintdream.com)
// 2014-12-3
//

#pragma once
#include "../../General/Interface/IShader.h"
#include "../../Core/Interface/IReflect.h"
#include "../../Core/Template/TEvent.h"
#include "../../Core/Template/TCache.h"

namespace PaintsNow {
	// New version of PassBase !
	class PassBase : public TReflected<PassBase, IReflectObjectComplex> {
	public:
		PassBase();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		~PassBase() override;

		virtual void SetInput(const String& stage, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config);
		virtual void SetCode(const String& stage, const String& code, const std::vector<std::pair<String, String> >& config);
		virtual void SetComplete();

		IRender::Resource* Compile(IRender& render, IRender::Queue* queue, IRender::Resource* existedShaderResource = nullptr, const TWrapper<void, IRender::Resource*, IRender::Resource::ShaderDescription&, IRender::Resource::ShaderDescription::Stage, const String&, const String&>& callback = nullptr, void* context = nullptr, void* instance = nullptr);
		void ClearBindings();
		virtual bool FlushOptions();
		Bytes ExportHash() const;
		uint32_t ExportShaderStageMask() const;

		class Parameter {
		public:
			Parameter();
			template <class T>
			Parameter& operator = (const T& value) {
				if (internalAddress != nullptr) {
					assert(type == UniqueType<T>::Get());
					memcpy(internalAddress, &value, sizeof(T));
				}

				return *this;
			}

			inline Parameter& operator = (const Bytes& data) {
				if (internalAddress != nullptr) {
					assert(type->GetSize() <= data.GetSize());
					memcpy(internalAddress, data.GetData(), type->GetSize());
				}

				return *this;
			}

			inline operator bool() const {
				return internalAddress != nullptr;
			}

			// output
			uint8_t linearLayout : 1;
			uint8_t resourceType : 7;
			uint8_t slot; // N'st slot of buffer/texture
			uint16_t stride;
			uint16_t offset;
			uint32_t length;
			void* internalAddress;
			IShader::BindBuffer* bindBuffer;
			Unique type;
#ifdef _DEBUG
			String name;
#endif
		};

		class Updater : public IReflect {
		public:
			Updater();

			const Parameter& operator [] (const Bytes& key);
			const Parameter& operator [] (IShader::BindInput::SCHEMA schema);

			void Initialize(PassBase& pass);
			void Capture(IRender::Resource::DrawCallDescription& drawCallDescription, std::vector<Bytes>& bufferData, uint32_t bufferMask);
			void Update(IRender& render, IRender::Queue* queue, IRender::Resource::DrawCallDescription& drawCall, std::vector<IRender::Resource*>& newBuffers, std::vector<Bytes>& bufferData, uint32_t bufferMask);
			void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override;
			void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}
			
			uint32_t GetBufferCount() const;
			uint32_t GetTextureCount() const;
			const std::vector<Parameter>& GetParameters() const;
			const std::vector<KeyValue<Bytes, uint32_t> >& GetParameterKeys() const;
			const std::vector<KeyValue<uint32_t, uint32_t> >& GetParameterSchemas() const;

		private:
			void Flush();
			std::vector<const IShader::BindBuffer*> buffers;
			std::vector<KeyValue<const IShader::BindBuffer*, std::pair<uint16_t, uint16_t> > > bufferIDSize;
			std::vector<KeyValue<Bytes, uint32_t> > mapParametersKey;
			std::vector<KeyValue<uint32_t, uint32_t> > mapParametersSchema;
			std::vector<uint32_t> quickUpdaters;
			std::vector<Parameter> parameters;
			uint32_t textureCount;

#ifdef _DEBUG
			std::set<const void*> disabled;
#endif
		};

		class PartialData;
		class PartialUpdater {
		public:
			Bytes ComputeHash() const;
			void Snapshot(std::vector<Bytes>& buffers, std::vector<IRender::Resource::DrawCallDescription::BufferRange>& bufferResources, std::vector<IRender::Resource*>& textureResources, const PartialData& data, BytesCache* bytesCache) const;
			void Snapshot(std::vector<Bytes, TCacheAllocator<Bytes> >& buffers, std::vector<IRender::Resource::DrawCallDescription::BufferRange, TCacheAllocator<IRender::Resource::DrawCallDescription::BufferRange> >& bufferResources, std::vector<IRender::Resource*, TCacheAllocator<IRender::Resource*> >& textureResources, const PartialData& data, BytesCache* bytesCache);
			
			std::vector<Parameter> parameters;
		};

		class PartialData : public TReflected<PartialData, IReflectObjectComplex> {
		public:
			void Export(PartialUpdater& particalUpdater, Updater& updater) const;
		};

		static inline bool ValidateDrawCall(const IRender::Resource::DrawCallDescription& drawCall) {
			// Check DrawCall completeness
			if (drawCall.indexBufferResource.buffer == nullptr) return false;
			if (drawCall.shaderResource == nullptr) return false;

			for (size_t i = 0; i < drawCall.bufferResources.size(); i++) {
				const IRender::Resource::DrawCallDescription::BufferRange& bufferRange = drawCall.bufferResources[i];
				if (bufferRange.buffer == nullptr) return false;
			}

			for (uint32_t j = 0; j < drawCall.textureResources.size(); j++) {
				const IRender::Resource* texture = drawCall.textureResources[j];
				if (texture == nullptr) return false;
			}

			return true;
		}

		static inline bool ValidateDrawCall(IRender::Resource::QuickDrawCallDescription& drawCall) {
			// Check DrawCall completeness
			if (drawCall.GetIndexBuffer()->buffer == nullptr) return false;
			if (drawCall.GetShader() == nullptr) return false;

			for (size_t i = 0; i < drawCall.bufferCount; i++) {
				const IRender::Resource::DrawCallDescription::BufferRange& bufferRange = drawCall.GetBuffers()[i];
				if (bufferRange.buffer == nullptr) return false;
			}

			for (uint32_t j = 0; j < drawCall.textureCount; j++) {
				const IRender::Resource* texture = drawCall.GetTextures()[j];
				if (texture == nullptr) return false;
			}

			return true;
		}
	};
}

