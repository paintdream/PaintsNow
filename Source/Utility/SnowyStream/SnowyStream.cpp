#define _CRT_SECURE_NO_WARNINGS
#include "SnowyStream.h"
#include "Manager/RenderResourceManager.h"
#include "Resource/BufferResource.h"
#include "Resource/FontResource.h"
#include "Resource/MaterialResource.h"
#include "Resource/MeshResource.h"
#include "Resource/SkeletonResource.h"
#include "Resource/StreamResource.h"
#include "Resource/TextureResource.h"
#include "Resource/TextureArrayResource.h"
#include "../../Core/System/MemoryStream.h"
#include "../../Core/Driver/Profiler/Optick/optick.h"
#include "../../General/Driver/Filter/Json/Core/json.h"

using namespace PaintsNow;

String SnowyStream::reflectedExtension = "*.rds";

SnowyStream::SnowyStream(Interfaces& inters, BridgeSunset& bs, const TWrapper<IArchive*, IStreamBase&, size_t>& psubArchiveCreator, const String& dm) : interfaces(inters), bridgeSunset(bs), subArchiveCreator(psubArchiveCreator), defMount(dm) {
	assert(resourceManagers.empty());
}

void SnowyStream::Initialize() {
	RegisterReflectedSerializers();
	// Mount default drive
	if (!defMount.empty()) {
		IArchive& archive = interfaces.archive;
		uint64_t length = 0;
		uint64_t modifiedTime = 0;
		IStreamBase* stream = archive.Open(defMount, false, length, &modifiedTime);
		if (stream != nullptr) {
			TShared<File> file = TShared<File>::From(new File(stream, verify_cast<size_t>(length), modifiedTime));
			IArchive* ar = subArchiveCreator(*file->GetStream(), file->GetLength());
			defMountInstance = TShared<Mount>::From(new Mount(archive, "", ar, file));
		} else {
			fprintf(stderr, "Unable to mount default archive: %s\n", defMount.c_str());
		}
	}
}

void SnowyStream::TickDevice(IDevice& device) {
	for (std::map<Unique, TShared<ResourceManager> >::iterator it = resourceManagers.begin(); it != resourceManagers.end(); ++it) {
		(*it).second->TickDevice(device);
	}
}

SnowyStream::~SnowyStream() {}

const TShared<RenderResourceManager>& SnowyStream::GetRenderResourceManager() const {
	return renderResourceManager;
}

Interfaces& SnowyStream::GetInterfaces() const {
	return interfaces;
}

void SnowyStream::Reset() {}

