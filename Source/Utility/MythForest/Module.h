// Module.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-15
//

#pragma once
#include "../../General/Interface/Interfaces.h"
#include "Entity.h"
#include "Component.h"

namespace PaintsNow {
	class Engine;
#define CREATE_MODULE(f) Module* Create##f(Engine& engine) { return new f(engine); }

	class Module : public TReflected<Module, IScript::Library> {
	public:
		Module(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual Unique GetTinyUnique() const;
		virtual Component* GetEntityUniqueComponent(Entity* entity) const;
		virtual void TickFrame();

	protected:
		Engine& engine;
	};

	template <class T>
	class ModuleImpl : public TReflected<ModuleImpl<T>, Module> {
	public:
		typedef TReflected<ModuleImpl<T>, Module> BaseClass;
		ModuleImpl(Engine& engine) : BaseClass(engine) {
			allocator = TShared<typename T::Allocator>::From(new typename T::Allocator());
		}
		Unique GetTinyUnique() const override {
			return UniqueType<T>::Get();
		}

		Component* GetEntityUniqueComponent(Entity* entity) const override {
			return entity->GetUniqueComponent(UniqueType<T>());
		}

		TShared<typename T::Allocator> allocator;
	};
}

