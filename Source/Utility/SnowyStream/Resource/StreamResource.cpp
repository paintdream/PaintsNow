#include "StreamResource.h"
#include "../../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

StreamResource::StreamResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {
	Flag().fetch_or(RESOURCE_STREAM, std::memory_order_relaxed);
}

void StreamResource::Refresh(IArchive& archive, void* deviceContext) {}
void StreamResource::Download(IArchive& archive, void* deviceContext) {}
void StreamResource::Upload(IArchive& archive, void* deviceContext) {}
void StreamResource::Attach(IArchive& archive, void* deviceContext) {}
void StreamResource::Detach(IArchive& archive, void* deviceContext) {}

IStreamBase& StreamResource::GetStream() {
	return shadowStream;
}

bool StreamResource::operator << (IStreamBase& base) {
	OPTICK_EVENT();
	if (!(BaseClass::operator << (base))) {
		return false;
	}

	shadowStream << base.GetBaseStream();
	return true;
}

bool StreamResource::operator >> (IStreamBase& base) const {
	OPTICK_EVENT();
	if (!(BaseClass::operator >> (base))) {
		return false;
	}

	return shadowStream >> base.GetBaseStream();
}

IReflectObject* StreamResource::Clone() const {
	OPTICK_EVENT();
	StreamResource* clone = new StreamResource(resourceManager, "");
	clone->shadowStream = shadowStream;
	return clone;
}