TObject<IReflect>& SnowyStream::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(interfaces)[Runtime];
		ReflectProperty(resourceSerializers)[Runtime];
		ReflectProperty(resourceManagers)[Runtime];
	}

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNewResource)[ScriptMethod = "NewResource"];
		ReflectMethod(RequestSetRenderResourceFrameStep)[ScriptMethod = "SetRenderResourceFrameStep"];
		ReflectMethod(RequestLoadExternalResourceData)[ScriptMethod = "LoadExternalResourceData"];
		ReflectMethod(RequestPersistResource)[ScriptMethod = "PersistResource"];
		ReflectMethod(RequestCloneResource)[ScriptMethod = "CloneResource"];
		ReflectMethod(RequestMapResource)[ScriptMethod = "MapResource"];
		ReflectMethod(RequestUnmapResource)[ScriptMethod = "UnmapResource"];
		ReflectMethod(RequestModifyResource)[ScriptMethod = "ModifyResource"];
		ReflectMethod(RequestInspectResource)[ScriptMethod = "InspectResource"];
		ReflectMethod(RequestCompressResourceAsync)[ScriptMethod = "CompressResourceAsync"];
		ReflectMethod(RequestNewResourcesAsync)[ScriptMethod = "NewResourcesAsync"];

		ReflectMethod(RequestParseJson)[ScriptMethod = "ParseJson"];
		ReflectMethod(RequestNewFile)[ScriptMethod = "NewFile"];
		ReflectMethod(RequestDeleteFile)[ScriptMethod = "DeleteFile"];
		ReflectMethod(RequestFlushFile)[ScriptMethod = "FlushFile"];
		ReflectMethod(RequestReadFile)[ScriptMethod = "ReadFile"];
		ReflectMethod(RequestGetFileSize)[ScriptMethod = "GetFileSize"];
		ReflectMethod(RequestCloseFile)[ScriptMethod = "CloseFile"];
		ReflectMethod(RequestGetFileLastModifiedTime)[ScriptMethod = "GetFileLastModifiedTime"];
		ReflectMethod(RequestWriteFile)[ScriptMethod = "WriteFile"];
		ReflectMethod(RequestSeekFile)[ScriptMethod = "SeekFile"];
		ReflectMethod(RequestQueryFiles)[ScriptMethod = "QueryFiles"];
		ReflectMethod(RequestFetchFileData)[ScriptMethod = "FetchFileData"];
		ReflectMethod(RequestFileExists)[ScriptMethod = "FileExists"];

		ReflectMethod(RequestMount)[ScriptMethod = "Mount"];
		ReflectMethod(RequestUnmount)[ScriptMethod = "Unmount"];
		ReflectMethod(RequestGetRenderProfile)[ScriptMethod = "GetRenderProfile"];
	}

	return *this;
}

static void WriteValue(IScript::Request& request, const Json::Value& value) {
	using namespace Json;
	Value::const_iterator it;
	size_t i;

	switch (value.type()) {
		case nullValue:
			request << nil;
			break;
		case intValue:
			request << (int64_t)value.asInt64();
			break;
		case uintValue:
			request << (uint64_t)value.asUInt64();
			break;
		case realValue:
			request << value.asDouble();
			break;
		case stringValue:
			request << StdToUtf8(value.asString());
			break;
		case booleanValue:
			request << value.asBool();
			break;
		case arrayValue:
			request << beginarray;
			for (i = 0; i < value.size(); i++) {
				WriteValue(request, value[(Value::ArrayIndex)i]);
			}
			request << endarray;
			break;
		case objectValue:
			request << begintable;
			for (it = value.begin(); it != value.end(); ++it) {
				String s = StdToUtf8(it.name());
				request << key(s);
				WriteValue(request, *it);
			}
			request << endtable;
	}
}

void SnowyStream::RequestParseJson(IScript::Request& request, const StringView& str) {
	using namespace Json;

	Reader reader;
	Value document;

	if (reader.parse(str.begin(), str.end(), document)) {
		request.DoLock();
		WriteValue(request, document);
		request.UnLock();
	}
}

TShared<Mount> SnowyStream::RequestMount(IScript::Request& request, const String& path, IScript::Delegate<File> file) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);

	IArchive* ar = subArchiveCreator(*file->GetStream(), file->GetLength());
	TShared<Mount> mount = TShared<Mount>::From(new Mount(interfaces.archive, path, ar, file.Get()));

	return mount;
}

void SnowyStream::RequestUnmount(IScript::Request& request, IScript::Delegate<Mount> mount) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(mount);

	mount->Unmount();
}

size_t SnowyStream::RequestGetRenderProfile(IScript::Request& request, const String& feature) {
	return renderResourceManager->GetProfile(feature);
}

void SnowyStream::RequestFileExists(IScript::Request& request, const String& path) {
	WarpYieldGuard guard(bridgeSunset.GetKernel());

	IArchive& archive = interfaces.archive;
	bool ret = archive.Exists(path);

	request.DoLock();
	request << ret;
	request.UnLock();
}

