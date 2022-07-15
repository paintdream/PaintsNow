// TapeComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../../Core/System/Tape.h"
#include "../../../../Core/System/MemoryStream.h"

namespace PaintsNow {
	class TapeComponent : public TAllocatedTiny<TapeComponent, Component> {
	public:
		TapeComponent(IStreamBase& streamBase, const TShared<SharedTiny>&streamHolder, size_t cacheBytes);

		std::pair<int64_t, String> Read();
		bool Write(int64_t seq, const String& data);
		bool Seek(int64_t seq);
		bool Flush(Engine& engine, IScript::Request::Ref callback);

	protected:
		bool FlushInternal();
		void OnAsyncFlush(Engine& engine, IScript::Request::Ref callback);

		size_t cacheBytes;
		Tape tape;
		MemoryStream bufferStream;
		TShared<SharedTiny> streamHolder;
		std::vector<std::pair<uint64_t, size_t> > cachedSegments;
	};
}

