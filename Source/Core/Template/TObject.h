// TObject.h
// PaintDream (paintdream@paintdream.com)
// 2014-12-19
//

#pragma once
#include "../PaintsNow.h"
#include "../Template/TProxy.h"

namespace PaintsNow {
	template <class T>
	class pure_interface TObject {
	public:
		TObject() {}
		virtual ~TObject() {}
		virtual TObject& operator ()(T& t) = 0;
	};
}
