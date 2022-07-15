#include "IDevice.h"
using namespace PaintsNow;

IDevice::~IDevice() {}

void IDevice::ReleaseDevice() {
	delete this;
}