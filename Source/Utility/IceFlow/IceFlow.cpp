#include "IceFlow.h"
using namespace PaintsNow;

namespace PaintsNow {
	IScript::Request& operator << (IScript::Request& request, const Float2& value) {
		return request << IScript::ManageReference(value);
	}

	IScript::Request& operator >> (IScript::Request& request, Float2& value) {
		return request >> IScript::ManageReference(value);
	}

	IScript::Request& operator >> (IScript::Request& request, Float2*& value) {
		IScript::TManagedReference<Float2> r(nullptr);
		request >> r;
		value = r ? &r.GetInstance() : nullptr;

		return request;
	}

	IScript::Request& operator << (IScript::Request& request, const Float3& value) {
		return request << IScript::ManageReference(value);
	}

	IScript::Request& operator >> (IScript::Request& request, Float3& value) {
		return request >> IScript::ManageReference(value);
	}

	IScript::Request& operator >> (IScript::Request& request, Float3*& value) {
		IScript::TManagedReference<Float3> r(nullptr);
		request >> r;
		value = r ? &r.GetInstance() : nullptr;

		return request;
	}

	IScript::Request& operator << (IScript::Request& request, const Float4& value) {
		return request << IScript::ManageReference(value);
	}

	IScript::Request& operator >> (IScript::Request& request, Float4& value) {
		return request >> IScript::ManageReference(value);
	}

	IScript::Request& operator >> (IScript::Request& request, Float4*& value) {
		IScript::TManagedReference<Float4> r(nullptr);
		request >> r;
		value = r ? &r.GetInstance() : nullptr;

		return request;
	}

	IScript::Request& operator << (IScript::Request& request, const QuaternionFloat& value) {
		return request << IScript::ManageReference(value);
	}

	IScript::Request& operator >> (IScript::Request& request, QuaternionFloat& value) {
		return request >> IScript::ManageReference(value);
	}

	IScript::Request& operator >> (IScript::Request& request, QuaternionFloat*& value) {
		IScript::TManagedReference<QuaternionFloat> r(nullptr);
		request >> r;
		value = r ? &r.GetInstance() : nullptr;

		return request;
	}

	IScript::Request& operator << (IScript::Request& request, const MatrixFloat4x4& value) {
		return request << IScript::ManageReference(value);
	}

	IScript::Request& operator >> (IScript::Request& request, MatrixFloat4x4& value) {
		return request >> IScript::ManageReference(value);
	}

	IScript::Request& operator >> (IScript::Request& request, MatrixFloat4x4*& value) {
		IScript::TManagedReference<MatrixFloat4x4> r(nullptr);
		request >> r;
		value = r ? &r.GetInstance() : nullptr;

		return request;
	}
}

IceFlow::IceFlow() {}
IceFlow::~IceFlow() {}

