// GalaxyWeaver
// By PaintDream
// 

#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/ITunnel.h"
#include "Weaver.h"
#include "../MythForest/Entity.h"

namespace PaintsNow {
	class GalaxyWeaver : public TReflected<GalaxyWeaver, IScript::Library> {
	public:
		GalaxyWeaver(IThread& threadApi, ITunnel& network, BridgeSunset& bridgeSunset, SnowyStream& snowyStream, MythForest& mythForest);
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		/// <summary>
		/// Queue a new weaver object
		/// </summary>
		/// <param name="config"> Weaver config </param>
		/// <returns> Weaver object </returns>
		TShared<Weaver> RequestNewWeaver(IScript::Request& request, const String& config);

		/// <summary>
		/// Set RPC callback for weaver
		/// </summary>
		/// <param name="weaver"> Weaver object </param>
		/// <param name="callback"> rpc callback </param>
		/// <returns></returns>
		void RequestSetWeaverRpcCallback(IScript::Request& request, IScript::Delegate<Weaver> weaver, IScript::Request::Ref callback);

		/// <summary>
		/// Set connection callback for weaver
		/// </summary>
		/// <param name="weaver"> Weaver object </param>
		/// <param name="callback"> connection callback </param>
		/// <returns></returns>
		void RequestSetWeaverConnectionCallback(IScript::Request& request, IScript::Delegate<Weaver> weaver, IScript::Request::Ref callback);

		/// <summary>
		/// Start weaver
		/// </summary>
		/// <param name="weaver"> Weaver object </param>
		/// <returns></returns>
		void RequestStartWeaver(IScript::Request& request, IScript::Delegate<Weaver> weaver);

		/// <summary>
		/// Stop weaver
		/// </summary>
		/// <param name="weaver"> Weaver object </param>
		/// <returns></returns>
		void RequestStopWeaver(IScript::Request& request, IScript::Delegate<Weaver> weaver);

	protected:
		ITunnel& network;
		BridgeSunset& bridgeSunset;
		SnowyStream& snowyStream;
		MythForest& mythForest;
	};
}

