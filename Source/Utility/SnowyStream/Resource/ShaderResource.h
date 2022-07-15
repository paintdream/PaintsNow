// ShaderResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "RenderResourceBase.h"
#include "../../../General/Interface/IAsset.h"
#include "../../../General/Misc/PassBase.h"

namespace PaintsNow {
	class ShaderResource : public TReflected<ShaderResource, RenderResourceBase> {
	public:
		ShaderResource(ResourceManager& manager, const String& uniqueID);
		~ShaderResource() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		static const String& GetShaderPathPrefix();

		virtual const String& GetShaderName() const;
		bool operator << (IStreamBase& stream) override;
		bool operator >> (IStreamBase& stream) const override;
		void Upload(IRender& render, void* deviceContext) override;
		void Download(IRender& render, void* deviceContext) override;
		void Attach(IRender& render, void* deviceContext) override;
		void Detach(IRender& render, void* deviceContext) override;
		IReflectObject* Clone() const override;
		bool Complete(size_t runtimeVersion) override;
		virtual PassBase& GetPass();
		virtual PassBase::Updater& GetPassUpdater();
		void Compile(IRender& render, IRender::Queue* queue, const Bytes* newHash = nullptr);

		IRender::Resource* GetShaderResource() const;
		void SetShaderResource(IRender::Resource* resource);
		const Bytes& GetHashValue() const;
		void OnResourceUploaded(IRender& render, IRender::Queue* queue);

	protected:
		Bytes hashValue;
		IRender::Resource* shaderResource;
	};

	template <class T>
	class ShaderResourceImpl : public TReflected<ShaderResourceImpl<T>, ShaderResource> {
	public:
		// gcc do not support referencing base type in template class. manually specified here.
		typedef TReflected<ShaderResourceImpl<T>, ShaderResource> BaseClass;
		ShaderResourceImpl(ResourceManager& manager, const String& uniqueID, Tiny::FLAG f = 0) : BaseClass(manager, uniqueID) { BaseClass::Flag().fetch_or(f, std::memory_order_relaxed); }

		PassBase& GetPass() override {
			return pass;
		}

		PassBase::Updater& GetPassUpdater() override {
			return updater;
		}

		IReflectObject* Clone() const override {
			ShaderResourceImpl<T>* resource = new ShaderResourceImpl<T>(BaseClass::resourceManager, ""); // on the fly
			resource->Flag().fetch_and(~BaseClass::RESOURCE_UPLOADED, std::memory_order_relaxed);
			resource->pass = pass; // copy pass default values
			resource->hashValue = BaseClass::hashValue;
			resource->GetPassUpdater().Initialize(resource->pass); // reinitialize pass updater
			resource->Flag().fetch_or(BaseClass::RESOURCE_ORPHAN, std::memory_order_release);

			return resource;
		}

		Unique GetBaseUnique() const override {
			return UniqueType<ShaderResource>::Get();
		}

	protected:
		T pass;
		PassBase::Updater updater;
	};
}

