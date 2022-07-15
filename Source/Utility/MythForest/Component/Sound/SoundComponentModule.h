// SoundComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "SoundComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class SoundComponent;
	class SoundComponentModule : public TReflected<SoundComponentModule, ModuleImpl<SoundComponent> > {
	public:
		SoundComponentModule(Engine& engine);
		~SoundComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		// static int main(int argc, char* argv[]);

	public:

		/// <summary>
		/// Create SoundComponent
		/// </summary>
		/// <param name="stream"> sound resource stream </param>
		/// <param name="callback"></param>
		/// <returns> the sound component </returns>
		TShared<SoundComponent> RequestNew(IScript::Request& request, IScript::Delegate<StreamResource>, IScript::Request::Ref callback);

		/// <summary>
		/// Get source duration
		/// </summary>
		/// <param name="soundComponent"> the SoundComponent </param>
		/// <returns> duration </returns>
		double RequestGetSourceDuration(IScript::Request& request, IScript::Delegate<SoundComponent> soundComponent);

		/// <summary>
		/// Seek source 
		/// </summary>
		/// <param name="soundComponent"> the SoundComponent =</param>
		/// <param name="time"> time </param>
		void RequestSeekSource(IScript::Request& request, IScript::Delegate<SoundComponent> soundComponent, double time);

		/// <summary>
		/// Play souce
		/// </summary>
		/// <param name="soundComponent"> the SoundComponent </param>
		void RequestPlaySource(IScript::Request& request, IScript::Delegate<SoundComponent> soundComponent);

		/// <summary>
		/// Pause souce
		/// </summary>
		/// <param name="soundComponent"> the SoundComponent </param>
		void RequestPauseSource(IScript::Request& request, IScript::Delegate<SoundComponent> soundComponent);

		/// <summary>
		/// Stop souce
		/// </summary>
		/// <param name="soundComponent"> the SoundComponent </param>
		void RequestStopSource(IScript::Request& request, IScript::Delegate<SoundComponent> soundComponent);

		/// <summary>
		/// Rewind souce
		/// </summary>
		/// <param name="soundComponent"> the SoundComponent </param>
		void RequestRewindSource(IScript::Request& request, IScript::Delegate<SoundComponent> soundComponent);

		/// <summary>
		/// Check if source is paused
		/// </summary>
		/// <param name="soundComponent"> the SoundComponent </param>
		/// <returns> true if paused </returns>
		bool RequestIsSourcePaused(IScript::Request& request, IScript::Delegate<SoundComponent> soundComponent);
	};
}