void SnowyStream::RequestFetchFileData(IScript::Request& request, const String& path) {
	WarpYieldGuard guard(bridgeSunset.GetKernel());
	uint64_t length;
	IArchive& archive = interfaces.archive;
	IStreamBase* stream = archive.Open(path, false, length);
	bool success = false;
	if (stream != nullptr) {
		String buf;
		size_t len = verify_cast<size_t>(length);
		buf.resize(len);

		if (stream->Read(const_cast<char*>(buf.data()), len)) {
			stream->Destroy();
			request.DoLock();
			request << buf;
			request.UnLock();
		}
	}
}

void SnowyStream::RequestCloseFile(IScript::Request& request, IScript::Delegate<File> file) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	WarpYieldGuard guard(bridgeSunset.GetKernel());
	file->Close();
}

void SnowyStream::RequestDeleteFile(IScript::Request& request, const String& path) {
	CHECK_REFERENCES_NONE();

	interfaces.archive.Delete(path);
}

TShared<File> SnowyStream::RequestNewFile(IScript::Request& request, const String& path, bool write) {
	CHECK_REFERENCES_NONE();
	WarpYieldGuard guard(bridgeSunset.GetKernel());
	IArchive& archive = interfaces.archive;
	uint64_t length = 0;
	uint64_t modifiedTime = 0;
	IStreamBase* stream = path == ":memory:" ? new MemoryStream(0x1000, true) : archive.Open(path, write, length, &modifiedTime);
	if (stream != nullptr) {
		return TShared<File>::From(new File(stream, verify_cast<size_t>(length), modifiedTime));
	} else {
		return nullptr;
	}
}

uint64_t SnowyStream::RequestGetFileLastModifiedTime(IScript::Request& request, IScript::Delegate<File> file) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	return file->GetLastModifiedTime();
}

uint64_t SnowyStream::RequestGetFileSize(IScript::Request& request, IScript::Delegate<File> file) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);

	return file->GetLength();
}

void SnowyStream::RequestReadFile(IScript::Request& request, IScript::Delegate<File> file, int64_t length, IScript::Request::Ref callback) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	if (callback) {
		CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);
	}

	WarpYieldGuard guard(bridgeSunset.GetKernel());
	IStreamBase* stream = file->GetStream();
	if (stream != nullptr) {
		String content;
		size_t len = (size_t)length;
		content.resize(len);
		if (stream->Read(const_cast<char*>(content.data()), len)) {
			request.DoLock();
			if (callback) {
				request.Push();
				request.Call(callback, content);
				request.Pop();
			} else {
				request << content;
			}
			request.UnLock();
		}
	} else {
		request.Error("SnowyStream::ReadFile() : File already closed.");
	}
}

void SnowyStream::RequestWriteFile(IScript::Request& request, IScript::Delegate<File> file, const String& content, IScript::Request::Ref callback) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	if (callback) {
		CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);
	}

	WarpYieldGuard guard(bridgeSunset.GetKernel());

	IStreamBase* stream = file->GetStream();
	if (stream != nullptr) {
		size_t len = content.size();
		if (stream->Write(content.data(), len)) {
			request.DoLock();
			if (callback) {
				request.Push();
				request.Call(callback, len);
				request.Pop();
			} else {
				request << len;
			}
			request.UnLock();
		}
	} else {
		request.Error("SnowyStream::WriteFile() : File already closed.");
	}
}

void SnowyStream::RequestFlushFile(IScript::Request& request, IScript::Delegate<File> file) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);

	IStreamBase* stream = file->GetStream();
	if (stream != nullptr) {
		stream->Flush();
	} else {
		request.Error("SnowyStream::FlushFile() : File already closed.");
	}
}

void SnowyStream::RequestSeekFile(IScript::Request& request, IScript::Delegate<File> file, const String& type, int64_t offset) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);

	WarpYieldGuard guard(bridgeSunset.GetKernel());
	IStreamBase* stream = file->GetStream();
	if (stream != nullptr) {
		IStreamBase::SEEK_OPTION option = IStreamBase::CUR;
		if (type == "Current") {
			option = IStreamBase::CUR;
		} else if (type == "Begin") {
			option = IStreamBase::BEGIN;
		} else if (type == "End") {
			option = IStreamBase::END;
		}

		if (stream->Seek(option, (long)offset)) {
			request.DoLock();
			request << true;
			request.UnLock();
		}
	} else {
		request.Error("SnowyStream::SeekFile() : File already closed.");
	}
}

