// Quota.h
// PaintDream (paintdream@paintdream.com)
// 2021-12-23
//

#pragma once
#include "../Template/TAtomic.h"

namespace PaintsNow {
	class Quota {
	public:
		Quota(size_t size);
		bool AcquireQuota(size_t count);
		void ReleaseQuota(size_t count);
		size_t GetQuota() const;
		size_t GetCapacity() const;
	
	protected:
		std::atomic<size_t> allocated;
		size_t capacity;
	};
}
