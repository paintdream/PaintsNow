#include "IAsset.h"

using namespace PaintsNow;
using namespace PaintsNow::IAsset;

TObject<IReflect>& MeshCollection::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(vertices);
		ReflectProperty(normals);
		ReflectProperty(tangents);
		ReflectProperty(colors);
		ReflectProperty(texCoords);
		ReflectProperty(indices);
		ReflectProperty(boneIndices);
		ReflectProperty(boneWeights);
		ReflectProperty(groups);
	}

	return *this;
}

void MeshCollection::CalulateNormals(Float3* outputNormals, const Float3* vertices, const Int3* faces, size_t vertexCount, size_t faceCount) {
	memset(outputNormals, 0, sizeof(Float3) * vertexCount);
	for (size_t i = 0; i < faceCount; i++) {
		const Int3& index = faces[i];
		Float3 normal = Math::CrossProduct(vertices[index.y()] - vertices[index.x()], vertices[index.z()] - vertices[index.x()]);
		float length = Math::Length(normal);
		if (length > 1e-5) {
			normal /= length;
			for (size_t j = 0; j < 3; j++) {
				outputNormals[index[j]] += normal;
			}
		}
	}

	for (size_t k = 0; k < vertexCount; k++) {
		Float3& normal = outputNormals[k];
		float length = Math::Length(normal);
		if (length > 1e-5) {
			normal /= length;
		}
	}
}

TObject<IReflect>& TexCoord::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(coords);
	}

	return *this;
}

TObject<IReflect>& MeshGroup::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(name);
		ReflectProperty(primitiveCount);
		ReflectProperty(primitiveOffset);
	}

	return *this;
}

Material::Variable::Variable() {}

TObject<IReflect>& Material::Variable::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(key);
		ReflectProperty(value);
		ReflectProperty(type);
	}

	return *this;
}

TObject<IReflect>& Material::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(variables);
		ReflectProperty(state);
	}

	return *this;
}

TObject<IReflect>& BoneAnimation::operator () (IReflect & reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(joints);
	}

	return *this;
}

TObject<IReflect>& BoneAnimation::Clip::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(name);
		ReflectProperty(fps);
		ReflectProperty(duration);
		ReflectProperty(rarity);
		ReflectProperty(speed);
		ReflectProperty(loop);
		ReflectProperty(channels);
	}

	return *this;
}

TObject<IReflect>& BoneAnimation::Joint::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(name);
		ReflectProperty(offsetMatrix);
		ReflectProperty(id);
		ReflectProperty(parent);
	}

	return *this;
}

TObject<IReflect>& BoneAnimation::Channel::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(jointIndex);
		ReflectProperty(rotSequence);
		ReflectProperty(transSequence);
		ReflectProperty(scalingSequence);
	}

	return *this;
}
