// TMaskType.h
// PaintDream (paintdream@paintdream.com)
// 2015-7-10
//

#pragma once
namespace PaintsNow {
#define BEGIN_TYPEMASK(TMaskCast, size) \
	namespace NsTypeMask##TMaskCast { \
		enum { MASK_SIZE = size}; \
		template <size_t mask> \
		struct TMaskCast { \
			template <class T> \
			struct Impl {}; \
		};

#define END_TYPEMASK(TMaskCast, TMaskType) \
		template <size_t mask> \
		struct MaskSelector { \
			template <class T> \
			struct Select { \
				/*typedef typename MaskSelector<mask / MASK_SIZE>::template Select<TMaskCast<mask % MASK_SIZE>::template Impl<T> >::type type;*/ \
				typedef typename TMaskCast<mask % MASK_SIZE>::template Impl<typename MaskSelector<mask / MASK_SIZE>::template Select<T>::type> type; \
			}; \
		}; \
		template <> \
		struct MaskSelector<0> { \
			template <class T> \
			struct Select { \
				typedef T type; \
			}; \
		}; \
	} \
	template <class T, size_t mask> \
	struct TMaskType { \
		typedef typename NsTypeMask##TMaskCast::MaskSelector<mask>::template Select<T>::type type; \
	};
}

