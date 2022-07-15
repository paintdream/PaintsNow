// Engine.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-1
//

#pragma once
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "../../Core/System/Tiny.h"
#include "../../Core/Template/TMap.h"
#include "../../General/Interface/Interfaces.h"
#include "../../Core/System/Kernel.h"

namespace PaintsNow {
	class Interfaces;
	class BridgeSunset;
	class SnowyStream;
	class Unit;
	class Entity;
	class Module;
	class Engine : public ISyncObject {
	public:
		Engine(Interfaces& interfaces, BridgeSunset& bridgeSunset, SnowyStream& snowyStream);

		~Engine() override;
		void Clear();
		void InstallModule(Module* module);
		Module* GetComponentModuleFromName(const String& name) const;
		std::unordered_map<String, Module*>& GetModuleMap();
		void TickFrame();
		Kernel& GetKernel();

		Interfaces& interfaces;
		BridgeSunset& bridgeSunset;
		SnowyStream& snowyStream;

		void NotifyUnitConstruct(Unit* unit);
		void NotifyUnitDestruct(Unit* unit);
		void NotifyUnitAttach(Unit* child, Unit* parent);
		void NotifyUnitDetach(Unit* child);
		uint64_t GetFrameTickDelta() const;
		uint64_t GetFrameTimestamp() const;

	protected:
		Engine(const Engine& engine);
		Engine& operator = (const Engine& engine);

		std::atomic<size_t> unitCount;
		IThread::Event* finalizeEvent;
		std::unordered_map<String, Module*> moduleMap;
		std::vector<Module*> moduleList;
		uint64_t lastFrameTimestamp;
		uint64_t currentFrameTickDelta;

#ifdef _DEBUG
		std::map<Unit*, Unit*> entityMap;
		std::atomic<size_t> unitCritical;
#endif
	};

#define CHECK_THREAD_IN_MODULE(warpTiny) \
	(MUST_CHECK_REFERENCE_ONCE); \
	if (engine.GetKernel().GetCurrentWarpIndex() != warpTiny->GetWarpIndex()) { \
		request.Error("Threading routine failed on " #warpTiny); \
		assert(false); \
	}
}

