// LeavesFlute.h
// The Leaves Wind
// PaintDream (paintdream@paintdream.com)
// 2015-6-20
//

#pragma once
#include "../../Core/PaintsNow.h"
#include "../../General/Interface/Interfaces.h"
#include "../../Core/Interface/IType.h"
#include "../../Utility/BridgeSunset/BridgeSunset.h"
#include "../../Utility/EchoLegend/EchoLegend.h"
#include "../../Utility/GalaxyWeaver/GalaxyWeaver.h"
#include "../../Utility/HeartVioliner/HeartVioliner.h"
#include "../../Utility/IceFlow/IceFlow.h"
#include "../../Utility/MythForest/MythForest.h"
#include "../../Utility/PurpleTrail/PurpleTrail.h"
#include "../../Utility/Remembery/Remembery.h"
#include "../../Utility/SnowyStream/SnowyStream.h"
#include "../../Core/Module/CrossScriptModule.h"
#include "IPlugin.h"

namespace PaintsNow {
	class LeavesFlute : public TReflected<LeavesFlute, IScript::Library>, public ISyncObject, public IFrame::Callback, protected IPlugin {
	public:
		LeavesFlute(bool nogui, Interfaces& interfaces, const TWrapper<IArchive*, IStreamBase&, size_t>& subArchiveCreator, const String& defMount, uint32_t threadCount, uint32_t warpCount, long threadBalancer);
		~LeavesFlute() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		Interfaces& GetInterfaces() const;
		Kernel& GetKernel();
		void Exit();

		// Script interfaces
		void ScriptRequire(IScript::Request& request) override;

		/// <summary>
		/// Get system platform string
		/// </summary>
		/// <returns> system platform string </returns>
		String RequestGetSystemPlatform(IScript::Request& request);

		/// <summary>
		/// Get processor bit witdh
		/// </summary>
		/// <returns> bit with (e.g. 64 for 64-bit system) </returns>
		size_t RequestGetProcessorBitWidth(IScript::Request& request);

		/// <summary>
		/// Listen console input
		/// </summary>
		/// <param name="callback"> callback </param>
		/// <returns></returns>
		void RequestListenConsole(IScript::Request& request, IScript::Request::Ref callback);

		/// <summary>
		/// Prints a string
		/// </summary>
		/// <param name="text"> string to print </param>
		/// <returns></returns>
		void RequestPrint(IScript::Request& request, const String& text);

		/// <summary>
		/// Exit the program normally.
		/// </summary>
		/// <returns></returns>
		void RequestExit(IScript::Request& request);

		/// <summary>
		/// Set position of cursor
		/// </summary>
		/// <param name="position"> target position </param>
		/// <returns></returns>
		void RequestWarpCursor(IScript::Request& request, Int2 position);

		/// <summary>
		/// Change cursor type
		/// </summary>
		/// <param name="type"> target cursor type </param>
		/// <returns></returns>
		void RequestShowCursor(IScript::Request& request, const String& type);

		/// <summary>
		/// Set title of current application
		/// </summary>
		/// <param name="title"> the title</param>
		/// <returns></returns>
		void RequestSetAppTitle(IScript::Request& request, const String& title);

		/// <summary>
		/// Set screen size
		/// </summary>
		/// <param name="size"> new screen size </param>
		/// <returns></returns>
		void RequestSetScreenSize(IScript::Request& request, Int2& size);

		/// <summary>
		/// Get screen size
		/// </summary>
		/// <returns> the screen size </returns>
		Int2 RequestGetScreenSize(IScript::Request& request);

		/// <summary>
		/// Search data in current process memory (win32 only)
		/// </summary>
		/// <param name="data"> data to search </param>
		/// <param name="start"> start address </param>
		/// <param name="end"> end address </param>
		/// <param name="alignment"> address alignment </param>
		/// <param name="maxResult"> result count </param>
		/// <returns> list of results </returns>
		void RequestSearchMemory(IScript::Request& request, const String& data, size_t start, size_t end, uint32_t alignment, uint32_t maxResult);

		/// <summary>
		/// Load library
		/// </summary>
		/// <param name="library"> library name </param>
		/// <returns> library handle </returns>
		size_t RequestLoadLibrary(IScript::Request& request, const String& library);

