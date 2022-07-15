// ZRandomLibnoisePerlin.h
// By PaintDream
//

#pragma once
#include "../../../Interface/IRandom.h"
#include "Core/module/perlin.h"

namespace PaintsNow {
	class ZRandomLibnoisePerlin final : public IRandom {
	public:
		void Seed(long seed) override;
		void Configure(const String& parameter, double value) override;
		double GetConfig(const String& parameter) const override;
		double GetValue(const double* coords, size_t dimension) override;

	private:
		noise::module::Perlin perlin;
	};
}