class QueryHandler {
public:
	QueryHandler(const String& p, IScript::Request& r) : prefix(p), request(r) {}
	void Accept(const String& path) {
		request << path;
	}

	const String prefix;
	IScript::Request& request;
};

bool SnowyStream::FilterPath(const String& path) {
	return path.find("..") == String::npos;
}

void SnowyStream::RequestQueryFiles(IScript::Request& request, const String& p) {
	if (!FilterPath(p)) return;

	String path = p;
	char ch = path[path.length() - 1];
	if (ch != '/' && ch != '\\') {
		path += "/";
	}

	IArchive& archive = interfaces.archive;
	QueryHandler handler(path, request);
	WarpYieldGuard guard(bridgeSunset.GetKernel());

	request.DoLock();
	request << beginarray;
	archive.Query(path, Wrap(&handler, &QueryHandler::Accept));
	request << endarray;
	request.UnLock();
}

TShared<ResourceBase> SnowyStream::RequestNewResource(IScript::Request& request, const String& path, const String& expectedResType, bool createAlways) {
	TShared<ResourceBase> resource = CreateResource(path, expectedResType, !createAlways, path.empty() ? ResourceBase::RESOURCE_VIRTUAL : 0);
	WarpYieldGuard guard(bridgeSunset.GetKernel());

	if (resource) {
		return resource;
	} else {
		request.Error(String("Unable to create resource ") + path);
		return nullptr;
	}
}

void SnowyStream::RequestSetRenderResourceFrameStep(IScript::Request& request, uint32_t limitStep) {
	renderResourceManager->SetRenderResourceFrameStep(limitStep);
}

class TaskResourceCreator final : public TReflected<TaskResourceCreator, SharedTiny> {
public:
	TaskResourceCreator(BridgeSunset& bs, SnowyStream& ss, rvalue<std::vector<String> > pl, rvalue<String> type, IScript::Request::Ref step, IScript::Request::Ref complete) : bridgeSunset(bs), snowyStream(ss), callbackStep(step), callbackComplete(complete), resType(std::move(type)) {
		std::swap(pathList, (std::vector<String>&)pl);
		resourceList.resize(pathList.size());
		completed.store(0, std::memory_order_release);
	}

	void Start(IScript::Request& request) {
		ThreadPool& threadPool = bridgeSunset.GetKernel().GetThreadPool();
		ReferenceObject();
		for (uint32_t i = 0; i < verify_cast<uint32_t>(pathList.size()); i++) {
			// Create async task and use low-level thread pool dispatching it
			// Do not occupy all threads.
			threadPool.Dispatch(CreateTask(Wrap(this, &TaskResourceCreator::RoutineCreateResource), i), 1);
		}
	}

private:
	inline void Finalize(IScript::Request& request) {
		request.DoLock();
		if (callbackComplete) {
			request.Push();
			request.Call(callbackComplete, resourceList);
			request.Pop();
			request.Dereference(callbackComplete);
		}
		request.Dereference(callbackStep);
		request.UnLock();
	}

	inline void RoutineCreateResource(void* context, bool run, uint32_t index) {
		if (run) {
			resourceList[index] = snowyStream.CreateResource(pathList[index], resType, true);
			assert(resourceList[index]);
		}

		if (callbackStep) {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
			request.DoLock();
			request.Push();
			request.Call(callbackStep, pathList[index], resourceList[index]);
			request.Pop();
			request.UnLock();
			bridgeSunset.requestPool.ReleaseSafe(&request);
		}

		// is abount to finish
		if (completed.fetch_add(1, std::memory_order_relaxed) + 1 == pathList.size()) {
			assert(context != nullptr);
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
			Finalize(request);
			bridgeSunset.requestPool.ReleaseSafe(&request);

			ReleaseObject();
		}
	}

