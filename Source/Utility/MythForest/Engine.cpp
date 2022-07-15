#include "Engine.h"
#include "Module.h"
#include "Entity.h"
#include "../../General/Interface/IRender.h"
#include "../BridgeSunset/BridgeSunset.h"
#include "../SnowyStream/SnowyStream.h"
#include "../SnowyStream/Manager/RenderResourceManager.h"

using namespace PaintsNow;

Engine::Engine(Interfaces& pinterfaces, BridgeSunset& pbridgeSunset, SnowyStream& psnowyStream) : ISyncObject(pinterfaces.thread), interfaces(pinterfaces), bridgeSunset(pbridgeSunset), snowyStream(psnowyStream), lastFrameTimestamp(0), currentFrameTickDelta(1) {
	unitCount.store(0, std::memory_order_relaxed);
	finalizeEvent = interfaces.thread.NewEvent();
}

Engine::~Engine() {
	assert(moduleMap.empty());
	assert(moduleList.empty());
	interfaces.thread.DeleteEvent(finalizeEvent);
}

void Engine::Clear() {
	bridgeSunset.LogInfo().Printf("[MythForest::Engine] Clean up ...\n");

	for (size_t i = moduleList.size(); i != 0; i--) {
		Module* module = moduleList[i - 1];
		bridgeSunset.LogInfo().Printf("[MythForest::Engine] Uninitialize Module: %s\n", module->GetTinyUnique()->GetBriefName().c_str());
		module->Uninitialize();
	}

	bridgeSunset.LogInfo().Printf("[MythForest::Engine] Waiting for allocated entities finalizing ...\n");

#ifdef _DEBUG
	size_t waitTime = 0;
	bool logOutput = false;
#endif

	while (unitCount.load(std::memory_order_acquire) != 0) {
		DoLock();
		threadApi.Wait(finalizeEvent, mutex, 50);
		UnLock();

#ifdef _DEBUG
		if ((waitTime += 50) > 10000 && !logOutput) {
			// print entity map out ...
			TSpinLockGuard<size_t> guard(unitCritical);
			std::set<Unit*> parentSet;
			for (std::map<Unit*, Unit*>::iterator it = entityMap.begin(); it != entityMap.end(); ++it) {
				if ((*it).second == nullptr) {
					parentSet.insert((*it).first);
				} else {
					parentSet.insert((*it).second);
				}
			}

			for (std::set<Unit*>::iterator ip = parentSet.begin(); ip != parentSet.end(); ++ip) {
				Unit* unit = *ip;
				// if (entityMap[unit] == nullptr) {
				bridgeSunset.LogError().Printf("Nested -> [%03d] Isolated Unit: %p, type: %s\n", unit->GetReferenceCount(), unit, unit->GetUnique()->GetBriefName().c_str());
				// }
			}

			waitTime = 0;
			logOutput = true;
		}
#endif
	}

	bridgeSunset.LogInfo().Printf("[MythForest::Engine] Destroy modules ...\n");

	for (size_t k = moduleList.size(); k != 0; k--) {
		moduleList[k - 1]->Destroy();
	}

	moduleMap.clear();
	moduleList.clear();
}

void Engine::InstallModule(Module* module) {
	String name = module->GetTinyUnique()->GetBriefName();
	assert(moduleMap.find(name) == moduleMap.end());
	moduleMap[name] = module;
	moduleList.push_back(module);
	bridgeSunset.LogInfo().Printf("[MythForest::Engine] Initialize Module: %s\n", name.c_str());

	module->Initialize();
}

Module* Engine::GetComponentModuleFromName(const String& name) const {
	std::unordered_map<String, Module*>::const_iterator it = moduleMap.find(name);
	if (it != moduleMap.end()) {
		return (*it).second;
	} else {
		return nullptr;
	}
}

std::unordered_map<String, Module*>& Engine::GetModuleMap() {
	return moduleMap;
}

uint64_t Engine::GetFrameTickDelta() const {
	return currentFrameTickDelta;
}

uint64_t Engine::GetFrameTimestamp() const {
	return lastFrameTimestamp;
}

void Engine::TickFrame() {
	uint64_t t = ITimer::GetSystemClock();
	currentFrameTickDelta = t - lastFrameTimestamp;
	lastFrameTimestamp = t;

	for (size_t i = 0; i < moduleList.size(); i++) {
		moduleList[i]->TickFrame();
	}
}

Kernel& Engine::GetKernel() {
	return bridgeSunset.GetKernel();
}

void Engine::NotifyUnitConstruct(Unit* unit) {
	unitCount.fetch_add(1, std::memory_order_relaxed);
#if defined(_DEBUG)
	singleton Unique entityUnique = UniqueType<Entity>::Get();
	if (unit->GetUnique() != entityUnique) return;

	TSpinLockGuard<size_t> guard(unitCritical);
	assert(entityMap.find(unit) == entityMap.end());
	entityMap[unit] = nullptr;
#endif
}

void Engine::NotifyUnitDestruct(Unit* unit) {
	if (unitCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
		interfaces.thread.Signal(finalizeEvent);
	}

#if defined(_DEBUG)
	singleton Unique entityUnique = UniqueType<Entity>::Get();
	if (unit->GetUnique() != entityUnique) return;

	TSpinLockGuard<size_t> guard(unitCritical);
	assert(entityMap.find(unit) != entityMap.end());
	entityMap.erase(unit);
#endif
}

void Engine::NotifyUnitAttach(Unit* unit, Unit* parent) {
#if defined(_DEBUG)
	singleton Unique entityUnique = UniqueType<Entity>::Get();
	assert(unit->GetUnique() == entityUnique && parent->GetUnique() == entityUnique);

	TSpinLockGuard<size_t> guard(unitCritical);
	assert(parent != nullptr);
	assert(entityMap.find(unit) != entityMap.end());
	assert(entityMap[unit] == nullptr);
	Unit* p = parent;
	while (true) {
		assert(p != unit); // cycle detected
		std::map<Unit*, Unit*>::const_iterator it = entityMap.find(p);
		if (it == entityMap.end())
			break;

		p = (*it).second;
	}

	entityMap[unit] = parent;
#endif
}

void Engine::NotifyUnitDetach(Unit* unit) {
#if defined(_DEBUG)
	singleton Unique entityUnique = UniqueType<Entity>::Get();
	assert(unit->GetUnique() == entityUnique);

	TSpinLockGuard<size_t> guard(unitCritical);
	assert(entityMap.find(unit) != entityMap.end());
	assert(entityMap[unit] != nullptr);
	entityMap[unit] = nullptr;
#endif
}