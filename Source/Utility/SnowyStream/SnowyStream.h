// SnowyStream.h
// PaintDream (paintdream@paintdream.com)
// 2015-1-8
//

#pragma once
#include "../../General/Interface/Interfaces.h"
#include "File.h"
#include "Mount.h"
#include "ResourceBase.h"
#include "ResourceManager.h"
#include "../BridgeSunset/BridgeSunset.h"

namespace PaintsNow {
	class ShaderResource;
	class RenderResourceManager;
	class SnowyStream : public TReflected<SnowyStream, IScript::Library>, public IUniformResourceManager {
	public:
		SnowyStream(Interfaces& interfaces, BridgeSunset& bs, const TWrapper<IArchive*, IStreamBase&, size_t>& subArchiveCreator, const String& defMount);
		~SnowyStream() override;
		const TShared<RenderResourceManager>& GetRenderResourceManager() const;

		void TickDevice(IDevice& device) override;
		void Initialize() override;
		void Uninitialize() override;
		void Reset();

		virtual Interfaces& GetInterfaces() const;
		TShared<ResourceBase> CreateResource(const String& location, const String& extension = "", bool openExisting = true, Tiny::FLAG flag = 0) override;

	protected:
		bool SaveResource(const TShared<ResourceBase>& resource, const String& extension = "") override;
		bool LoadResource(const TShared<ResourceBase>& resource, const String& extension = "") override;

		bool RegisterResourceManager(Unique unique, ResourceManager* resourceManager);
		bool RegisterResourceSerializer(Unique unique, const String& extension, ResourceCreator* serializer);
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		/// <summary>
		/// Open or create a resource
		/// </summary>
		/// <param name="path"> resouce path, can be a file path or virtual path </param>
		/// <param name="expectedResType"> resource type, specify an empty string to extract from path </param>
		/// <param name="createAlways"> always create a new resource (regardless whether it exists or not </param>
		/// <returns> Resource object </returns>
		TShared<ResourceBase> RequestNewResource(IScript::Request& request, const String& path, const String& expectedResType, bool createAlways);

		/// <summary>
		/// Set render resource upload/update count limit per frame.
		/// </summary>
		/// <param name="limitStep"> new limit per frame </param>
		void RequestSetRenderResourceFrameStep(IScript::Request& request, uint32_t limitStep);

		/// <summary>
		/// Open bulk of resources in asynchorinzed way
		/// </summary>
		/// <param name="pathList"> resouce path list </param>
		/// <param name="expectedResType"> resource type, specify an empty string to extract from path </param>
		/// <param name="callbackStep"> callback on one resource created </param>
		/// <param name="callbackComplete"> callback on all resources created </param>
		/// <returns> Resource object </returns>
		void RequestNewResourcesAsync(IScript::Request& request, std::vector<String>& pathList, String& expectedResType, IScript::Request::Ref callbackStep, IScript::Request::Ref callbackComplete);

		/// <summary>
		/// Load external (raw) resource data from local file(s)
		/// </summary>
		/// <param name="resource"> Resource object </param>
		/// <param name="externalPath"> external (raw) local file path </param>
		/// <returns></returns>
		void RequestLoadExternalResourceData(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& externalPath);

		/// <summary>
		/// Inspect resource information
		/// </summary>
		/// <param name="resource"> Resource object </param>
		/// <returns> A dict with { "Flag" : integer, "Path" : string, "Depends", { Resource } } </returns>
		void RequestInspectResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource);