	std::vector<String> pathList;
	std::vector<TShared<ResourceBase> > resourceList;
	std::atomic<uint32_t> completed;

	String resType;
	BridgeSunset& bridgeSunset;
	SnowyStream& snowyStream;
	IScript::Request::Ref callbackStep;
	IScript::Request::Ref callbackComplete;
};

void SnowyStream::RequestNewResourcesAsync(IScript::Request& request, std::vector<String>& pathList, String& expectedResType, IScript::Request::Ref callbackStep, IScript::Request::Ref callbackComplete) {
	TShared<TaskResourceCreator> taskResourceCreator = TShared<TaskResourceCreator>::From(new TaskResourceCreator(bridgeSunset, *this, std::move(pathList), std::move(expectedResType), callbackStep, callbackComplete));
	taskResourceCreator->Start(request);
}

void SnowyStream::RequestLoadExternalResourceData(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& externalData) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);

	WarpYieldGuard guard(bridgeSunset.GetKernel());
	MemoryStream ms(externalData.size());
	size_t len = externalData.size();
	ms.Write(externalData.c_str(), len);
	ms.Seek(IStreamBase::BEGIN, 0);
	bool result = resource->LoadExternalResource(interfaces, ms, len);

	if (result) {
		resource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		resource->GetResourceManager().InvokeUpload(resource.Get(), resource->GetResourceManager().GetContext());
	}

	request.DoLock();
	request << result;
	request.UnLock();
}

void SnowyStream::RequestInspectResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);

	WarpYieldGuard guard(bridgeSunset.GetKernel());
	Tiny::FLAG flag = resource->Flag().load(std::memory_order_relaxed);
	std::vector<ResourceBase::Dependency> dependencies;
	resource->GetDependencies(dependencies);

	request.DoLock();
	request << begintable
		<< key("Flag") << flag
		<< key("Path") << resource->GetLocation()
		<< key("Depends") << begintable;

	for (size_t k = 0; k < dependencies.size(); k++) {
		ResourceBase::Dependency& d = dependencies[k];
		request << key("Key") << d.value;
	}

	request << endtable << endtable;
	request.UnLock();
}

TShared<ResourceBase> SnowyStream::RequestCloneResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& path) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);

	ResourceManager& resourceManager = resource->GetResourceManager();
	TShared<ResourceBase> exist = resourceManager.LoadExistSafe(path);
	if (!exist) {
		TShared<ResourceBase> p = TShared<ResourceBase>::From(static_cast<ResourceBase*>(resource->Clone()));
		if (p) {
			p->SetLocation(path);
			resourceManager.Insert(p());
			return p;
		}
	} else {
		request.Error(String("SnowyStream::CloneResource() : The path ") + path + " already exists");
	}

	return nullptr;
}

void SnowyStream::RequestMapResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);

	WarpYieldGuard guard(bridgeSunset.GetKernel());
	bool result = resource->Map();

	request.DoLock();
	request << result;
	request.UnLock();
}

void SnowyStream::RequestUnmapResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);

	resource->UnMap();
}

void SnowyStream::RequestModifyResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& action, IScript::Request::Arguments payload) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);

	resource->ScriptModify(request, action, payload);
}

void SnowyStream::RequestPersistResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& extension) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);

	WarpYieldGuard guard(bridgeSunset.GetKernel());
	bool result = SaveResource(resource.Get(), extension);
	request.DoLock();
	request << result;
	request.UnLock();
}

