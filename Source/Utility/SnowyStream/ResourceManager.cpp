#include "ResourceBase.h"
#include "ResourceManager.h"
#include "../../Core/Interface/IArchive.h"

using namespace PaintsNow;

ResourceManager::ResourceManager(Kernel& ker, IUniformResourceManager& hostManager, IStreamBase& info, IStreamBase& err, void* c) : ISyncObjectShared(ker.GetThreadPool().GetThreadApi()), kernel(ker), uniformResourceManager(hostManager), logInfoStream(info), logErrorStream(err), context(c) {
}

IStreamBase& ResourceManager::LogInfo() {
	return logInfoStream;
}

IStreamBase& ResourceManager::LogError() {
	return logErrorStream;
}

const String& ResourceManager::GetLocationPostfix() const {
	static const String uniExtension = ".pod";
	return uniExtension;
}

void ResourceManager::TickDevice(IDevice& device) {}

ResourceManager::~ResourceManager() {
	assert(resourceMap.empty());
}

void ResourceManager::Report(const String& err) {
	logErrorStream.Printf("[ResourceManager] Error: %s\n", err.c_str());
}

ThreadPool& ResourceManager::GetThreadPool() {
	return kernel.GetThreadPool();
}

Kernel& ResourceManager::GetKernel() {
	return kernel;
}

void ResourceManager::RemoveAll() {
	assert(IsLocked());

	// remove all eternal resources
	std::unordered_map<String, ResourceBase*> temp;
	std::swap(temp, resourceMap);

	std::vector<ResourceBase*> externals;
	for (std::unordered_map<String, ResourceBase*>::const_iterator it = temp.begin(); it != temp.end(); ++it) {
		ResourceBase* resource = (*it).second;
		InvokeDetach(resource, GetContext());

		if (resource->Flag().load(std::memory_order_acquire) & ResourceBase::RESOURCE_ETERNAL) {
			externals.emplace_back(resource);
		}

		resource->Flag().fetch_or(ResourceBase::RESOURCE_ORPHAN, std::memory_order_relaxed);
	}

	UnLockWriter();
	for (size_t i = 0; i < externals.size(); i++) {
		externals[i]->ReleaseObject();
	}
	DoLockWriter();
}

void ResourceManager::Insert(ResourceBase* resource) {
	assert(resource != nullptr);
	assert(resource->Flag().load(std::memory_order_acquire) & ResourceBase::RESOURCE_ORPHAN);

	// allowing anounymous resource
	const String& id = resource->GetLocation();
	if (!id.empty()) {
		assert(IsLocked());
		assert(!id.empty());
		assert(&resource->GetResourceManager() == this);
		assert(resourceMap.find(id) == resourceMap.end());

		resourceMap[id] = resource;

		if (resource->Flag().load(std::memory_order_acquire) & ResourceBase::RESOURCE_ETERNAL) {
			resource->ReferenceObject();
		}

		if (resource->Flag().fetch_and(~ResourceBase::RESOURCE_ORPHAN, std::memory_order_release) & ResourceBase::RESOURCE_ORPHAN) {
			InvokeAttach(resource, GetContext());
		}
	} else {
		InvokeAttach(resource, GetContext());
	}
}

IUniformResourceManager& ResourceManager::GetUniformResourceManager() {
	return uniformResourceManager;
}

TShared<ResourceBase> ResourceManager::LoadExist(const String& id) {
	assert(IsLocked());
	std::unordered_map<String, ResourceBase*>::iterator p = resourceMap.find(id);
	ResourceBase* pointer = nullptr;
	if (p != resourceMap.end()) {
		pointer = (*p).second;
		assert(pointer != nullptr);
	}

	return pointer;
}

TShared<ResourceBase> ResourceManager::LoadExistSafe(const String& id) {
	SharedLockGuardReader guard(threadApi, mutex);
	TShared<ResourceBase> pointer = LoadExist(id);

	return pointer;
}

void* ResourceManager::GetContext() const {
	return context;
}

void ResourceManager::Remove(ResourceBase* resource) {
	assert(IsLocked());
	assert(resource != nullptr);
	if (resource->Flag().load(std::memory_order_acquire) & (ResourceBase::RESOURCE_ORPHAN | ResourceBase::RESOURCE_ETERNAL))
		return;

	assert(!resource->GetLocation().empty());
	resourceMap.erase(resource->GetLocation());

	if (!(resource->Flag().fetch_or(ResourceBase::RESOURCE_ORPHAN, std::memory_order_release) & ResourceBase::RESOURCE_ORPHAN)) {
		assert(resource->Flag().load(std::memory_order_acquire) & ResourceBase::RESOURCE_ATTACHED);

		UnLockWriter();
		InvokeDetach(resource, GetContext());
		DoLockWriter();
	}
}

ResourceCreator::~ResourceCreator() {}

