// Unit.h
// PaintDream (paintdream@paintdream.com)
// 2018-7-27
//

#pragma once
#include "../../Core/System/Kernel.h"
#include "../../Core/Template/TBuffer.h"
#include "../../Core/Interface/IReflect.h"
#include "../../Core/Interface/IStreamBase.h"

namespace PaintsNow {
	class Unit : public TReflected<Unit, WarpTiny> {
	public:
		virtual String GetDescription() const;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		enum {
			UNIT_CUSTOM_BEGIN = WARP_CUSTOM_BEGIN
		};
	};

	class MetaUnitIdentifier : public TReflected<MetaUnitIdentifier, MetaStreamPersist> {
	public:
		MetaUnitIdentifier();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		template <class T, class D>
		inline const MetaUnitIdentifier& FilterField(T* t, D* d) const {
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaUnitIdentifier Type;
		};

		typedef MetaUnitIdentifier Type;

		bool Read(IStreamBase& streamBase, void* ptr) const override;
		bool Write(IStreamBase& streamBase, const void* ptr) const override;
		String GetUniqueName() const override;

	private:
		String uniqueName;
	};
}

