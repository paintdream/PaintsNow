// INetwork.h
// PaintDream (paintdream@paintdream.com)
// 2016-5-16
//

#pragma once
#include "../../Core/PaintsNow.h"
#include "../../Core/Template/TProxy.h"
#include "../../Core/Interface/IType.h"
#include "../../Core/Interface/IDevice.h"

namespace PaintsNow {
	class pure_interface IRandom : public IDevice {
	public:
		~IRandom() override;
		virtual void Seed(long seed) = 0;
		virtual void Configure(const String& parameter, double value) = 0;
		virtual double GetConfig(const String& parameter) const = 0;
		virtual double GetValue(const double* coords, size_t dimension) = 0;

		template <class T>
		typename T::type GetValue(const T& input) {
			double value[T::size];
			for (size_t i = 0; i < T::size; i++) {
				value[i] = (double)input[i];
			}

			return (typename T::type)GetValue(value, T::size);
		}

	};
}

