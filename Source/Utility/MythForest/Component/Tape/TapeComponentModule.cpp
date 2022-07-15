#include "TapeComponentModule.h"
#include "TapeComponent.h"
#include "../../../SnowyStream/File.h"
#include "../../../SnowyStream/Resource/StreamResource.h"

using namespace PaintsNow;

CREATE_MODULE(TapeComponentModule);
TapeComponentModule::TapeComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& TapeComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestRead)[ScriptMethod = "Read"];
		ReflectMethod(RequestWrite)[ScriptMethod = "Write"];
		ReflectMethod(RequestSeek)[ScriptMethod = "Seek"];
		ReflectMethod(RequestFlush)[ScriptMethod = "Flush"];
	}

	return *this;
}

TShared<TapeComponent> TapeComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<SharedTiny> streamHolder, size_t cacheBytes) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(streamHolder);
	TShared<TapeComponent> tapeComponent;

	File* file = streamHolder->QueryInterface(UniqueType<File>());
	if (file != nullptr) {
		assert(file->GetStream() != nullptr);
		tapeComponent = TShared<TapeComponent>::From(allocator->New(std::ref(*file->GetStream()), file, cacheBytes));
	} else {
		StreamResource* res = streamHolder->QueryInterface(UniqueType<StreamResource>());
		if (res != nullptr) {
			tapeComponent = TShared<TapeComponent>::From(allocator->New(std::ref(res->GetStream()), res, cacheBytes));
		}
	}

	if (tapeComponent) {
		tapeComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	}

	return tapeComponent;
}

std::pair<int64_t, String> TapeComponentModule::RequestRead(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(tapeComponent);

	return tapeComponent->Read();
}

bool TapeComponentModule::RequestWrite(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent, int64_t seq, const String& data) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(tapeComponent);

	return tapeComponent->Write(seq, data);
}

bool TapeComponentModule::RequestSeek(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent, int64_t seq) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(tapeComponent);

	return tapeComponent->Seek(seq);
}

bool TapeComponentModule::RequestFlush(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent, IScript::Request::Ref asyncCallback) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(tapeComponent);
	return tapeComponent->Flush(engine, asyncCallback);
}
