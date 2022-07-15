#include "DotNetBridge.h"
#include "../../Source/Core/System/Tiny.h"
#include <vcclr.h>

using namespace System::Text;
using namespace System::Collections;
using namespace System::Diagnostics;
using namespace System::Reflection;
using namespace System::Linq;
using namespace System::Linq::Expressions;
using namespace PaintsNow;
using namespace DotNetBridge;

class SharpObjectReference : public SharedTiny
{
public:
	gcroot<System::Object^> object;
};

static PaintsNow::String FromManagedString(System::String^ str)
{
	array<Byte>^ byteArray = Encoding::UTF8->GetBytes(str);
	pin_ptr<unsigned char> v = &byteArray[0];
	return PaintsNow::String(reinterpret_cast<char*>(v), byteArray->Length);
}

static System::String^ ToManagedString(const PaintsNow::String& str)
{
	return gcnew System::String(str.c_str(), 0, (int)str.length(), Encoding::UTF8);
}

static System::String^ ToManagedString(const char* str)
{
	return gcnew System::String(str, 0, (int)strlen(str), Encoding::UTF8);
}

// Serializer Helpers
static void WriteValueArray(IScript::Request& request, array<Object^>^ args);
static void WriteValue(IScript::Request& request, Object^ object)
{
	if (object == nullptr) return;

	Type^ type = object->GetType();
	if (type->IsArray)
	{
		array<Object^>^ arr = dynamic_cast<array<Object^>^>(object);
		if (arr)
		{
			request << beginarray;
			WriteValueArray(request, arr);
			request << endarray;
		}
	}
	else
	{
		switch (Type::GetTypeCode(type))
		{
		case TypeCode::Byte:
		case TypeCode::SByte:
		case TypeCode::UInt16:
		case TypeCode::UInt32:
		case TypeCode::UInt64:
		case TypeCode::Int16:
		case TypeCode::Int32:
		case TypeCode::Int64:
			request << safe_cast<int64_t>(object);
			return;
		case TypeCode::Decimal:
		case TypeCode::Double:
		case TypeCode::Single:
			request << safe_cast<double>(object);
			return;
		case TypeCode::String:
			request << FromManagedString(safe_cast<System::String^>(object));
			return;
		}

		IDictionary^ dictionary = dynamic_cast<IDictionary^>(object);
		if (dictionary != nullptr)
		{
			IDictionaryEnumerator^ it = dictionary->GetEnumerator();
			request << begintable;
			while (it->MoveNext())
			{
				request << key(FromManagedString(it->Key->ToString()));
				WriteValue(request, it->Value);
			}
			request << endtable;
		}
		else
		{
			// A enumerable?
			IEnumerable^ enumerable = dynamic_cast<IEnumerable^>(object);
			if (enumerable != nullptr)
			{
				IEnumerator^ it = enumerable->GetEnumerator();
				request << beginarray;
				while (it->MoveNext())
				{
					WriteValue(request, it->Current);
				}
				request << endarray;
			}
			else
			{
				SharpObjectReference* dref = new SharpObjectReference();
				dref->object = object;
				request << dref;
				dref->ReleaseObject();
			}
		}
	}
}

static void WriteValueArray(IScript::Request& request, array<Object^>^ args)
{
	int lowerBound = args->GetLowerBound(0);
	int upperBound = args->GetUpperBound(0);

	if (upperBound != -1)
	{
		for (int i = lowerBound; i <= upperBound; i++)
		{
			Object^ object = args[i];
			WriteValue(request, object);
		}
	}
}

