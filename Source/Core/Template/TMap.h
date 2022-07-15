// TMap.h
// PaintDream (paintdream@paintdream.com)
// 2015-3-6
//

#pragma once
#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma warning (disable:4786)
#pragma warning (disable:4503)

#include "../PaintsNow.h"
#include "../Interface/IType.h"
#include <string>
#include <sgistl/hash_map>

namespace std {
	template <class K, class V, class P = hash<K> >
	class unordered_map : public hash_map<K, V, P> {};
}

template <>
struct hash<PaintsNow::String> {
	size_t operator () (const PaintsNow::String& str) const {
		const size_t _FNV_offset_basis = 2166136261U;
		const size_t _FNV_prime = 16777619U;
		const unsigned char* _First = (const unsigned char*)str.data();
		size_t _Count = str.size();
		size_t _Val = _FNV_offset_basis;
		for (size_t _Next = 0; _Next < _Count; ++_Next) {	// fold in another byte
			_Val ^= (size_t)_First[_Next];
			_Val *= _FNV_prime;
		}

		return _Val;
	}
};

template <>
struct hash<bool> {
	size_t operator () (const bool& s) const {
		return s;
	}
};
#else
#include <unordered_map>
#endif

#if !defined(_MSC_VER) || _MSC_VER > 1200
namespace std {
#endif
	template <>
	struct hash<PaintsNow::StringView> {
		size_t operator () (const PaintsNow::StringView& view) const {
			return (size_t)PaintsNow::HashBuffer(view.data(), view.length());
		}
	};
#if !defined(_MSC_VER) || _MSC_VER > 1200
}
#endif


