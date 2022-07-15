// Protocol.h
// PaintDream (paintdream@paintdream.com)
// 2022-6-12
//

#pragma once

#include "../../Core/Interface/IReflect.h"

namespace PaintsNow {
	class ProtoInput : public TReflected<ProtoInput, IReflectObjectComplex> {};
	class ProtoOutput : public TReflected<ProtoOutput, IReflectObjectComplex> {};

	typedef ProtoInput ProtoInputCheckVersion;
	class ProtoOutputCheckVersion : public TReflected<ProtoOutputCheckVersion, ProtoOutput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(clientVersion);
			}

			return *this;
		}

		String clientVersion;
	};

	class ProtoInputInitialize : public TReflected<ProtoInputInitialize, ProtoInput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(clientVersion);
			}

			return *this;
		}

		String clientVersion;
	};
	typedef ProtoOutput ProtoOutputInitialize;

	typedef ProtoInput ProtoInputUninitialize;
	typedef ProtoOutput ProtoOutputUninitialize;

	class ProtoInputDebugPrint : public TReflected<ProtoInputDebugPrint, ProtoInput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(text);
			}

			return *this;
		}

		String text;
	};
	typedef ProtoOutput ProtoOutputDebugPrint;

	class ProtoInputPostResource : public TReflected<ProtoInputPostResource, ProtoInput> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(location);
				ReflectProperty(extension);
				ReflectProperty(resourceData);
			}

			return *this;
		}

		String location;
		String extension;
		String resourceData;
	};
	typedef ProtoOutput ProtoOutputPostResource;
	
	class ProtoInputPostEntity : public TReflected<ProtoInputPostEntity, ProtoInput> {
	public:
		ProtoInputPostEntity() : entityID(0), groupID(0) {}
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(entityID);
				ReflectProperty(groupID);
				ReflectProperty(entityName);
			}

			return *this;
		}

		uint32_t entityID;
		uint32_t groupID;
		String entityName;
	};
	typedef ProtoOutput ProtoOutputPostEntity;

	class ProtoInputPostEntityGroup : public TReflected<ProtoInputPostEntityGroup, ProtoInput> {
	public:
		ProtoInputPostEntityGroup() : groupID(0) {}
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(groupID);
				ReflectProperty(groupName);
			}

			return *this;
		}

		uint32_t groupID;
		String groupName;
	};
	typedef ProtoOutput ProtoOutputPostEntityGroup;

	class ProtoInputPostEntityComponent : public TReflected<ProtoInputPostEntityComponent, ProtoInput> {
	public:
		ProtoInputPostEntityComponent() : entityID(0), componentID(0) {}
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(entityID);
				ReflectProperty(componentID);
			}

			return *this;
		}

		uint32_t entityID;
		uint32_t componentID;
	};
	typedef ProtoOutput ProtoOutputPostEntityComponent;

	class ProtoInputPostModelComponent : public TReflected<ProtoInputPostModelComponent, ProtoInput> {
	public:
		ProtoInputPostModelComponent() : componentID(0), viewDistance(0) {}
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(componentID);
				ReflectProperty(viewDistance);
				ReflectProperty(meshResource);
			}

			return *this;
		}

		uint32_t componentID;
		float viewDistance;
		String meshResource;
	};
	typedef ProtoOutput ProtoOutputPostModelComponent;
	
	class ProtoInputPostModelComponentMaterial : public TReflected<ProtoInputPostModelComponentMaterial, ProtoInput> {
	public:
		ProtoInputPostModelComponentMaterial() : componentID(0), meshGroupID(0) {}
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(componentID);
				ReflectProperty(meshGroupID);
				ReflectProperty(materialResource);
			}

			return *this;
		}

		uint32_t componentID;
		uint32_t meshGroupID;
		String materialResource;
	};
	typedef ProtoOutput ProtoOutputPostModelComponentMaterial;

	class ProtoInputPostTransformComponent : public TReflected<ProtoInputPostTransformComponent, ProtoInput> {
	public:
		ProtoInputPostTransformComponent() : componentID(0) {}
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(position);
				ReflectProperty(scale);
				ReflectProperty(rotation);
				ReflectProperty(componentID);
			}

			return *this;
		}

		Float3 position;
		Float3 scale;
		Float3 rotation;
		uint32_t componentID;
	};
	typedef ProtoOutput ProtoOutputPostTransformComponent;
	
	class ProtoInputPostSpaceComponent : public TReflected<ProtoInputPostSpaceComponent, ProtoInput> {
	public:
		ProtoInputPostSpaceComponent() : componentID(0), groupID(0) {}
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(componentID);
				ReflectProperty(groupID);
			}

			return *this;
		}

		uint32_t componentID;
		uint32_t groupID;
	};
	typedef ProtoOutput ProtoOutputPostSpaceComponent;

	class ProtoInputPostEnvCubeComponent : public TReflected<ProtoInputPostEnvCubeComponent, ProtoInput> {
	public:
		ProtoInputPostEnvCubeComponent() : componentID(0) {}
		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator ()(reflect);

			if (reflect.IsReflectProperty()) {
				ReflectProperty(componentID);
				ReflectProperty(texturePath);
			}

			return *this;
		}

		uint32_t componentID;
		String texturePath;
	};
	typedef ProtoOutput ProtoOutputPostEnvCubeComponent;

	typedef ProtoInput ProtoInputComplete;
	typedef ProtoOutput ProtoOutputComplete;
}