class CompressTask : public TaskOnce {
public:
	CompressTask(SnowyStream& s) : snowyStream(s), refreshRuntime(false) {}
	void Execute(void* context) override {
		OPTICK_EVENT();

		resource->Map();
		bool success = resource->Compress(compressType, refreshRuntime);
		if (callback) {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
			request.DoLock();
			request.Push();
			request.Call(callback, success);
			request.Pop();
			request.Dereference(callback);
			request.UnLock();
			bridgeSunset.requestPool.ReleaseSafe(&request);
		} else {
			Finalize(context);
		}

		resource->UnMap();
		ITask::Delete(this);
	}

	void Abort(void* context) override {
		Finalize(context);
		ITask::Delete(this);
	}

	void Finalize(void* context) {
		if (callback) {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
			request.DoLock();
			request.Dereference(callback);
			request.UnLock();
			bridgeSunset.requestPool.ReleaseSafe(&request);
		}
	}

	SnowyStream& snowyStream;
	TShared<ResourceBase> resource;
	String compressType;
	bool refreshRuntime;
	IScript::Request::Ref callback;
};

void SnowyStream::RequestCompressResourceAsync(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& compressType, bool refreshRuntime, IScript::Request::Ref callback) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);
	WarpYieldGuard guard(bridgeSunset.GetKernel());
	void* p = ITask::Allocate(sizeof(CompressTask));
	CompressTask* task = new (p) CompressTask(*this);
	task->resource = resource.Get();
	task->refreshRuntime = refreshRuntime;
	task->compressType = std::move(compressType);
	task->callback = callback;

	bridgeSunset.GetKernel().GetThreadPool().Dispatch(task);
}

void SnowyStream::Uninitialize() {
	defMountInstance = nullptr;

	for (std::map<Unique, TShared<ResourceManager> >::const_iterator p = resourceManagers.begin(); p != resourceManagers.end(); ++p) {
		ResourceManager& resourceManager = *(*p).second();
		SharedLockGuardWriter guard(resourceManager.GetThreadApi(), resourceManager.GetLock());
		resourceManager.RemoveAll();
	}

	renderResourceManager = nullptr;
}

void SnowyStream::RegisterReflectedSerializers() {
	assert(resourceSerializers.empty()); // can only be registerred once

	// Commented resources may have dependencies, so we do not serialize/deserialize them at once.
	// PaintsNow recommends database-managed resource dependencies ...

	UniqueType<RenderResourceManager> renderResourceManagerType;
	RegisterReflectedSerializer(UniqueType<BufferResource>(), interfaces.render, nullptr, renderResourceManagerType);
	RegisterReflectedSerializer(UniqueType<FontResource>(), interfaces.fontBase, this, UniqueType<DeviceResourceManager<FontResource::DriverType> >());
	RegisterReflectedSerializer(UniqueType<MaterialResource>(), interfaces.render, nullptr, renderResourceManagerType);
	RegisterReflectedSerializer(UniqueType<ShaderResource>(), interfaces.render, nullptr, renderResourceManagerType);
	RegisterReflectedSerializer(UniqueType<MeshResource>(), interfaces.render, nullptr, renderResourceManagerType);
	RegisterReflectedSerializer(UniqueType<SkeletonResource>(), interfaces.render, nullptr, renderResourceManagerType);
	RegisterReflectedSerializer(UniqueType<TextureResource>(), interfaces.render, nullptr, renderResourceManagerType);
	RegisterReflectedSerializer(UniqueType<TextureArrayResource>(), interfaces.render, nullptr, renderResourceManagerType);
	RegisterReflectedSerializer(UniqueType<StreamResource>(), interfaces.archive, this, UniqueType<DeviceResourceManager<StreamResource::DriverType> >());

	TShared<ResourceManager> manager = resourceManagers[UniqueType<IRender>::Get()];
	assert(manager);
	renderResourceManager = manager->QueryInterface(UniqueType<RenderResourceManager>());
	assert(renderResourceManager);
}

bool SnowyStream::RegisterResourceManager(Unique unique, ResourceManager* resourceManager) {
	if (resourceManagers.count(unique) == 0) {
		resourceManagers[unique] = resourceManager;
		return true;
	} else {
		return false;
	}
}

