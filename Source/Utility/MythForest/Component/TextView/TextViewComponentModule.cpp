#include "TextViewComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

CREATE_MODULE(TextViewComponentModule);
TextViewComponentModule::TextViewComponentModule(Engine& engine) : BaseClass(engine) {
	batchComponentModule = (engine.GetComponentModuleFromName("BatchComponent")->QueryInterface(UniqueType<BatchComponentModule>()));
	assert(batchComponentModule != nullptr);
}

void TextViewComponentModule::Initialize() {
	defaultTextMaterial = engine.snowyStream.CreateReflectedResource(UniqueType<MaterialResource>(), "[Runtime]/MaterialResource/Text", true, ResourceBase::RESOURCE_VIRTUAL);
	defaultTextMesh = engine.snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), "[Runtime]/MeshResource/StandardQuad", true, ResourceBase::RESOURCE_VIRTUAL);
}

TextViewComponentModule::~TextViewComponentModule() {}

TObject<IReflect>& TextViewComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestSetFontSize)[ScriptMethodLocked = "SetFontSize"];
		ReflectMethod(RequestGetText)[ScriptMethodLocked = "GetText"];
		ReflectMethod(RequestSetText)[ScriptMethodLocked = "SetText"];
		ReflectMethod(RequestLocateText)[ScriptMethod = "LocateText"];
	}

	return *this;
}

TShared<TextViewComponent> TextViewComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<FontResource> fontResource, IScript::Delegate<MeshResource> meshResource, IScript::Delegate<MaterialResource> materialResource, IScript::Delegate<BatchComponent> batch) {
	CHECK_REFERENCES_NONE();

	TShared<BatchComponent> batchComponent;
	if (batch.Get() == nullptr) {
		batchComponent = batchComponentModule->Create(IRender::Resource::BufferDescription::UNIFORM);
	} else {
		batchComponent = batch.Get();
	}

	TShared<FontResource> res = fontResource.Get();
	TShared<TextViewComponent> textViewComponent = TShared<TextViewComponent>::From(allocator->New(res, meshResource ? TShared<MeshResource>(meshResource.Get()) : defaultTextMesh, batchComponent));
	textViewComponent->SetMaterial(0, 0, materialResource ? TShared<MaterialResource>(materialResource.Get()) : defaultTextMaterial);
	textViewComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return textViewComponent;
}

void TextViewComponentModule::RequestSetFontSize(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent, uint32_t fontSize) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(textViewComponent);

	// Get font
	textViewComponent->fontSize = verify_cast<uint32_t>(fontSize);
	textViewComponent->SetUpdateMark();
}

String TextViewComponentModule::RequestGetText(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(textViewComponent);
	return textViewComponent->text;
}

void TextViewComponentModule::RequestSetText(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent, const String& text) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(textViewComponent);

	textViewComponent->SetText(engine, text);
}

Short3 TextViewComponentModule::RequestLocateText(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent, const Short2& offset, bool isRowCol) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(textViewComponent);

	Short2 rowCol;

	// TODO: fix offset
	int loc = textViewComponent->Locate(rowCol, offset/* - textViewComponent->clippedRect.first*/, isRowCol);
	return Short3(rowCol.x(), rowCol.y(), loc);
}