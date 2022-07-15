// IFilterBase.h
// PaintDream (paintdream@paintdream.com)
// 2014-12-13
//

#pragma once

#include "../PaintsNow.h"
#include "IDevice.h"
#include "IStreamBase.h"

namespace PaintsNow {
	class pure_interface IFilterBase : public IDevice {
	public:
		~IFilterBase() override;
		// attach a stream, which all read operations depend on.
		virtual IStreamBase* CreateFilter(IStreamBase& inputStream) = 0;
	};

	class NoFilter : public IFilterBase {
	public:
		IStreamBase* CreateFilter(IStreamBase& inputStream) override;
	};
}

