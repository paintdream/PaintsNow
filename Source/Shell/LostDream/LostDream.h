// LostDream.h - Simple unit testing
// PaintDream (paintdream@paintdream.com)
// 2017-12-30
//

#pragma once
#include "../../Core/Interface/IReflect.h"
#include <list>

namespace PaintsNow {
	class LostDream {
	public:
		class Qualifier : public TReflected<Qualifier, IReflectObjectComplex> {
		public:
			virtual ~Qualifier();
			virtual bool Initialize() = 0;
			virtual bool Run(int randomSeed, int length) = 0;
			virtual void Summary() = 0;
		};

		virtual ~LostDream();
		bool RegisterQualifier(const TWrapper<Qualifier*>& q, int count);
		bool RunQualifiers(bool stopOnError, int initRandomSeed, int length);

	protected:
		std::list<std::pair<Qualifier*, int> > qualifiers;
	};
}