static Object^ ReadValue(LeavesBridge^ bridge, IScript::Request& request)
{
	switch (request.GetCurrentType())
	{
	case IScript::Request::NIL:
	{
		uint64_t v;
		request >> v;
		return nullptr;
	}
	case IScript::Request::BOOLEAN:
	{
		bool v = false;
		request >> v;
		return Boolean(v);
	}
	case IScript::Request::NUMBER:
	{
		double v = 0;
		request >> v;
		return Double(v);
	}
	case IScript::Request::INTEGER:
	{
		int v = 0;
		request >> v;
		return v;
	}
	case IScript::Request::STRING:
	{
		const char* s = nullptr;
		request >> s;
		return ToManagedString(s);
	}
	case IScript::Request::TABLE:
	case IScript::Request::ARRAY:
	{
		IScript::Request::TableStart ts;
		request >> ts;
		Object^ ret = nullptr;
		if (ts.count != 0)
		{
			array<Object^>^ arr = gcnew array<Object^>((int)ts.count);
			for (int i = 0; i < (int)ts.count; i++)
			{
				arr[i] = ReadValue(bridge, request);
			}

			ret = arr;
		}
		else
		{
			Generic::Dictionary<Object^, Object^>^ dict = gcnew Generic::Dictionary<Object^, Object^>();
			IScript::Request::Iterator it;
			while (true)
			{
				request >> it;
				if (!it) break;

				Object^ key = ReadValue(bridge, request);
				Object^ value = ReadValue(bridge, request);
				dict->Add(key, value);
			}

			ret = dict;
		}
		request << endtable;
		return ret;
	}
	case IScript::Request::FUNCTION:
	case IScript::Request::OBJECT:
	{
		IScript::Request::Ref r;
		request >> r;
		if (r)
		{
			return gcnew ScriptReference(bridge, r.value);
		}
		else
		{
			return nullptr;
		}
	}
	}

	return nullptr;
}

static Object^ ReadValueTyped(LeavesBridge^ bridge, IScript::Request& request, Type^ type)
{
	if (type->IsArray)
	{
		Type^ elementType = type->GetElementType();
		IScript::Request::ArrayStart as;
		request >> as;
		array<System::Object^>^ retValue = gcnew array<System::Object^>((int)as.count);
		for (int i = 0; i < (int)as.count; i++)
		{
			retValue[i] = ReadValueTyped(bridge, request, elementType);
		}
		request << endarray;
		return retValue;
	}
	else
	{
		switch (Type::GetTypeCode(type))
		{
		case TypeCode::Byte:
		{
			uint64_t value;
			request >> value;
			return (Byte)value;
		}
		case TypeCode::SByte:
		{
			int64_t value;
			request >> value;
			return (SByte)value;
		}
		case TypeCode::UInt16:
		{
			uint64_t value;
			request >> value;
			return (UInt16)value;
		}
		case TypeCode::UInt32:
		{
			uint64_t value;
			request >> value;
			return (UInt32)value;
		}
		case TypeCode::UInt64:
		{
			uint64_t value;
			request >> value;
			return (UInt64)value;
		}
		case TypeCode::Int16:
		{
			int64_t value;
			request >> value;
			return (Int16)value;
		}
		case TypeCode::Int32:
		{
			int64_t value;
			request >> value;
			return (Int32)value;
		}
		case TypeCode::Int64:
		{
			int64_t value;
			request >> value;
			return (Int64)value;
		}
		case TypeCode::Decimal:
		{
			double value;
			request >> value;
			return (Decimal)value;
		}
		case TypeCode::Double:
		{
			double value;
			request >> value;
			return (Double)value;
		}
		case TypeCode::Single:
		{
			double value;
			request >> value;
			return (Single)value;
		}
		case TypeCode::String:
		{
			const char* value;
			request >> value;
			return ToManagedString(value);
		}
		case TypeCode::Object:
		{
			IScript::Request::Ref r;
			request >> r;
			if (r)
			{
				return gcnew ScriptReference(bridge, r.value);
			}
		}
		}

		// check dict, slow
		array<Type^>^ args = type->GenericTypeArguments;
		if (args->GetUpperBound(0) - args->GetLowerBound(0) == 1) // 2 args, test dict
		{
			MethodInfo^ add = type->GetMethod("Add");
			if (add != nullptr)
			{
				ConstructorInfo^ info = type->GetConstructor(bridge->emptyTypeArray);
				if (info != nullptr)
				{
					Type^ keyType = args[args->GetLowerBound(0)];
					Type^ valueType = args[args->GetUpperBound(0)];
					Object^ object = info->Invoke(nullptr);

					IScript::Request::Iterator it;
					array<Object^>^ params = gcnew array<Object^>(2);
					while (true)
					{
						request >> it;
						if (!it) break;

						params[0] = ReadValueTyped(bridge, request, keyType);
						params[1] = ReadValueTyped(bridge, request, valueType);

						add->Invoke(object, params);
					}
				}
			}
		}
	}

	return nullptr;
}


