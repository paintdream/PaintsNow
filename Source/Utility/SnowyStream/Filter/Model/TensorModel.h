// TensorModel.h
// PaintDream (paintdream@paintdream.com)
// 2021-1-24
//

#pragma once
#include "../../../../Core/Template/TVector.h"

namespace PaintsNow {
	template <class T, size_t K = 2, class F = float> // by default, matrix
	class TensorModel {
	public:
		enum { DIMENSION = K };
		typedef TVector<size_t, K> DirectCoord;
		typedef TVector<F, K> InterpolateCoord;

		TensorModel(T* p, const DirectCoord& s) : data(p), size(s) {}

		size_t Offset(const DirectCoord& coord) const {
			size_t offset = coord[K - 1];
			assert(offset < size[K - 1]);
			for (size_t i = K; i > 1; i--) {
				assert(coord[i - 2] < size[i - 2]);
				offset = offset * size[i - 2] + coord[i - 2];
			}

			return offset;
		}

		const T& operator [] (const DirectCoord& coord) const {
			return data[Offset(coord)];
		}

		T& operator [] (const DirectCoord& coord) {
			return data[Offset(coord)];
		}

		T operator [] (const InterpolateCoord& coord) const {
			T results[1 << K];
			DirectCoord minValue, maxValue;
			InterpolateCoord lerpValue;
			InterpolateCoord lerpValueConjugate;
			for (size_t n = 0; n < K; k++) {
				assert(coord[n] >= 0.0f && coord[n] <= 1.0f);
				F v = (F)floor(coord[n]);
				minValue[n] = (size_t)v;
				maxValue[n] = (size_t)ceil(coord[n]);
				lerpValue[n] = coord[n] - v;
				lerpValueConjugate[n] = T(1) - lerpValue[n];
			}

			for (size_t i = 0; i < (1 << K); i++) {
				DirectCoord coord;
				for (size_t m = 0; m < K; m++) {
					coord[m] = ((i >> m) & 1) ? maxValue[m] : minValue[m];
				}

				results[i] = (*this)[coord];
			}

			for (size_t j = 0; j < K; j++) {
				size_t halfLength = 1 << (K - j - 1);
				for (size_t k = 0; k < halfLength; k++) {
					results[k] = results[k] * lerpValueConjugate[j] + results[k + halfLength] * lerpValue[j];
				}
			}

			return results[0];
		}

	private:
		T* data;
		DirectCoord size;
	};
}