bool SnowyStream::RegisterResourceSerializer(Unique unique, const String& extension, ResourceCreator* serializer) {
	if (resourceSerializers.count(extension) == 0) {
		resourceSerializers[extension] = std::make_pair(unique, serializer);
		return true;
	} else {
		return false;
	}
}

TShared<ResourceBase> SnowyStream::CreateResource(const String& path, const String& ext, bool openExisting, Tiny::FLAG flag) {
	OPTICK_EVENT();
	assert(!path.empty() || (!openExisting && !ext.empty()));
	// try to parse extension from location if no ext provided
	String location;
	String extension;
	if (ext.empty()) {
		String::size_type pos = path.rfind('.');
		if (pos == String::npos) return nullptr; // unknown resource type

		location = path.substr(0, pos);
		extension = path.substr(pos + 1);
	} else {
		location = path;
		extension = ext;
	}

	// Find resource serializer
	std::unordered_map<String, std::pair<Unique, TShared<ResourceCreator> > >::iterator p = resourceSerializers.find(extension);
	IArchive& archive = interfaces.archive;
	IFilterBase& protocol = interfaces.assetFilterBase;
	bool mapOnCreate = !!(flag & ResourceBase::RESOURCE_MAPPED);

	TShared<ResourceBase> resource;
	if (p != resourceSerializers.end()) {
		// query manager
		std::map<Unique, TShared<ResourceManager> >::iterator t = resourceManagers.find((*p).second.first);
		assert(t != resourceManagers.end());
		ResourceManager& resourceManager = *(*t).second();

		if (!location.empty()) {
			TShared<ResourceBase> exist = resourceManager.LoadExistSafe(location);
			if (exist) {
				// Create failed, already exists
				if (!openExisting) {
					return nullptr;
				} else {
					if (mapOnCreate) {
						exist->Map();
					}

					return exist;
				}
			}
		}

		resource = (*p).second.second->Create(resourceManager, location);
		resource->Flag().fetch_or(flag & ~ResourceBase::RESOURCE_MAPPED, std::memory_order_relaxed);

		if (!location.empty()) {
			SharedLockGuardWriter guard(resourceManager.GetThreadApi(), resourceManager.GetLock());
			TShared<ResourceBase> exist = resourceManager.LoadExist(location);
			if (exist) {
				guard.UnLock();
				if (mapOnCreate) {
					exist->Map();
				}

				return exist;
			} else {
				resourceManager.Insert(resource());
			}
		} else {
			resourceManager.Insert(resource());
		}

		if (!(resource->Flag().load(std::memory_order_relaxed) & ResourceBase::RESOURCE_MANUAL_UPLOAD)) {
			if (resource->Map()) {
				resourceManager.InvokeUpload(resource());
			}

			if (!mapOnCreate) {
				resource->UnMap();
			}
		} else if (mapOnCreate) {
			resource->Map();
		}
	}

	return resource;
}

String SnowyStream::GetReflectedExtension(Unique unique) {
	return unique->GetBriefName();
}

