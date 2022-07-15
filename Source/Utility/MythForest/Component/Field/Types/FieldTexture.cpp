#include "FieldTexture.h"

using namespace PaintsNow;

FieldTexture::FieldTexture(TShared<TextureResource> t, const Float3Pair& r) : textureResource(t), range(r) {
	textureResource->Map();
}

FieldTexture::~FieldTexture() {
	textureResource->UnMap();
}

Bytes FieldTexture::operator [] (const Float3& position) const {
	assert(!textureResource->description.state.compress);
	assert(textureResource->description.state.type == IRender::Resource::TextureDescription::TEXTURE_2D || textureResource->description.state.type);
	assert(textureResource->description.state.layout <= IRender::Resource::TextureDescription::DEPTH);

	Float3 uv = Math::ToLocal(range, position);
	Bytes& data = textureResource->description.data;
	assert(!data.Empty());
	const UShort3& dimension = textureResource->description.dimension;
	Int3 uvInt((int)(uv.x() / dimension.x() + 0.5f), (int)(uv.y() / dimension.y() + 0.5f), (int)(uv.z() / dimension.z() + 0.5f));
	// wrap or clamp
	IRender::Resource::TextureDescription::State state = textureResource->description.state;
	uint32_t addressUVW[] = { state.addressU, state.addressV, state.addressW };
	for (uint32_t i = 0; i < 3; i++) {
		switch (addressUVW[i]) {
			case IRender::Resource::TextureDescription::REPEAT:
				uvInt[i] = (uvInt[i] % dimension[i] + dimension[i]) % dimension[i];
				break;
			case IRender::Resource::TextureDescription::CLAMP:
				uvInt[i] = Math::Clamp(uvInt[i], 0, dimension[i] - 1);
				break;
			default:
				assert(false); // not supported
		}
	}

	uint32_t byteDepth = 1;
	switch (textureResource->description.state.format) {
		case IRender::Resource::Description::UNSIGNED_BYTE:
			byteDepth = 1;
			break;
		case IRender::Resource::Description::UNSIGNED_SHORT:
			byteDepth = 2;
			break;
		case IRender::Resource::Description::HALF:
			byteDepth = 2;
			break;
		case IRender::Resource::Description::FLOAT:
			byteDepth = 4;
			break;
	}

	switch (textureResource->description.state.layout) {
		case IRender::Resource::TextureDescription::R:
			byteDepth *= 1;
			break;
		case IRender::Resource::TextureDescription::RG:
			byteDepth *= 2;
			break;
		case IRender::Resource::TextureDescription::RGB:
			byteDepth *= 3;
			break;
		case IRender::Resource::TextureDescription::RGBA:
			byteDepth *= 4;
			break;
		case IRender::Resource::TextureDescription::DEPTH:
			byteDepth *= 4;
			break;
	}

	Bytes ret;
	ret.Append(&data[(uvInt.x() + dimension.x() * (uvInt.y() + uvInt.z() * dimension.z())) * byteDepth], byteDepth);
	return ret;
}