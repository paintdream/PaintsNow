// IDevice.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-12
//

#pragma once

#include "../PaintsNow.h"

namespace PaintsNow {
	class pure_interface IDevice {
	public:
		virtual ~IDevice();
		virtual void ReleaseDevice();
	};
}