TObject<IReflect>& IceFlow::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestClone)[ScriptMethodLocked = "Clone"];
		ReflectMethod(RequestNewVector)[ScriptMethodLocked = "NewVector"];
		ReflectMethod(RequestNewQuaternion)[ScriptMethodLocked = "NewQuaternion"];
		ReflectMethod(RequestNewMatrix)[ScriptMethodLocked = "NewMatrix"];
		ReflectMethod(RequestGetQuaternionEulerAngles)[ScriptMethodLocked = "GetQuaternionEulerAngles"];
		ReflectMethod(RequestSetQuaternionEulerAngles)[ScriptMethodLocked = "SetQuaternionEulerAngles"];
		ReflectMethod(RequestRotateVector)[ScriptMethodLocked = "RotateVector"];
		ReflectMethod(RequestMultiplyQuaternion)[ScriptMethodLocked = "MultiplyQuaternion"];
		ReflectMethod(RequestConjugateQuaternion)[ScriptMethodLocked = "ConjugateQuaternion"];
		ReflectMethod(RequestSlerpQuaternion)[ScriptMethodLocked = "SlerpQuaternion"];
		ReflectMethod(RequestQuaternionToMatrix)[ScriptMethodLocked = "QuaternionToMatrix"];
		ReflectMethod(RequestMultiplyMatrix)[ScriptMethodLocked = "MultiplyMatrix"];
		ReflectMethod(RequestInverseMatrix)[ScriptMethodLocked = "InverseMatrix"];
		ReflectMethod(RequestQuickInverseMatrix)[ScriptMethodLocked = "QuickInverseMatrix"];
		ReflectMethod(RequestGetMatrixTranslation)[ScriptMethodLocked = "GetMatrixTranslation"];
		ReflectMethod(RequestGetMatrixRotation)[ScriptMethodLocked = "GetMatrixRotation"];
		ReflectMethod(RequestGetMatrixScale)[ScriptMethodLocked = "GetMatrixScale"];
		ReflectMethod(RequestGetComponent)[ScriptMethodLocked = "GetComponent"];
		ReflectMethod(RequestSetComponent)[ScriptMethodLocked = "SetComponent"];
		ReflectMethod(RequestGetArray)[ScriptMethodLocked = "GetArray"];
		ReflectMethod(RequestSetArray)[ScriptMethodLocked = "SetArray"];
		ReflectMethod(RequestDotProduct)[ScriptMethodLocked = "DotProduct"];
		ReflectMethod(RequestCrossProduct)[ScriptMethodLocked = "CrossProduct"];
		ReflectMethod(RequestMin)[ScriptMethodLocked = "Min"];
		ReflectMethod(RequestMax)[ScriptMethodLocked = "Max"];
		ReflectMethod(RequestClamp)[ScriptMethodLocked = "Clamp"];
	}

	return *this;
}

void IceFlow::RequestClone(IScript::Request& request, IScript::ManagedObject* object) {
	if (object != nullptr) {
		request << *object;
	}
}

void IceFlow::RequestNewVector(IScript::Request& request, IScript::Request::Arguments args) {
	switch (args.count) {
		case 2:
		{
			Float2 v;
			request >> v.x() >> v.y();
			request << v;
			break;
		}
		case 3:
		{
			Float3 v;
			request >> v.x() >> v.y() >> v.z();
			request << v;
			break;
		}
		case 4:
		{
			Float4 v;
			request >> v.x() >> v.y() >> v.z() >> v.w();
			request << v;
			break;
		}
	}
}

QuaternionFloat IceFlow::RequestNewQuaternion(IScript::Request& request) {
	return QuaternionFloat();
}

MatrixFloat4x4 IceFlow::RequestNewMatrix(IScript::Request& request) {
	return MatrixFloat4x4::Identity();
}

Float3 IceFlow::RequestGetQuaternionEulerAngles(IScript::Request& request, QuaternionFloat* value) {
	return value != nullptr ? value->ToEulerAngle() : Float3(0, 0, 0);
}

void IceFlow::RequestSetQuaternionEulerAngles(IScript::Request& request, QuaternionFloat* value, Float3* angles) {
	if (value != nullptr && angles != nullptr) {
		*value = QuaternionFloat(*angles);
	}
}

Float3 IceFlow::RequestRotateVector(IScript::Request& request, QuaternionFloat* value, Float3* v) {
	if (value != nullptr && v != nullptr) {
		Float3 ret = *v;
		value->Transform(ret);
		return ret;
	} else {
		return Float3(0, 0, 0);
	}
}

QuaternionFloat IceFlow::RequestMultiplyQuaternion(IScript::Request& request, QuaternionFloat* lhs, QuaternionFloat* rhs) {
	if (lhs != nullptr && rhs != nullptr) {
		return (*lhs) * (*rhs);
	} else {
		return QuaternionFloat();
	}
}

QuaternionFloat IceFlow::RequestConjugateQuaternion(IScript::Request& request, QuaternionFloat* value) {
	if (value != nullptr) {
		QuaternionFloat v = *value;
		v.Conjugate();
		return v;
	} else {
		return QuaternionFloat();
	}
}