class SharpDelegateReference : public SharpObjectReference
{
public:
	void RequestCall(IScript::Request& request, IScript::Request::Arguments args)
	{
		Delegate^ d = static_cast<Delegate^>((System::Object^)object);
		if (d)
		{
			array<System::Object^>^ params = gcnew array<System::Object^>((int)args.count);
			int count = (int)args.count;
			array<ParameterInfo^>^ parameters = d->Method->GetParameters();
			int lowerBound = parameters->GetLowerBound(0);
			int upperBound = parameters->GetUpperBound(0);

			upperBound = count == 0 ? -1 : std::min(lowerBound + count - 1, upperBound);

			if (upperBound != -1)
			{
				request.DoLock();
				for (int i = lowerBound; i <= upperBound; i++)
				{
					params[i - lowerBound] = ReadValueTyped(bridge, request, parameters[i]->ParameterType);
				}
				request.UnLock();
			}

			System::Object^ retValue = d->DynamicInvoke(params);
			if (retValue != nullptr)
			{
				request.DoLock();
				WriteValue(request, retValue);
				request.UnLock();
			}
		}
	}

	gcroot<LeavesBridge^> bridge;
};

class SharpBridgeReference : public SharpObjectReference
{
public:
	void RequestCreateInstance(PaintsNow::IScript::Request& request, const char* libraryName, const char* entryTypeName)
	{
		Assembly^ assembly = Assembly::LoadFile(IO::Path::GetFullPath(ToManagedString(libraryName)));
		System::Object^ instance = nullptr;

		try
		{
			instance = assembly->CreateInstance(ToManagedString(entryTypeName), false, BindingFlags::Default, nullptr, nullptr, nullptr, nullptr);
		}
		catch (MissingMethodException^ )
		{
			return;
		}

		if (instance != nullptr)
		{
			SharpObjectReference* ref = new SharpObjectReference();
			ref->object = instance;
			Type^ type = instance->GetType();
			array<MethodInfo^>^ methods = type->GetMethods();

			std::vector<SharedTiny*> holdings;
			holdings.emplace_back(ref);

			request.DoLock();
			request << begintable;
			// Register delegates
			int lowerBound = methods->GetLowerBound(0);
			int upperBound = methods->GetUpperBound(0);

			if (upperBound != -1)
			{
				for (int i = lowerBound; i <= upperBound; i++)
				{
					MethodInfo^ info = methods[i];
					// https://stackoverflow.com/questions/16364198/how-to-create-a-delegate-from-a-methodinfo-when-method-signature-cannot-be-known
					array<ParameterInfo^>^ parameters = info->GetParameters();
					int lowerBound = parameters->GetLowerBound(0);
					int upperBound = parameters->GetUpperBound(0);
					array<Type^>^ types = nullptr;

					if (upperBound != -1)
					{
						types = gcnew array<Type^>(upperBound - lowerBound + 2);
						for (int j = lowerBound; j <= upperBound; j++)
						{
							types[j - lowerBound] = parameters[j]->ParameterType;
						}

						types[upperBound - lowerBound + 1] = info->ReturnType;
					}
					else
					{
						types = gcnew array<Type^>(1);
						types[0] = info->ReturnType;
					}

					Delegate^ d = info->CreateDelegate(Expression::GetDelegateType(types), instance);
					SharpDelegateReference* dref = new SharpDelegateReference();
					dref->object = d;
					dref->bridge = static_cast<LeavesBridge^>((System::Object^)object);

					request << key(FromManagedString(info->Name)) << request.Adapt(Wrap(dref, &SharpDelegateReference::RequestCall));
					holdings.emplace_back(dref);
				}
			}

			request << key("__delegates__") << holdings;
			request << endtable;
			request.UnLock();

			for (size_t i = 0; i < holdings.size(); i++)
			{
				holdings[i]->ReleaseObject();
			}
		}
	}
};

ScriptReference::ScriptReference(LeavesBridge^ b, size_t h) : bridge(b), handle(h) {}
ScriptReference::~ScriptReference()
{
	IScript* script = bridge->script;
	script->DoLock();
	script->GetDefaultRequest().Dereference(IScript::Request::Ref(handle));
	script->UnLock();
}

