#pragma once

using namespace System;
#include "../../Source/Core/Interface/IScript.h"

namespace DotNetBridge {
	ref class LeavesBridge;
	public ref class ScriptReference
	{
	private:
		size_t handle;
		LeavesBridge^ bridge;

	public:
		ScriptReference(LeavesBridge^ bridge, size_t h);
		~ScriptReference();

		property bool Valid { bool get() { return handle != 0; } };
		property size_t Handle { size_t get() { return handle; } };
		int AsInteger();
		double AsDouble();
		float AsFloat();
		System::IntPtr AsHandle();
		System::String^ AsString();
		Object^ AsObject();
		Object^ Call(... array<Object^>^ args);
	};

	public ref class LeavesBridge
	{
	public:
		LeavesBridge();

	public:
		array<Type^>^ emptyTypeArray;

	public:
		void Initialize(PaintsNow::IScript::Request& request);
		void Uninitialize(PaintsNow::IScript::Request& request);
		UIntPtr GetScriptHandle();
		ScriptReference^ GetGlobal(System::String^ name);
		ScriptReference^ Load(System::String^ code);
		PaintsNow::IScript* script = nullptr;
		PaintsNow::IScript::RequestPool* requestPool = nullptr;
	};
}
