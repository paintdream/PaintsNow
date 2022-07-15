// RenderableComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "RenderableComponent.h"
#include "RenderPolicy.h"
#include "../../Module.h"

namespace PaintsNow {
	class RenderableComponent;

	template <class T>
	class TRenderableComponentModule : public TReflected<TRenderableComponentModule<T>, ModuleImpl<T> > {
	public:
		typedef TReflected<TRenderableComponentModule<T>, ModuleImpl<T> > BaseClass;
		typedef TRenderableComponentModule<T> Class;
		typedef TRenderableComponentModule<T> RenderableComponentModule;

		TRenderableComponentModule(Engine& engine) : BaseClass(engine) {
			// check compatibility
#ifndef _DEBUG
			RenderableComponent* p = (T*)nullptr; (void)p;
#endif
		}

		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator () (reflect);

			if (reflect.IsReflectMethod()) {
				ReflectMethod(RequestNewRenderPolicy)[ScriptMethodLocked = "NewRenderPolicy"];
				ReflectMethod(RequestSetRenderPolicySort)[ScriptMethodLocked = "SetRenderPolicySort"];
				ReflectMethod(RequestAddRenderPolicy)[ScriptMethodLocked = "AddRenderPolicy"];
				ReflectMethod(RequestRemoveRenderPolicy)[ScriptMethodLocked = "RemoveRenderPolicy"];
				ReflectMethod(RequestGetVisible)[ScriptMethodLocked = "GetVisible"];
				ReflectMethod(RequestSetVisible)[ScriptMethodLocked = "SetVisible"];
			}

			return *this;
		}

		/// <summary>
		/// Create RenderPolicy
		/// </summary>
		/// <param name="name"> name </param>
		/// <param name="priority"> priority </param>
		/// <returns> RenderPolicy object </returns>

		TShared<RenderPolicy> RequestNewRenderPolicy(IScript::Request& request, const String& name, uint16_t priorityBegin, uint16_t priorityEnd) {
			CHECK_REFERENCES_NONE();

			TShared<RenderPolicy> policy = TShared<RenderPolicy>::From(new RenderPolicy());
			policy->renderPortName = name;
			policy->priorityRange = std::make_pair(priorityBegin, priorityEnd);
			return policy;
		}

		/// <summary>
		/// Set RenderPolicy sort 
		/// </summary>
		/// <param name="name"> key </param>
		/// <param name="priority"> enable/disable </param>
		void RequestSetRenderPolicySort(IScript::Request& request, IScript::Delegate<RenderPolicy> renderPolicy, const String& key, bool enable) {
			CHECK_REFERENCES_NONE();
			CHECK_DELEGATE(renderPolicy);

			if (key == "NearToFar") {
				renderPolicy->sortType &= ~(RenderPolicy::SORT_NEAR_TO_FAR | RenderPolicy::SORT_NEAR_TO_FAR);
				if (enable) {
					renderPolicy->sortType |= RenderPolicy::SORT_NEAR_TO_FAR;
				}
			} else if (key == "FarToNear") {
				renderPolicy->sortType &= ~(RenderPolicy::SORT_NEAR_TO_FAR | RenderPolicy::SORT_NEAR_TO_FAR);
				if (enable) {
					renderPolicy->sortType |= RenderPolicy::SORT_FAR_TO_NEAR;
				}
			} else if (key == "Material") {
				renderPolicy->sortType &= ~(RenderPolicy::SORT_MATERIAL);
				if (enable) {
					renderPolicy->sortType |= RenderPolicy::SORT_MATERIAL;
				}
			} else if (key == "RenderState") {
				renderPolicy->sortType &= ~(RenderPolicy::SORT_RENDERSTATE);
				if (enable) {
					renderPolicy->sortType |= RenderPolicy::SORT_RENDERSTATE;
				}
			}
		}


		/// <summary>
		/// Add render policy of RenderableComponent or its derivations
		/// </summary>
		/// <param name="renderableComponent"> the RenderableComponent </param>
		/// <param name="renderPolicy"> the RenderPolicy </param>
		void RequestAddRenderPolicy(IScript::Request& request, IScript::Delegate<T> renderableComponent, IScript::Delegate<RenderPolicy> renderPolicy) {
			CHECK_REFERENCES_NONE();
			CHECK_DELEGATE(renderableComponent);
			CHECK_DELEGATE(renderPolicy);

			renderableComponent->AddRenderPolicy(renderPolicy.Get());
		}

		/// <summary>
		/// Remove render policy of RenderableComponent or its derivations
		/// </summary>
		/// <param name="renderableComponent"> the RenderableComponent </param>
		/// <param name="renderPolicy"> the RenderPolicy </param>
		void RequestRemoveRenderPolicy(IScript::Request& request, IScript::Delegate<T> renderableComponent, IScript::Delegate<RenderPolicy> renderPolicy) {
			CHECK_REFERENCES_NONE();
			CHECK_DELEGATE(renderableComponent);
			CHECK_DELEGATE(renderPolicy);

			renderableComponent->RemoveRenderPolicy(renderPolicy.Get());
		}

		/// <summary>
		/// Set visibility of RenderableComponent or its derivations
		/// </summary>
		/// <param name="renderableComponent"> the RenderableComponent </param>
		/// <param name="visible"> visible </param>
		void RequestSetVisible(IScript::Request& request, IScript::Delegate<T> renderableComponent, bool visible) {
			CHECK_REFERENCES_NONE();
			CHECK_DELEGATE(renderableComponent);

			renderableComponent->SetVisible(visible);
		}

		/// <summary>
		/// Get visibility of RenderableComponent or its derivations
		/// </summary>
		/// <param name="renderableComponent"> the RenderableComponent </param>
		bool RequestGetVisible(IScript::Request& request, IScript::Delegate<T> renderableComponent) {
			CHECK_REFERENCES_NONE();
			CHECK_DELEGATE(renderableComponent);

			return renderableComponent->GetVisible();
		}
	};
}
