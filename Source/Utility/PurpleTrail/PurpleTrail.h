// PurpleTrail.h
// PaintDream (paintdream@paintdream.com)
// 2015-1-2
//

#pragma once
#include "../../Core/Interface/IScript.h"

namespace PaintsNow {
	class PurpleTrail : public TReflected<PurpleTrail, IScript::Library> {
	public:
		PurpleTrail();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		~PurpleTrail() override;

		class IInspectPrimitive;
	protected:
		/// <summary>
		/// Inspect a reflected object.
		/// </summary>
		/// <param name="object"> reflected object </param>
		/// <returns> reflect info </returns>
		void RequestInspect(IScript::Request& request, IScript::BaseDelegate object);

		/// <summary>
		/// Get content of any reflected object
		/// </summary>
		/// <param name="object"> reflected object </param>
		/// <param name="path"> path list </param>
		void RequestGetValue(IScript::Request& request, IScript::BaseDelegate d, const std::vector<String>& path);

		/// <summary>
		/// Set content of any reflected object
		/// </summary>
		/// <param name="object"> reflected object </param>
		/// <param name="path"> path list </param>
		/// <param name="arguments"> target value </param>
		void RequestSetValue(IScript::Request& request, IScript::BaseDelegate d, const std::vector<String>& path, IScript::Request::Arguments value);

	private:
		std::unordered_map<Unique, IInspectPrimitive*> mapInspectors;
	};
}
