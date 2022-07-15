// IAsset.h -- Asset data exchangement
// PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#pragma once
#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IFilterBase.h"
#include "../../Core/Interface/IType.h"
#include "../../Core/Interface/IReflect.h"
#include "../../General/Interface/IRender.h"
#include <list>
#include <vector>

namespace PaintsNow {
	namespace IAsset {
		enum Type { TYPE_CONST, TYPE_FLOAT, TYPE_FLOAT2, TYPE_FLOAT3, TYPE_FLOAT4, TYPE_MATRIX3, TYPE_MATRIX4, TYPE_TEXTURE, TYPE_TEXTURE_RUNTIME, TYPE_BUFFER_RUNTIME, TYPE_STRUCTURE };

		template <class T, size_t type>
		class TypeWrapper {
		public:
			TypeWrapper(T t = T()) : value(t) {}
			T value;
		};

		typedef TypeWrapper<uint32_t, TYPE_TEXTURE> TextureIndex;
		typedef TypeWrapper<IRender::Resource*, TYPE_TEXTURE_RUNTIME> TextureRuntime;
		typedef TypeWrapper<IRender::Resource*, TYPE_BUFFER_RUNTIME> BufferRuntime;

		template <class T>
		struct MapType {};

		// Static switches
		template <>
		struct MapType<bool> { enum { type = TYPE_CONST }; };
		template <>
		struct MapType<uint8_t> { enum { type = TYPE_CONST }; };
		template <>
		struct MapType<uint16_t> { enum { type = TYPE_CONST }; };
		template <>
		struct MapType<uint32_t> { enum { type = TYPE_CONST }; };

		// Variables
		template <>
		struct MapType<float> { enum { type = TYPE_FLOAT }; };
		template <>
		struct MapType<TextureIndex> { enum { type = TYPE_TEXTURE }; };
		template <>
		struct MapType<TextureRuntime> { enum { type = TYPE_TEXTURE_RUNTIME }; };
		template <>
		struct MapType<BufferRuntime> { enum { type = TYPE_BUFFER_RUNTIME }; };
		template <>
		struct MapType<Float2> { enum { type = TYPE_FLOAT2 }; };
		template <>
		struct MapType<Float3> { enum { type = TYPE_FLOAT3 }; };
		template <>
		struct MapType<Float4> { enum { type = TYPE_FLOAT4 }; };
		template <>
		struct MapType<MatrixFloat3x3> { enum { type = TYPE_MATRIX3 }; };
		template <>
		struct MapType<MatrixFloat4x4> { enum { type = TYPE_MATRIX4 }; };

		class MeshGroup : public TReflected<MeshGroup, IReflectObjectComplex> {
		public:
			TObject<IReflect>& operator () (IReflect& reflect) override;
			String name;
			uint32_t primitiveOffset;
			uint32_t primitiveCount;
		};

		class TexCoord : public TReflected<TexCoord, IReflectObjectComplex> {
		public:
			TObject<IReflect>& operator () (IReflect& reflect) override;

			std::vector<Float4> coords;
		};

		class MeshCollection : public TReflected<MeshCollection, IReflectObjectComplex> {
		public:
			TObject<IReflect>& operator () (IReflect& reflect) override;

			std::vector<UInt3> indices;
			std::vector<Float3> vertices;
			std::vector<UChar4> normals;
			std::vector<UChar4> tangents;
			std::vector<UChar4> colors;
			std::vector<UChar4> boneIndices;
			std::vector<UChar4> boneWeights;
			std::vector<TexCoord> texCoords;
			std::vector<MeshGroup> groups;

			static void CalulateNormals(Float3* outputNormals, const Float3* vertices, const Int3* faces, size_t vertexCount, size_t faceCount);
		};

		enum INTERPOLATE { INTERPOLATE_NONE, INTERPOLATE_LINEAR, INTERPOLATE_HERMITE, INTERPOLATE_BEZIER };
		enum FILTER { FILTER_NONE, FILTER_TRANSPARENT, FILTER_BLEND, FILTER_ADDITIVE, FILTER_ADD_ALPHA, FILTER_MODULATE };

		template <class T>
		class Sequence : public TReflected<Sequence<T>, IReflectObjectComplex> {
		public:
			typedef TReflected<Sequence<T>, IReflectObjectComplex> BaseClass;
			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);
				if (reflect.IsReflectProperty()) {
					ReflectProperty(timestamps);
					ReflectProperty(frames);
					ReflectProperty(frameTangents);
					ReflectProperty(interpolate);
				}

				return *this;
			}

			std::vector<float> timestamps;
			std::vector<T> frames;
			std::vector<std::pair<T, T> > frameTangents;
			INTERPOLATE interpolate;
		};

		class Material : public TReflected<Material, IReflectObjectComplex> {
		public:
			class Variable : public TReflected<Variable, IReflectObjectComplex> {
			public:
				TObject<IReflect>& operator () (IReflect& reflect) override;
				Variable();
				template <class T>
				Variable(const String& k, const T& value) {
					key.Assign((uint8_t*)k.data(), k.length());
					SetValue(value);
				}

				template <class T>
				T Parse(UniqueType<T>) const {
					T ret;

#if defined(_MSC_VER) && _MSC_VER <= 1200
					assert((Type)MapType<std::decay<T>::type>::type == type);
#else
					assert((Type)MapType<typename std::decay<T>::type>::type == type);
#endif
					memcpy(&ret, value.GetData(), sizeof(T));
					return ret;
				}
				
				template <class T>
				Variable& SetValue(const T& object) {
#if defined(_MSC_VER) && _MSC_VER <= 1200
					type = (Type)MapType<std::decay<T>::type>::type;
#else
					type = (Type)MapType<typename std::decay<T>::type>::type;
#endif
					value.Resize(sizeof(T));
					memcpy(value.GetData(), &object, sizeof(T));
					return *this;
				}

				Bytes key;
				Bytes value;
				Type type;
			};

			TObject<IReflect>& operator () (IReflect& reflect) override;

			std::vector<Variable> variables;
			IRender::Resource::RenderStateDescription state;
			IRender::Resource::RenderStateDescription stateMask; // for runtime overriding materials, no effects on root material.
		};

		typedef Sequence<Float4> RotSequence;
		typedef Sequence<Float3> TransSequence;
		typedef Sequence<Float3> ScalingSequence;

		class BoneAnimation : public TReflected<BoneAnimation, IReflectObjectComplex> {
		public:
			TObject<IReflect>& operator () (IReflect& reflect) override;

			class Joint : public TReflected<Joint, IReflectObjectComplex> {
			public:
				TObject<IReflect>& operator () (IReflect& reflect) override;

				String name;
				MatrixFloat4x4 offsetMatrix;

				int id;
				int parent;
			};

			class Channel : public TReflected<Channel, IReflectObjectComplex> {
			public:
				TObject<IReflect>& operator () (IReflect& reflect) override;
				int jointIndex;
				RotSequence rotSequence;
				TransSequence transSequence;
				ScalingSequence scalingSequence;
			};

			class Clip : public TReflected<Clip, IReflectObjectComplex> {
			public:
				TObject<IReflect>& operator () (IReflect& reflect) override;
				String name;
				float fps;
				float duration;
				float rarity;
				float speed;
				bool loop;

				std::vector<Channel> channels;
			};

			std::vector<Joint> joints;
			std::vector<Clip> clips;
		};
	}
}