Object^ ScriptReference::Call(... array<Object^>^ args)
{
	if (!Valid)
	{
		throw gcnew NullReferenceException("Unable to call invalid function.");
	}

	IScript::RequestPool* requestPool = bridge->requestPool;
	IScript::Request& request = *requestPool->requestPool.AcquireSafe();
	IScript::Request::Ref f(handle);

	request.DoLock();
	request.Push();
	WriteValueArray(request, args);
	request.Call(f);
	Object^ retValue = ReadValue(bridge, request);
	request.Pop();
	request.UnLock();

	requestPool->requestPool.ReleaseSafe(&request);
	return retValue;
}

template <typename T>
static T GetValue(LeavesBridge^ bridge, size_t handle)
{
	if (handle == 0) return T();

	T t;
	IScript::Request& request = bridge->script->GetDefaultRequest();
	request.DoLock();
	request.Push();
	request << IScript::Request::Ref(handle);
	request >> t;
	request.Pop();
	request.UnLock();

	return t;
}

int ScriptReference::AsInteger()
{
	return GetValue<int>(bridge, handle);
}

double ScriptReference::AsDouble()
{
	return GetValue<double>(bridge, handle);
}

float ScriptReference::AsFloat()
{
	return GetValue<float>(bridge, handle);
}

System::IntPtr ScriptReference::AsHandle()
{
	return System::IntPtr((void*)GetValue<uint64_t>(bridge, handle));
}

System::String^ ScriptReference::AsString()
{
	return ToManagedString(GetValue<const char*>(bridge, handle));
}

Object^ ScriptReference::AsObject()
{
	if (handle == 0) return nullptr;

	IScript::Request& request = bridge->script->GetDefaultRequest();
	request.DoLock();
	request.Push();
	request << IScript::Request::Ref(handle);
	Object^ value = ReadValue(bridge, request);
	request.Pop();
	request.UnLock();

	return value;
}

UIntPtr LeavesBridge::GetScriptHandle()
{
	return UIntPtr(script);
}

ScriptReference^ LeavesBridge::GetGlobal(System::String^ name)
{
	Debug::Assert(script != nullptr);

	IScript::Request& request = script->GetDefaultRequest();
	IScript::Request::Ref r;
	request.DoLock();
	request << global << key(FromManagedString(name).c_str()) >> r << endtable;
	request.UnLock();

	return gcnew ScriptReference(this, r.value);
}

ScriptReference^ LeavesBridge::Load(System::String^ code)
{
	Debug::Assert(script != nullptr);

	IScript::Request& request = script->GetDefaultRequest();
	request.DoLock();
	IScript::Request::Ref r = request.Load(FromManagedString(code));
	request.UnLock();

	return gcnew ScriptReference(this, r.value);
}

void LeavesBridge::Initialize(IScript::Request& request)
{
	Debug::Assert(script == nullptr);

	script = request.GetScript();
	SharpBridgeReference* ref = new SharpBridgeReference();
	ref->object = this;

	request.DoLock();
	requestPool = request.GetRequestPool();
	request << begintable;
	request << key("CreateInstance") << request.Adapt(Wrap(ref, &SharpBridgeReference::RequestCreateInstance));
	request << key("__delegate__") << ref;
	request << endtable;
	request.UnLock();

	ref->ReleaseObject();
}

void LeavesBridge::Uninitialize(IScript::Request& request)
{
	Debug::Assert(script != nullptr);

	script = nullptr;
	requestPool = nullptr;
}

LeavesBridge::LeavesBridge()
{
	emptyTypeArray = gcnew array<Type^>{};
}

extern "C" __declspec(dllexport) size_t Main(IScript::Request& request)
{
	const char* command = nullptr;
	request.DoLock();
	request >> command; // do not use String on ABI
	request.UnLock();

	PaintsNow::String strCommand = command;

	if (strCommand == "Initialize")
	{
		LeavesBridge^ bridge = gcnew LeavesBridge();
		bridge->Initialize(request);
	}
	else if (strCommand == "Uninitialize")
	{
		IScript::Delegate<SharpObjectReference> sharpObject;
		request.DoLock();
		request >> sharpObject;
		request.UnLock();

		if (sharpObject)
		{
			LeavesBridge^ bridge = dynamic_cast<LeavesBridge^>((Object^)sharpObject.Get()->object);
			if (bridge != nullptr)
			{
				bridge->Uninitialize(request);
			}
		}
	}

	return 1;
}
