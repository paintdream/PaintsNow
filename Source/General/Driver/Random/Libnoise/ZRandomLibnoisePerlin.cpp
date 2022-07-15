#include "ZRandomLibnoisePerlin.h"

using namespace PaintsNow;
using namespace noise::module;

void ZRandomLibnoisePerlin::Seed(long seed) {
	perlin.SetSeed(seed);
}

void ZRandomLibnoisePerlin::Configure(const String& parameter, double value) {
	if (parameter == "Frequency") {
		perlin.SetFrequency(value);
	} else if (parameter == "Lacunarity") {
		perlin.SetLacunarity(value);
	} else if (parameter == "Quality") {
		perlin.SetNoiseQuality(value > 1 ? noise::QUALITY_BEST : value == 0 ? noise::QUALITY_STD : noise::QUALITY_FAST);
	} else if (parameter == "OctaveCount") {
		perlin.SetOctaveCount((int)value);
	} else if (parameter == "Persistence") {
		perlin.SetPersistence(value);
	}
}

double ZRandomLibnoisePerlin::GetConfig(const String& parameter) const {
	if (parameter == "Frequency") {
		return perlin.GetFrequency();
	} else if (parameter == "Lacunarity") {
		return perlin.GetLacunarity();
	} else if (parameter == "Quality") {
		noise::NoiseQuality value = perlin.GetNoiseQuality();
		switch (value) {
			case noise::QUALITY_BEST:
				return 1.0;
			case noise::QUALITY_FAST:
				return -1.0;
			default:
				return 0;
		}
	} else if (parameter == "OctaveCount") {
		return (double)perlin.GetOctaveCount();
	} else if (parameter == "Persistence") {
		return perlin.GetPersistence();
	} else {
		return 0;
	}
}

double ZRandomLibnoisePerlin::GetValue(const double* coords, size_t dimension) {
	assert(dimension != 0 && dimension <= 3);
	Double3 db;
	memcpy(&db, coords, dimension * sizeof(double));
	return perlin.GetValue(db.x(), db.y(), db.z());
}