bool SnowyStream::LoadResource(const TShared<ResourceBase>& resource, const String& extension) {
	OPTICK_EVENT();
	// Find resource serializer
	assert(resource);
	String typeExtension = extension.empty() ? GetReflectedExtension(resource->GetUnique()) : extension;
	IArchive& archive = interfaces.archive;
	IFilterBase& protocol = interfaces.assetFilterBase;

	std::unordered_map<String, std::pair<Unique, TShared<ResourceCreator> > >::iterator p = resourceSerializers.find(typeExtension);
	if (p != resourceSerializers.end()) {
		// query manager
		std::map<Unique, TShared<ResourceManager> >::iterator t = resourceManagers.find((*p).second.first);
		assert(t != resourceManagers.end());

		uint64_t length;
		IStreamBase* stream = resource->OpenArchive(archive, (*p).second.second->GetExtension(), false, length);
		// IStreamBase* stream = archive.Open(resource->GetLocation() + "." + (*p).second.second->GetExtension() + (*t).second->GetLocationPostfix() , false, length);

		bool result = false;
		if (stream != nullptr) {
			IStreamBase* filter = protocol.CreateFilter(*stream);
			assert(filter != nullptr);

			ThreadPool& threadPool = bridgeSunset.GetThreadPool();
			
			if (threadPool.PollExchange(threadPool.GetCurrentThreadIndex(), resource->critical, 1u) == 0u) {
				result = *filter >> *resource();
				resource->Flag().fetch_or(Tiny::TINY_MODIFIED);
				SpinUnLock(resource->critical);
			}

			filter->Destroy();
			stream->Destroy();

			ResourceManager& resourceManager = *(*t).second();
			resourceManager.InvokeRefresh(resource());
		}

		return result;
	} else {
		return false;
	}
}

bool SnowyStream::SaveResource(const TShared<ResourceBase>& resource, const String& extension) {
	OPTICK_EVENT();

	// Find resource serializer
	assert(resource);
	// assert(resource->IsMapped());
	String typeExtension = extension.empty() ? GetReflectedExtension(resource->GetUnique()) : extension;
	IArchive& archive = interfaces.archive;
	IFilterBase& protocol = interfaces.assetFilterBase;

	std::unordered_map<String, std::pair<Unique, TShared<ResourceCreator> > >::iterator p = resourceSerializers.find(typeExtension);
	if (p != resourceSerializers.end()) {
		// query manager
		std::map<Unique, TShared<ResourceManager> >::iterator t = resourceManagers.find((*p).second.first);
		assert(t != resourceManagers.end());

		bool result = false;
		ThreadPool& threadPool = bridgeSunset.GetThreadPool();
		if (threadPool.PollExchange(threadPool.GetCurrentThreadIndex(), resource->critical, 1u) == 0u) {
			uint64_t length;
			IStreamBase* stream = archive.Open(resource->GetLocation() + "." + (*p).second.second->GetExtension() + (*t).second->GetLocationPostfix(), true, length);

			if (stream != nullptr) {
				IStreamBase* filter = protocol.CreateFilter(*stream);
				assert(filter != nullptr);
				result = *filter << *resource();

				filter->Destroy();
				stream->Destroy();
			}

			SpinUnLock(resource->critical);
		}

		return result;
	} else {
		return false;
	}
}

IReflectObject* MetaResourceExternalPersist::Clone() const {
	return new MetaResourceExternalPersist(*this);
}

bool MetaResourceExternalPersist::Read(IStreamBase& streamBase, void* ptr) const {
	IReflectObject& env = streamBase.GetEnvironment();
	assert(!env.IsBasicObject());
	assert(env.QueryInterface(UniqueType<SnowyStream>()) != nullptr);

	SnowyStream& snowyStream = static_cast<SnowyStream&>(env);
	TShared<ResourceBase>& resource = *reinterpret_cast<TShared<ResourceBase>*>(ptr);
	String path;
	if (streamBase >> path) {
		resource = snowyStream.CreateResource(path);
	}

	return resource;
}

bool MetaResourceExternalPersist::Write(IStreamBase& streamBase, const void* ptr) const {
	IReflectObject& env = streamBase.GetEnvironment();
	assert(!env.IsBasicObject());
	assert(env.QueryInterface(UniqueType<SnowyStream>()) != nullptr);

	const TShared<ResourceBase>& resource = *reinterpret_cast<const TShared<ResourceBase>*>(ptr);
	String path;
	if (resource) {
		path = resource->GetLocation() + "." + resource->GetUnique()->GetBriefName();
	}

	return streamBase << path;
}

String MetaResourceExternalPersist::GetUniqueName() const {
	return uniqueName;
}

MetaResourceExternalPersist::MetaResourceExternalPersist() {}