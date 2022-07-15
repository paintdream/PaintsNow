#include "TapeComponent.h"
#include "../../../../Core/System/StringStream.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
#include <utility>

using namespace PaintsNow;

TapeComponent::TapeComponent(IStreamBase& stream, const TShared<SharedTiny>&holder, size_t cache) : tape(stream), streamHolder(std::move(holder)), cacheBytes(cache), bufferStream(cache) {}

std::pair<int64_t, String> TapeComponent::Read() {
	assert(!(Flag().load(std::memory_order_acquire) & TINY_UPDATING));
	if (Flag().load(std::memory_order_acquire) & TINY_UPDATING) return std::make_pair(0, String());

	int64_t seq;
	int64_t length;
	bufferStream.Seek(IStreamBase::BEGIN, 0);

	if (tape.ReadPacket(seq, bufferStream, length)) {
		return std::make_pair(seq, String(reinterpret_cast<const char*>(bufferStream.GetBuffer()), (size_t)length));
	} else {
		return std::make_pair(0, String());
	}
}

bool TapeComponent::Write(int64_t seq, const String& data) {
	assert(!(Flag().load(std::memory_order_acquire) & TINY_UPDATING));
	if (Flag().load(std::memory_order_acquire) & TINY_UPDATING) return false;

	size_t len = data.length();
	if (!bufferStream.Write(data.c_str(), len)) {
		return false;
	}

	if (bufferStream.GetTotalLength() > cacheBytes) {
		// force flush
		return FlushInternal();
	} else {
		return true;
	}
}

bool TapeComponent::FlushInternal() {
	size_t i;
	for (i = 0; i < cachedSegments.size(); i++) {
		const std::pair<uint64_t, size_t>& p = cachedSegments[i];
		if (!tape.WritePacket(p.first, bufferStream, p.second)) {
			break;
		}
	}

	bool success = i == cachedSegments.size();
	bufferStream.Seek(IStreamBase::BEGIN, 0);
	cachedSegments.clear();
	return success;
}

bool TapeComponent::Seek(int64_t seq) {
	return tape.Seek(seq);
}

void TapeComponent::OnAsyncFlush(Engine& engine, IScript::Request::Ref callback) {
	OPTICK_EVENT();
	bool result = FlushInternal();

	IScript::Request& request = *engine.bridgeSunset.requestPool.AcquireSafe();
	request.DoLock();
	request.Call(callback, result);
	request.UnLock();
	engine.bridgeSunset.requestPool.ReleaseSafe(&request);

	Flag().fetch_and(~TINY_UPDATING);
	ReleaseObject();
}

bool TapeComponent::Flush(Engine& engine, IScript::Request::Ref callback) {
	if (callback) {
		Flag().fetch_or(TINY_UPDATING);
		ReferenceObject();

		engine.GetKernel().GetThreadPool().Dispatch(CreateTaskContextFree(Wrap(this, &TapeComponent::OnAsyncFlush), std::ref(engine), callback), 1);
		return true;
	} else {
		return FlushInternal();
	}
}