QuaternionFloat IceFlow::RequestSlerpQuaternion(IScript::Request& request, QuaternionFloat* lhs, QuaternionFloat* rhs, float value) {
	if (lhs != nullptr && rhs != nullptr) {
		QuaternionFloat v;
		QuaternionFloat::Interpolate(v, *lhs, *rhs, value);
		return v;
	} else {
		return QuaternionFloat();
	}
}

MatrixFloat4x4 IceFlow::RequestQuaternionToMatrix(IScript::Request& request, QuaternionFloat* value) {
	if (value != nullptr) {
		MatrixFloat4x4 mat;
		value->WriteMatrix(mat);
		return mat;
	} else {
		return MatrixFloat4x4::Identity();
	}
}

MatrixFloat4x4 IceFlow::RequestMultiplyMatrix(IScript::Request& request, MatrixFloat4x4* lhs, MatrixFloat4x4* rhs) {
	if (lhs != nullptr && lhs != nullptr) {
		return (*lhs) * (*rhs);
	} else {
		return MatrixFloat4x4::Identity();
	}
}

MatrixFloat4x4 IceFlow::RequestInverseMatrix(IScript::Request& request, MatrixFloat4x4* value) {
	if (value != nullptr) {
		return Math::Inverse(*value);
	} else {
		return MatrixFloat4x4::Identity();
	}
}

MatrixFloat4x4 IceFlow::RequestQuickInverseMatrix(IScript::Request& request, MatrixFloat4x4* value) {
	if (value != nullptr) {
		return Math::QuickInverse(*value);
	} else {
		return MatrixFloat4x4::Identity();
	}
}

Float3 IceFlow::RequestGetMatrixTranslation(IScript::Request& request, MatrixFloat4x4* value) {
	if (value != nullptr) {
		MatrixFloat4x4& v = *value;
		return Float3(v(3, 0), v(3, 1), v(3, 2));
	} else {
		return Float3(0, 0, 0);
	}
}

QuaternionFloat IceFlow::RequestGetMatrixRotation(IScript::Request& request, MatrixFloat4x4* value) {
	if (value != nullptr) {
		return QuaternionFloat(*value);
	} else {
		return QuaternionFloat();
	}
}

Float3 IceFlow::RequestGetMatrixScale(IScript::Request& request, MatrixFloat4x4* value) {
	if (value != nullptr) {
		MatrixFloat4x4& v = *value;
		return Float3(
			Math::Length(Float3(v(0, 0), v(0, 1), v(0, 2))),
			Math::Length(Float3(v(1, 0), v(1, 1), v(1, 2))),
			Math::Length(Float3(v(2, 0), v(2, 1), v(2, 2)))
		);
	} else {
		return Float3(1, 1, 1);
	}
}

float IceFlow::RequestGetComponent(IScript::Request& request, IScript::ManagedObject* object, size_t index) {
	if (object == nullptr) {
		return 0.0f;
	}

	size_t count = object->GetInstanceLength() / sizeof(float);
	if (index >= count) {
		return 0.0f;
	}

	const float* data = reinterpret_cast<const float*>(object->GetInstanceData());
	return data[index];
}

void IceFlow::RequestSetComponent(IScript::Request& request, IScript::ManagedObject* object, size_t index, float value) {
	if (object == nullptr) {
		return;
	}

	size_t count = object->GetInstanceLength() / sizeof(float);
	if (index >= count) {
		return;
	}

	float* data = reinterpret_cast<float*>(object->GetInstanceData());
	data[index] = value;
}

void IceFlow::RequestGetArray(IScript::Request& request, IScript::ManagedObject* object) {
	if (object == nullptr) {
		return;
	}

	size_t count = object->GetInstanceLength() / sizeof(float);
	const float* data = reinterpret_cast<const float*>(object->GetInstanceData());
	request << beginarray;
	for (size_t i = 0; i < count; i++) {
		request << data[i];
	}

	request << endarray;
}

