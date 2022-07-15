#include "RenderFlowComponentModule.h"

#include "RenderStage/AntiAliasingRenderStage.h"
#include "RenderStage/BloomRenderStage.h"
#include "RenderStage/FrameBarrierRenderStage.h"
#include "RenderStage/ForwardLightingRenderStage.h"
#include "RenderStage/DeferredLightingBufferEncodedRenderStage.h"
#include "RenderStage/DeferredLightingTextureEncodedRenderStage.h"
#include "RenderStage/DepthBoundingRenderStage.h"
#include "RenderStage/DepthBoundingSetupRenderStage.h"
#include "RenderStage/DepthResolveRenderStage.h"
#include "RenderStage/DeviceRenderStage.h"
#include "RenderStage/GeometryBufferRenderStage.h"
#include "RenderStage/LightBufferEncodeRenderStage.h"
#include "RenderStage/LightTextureEncodeRenderStage.h"
#include "RenderStage/PhaseLightRenderStage.h"
#include "RenderStage/ScreenRenderStage.h"
#include "RenderStage/ScreenSpaceFilterRenderStage.h"
#include "RenderStage/ScreenSpaceTraceRenderStage.h"
#include "RenderStage/ShadowMaskRenderStage.h"
#include "RenderStage/WidgetRenderStage.h"

using namespace PaintsNow;

template <class T>
struct Create {
	static RenderStage* NewRenderStage(const String& option) {
		return new T(option);
	}
};

#define REGISTER_TEMPLATE(type) \
	stageTemplates[#type] = Wrap(&Create<type>::NewRenderStage);

CREATE_MODULE(RenderFlowComponentModule);
RenderFlowComponentModule::RenderFlowComponentModule(Engine& engine) : BaseClass(engine) {
	// Register built-ins
	REGISTER_TEMPLATE(AntiAliasingRenderStage);
	REGISTER_TEMPLATE(BloomRenderStage);
	REGISTER_TEMPLATE(FrameBarrierRenderStage);
	REGISTER_TEMPLATE(ForwardLightingRenderStage);
	REGISTER_TEMPLATE(DepthResolveRenderStage);
	REGISTER_TEMPLATE(DepthBoundingRenderStage);
	REGISTER_TEMPLATE(DepthBoundingSetupRenderStage);
	REGISTER_TEMPLATE(DeferredLightingBufferEncodedRenderStage);
	REGISTER_TEMPLATE(DeferredLightingTextureEncodedRenderStage);
	REGISTER_TEMPLATE(DeviceRenderStage);
	REGISTER_TEMPLATE(GeometryBufferRenderStage);
	REGISTER_TEMPLATE(LightBufferEncodeRenderStage);
	REGISTER_TEMPLATE(LightTextureEncodeRenderStage);
	REGISTER_TEMPLATE(PhaseLightRenderStage);
	REGISTER_TEMPLATE(ScreenRenderStage);
	REGISTER_TEMPLATE(ScreenSpaceFilterRenderStage);
	REGISTER_TEMPLATE(ScreenSpaceTraceRenderStage);
	REGISTER_TEMPLATE(ShadowMaskRenderStage);
	REGISTER_TEMPLATE(WidgetRenderStage);
}

TObject<IReflect>& RenderFlowComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestNewRenderStage)[ScriptMethodLocked = "NewRenderStage"];
		ReflectMethod(RequestEnumerateRenderStagePorts)[ScriptMethodLocked = "EnumerateRenderStagePorts"];
		ReflectMethod(RequestLinkRenderStagePort)[ScriptMethodLocked = "LinkRenderStagePort"];
		ReflectMethod(RequestUnlinkRenderStagePort)[ScriptMethodLocked = "UnlinkRenderStagePort"];
		ReflectMethod(RequestExportRenderStagePort)[ScriptMethodLocked = "ExportRenderStagePort"];
		ReflectMethod(RequestOverrideRenderStageMaterial)[ScriptMethodLocked = "OverrideRenderStageMaterial"];
		ReflectMethod(RequestBindRenderTargetTexture)[ScriptMethodLocked = "BindRenderTargetTexture"];
		ReflectMethod(RequestDeleteRenderStage)[ScriptMethodLocked = "DeleteRenderStage"];
	}

	return *this;
}

void RenderFlowComponentModule::RegisterNodeTemplate(String& key, const TWrapper<RenderStage*, const String&>& t) {
	stageTemplates[key] = t;
}

TShared<RenderFlowComponent> RenderFlowComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<RenderFlowComponent> renderFlowComponent = TShared<RenderFlowComponent>::From(allocator->New());
	renderFlowComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return renderFlowComponent;
}

