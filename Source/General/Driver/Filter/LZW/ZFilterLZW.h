// ZFilterLZW.h
// By PaintDream (paintdream@paintdream.com)
// 2015-6-10
//

#ifndef __ZFILTERLZW_H__
#define __ZFILTERLZW_H__

#include "../../../../Core/Interface/IFilterBase.h"

namespace PaintsNow {
	class ZFilterLZW final : public IFilterBase {
	public:
		virtual IStreamBase* CreateFilter(IStreamBase& streamBase);
	};
}

#endif // __ZFILTERLZW_H__