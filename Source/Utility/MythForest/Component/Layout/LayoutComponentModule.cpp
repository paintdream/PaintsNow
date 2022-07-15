#include "LayoutComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

CREATE_MODULE(LayoutComponentModule);
LayoutComponentModule::LayoutComponentModule(Engine& engine) : BaseClass(engine) {}
LayoutComponentModule::~LayoutComponentModule() {}

TObject<IReflect>& LayoutComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestGetScrollSize)[ScriptMethodLocked = "GetScrollSize"];
		ReflectMethod(RequestGetScrollOffset)[ScriptMethodLocked = "GetScrollOffset"];
		ReflectMethod(RequestSetScrollOffset)[ScriptMethodLocked = "SetScrollOffset"];
		ReflectMethod(RequestSetLayout)[ScriptMethodLocked = "SetLayout"];
		ReflectMethod(RequestSetWeight)[ScriptMethodLocked = "SetWeight"];
		ReflectMethod(RequestGetWeight)[ScriptMethodLocked = "GetWeight"];
		ReflectMethod(RequestSetRect)[ScriptMethodLocked = "SetRect"];
		ReflectMethod(RequestGetRect)[ScriptMethodLocked = "GetRect"];
		ReflectMethod(RequestGetClippedRect)[ScriptMethodLocked = "GetClippedRect"];
		ReflectMethod(RequestSetSize)[ScriptMethodLocked = "SetSize"];
		ReflectMethod(RequestGetSize)[ScriptMethodLocked = "GetSize"];
		ReflectMethod(RequestSetPadding)[ScriptMethodLocked = "SetPadding"];
		ReflectMethod(RequestGetPadding)[ScriptMethodLocked = "GetPadding"];
		ReflectMethod(RequestSetMargin)[ScriptMethodLocked = "SetMargin"];
		ReflectMethod(RequestGetMargin)[ScriptMethodLocked = "GetMargin"];
		ReflectMethod(RequestSetFitContent)[ScriptMethodLocked = "SetFitContent"];
		ReflectMethod(RequestGetFitContent)[ScriptMethodLocked = "GetFitContent"];
		ReflectMethod(RequestSetIndexRange)[ScriptMethodLocked = "SetIndexRange"];
	}

	return *this;
}

TShared<LayoutComponent> LayoutComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<LayoutComponent> layoutComponent = TShared<LayoutComponent>::From(allocator->New());
	layoutComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	return layoutComponent;
}

Float4 LayoutComponentModule::RequestGetClippedRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return (Float4)layoutComponent->clippedRect;
}

Float4 LayoutComponentModule::RequestGetRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return Float4(layoutComponent->rect.first.x(), layoutComponent->rect.first.y(), layoutComponent->rect.second.x(), layoutComponent->rect.second.y());
}

void LayoutComponentModule::RequestSetLayout(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const String& layout) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	if (layout == "horizontal") {
		layoutComponent->Flag().fetch_and(~LayoutComponent::LAYOUT_VERTICAL, std::memory_order_relaxed);
	} else {
		layoutComponent->Flag().fetch_or(LayoutComponent::LAYOUT_VERTICAL, std::memory_order_relaxed);
	}
}

void LayoutComponentModule::RequestSetFitContent(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, bool fitContent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	layoutComponent->Flag().fetch_or(LayoutComponent::LAYOUT_ADAPTABLE, std::memory_order_relaxed);
	layoutComponent->SetUpdateMark();
}

bool LayoutComponentModule::RequestGetFitContent(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);
	return !!(layoutComponent->Flag().load(std::memory_order_acquire) & LayoutComponent::LAYOUT_ADAPTABLE);
}

Float2 LayoutComponentModule::RequestGetScrollSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return layoutComponent->scrollSize;
}

Float2 LayoutComponentModule::RequestGetScrollOffset(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return layoutComponent->scrollOffset;
}

void LayoutComponentModule::RequestSetScrollOffset(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float2& position) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	layoutComponent->scrollOffset = position;
	layoutComponent->SetUpdateMark();
}

void LayoutComponentModule::RequestSetRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float4& rect) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	// assert(rect.x() >= 0 && rect.y() >= 0 && rect.z() >= 0 && rect.w() >= 0);
	
	layoutComponent->rect = Float2Pair(Float2(rect.x(), rect.y()), Float2(rect.z(), rect.w()));
	layoutComponent->SetUpdateMark();
}

void LayoutComponentModule::RequestSetWeight(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, int64_t weight) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);
	
	layoutComponent->weight = (int32_t)weight;
	layoutComponent->SetUpdateMark();
}

int32_t LayoutComponentModule::RequestGetWeight(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);
	return layoutComponent->weight;
}

Float4 LayoutComponentModule::RequestGetSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return Float4(layoutComponent->size.first.x(), layoutComponent->size.first.y(), layoutComponent->size.second.x(), layoutComponent->size.second.y());
}

void LayoutComponentModule::RequestSetSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float4& size) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	layoutComponent->size = Float2Pair(Float2(size.x(), size.y()), Float2(size.z(), size.w()));
	layoutComponent->SetUpdateMark();
}

void LayoutComponentModule::RequestSetPadding(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float4& size) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);
	assert(size.x() >= 0 && size.y() >= 0 && size.z() >= 0 && size.w() >= 0);

	layoutComponent->padding = Float2Pair(Float2(size.x(), size.y()), Float2(size.z(), size.w()));
	layoutComponent->SetUpdateMark();
}

Float4 LayoutComponentModule::RequestGetPadding(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return Float4(Float4(layoutComponent->padding.first.x(), layoutComponent->padding.first.y(), layoutComponent->padding.second.x(), layoutComponent->padding.second.y()));
}

void LayoutComponentModule::RequestSetMargin(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float4& size) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);
	
	layoutComponent->margin = Float2Pair(Float2(size.x(), size.y()), Float2(size.z(), size.w()));
	layoutComponent->SetUpdateMark();
}

Float4 LayoutComponentModule::RequestGetMargin(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return Float4(Float4(layoutComponent->margin.first.x(), layoutComponent->margin.first.y(), layoutComponent->margin.second.x(), layoutComponent->margin.second.y()));
}

void LayoutComponentModule::RequestSetIndexRange(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, int start, int count) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	layoutComponent->start = start;
	layoutComponent->count = count;
	layoutComponent->SetUpdateMark();
}

