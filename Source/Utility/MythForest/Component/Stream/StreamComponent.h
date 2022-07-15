// StreamComponent.h
// PaintDream (paintdream@paintdream.com)
// 2020-04-30
//

#pragma once
#include "../../Component.h"

namespace PaintsNow {
	class StreamComponent : public TAllocatedTiny<StreamComponent, Component> {
	public:
		StreamComponent(const UShort3& dimension, uint16_t cacheCount);
		SharedTiny* Load(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& context);
		void Unload(Engine& engine, const UShort3& coord, const TShared<SharedTiny>&context);
		void SetLoadHandler(IScript::Request& request, IScript::Request::Ref ref);
		void SetLoadHandler(const TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>& >& handler);
		void SetRefreshHandler(IScript::Request& request, IScript::Request::Ref ref);
		void SetRefreshHandler(const TWrapper<void, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>& >& handler);
		void SetUnloadHandler(IScript::Request& request, IScript::Request::Ref ref);
		void SetUnloadHandler(const TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>& >& handler);
		void Uninitialize(Engine& engine, Entity* entity) override;
		const UShort3& GetDimension() const;
		UShort3 ComputeWrapCoordinate(const Int3& pos) const;
		uint16_t GetCacheCount() const;

	protected:
		class Grid {
		public:
			Grid() : coord(0, 0, 0), recycleIndex(0) {}
			TShared<SharedTiny> object;
			UShort3 coord;
			uint16_t recycleIndex;
		};

		void UnloadInternal(Engine& engine, Grid& grid, const TShared<SharedTiny>&context);

		UShort3 dimension;
		uint16_t recycleStart;
		std::vector<Grid> grids;
		std::vector<uint16_t> idGrids;
		std::vector<uint16_t> recycleQueue;

		template <class T>
		class Handler {
		public:
			void ReplaceScript(IScript::Request& request, IScript::Request::Ref ref) {
				if (script != ref && script) {
					request.DoLock();
					request.Dereference(script);
					script = ref;
					request.UnLock();
				}
			}

			T native;
			IScript::Request::Ref script;
		};

		Handler<TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>& > > loadHandler;
		Handler<TWrapper<void, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>& > > refreshHandler;
		Handler<TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>& > > unloadHandler;
	};
}