		/// <summary>
		/// Save resource to disk
		/// </summary>
		/// <param name="resource"> Resource object </param>
		/// <returns></returns>
		void RequestPersistResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& extension);

		/// <summary>
		/// Map resource to preserve local data on memory (especially for render resources), internal counted.
		/// </summary>
		/// <param name="resource"> Resource object </param>
		/// <returns></returns>
		void RequestMapResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource);

		/// <summary>
		/// UnMap resource to release local data on memory (especially for render resources), internal counted.
		/// </summary>
		/// <param name="resource"> Resource object </param>
		/// <returns></returns>
		void RequestUnmapResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource);

		/// <summary>
		/// Modify resource content
		/// </summary>
		/// <param name="resource"> Resource object </param>
		/// <param name="action"> Modify action </param>
		/// <param name="payload"> payload data </param>
		/// <returns></returns>
		void RequestModifyResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& action, IScript::Request::Arguments payload);

		/// <summary>
		/// Clone resource to new path
		/// </summary>
		/// <param name="resource"> Resource object </param>
		/// <param name="path"> new path </param>
		/// <returns></returns>
		TShared<ResourceBase> RequestCloneResource(IScript::Request& request, IScript::Delegate<ResourceBase>, const String& path);

		/// <summary>
		/// Compress resource in asynchronized way
		/// </summary>
		/// <param name="resource"> Resource object </param>
		/// <param name="compressType"> compress type </param>
		/// <param name="callback"> completion callback </param>
		/// <returns></returns>
		void RequestCompressResourceAsync(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& compressType, bool refreshRuntime, IScript::Request::Ref callback);

		/// <summary>
		/// Parse json object from string
		/// </summary>
		/// <param name="str"> json string </param>
		/// <returns> json object in cascaded dicts and arrays </returns>
		void RequestParseJson(IScript::Request& request, const StringView& str);

		/// <summary>
		/// Open or create a file from local file system
		/// </summary>
		/// <param name="path"> file path </param>
		/// <param name="write"> to write file or not </param>
		/// <returns> File object </returns>
		TShared<File> RequestNewFile(IScript::Request& request, const String& path, bool write);

		/// <summary>
		/// Delete file
		/// </summary>
		/// <param name="path"> file path </param>
		/// <returns></returns>
		void RequestDeleteFile(IScript::Request& request, const String& path);

		/// <summary>
		/// Check if specified file exists
		/// </summary>
		/// <param name="path"> file path </param>
		/// <returns></returns>
		void RequestFileExists(IScript::Request& request, const String& path);

		/// <summary>
		/// Flush write operations on a file.
		/// </summary>
		/// <param name="file"> File object </param>
		/// <returns></returns>
		void RequestFlushFile(IScript::Request& request, IScript::Delegate<File> file);

		/// <summary>
		/// Read data from file
		/// </summary>
		/// <param name="file"> File object </param>
		/// <param name="length"> Read length </param>
		/// <param name="callback"> optional callback, if it's not empty, the content will be passed into callback in asynchronized way, otherwise the content will be returned in synchronized way </param>
		/// <returns></returns>
		void RequestReadFile(IScript::Request& request, IScript::Delegate<File> file, int64_t length, IScript::Request::Ref callback);

		/// <summary>
		/// Get file size
		/// </summary>
		/// <param name="file"> File object </param>
		/// <returns> file size </returns>
		uint64_t RequestGetFileSize(IScript::Request& request, IScript::Delegate<File> file);

		/// <summary>
		/// Get file last modification time
		/// </summary>
		/// <param name="file"> File object </param>
		/// <returns> file last modification time </returns>
		uint64_t RequestGetFileLastModifiedTime(IScript::Request& request, IScript::Delegate<File> file);

		/// <summary>
		/// Write data to file
		/// </summary>
		/// <param name="file"> File object </param>
		/// <param name="content"> data to write </param>
		/// <returns></returns>
		void RequestWriteFile(IScript::Request& request, IScript::Delegate<File> file, const String& content, IScript::Request::Ref callback);

		/// <summary>
		/// Close file in an explicit way.
		/// </summary>
		/// <param name="file"> File object </param>
		/// <returns></returns>
		void RequestCloseFile(IScript::Request& request, IScript::Delegate<File> file);

		/// <summary>
		/// Seek file
		/// </summary>
		/// <param name="file"> File object </param>
		/// <param name="type"> seek type, can be selected from { "Current", "Begin", "End" } </param>
		/// <param name="offset"> seek offset </param>
		/// <returns></returns>
		void RequestSeekFile(IScript::Request& request, IScript::Delegate<File> file, const String& type, int64_t offset);

		/// <summary>
		/// Query files in folder
		/// </summary>
		/// <param name="path"> Folder path </param>
		/// <returns> file path list with { string } </returns>
		void RequestQueryFiles(IScript::Request& request, const String& path);

		/// <summary>
		/// Fetch whole file data, usually for reading small files.
		/// </summary>
		/// <param name="path"> file path </param>
		/// <returns> file content </returns>
		void RequestFetchFileData(IScript::Request& request, const String& path);
		
		/// <summary>
		/// Mount an archive file to specified mount point
		/// </summary>
		/// <param name="mountPoint"> target mount point </param>
		/// <param name="file"> archive file </param>
		/// <returns> Mount object </returns>
		TShared<Mount> RequestMount(IScript::Request& request, const String& mountPoint, IScript::Delegate<File> file);

		/// <summary>
		/// Umount an archive
		/// </summary>
		/// <param name="mount"> Mount object </param>
		void RequestUnmount(IScript::Request& request, IScript::Delegate<Mount> mount);

		/// <summary>
		/// Get profile of render device
		/// </summary>
		/// <param name="feature"> the feature to get </param>
		/// <returns> the profile information </returns>
		size_t RequestGetRenderProfile(IScript::Request& request, const String& feature);

	public:
		template <class T, class M>
		bool RegisterResourceSerializer(const String& extension, T& device, ResourceCreator* serializer, void* context, UniqueType<M> managerType) {
			Unique unique = UniqueType<T>::Get();
			if (!RegisterResourceSerializer(unique, extension, serializer)) {
				return false;
			}

			std::map<Unique, TShared<ResourceManager> >::iterator it = resourceManagers.find(unique);
			if (it == resourceManagers.end()) {
				ResourceManager* resourceManager = new M(bridgeSunset.GetKernel(), *this, device, bridgeSunset.LogInfo(), bridgeSunset.LogError(), context);
				RegisterResourceManager(unique, resourceManager);
				resourceManager->ReleaseObject();
			}

			return true;
		}

		static String GetReflectedExtension(Unique unique);

		template <class T, class M>
		void RegisterReflectedSerializer(UniqueType<T> type, typename T::DriverType& device, void* context, UniqueType<M> managerType) {
			String extension = GetReflectedExtension(type.Get());
			// bridgeSunset.LogInfo().Printf("[SnowyStream] Register resource type <%s> with extension <%s>\n", type.Get()->GetBriefName().c_str(), extension.c_str());
			ResourceCreator* serializer = new ResourceReflectedCreator<T>(extension);
			RegisterResourceSerializer(extension, device, serializer, context, managerType);
			serializer->ReleaseObject();
		}

		template <class T>
		TShared<T> CreateReflectedResource(UniqueType<T> type, const String& location, bool openExisting = true, Tiny::FLAG flag = 0) {
			return static_cast<T*>(CreateResource(location, GetReflectedExtension(type.Get()), openExisting, flag)());
		}

	protected:
		void RegisterReflectedSerializers();
		bool FilterPath(const String& path);
		void CreateBuiltinSolidTexture(const String& path, const UChar4& color);
		void CreateBuiltinMesh(const String& path, const Float3* vertices, size_t vertexCount, const UInt3* indices, size_t indexCount);

	protected:
		Interfaces& interfaces;
		BridgeSunset& bridgeSunset;

		// managers
		TShared<RenderResourceManager> renderResourceManager; // for frequently use ...
		TShared<Mount> defMountInstance;
		std::unordered_map<String, std::pair<Unique, TShared<ResourceCreator> > > resourceSerializers;
		std::map<Unique, TShared<ResourceManager> > resourceManagers;
		String defMount;
		static String reflectedExtension;

		// handlers
		const TWrapper<IArchive*, IStreamBase&, size_t> subArchiveCreator;
	};

	class MetaResourceExternalPersist : public TReflected<MetaResourceExternalPersist, MetaStreamPersist> {
	public:
		MetaResourceExternalPersist();
		IReflectObject* Clone() const override;

		template <class T, class D>
		inline const MetaResourceExternalPersist& FilterField(T* t, D* d) const {
			const_cast<String&>(uniqueName) = UniqueType<D>::Get()->GetBriefName();
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaResourceExternalPersist Type;
		};

		typedef MetaResourceExternalPersist Type;

		bool Read(IStreamBase& streamBase, void* ptr) const override;
		bool Write(IStreamBase& streamBase, const void* ptr) const override;
		String GetUniqueName() const override;

	private:
		String uniqueName;
	};
}
