#include "StreamComponent.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
#include <utility>

using namespace PaintsNow;

StreamComponent::StreamComponent(const UShort3& dim, uint16_t cacheCount) : dimension(dim), recycleStart(0) {
	assert(dim.x() != 0 && dim.y() != 0 && dim.z() != 0);
	idGrids.resize(dim.x() * dim.y() * dim.z(), (uint16_t)~0);
	grids.resize(cacheCount);
	recycleQueue.resize(cacheCount);
	for (uint16_t i = 0; i < cacheCount; i++) {
		recycleQueue[i] = i;
	}
}

UShort3 StreamComponent::ComputeWrapCoordinate(const Int3& pos) const {
	return UShort3(
		verify_cast<uint16_t>((pos.x() % dimension.x() + dimension.x()) % dimension.x()),
		verify_cast<uint16_t>((pos.y() % dimension.y() + dimension.y()) % dimension.y()),
		verify_cast<uint16_t>((pos.z() % dimension.z() + dimension.z()) % dimension.z()));
}

uint16_t StreamComponent::GetCacheCount() const {
	return verify_cast<uint16_t>(grids.size());
}

void StreamComponent::Unload(Engine& engine, const UShort3& coord, const TShared<SharedTiny>&context) {
	uint16_t id = idGrids[(coord.z() * dimension.y() + coord.y()) * dimension.x() + coord.x()];
	if (id == (uint16_t)~0) return;

	UnloadInternal(engine, grids[id], context);
}

void StreamComponent::UnloadInternal(Engine& engine, Grid& grid, const TShared<SharedTiny>&context) {
	OPTICK_EVENT();
	if (unloadHandler.script) {
		IScript::Request& request = *engine.bridgeSunset.requestPool.AcquireSafe();
		IScript::Delegate<SharedTiny> w;

		request.DoLock();
		request.Push();
		request.Call(unloadHandler.script, grid.coord, grid.object, context);
		request >> w;
		request.Pop();
		request.UnLock();

		grid.object = w.Get();
		engine.bridgeSunset.requestPool.ReleaseSafe(&request);
	} else if (unloadHandler.native) {
		grid.object = unloadHandler.native(engine, grid.coord, grid.object, context);
	}
}

SharedTiny* StreamComponent::Load(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& context) {
	OPTICK_EVENT();
	size_t offset = (coord.z() * dimension.y() + coord.y()) * dimension.x() + coord.x();
	uint16_t id = idGrids[offset];
	SharedTiny* object = nullptr;
	while (true) {
		if (id == (uint16_t)~0) {
			// allocate id ...
			id = recycleQueue[recycleStart];
			Grid& grid = grids[id];

			if (grid.object) {
				UnloadInternal(engine, grid, context);
			}

			TShared<SharedTiny> last = grid.object;
			grid.recycleIndex = recycleStart;
			assert(recycleQueue[grid.recycleIndex] == id);
			recycleStart = (recycleStart + 1) % verify_cast<uint16_t>(recycleQueue.size());

			if (loadHandler.script) {
				IScript::Request& request = *engine.bridgeSunset.requestPool.AcquireSafe();
				IScript::Delegate<SharedTiny> w;

				request.DoLock();
				request.Push();
				request.Call(loadHandler.script, coord, last, context);
				request >> w;
				request.Pop();
				request.UnLock();

				assert(w);
				grid.object = w.Get();
				engine.bridgeSunset.requestPool.ReleaseSafe(&request);
			} else {
				assert(loadHandler.native);
				grid.object = loadHandler.native(engine, coord, last, context);
			}

			grid.coord = coord;
			object = grid.object();
			idGrids[offset] = id;
			break;
		} else {
			Grid& grid = grids[id];

			if (grid.coord == coord) {
				if ((grid.recycleIndex + 1) % verify_cast<uint16_t>(recycleQueue.size()) != recycleStart) {
					std::swap(recycleQueue[recycleStart], recycleQueue[grid.recycleIndex]);
					std::swap(grids[recycleStart].recycleIndex, grid.recycleIndex);

					recycleStart = (recycleStart + 1) % verify_cast<uint16_t>(recycleQueue.size());
				}

				object = grid.object();
				break;
			} else {
				id = (uint16_t)~0; // invalid, must reload
			}
		}
	}

	if (refreshHandler.script) {
		IScript::Request& request = *engine.bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(refreshHandler.script, coord, object, context);
		request.Pop();
		request.UnLock();
		engine.bridgeSunset.requestPool.ReleaseSafe(&request);
	} else if (refreshHandler.native) {
		refreshHandler.native(engine, coord, object, context);
	}

	return object;
}

const UShort3& StreamComponent::GetDimension() const {
	return dimension;
}

void StreamComponent::Uninitialize(Engine& engine, Entity* entity) {
	if (!engine.interfaces.script.IsResetting()) {
		IScript::Request& request = engine.interfaces.script.GetDefaultRequest();
		SetLoadHandler(request, IScript::Request::Ref());
		SetRefreshHandler(request, IScript::Request::Ref());
		SetUnloadHandler(request, IScript::Request::Ref());
	} else {
		loadHandler.script = IScript::Request::Ref();
		refreshHandler.script = IScript::Request::Ref();
		unloadHandler.script = IScript::Request::Ref();
	}

	BaseClass::Uninitialize(engine, entity);
}

void StreamComponent::SetLoadHandler(IScript::Request& request, IScript::Request::Ref ref) {
	loadHandler.ReplaceScript(request, ref);
}

void StreamComponent::SetRefreshHandler(IScript::Request& request, IScript::Request::Ref ref) {
	refreshHandler.ReplaceScript(request, ref);
}

void StreamComponent::SetUnloadHandler(IScript::Request& request, IScript::Request::Ref ref) {
	unloadHandler.ReplaceScript(request, ref);
}

void StreamComponent::SetLoadHandler(const TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>& >& handler) {
	loadHandler.native = handler;
}

void StreamComponent::SetRefreshHandler(const TWrapper<void, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>& >& handler) {
	refreshHandler.native = handler;
}

void StreamComponent::SetUnloadHandler(const TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>& >& handler) {
	unloadHandler.native = handler;
}