		/// <summary>
		/// Get library function
		/// </summary>
		/// <param name="handle"> library handle, 0 for current executable </param>
		/// <param name="entry"> library entry function name </param>
		/// <returns> function address </returns>
		size_t RequestGetSymbolAddress(IScript::Request& request, size_t handle, const String& entry);

		/// <summary>
		/// Call symbol
		/// </summary>
		/// <param name="address"> library function address </param>
		/// <param name="args"> [optional] arguments </param>
		/// <returns> return value from function call </returns>
		size_t RequestCallSymbol(IScript::Request& request, size_t address, IScript::Request::Arguments args);

		/// <summary>
		/// Free library
		/// </summary>
		/// <param name="handle"> library handle </param>
		/// <returns> true if successfully free </returns>
		bool RequestFreeLibrary(IScript::Request& request, size_t handle);

	public:
		void EnterMainLoop();
		void EnterStdinLoop();
		bool ConsoleProc(IThread::Thread* thread, size_t index);
		bool ProcessCommand(const String& command);
		void OverrideSetupProc(const TWrapper<void, LeavesFlute&>& handler);
		void OverrideConsoleProc(const TWrapper<void, LeavesFlute&>& handler);
		void Execute(const String& file, const std::vector<String>& params);
		void Setup();

	public:
		void OnRender() override;
		void OnWindowSize(const IFrame::EventSize&) override;
		void OnMouse(const IFrame::EventMouse& mouse) override;
		void OnKeyboard(const IFrame::EventKeyboard& keyboard) override;

	protected:
		// Plugin
		const char* GetVersionMajor() override;
		const char* GetVersionMinor() override;
		void* GetSymbolAddress(const char* name) override;
		unsigned int GetThreadCount() override;
		unsigned int GetCurrentThreadIndex() override;
		void* AllocateMemory(unsigned int size) override;
		void FreeMemory(void* p, unsigned int size) override;
		void QueueTask(Task* task, int priority) override;
		void QueueWarpTask(Task* task, unsigned int warp) override;
		unsigned int GetCurrentWarpIndex() override;
		void SetWarpPriority(unsigned int warp, int priority) override;
		unsigned int YieldCurrentWarp() override;
		void SuspendWarp(unsigned int warp) override;
		void ResumeWarp(unsigned int warp) override;
		unsigned int AllocateWarpIndex() override;
		void FreeWarpIndex(unsigned int warp) override;
		void RegisterEngineCallback(EngineCallback* frameCallback) override;
		void UnregisterEngineCallback(EngineCallback* frameCallback) override;
		Stream* OpenStream(const char* path, bool openExisting, unsigned long& len) override;
		void FlushStream(Stream* stream) override;
		bool ReadStream(Stream* stream, void* p, unsigned long& len) override;
		bool WriteStream(Stream* stream, const void* p, unsigned long& len) override;
		bool SeekStream(Stream* stream, SEEK_OPTION option, long offset) override;
		void CloseStream(Stream* stream) override;
		void PushProfilerSection(const char* text) override;
		void PopProfilerSection() override;

		Script* AllocateScript() override;
		void RegisterScriptHandler(Script* script, const char* procedure, ScriptHandler scriptHandler, ScriptHandlerResponse response, void* context) override;
		void UnregisterScriptHandler(Script* script, const char* procedure) override;
		bool CallScript(Script* script, const char* procedure, const char* requestData, unsigned long len, ScriptHandlerResponse response, void* context) override;
		void FreeScript(Script* script) override;

	protected:
		Interfaces& interfaces;
		std::vector<IScript::Library*> modules;
		std::vector<EngineCallback*> engineCallbacks;
		std::unordered_map<String, void*> symbolMap;

	public:
		IceFlow iceFlow;
		PurpleTrail purpleTrail;
		BridgeSunset bridgeSunset;
		EchoLegend echoLegend;
		SnowyStream snowyStream;
		MythForest mythForest;
		HeartVioliner heartVioliner;
		Remembery remembery;
		GalaxyWeaver galaxyWeaver;

	public:
		CrossScriptModule crossScriptModule;

	protected:
		TWrapper<void, LeavesFlute&> setupHandler;
		TWrapper<void, LeavesFlute&> consoleHandler;
		std::atomic<size_t> looping;
		IScript::Request::Ref listenConsole;
		String newAppTitle;
		String appTitle;
#ifdef _WIN32
		size_t consoleThreadID;
#endif
	};
}

