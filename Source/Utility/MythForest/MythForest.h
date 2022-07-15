// MythForest.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-1
//

#pragma once
#include "../SnowyStream/SnowyStream.h"
#include "../../Core/Interface/IScript.h"
#include "Entity.h"
#include "Engine.h"
#include "Module.h"

namespace PaintsNow {
	class MythForest : public TReflected<MythForest, IScript::Library> {
	public:
		MythForest(Interfaces& interfaces, SnowyStream& snowyStream, BridgeSunset& bridgeSunset);
		~MythForest() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		// static int main(int argc, char* argv[]);
		void TickDevice(IDevice& device) override;
		void Initialize() override;
		void Uninitialize() override;
		void ScriptUninitialize(IScript::Request& request) override;
		void OnSize(const Int2& size);
		void OnMouse(const IFrame::EventMouse& mouse);
		void OnKeyboard(const IFrame::EventKeyboard& keyboard);
		void StartCaptureFrame(const String& path, const String& options);
		void EndCaptureFrame();
		void InvokeCaptureFrame(const String& path, const String& options);

		TShared<Entity> CreateEntity(int32_t warp = 0);
		Engine& GetEngine();

	public:
		/// <summary>
		/// Enumerate all component modules
		/// </summary>
		/// <returns> A component module list with { Module } </returns>
		void RequestEnumerateComponentModules(IScript::Request& request);

		/// <summary>
		/// Create new entity
		/// </summary>
		/// <param name="warp"> warp index </param>
		/// <returns></returns>
		TShared<Entity> RequestNewEntity(IScript::Request& request, int32_t warp);

		/// <summary>
		/// Add component to entity
		/// </summary>
		/// <param name="entity"> the Entity </param>
		/// <param name="component"> the Component to be added </param>
		void RequestAddEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Delegate<Component> component);

		/// <summary>
		/// Remove component from entity
		/// </summary>
		/// <param name="entity"> the Entity </param>
		/// <param name="component"> the Component to remove </param>
		void RequestRemoveEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Delegate<Component> component);

		/// <summary>
		/// Remove all components from entity
		/// </summary>
		/// <param name="entity"> the Entity </param>
		void RequestClearEntityComponents(IScript::Request& request, IScript::Delegate<Entity> entity);

		/// <summary>
		/// Update entity mask & boundingbox
		/// </summary>
		/// <param name="entity"> the Entity </param>
		void RequestUpdateEntity(IScript::Request& request, IScript::Delegate<Entity> entity, bool recursive);

		/// <summary>
		/// Get all components of an entity
		/// </summary>
		/// <param name="entity"> the Entity </param>
		/// <returns> A component list with { Component } </returns>
		void RequestGetEntityComponents(IScript::Request& request, IScript::Delegate<Entity> entity);

		/// <summary>
		/// Get type of given component
		/// </summary>
		/// <param name="component"> the Component </param>
		/// <returns> type of component (in string form) </returns>
		String RequestGetComponentType(IScript::Request& request, IScript::Delegate<Component> component);

		/// <summary>
		/// Get unique component from entity with specified type
		/// </summary>
		/// <param name="entity"> the Entity </param>
		/// <param name="componentType"> component type name</param>
		/// <returns> Component if exists, otherwise null would be returned </returns>
		TShared<Component> RequestGetUniqueEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, const String& componentType);

		/// <summary>
		/// Get last tick time of render device
		/// </summary>
		/// <returns> tick time </returns>
		uint64_t RequestGetFrameTickDelta(IScript::Request& request);

		/// <summary>
		/// Get bound of entity.
		/// </summary>
		/// <returns> bound </returns>
		Float3Pair RequestGetEntityBoundingBox(IScript::Request& request, IScript::Delegate<Entity> entity);

		/// <summary>
		/// Add callback for next device frame
		/// </summary>
		/// <param name="entity"> the Entity that holds the callback </param>
		/// <param name="callback"> the callback </param>
		void RequestWaitForNextFrame(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Request::Ref callback);

		/// <summary>
		/// Cast a ray from specified entity's space and gather the hit geometries
		/// </summary>
		/// <param name="entity"> the Entity </param>
		/// <param name="callback"> on hit callback </param>
		/// <param name="from"> ray start position </param>
		/// <param name="dir"> ray direction </param>
		/// <param name="count"> max hit count </param>
		void RequestRaycast(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Request::Ref callback, const Float3& from, const Float3& dir, uint32_t count);

		/// <summary>
		/// [Debug] capture a render frame (usually a renderdoc dump file)
		/// </summary>
		/// <param name="path"> output path </param>
		/// <param name="options"> options </param>
		void RequestCaptureFrame(IScript::Request& request, const String& path, const String& options);

		/// <summary>
		/// Post custom or update event to entity manually
		/// </summary>
		/// <param name="entity"> the entity </param>
		/// <param name="type"> event type, can be 'Update' or 'Custom' </param>
		/// <param name="sender"> event sender </param>
		/// <param name="param"> custom param </param>
		void RequestPostEvent(IScript::Request& request, IScript::Delegate<Entity> entity, const String& type, IScript::Delegate<SharedTiny> sender, IScript::Request::Ref param);

		// Build-in sub modules
	private:
		Engine engine;
		TShared<Entity::Allocator> entityAllocator;
		TQueueList<std::pair<TShared<Entity>, IScript::Request::Ref> > nextFrameListeners;
	};
}
