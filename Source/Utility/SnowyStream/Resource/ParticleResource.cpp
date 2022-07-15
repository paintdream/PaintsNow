#include "ParticleResource.h"

using namespace PaintsNow;

ParticleResource::ParticleResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {}

bool ParticleResource::operator << (IStreamBase& stream) {
	return false;
}

bool ParticleResource::operator >> (IStreamBase& stream) const {
	return false;
}

void ParticleResource::Attach(IRender& render, void* deviceContext) {
}

void ParticleResource::Detach(IRender& render, void* deviceContext) {

}

void ParticleResource::Upload(IRender& render, void* deviceContext) {

}

void ParticleResource::Download(IRender& render, void* deviceContext) {

}