void IceFlow::RequestSetArray(IScript::Request& request, IScript::ManagedObject* object, IScript::Request::Arguments args) {
	if (object == nullptr) {
		return;
	}

	size_t count = object->GetInstanceLength() / sizeof(float);
	float* data = reinterpret_cast<float*>(object->GetInstanceData());
	IScript::Request::ArrayStart as;
	as.count = 0;
	request >> as;
	for (size_t i = 0; i < count; i++) {
		request >> data[i];
	}

	request << endarray;
}

float IceFlow::RequestDotProduct(IScript::Request& request, IScript::ManagedObject* lhs, IScript::ManagedObject* rhs) {
	if (lhs == nullptr || rhs == nullptr) {
		return 0.0f;
	}

	size_t len = Math::Min(lhs->GetInstanceLength(), rhs->GetInstanceLength());
	size_t count = len / sizeof(float);

	float sum = 0.0f;
	const float* p = reinterpret_cast<const float*>(lhs->GetInstanceData());
	const float* q = reinterpret_cast<const float*>(rhs->GetInstanceData());

	for (size_t i = 0; i < count; i++) {
		sum += p[i] * q[i];
	}

	return sum;
}

Float3 IceFlow::RequestCrossProduct(IScript::Request& request, Float3* lhs, Float3* rhs) {
	if (lhs != nullptr && rhs != nullptr) {
		return Math::CrossProduct(*lhs, *rhs);
	} else {
		return Float3(0, 0, 0);
	}
}

void IceFlow::RequestMin(IScript::Request& request, IScript::ManagedObject* lhs, IScript::ManagedObject* rhs) {
	if (lhs == nullptr || rhs == nullptr) {
		return;
	}

	size_t len = Math::Min(lhs->GetInstanceLength(), rhs->GetInstanceLength());
	IScript::ManagedObject* object = request.WriteClone(*lhs);
	size_t count = len / sizeof(float);
	
	float* p = reinterpret_cast<float*>(object->GetInstanceData());
	const float* q = reinterpret_cast<const float*>(rhs->GetInstanceData());
	for (size_t i = 0; i < count; i++) {
		p[i] = Math::Min(p[i], q[i]);
	}
}

void IceFlow::RequestMax(IScript::Request& request, IScript::ManagedObject* lhs, IScript::ManagedObject* rhs) {
	if (lhs == nullptr || rhs == nullptr) {
		return;
	}

	size_t len = Math::Min(lhs->GetInstanceLength(), rhs->GetInstanceLength());
	IScript::ManagedObject* object = request.WriteClone(*lhs);
	size_t count = len / sizeof(float);
	
	float* p = reinterpret_cast<float*>(object->GetInstanceData());
	const float* q = reinterpret_cast<const float*>(rhs->GetInstanceData());
	for (size_t i = 0; i < count; i++) {
		p[i] = Math::Min(p[i], q[i]);
	}
}

void IceFlow::RequestClamp(IScript::Request& request, IScript::ManagedObject* value, IScript::ManagedObject* lhs, IScript::ManagedObject* rhs) {
	if (lhs == nullptr || rhs == nullptr) {
		return;
	}

	size_t len = Math::Min(value->GetInstanceLength(), Math::Min(lhs->GetInstanceLength(), rhs->GetInstanceLength()));
	IScript::ManagedObject* object = request.WriteClone(*value);
	size_t count = len / sizeof(float);
	
	float* r = reinterpret_cast<float*>(object->GetInstanceData());
	const float* p = reinterpret_cast<const float*>(lhs->GetInstanceData());
	const float* q = reinterpret_cast<const float*>(rhs->GetInstanceData());
	for (size_t i = 0; i < count; i++) {
		r[i] = Math::Clamp(r[i], p[i], q[i]);
	}
}