TShared<RenderStage> RenderFlowComponentModule::RequestNewRenderStage(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& name, const String& config) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_THREAD_IN_MODULE(renderFlowComponent);

	std::map<String, TWrapper<RenderStage*, const String&> >::const_iterator it = stageTemplates.find(name);
	if (it != stageTemplates.end()) {
		TShared<RenderStage> renderStage = TShared<RenderStage>::From(it->second(config));
		renderStage->ReflectNodePorts();
		renderFlowComponent->AddNode(renderStage());
		return renderStage;
	} else {
		request.Error(String("Could not find render stage: ") + name);
		return nullptr;
	}
}

void RenderFlowComponentModule::RequestEnumerateRenderStagePorts(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> renderStage) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_DELEGATE(renderStage);

	RenderStage* s = renderStage.Get();
	request << beginarray;
	for (size_t i = 0; i < s->GetPorts().size(); i++) {
		RenderStage::Port* port = s->GetPorts()[i].port;
		request << begintable <<
			key("Name") << s->GetPorts()[i].name <<
			key("Type") << port->GetUnique()->GetName() <<
			key("Targets") << beginarray;

		for (size_t j = 0; j < port->GetLinks().size(); j++) {
			RenderStage::Port* targetPort = static_cast<RenderStage::Port*>(port->GetLinks()[j].port);
			RenderStage* target = static_cast<RenderStage*>(targetPort->GetNode());
			String targetPortName;
			// locate port
			for (size_t k = 0; k < target->GetPorts().size(); k++) {
				if (target->GetPorts()[k].port == targetPort) {
					targetPortName = target->GetPorts()[k].name;
					break;
				}
			}

			request << begintable
				<< key("RenderStage") << target
				<< key("Port") << targetPortName
				<< key("Direction") << !!(port->GetLinks()[j].flag & Tiny::TINY_PINNED)
				<< endtable;
		}

		request << endarray << endtable;
	}
	
	request << endarray;
}

void RenderFlowComponentModule::RequestLinkRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> from, const String& fromPortName, IScript::Delegate<RenderStage> to, const String& toPortName) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_DELEGATE(from);
	CHECK_DELEGATE(to);

	RenderStage::Port* fromPort = (*from.Get())[fromPortName];
	RenderStage::Port* toPort = (*to.Get())[toPortName];

	if (fromPort == nullptr) {
		request.Error(String("Unable to locate port: ") + fromPortName);
		return;
	}

	if (toPort == nullptr) {
		request.Error(String("Unable to locate port: ") + toPortName);
		return;
	}

	if (!fromPort->GetLinks().empty() && ((fromPort->Flag().load(std::memory_order_relaxed) & Tiny::TINY_UNIQUE) || (toPort->Flag().load(std::memory_order_relaxed) & Tiny::TINY_UNIQUE))) {
		request.Error(String("Sharing policy conflicts when connecting from: ") + fromPortName + " to " + toPortName);
		return;
	}

	fromPort->Link(toPort, toPort->Flag().load(std::memory_order_relaxed) & RenderStage::RENDERSTAGE_WEAK_LINKAGE ? 0 : Tiny::TINY_PINNED);
}

void RenderFlowComponentModule::RequestUnlinkRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> from, const String& fromPortName, IScript::Delegate<RenderStage> to, const String& toPortName) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_DELEGATE(from);
	CHECK_DELEGATE(to);

	RenderStage::Port* fromPort = (*from.Get())[fromPortName];
	RenderStage::Port* toPort = (*to.Get())[toPortName];

	if (fromPort == nullptr) {
		request.Error(String("Unable to locate port: ") + fromPortName);
		return;
	}

	if (toPort == nullptr) {
		request.Error(String("Unable to locate port: ") + toPortName);
		return;
	}
	
	fromPort->UnLink(toPort);
}

void RenderFlowComponentModule::RequestExportRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> stage, const String& port, const String& symbol) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_DELEGATE(stage);
	CHECK_THREAD_IN_MODULE(renderFlowComponent);

	if (!renderFlowComponent->ExportSymbol(symbol, stage.Get(), port)) {
		request.Error(String("Unable to export port: ") + port);
	}
}

void RenderFlowComponentModule::RequestBindRenderTargetTexture(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& symbol, IScript::Delegate<TextureResource> renderTargetResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_THREAD_IN_MODULE(renderFlowComponent);

	// TODO:
}

void RenderFlowComponentModule::RequestDeleteRenderStage(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> stage) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_DELEGATE(stage);
	CHECK_THREAD_IN_MODULE(renderFlowComponent);

	renderFlowComponent->RemoveNode(stage.Get());
}

void RenderFlowComponentModule::RequestOverrideRenderStageMaterial(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> renderStage, IScript::Delegate<MaterialResource> materialResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);
	CHECK_DELEGATE(renderStage);
	CHECK_THREAD_IN_MODULE(renderFlowComponent);

	renderStage->overrideMaterial = materialResource.Get();
}
