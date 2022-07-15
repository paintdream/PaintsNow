// ResourceBase.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "../../Core/System/Tiny.h"
#include "../../Core/Interface/IStreamBase.h"

namespace PaintsNow {
	class Interfaces;
	class IArchive;
	class ResourceManager;
	class ResourceBase : public TReflected<ResourceBase, SharedTiny> {
	public:
		enum {
			RESOURCE_ATTACHED = TINY_CUSTOM_BEGIN << 0,
			RESOURCE_UPLOADED = TINY_CUSTOM_BEGIN << 1,
			RESOURCE_DOWNLOADED = TINY_CUSTOM_BEGIN << 2,
			RESOURCE_INVALID = TINY_CUSTOM_BEGIN << 3,
			RESOURCE_MAPPED = TINY_CUSTOM_BEGIN << 4,
			RESOURCE_STREAM = TINY_CUSTOM_BEGIN << 5,
			RESOURCE_ETERNAL = TINY_CUSTOM_BEGIN << 6,
			RESOURCE_ORPHAN = TINY_CUSTOM_BEGIN << 7,
			RESOURCE_COMPRESSED = TINY_CUSTOM_BEGIN << 8,
			RESOURCE_MANUAL_UPLOAD = TINY_CUSTOM_BEGIN << 9,
			RESOURCE_VIRTUAL = (TINY_CUSTOM_BEGIN << 10) | RESOURCE_MANUAL_UPLOAD,
			RESOURCE_CUSTOM_BEGIN = TINY_CUSTOM_BEGIN << 11
		};

		static String GenerateLocation(const String& prefix, const void* ptr);
		typedef Void DriverType;
		ResourceBase(ResourceManager& manager, const String& uniqueLocation);
		~ResourceBase() override;
		ResourceManager& GetResourceManager() const;
		const String& GetLocation() const;
		void SetLocation(const String& location);

		virtual IStreamBase* OpenArchive(IArchive& archive, const String& extension, bool write, uint64_t& length);
		virtual Unique GetDeviceUnique() const;
		virtual bool Complete(size_t version);
		virtual bool LoadExternalResource(Interfaces& interfaces, IStreamBase& streamBase, size_t length);
		virtual bool Compress(const String& compressType, bool refreshRuntime);
		virtual bool Persist();
		virtual bool Map();
		virtual bool UnMap();
		virtual void ScriptModify(IScript::Request& request, const String& action, IScript::Request::Arguments arguments);
		bool IsMapped() const;

		class Dependency {
		public:
			String key;
			String value;
		};

		virtual Unique GetBaseUnique() const;
		virtual void GetDependencies(std::vector<Dependency>& deps) const;
		virtual bool IsPrepared() const;
		virtual std::pair<uint16_t, uint16_t> GetProgress() const;
		void Destroy() override;
		// override them derived from IReflectObject to change behaviors on serialization & deserialization
		// bool operator >> (IStreamBase& stream) const override;
		// bool operator << (IStreamBase& stream) override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		std::atomic<uint32_t>& GetMapCounter();

	protected:
		String uniqueLocation;
		ResourceManager& resourceManager;
		std::atomic<uint32_t> mapCount;

	public:
		std::atomic<uint32_t> critical;
	};

	template <class T>
	class DeviceResourceBase : public TReflected<DeviceResourceBase<T>, ResourceBase> {
	public:
		typedef TReflected<DeviceResourceBase<T>, ResourceBase> BaseClass;
		typedef T DriverType;
		DeviceResourceBase(ResourceManager& manager, const String& uniqueLocation) : BaseClass(manager, uniqueLocation) {}

		virtual void Refresh(T& device, void* deviceContext) = 0;
		virtual void Download(T& device, void* deviceContext) = 0;
		virtual void Upload(T& device, void* deviceContext) = 0;
		virtual void Attach(T& device, void* deviceContext) = 0;
		virtual void Detach(T& device, void* deviceContext) = 0;
		virtual size_t ReportDeviceMemoryUsage() const { return 0; }

		Unique GetDeviceUnique() const override {
			return UniqueType<T>::Get();
		}
	};

	class MetaResourceInternalPersist : public TReflected<MetaResourceInternalPersist, MetaStreamPersist> {
	public:
		MetaResourceInternalPersist(ResourceManager& resourceManager);
		IReflectObject* Clone() const override;

		template <class T, class D>
		inline const MetaResourceInternalPersist& FilterField(T* t, D* d) const {
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaResourceInternalPersist Type;
		};

		typedef MetaResourceInternalPersist Type;

		bool Read(IStreamBase& streamBase, void* ptr) const override;
		bool Write(IStreamBase& streamBase, const void* ptr) const override;
		String GetUniqueName() const override;

	private:
		ResourceManager& resourceManager;
		String uniqueName;
	};

	class IUniformResourceManager {
	public:
		virtual TShared<ResourceBase> CreateResource(const String& location, const String& extension = "", bool openExisting = true, Tiny::FLAG flag = 0) = 0;
		virtual bool SaveResource(const TShared<ResourceBase>& resource, const String& extension = "") = 0;
		virtual bool LoadResource(const TShared<ResourceBase>& resource, const String& extension = "") = 0;
	};
}

