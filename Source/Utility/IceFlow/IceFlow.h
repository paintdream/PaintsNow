// IceFlow.h
// PaintDream (paintdream@paintdream.com)
// 2015-1-2
//

#pragma once
#include "../../Core/Interface/IScript.h"

namespace PaintsNow {
	// Override for TVectors
	IScript::Request& operator << (IScript::Request& request, const Float2& value);
	IScript::Request& operator >> (IScript::Request& request, Float2& value);
	IScript::Request& operator >> (IScript::Request& request, Float2*& value);
	IScript::Request& operator << (IScript::Request& request, const Float3& value);
	IScript::Request& operator >> (IScript::Request& request, Float3& value);
	IScript::Request& operator >> (IScript::Request& request, Float3*& value);
	IScript::Request& operator << (IScript::Request& request, const Float4& value);
	IScript::Request& operator >> (IScript::Request& request, Float4& value);
	IScript::Request& operator >> (IScript::Request& request, Float4*& value);
	IScript::Request& operator << (IScript::Request& request, const QuaternionFloat& value);
	IScript::Request& operator >> (IScript::Request& request, QuaternionFloat& value);
	IScript::Request& operator >> (IScript::Request& request, QuaternionFloat*& value);
	IScript::Request& operator << (IScript::Request& request, const MatrixFloat4x4& value);
	IScript::Request& operator >> (IScript::Request& request, MatrixFloat4x4& value);
	IScript::Request& operator >> (IScript::Request& request, MatrixFloat4x4*& value);

	class IceFlow : public TReflected<IceFlow, IScript::Library> {
	public:
		IceFlow();
		~IceFlow() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		void RequestClone(IScript::Request& request, IScript::ManagedObject* object);
		void RequestNewVector(IScript::Request& request, IScript::Request::Arguments args);
		QuaternionFloat RequestNewQuaternion(IScript::Request& request);
		MatrixFloat4x4 RequestNewMatrix(IScript::Request& request);

		Float3 RequestGetQuaternionEulerAngles(IScript::Request& request, QuaternionFloat* value);
		void RequestSetQuaternionEulerAngles(IScript::Request& request, QuaternionFloat* value, Float3* angles);
		Float3 RequestRotateVector(IScript::Request& request, QuaternionFloat* value, Float3* vector);
		QuaternionFloat RequestMultiplyQuaternion(IScript::Request& request, QuaternionFloat* lhs, QuaternionFloat* rhs);
		QuaternionFloat RequestConjugateQuaternion(IScript::Request& request, QuaternionFloat* value);
		QuaternionFloat RequestSlerpQuaternion(IScript::Request& request, QuaternionFloat* lhs, QuaternionFloat* rhs, float value);
		MatrixFloat4x4 RequestQuaternionToMatrix(IScript::Request& request, QuaternionFloat* value);

		MatrixFloat4x4 RequestMultiplyMatrix(IScript::Request& request, MatrixFloat4x4* lhs, MatrixFloat4x4* rhs);
		MatrixFloat4x4 RequestInverseMatrix(IScript::Request& request, MatrixFloat4x4* matrix);
		MatrixFloat4x4 RequestQuickInverseMatrix(IScript::Request& request, MatrixFloat4x4* matrix);
		Float3 RequestGetMatrixTranslation(IScript::Request& request, MatrixFloat4x4* matrix);
		QuaternionFloat RequestGetMatrixRotation(IScript::Request& request, MatrixFloat4x4* matrix);
		Float3 RequestGetMatrixScale(IScript::Request& request, MatrixFloat4x4* matrix);
		float RequestGetComponent(IScript::Request& request, IScript::ManagedObject* object, size_t index);
		void RequestSetComponent(IScript::Request& request, IScript::ManagedObject* object, size_t index, float value);
		void RequestGetArray(IScript::Request& request, IScript::ManagedObject* object);
		void RequestSetArray(IScript::Request& request, IScript::ManagedObject* object, IScript::Request::Arguments args);
		float RequestDotProduct(IScript::Request& request, IScript::ManagedObject* lhs, IScript::ManagedObject* rhs);
		Float3 RequestCrossProduct(IScript::Request& request, Float3* lhs, Float3* rhs);
		void RequestMin(IScript::Request& request, IScript::ManagedObject* lhs, IScript::ManagedObject* rhs);
		void RequestMax(IScript::Request& request, IScript::ManagedObject* lhs, IScript::ManagedObject* rhs);
		void RequestClamp(IScript::Request& request, IScript::ManagedObject* value, IScript::ManagedObject* lhs, IScript::ManagedObject* rhs);
	};
}
