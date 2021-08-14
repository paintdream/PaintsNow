// LeavesFlute.h
// The Leaves Wind
// PaintDream (paintdream@paintdream.com)
// 2015-6-20
//

#pragma once
#include "../../Core/PaintsNow.h"
#include "../../General/Interface/Interfaces.h"
#include "../../Core/Interface/IType.h"
#include "../../Utility/PurpleTrail/PurpleTrail.h"
#include "../../Utility/BridgeSunset/BridgeSunset.h"
#include "../../Utility/HeartVioliner/HeartVioliner.h"
#include "../../Utility/SnowyStream/SnowyStream.h"
#include "../../Utility/MythForest/MythForest.h"
#include "../../Utility/EchoLegend/EchoLegend.h"
#include "../../Utility/Remembery/Remembery.h"
#include "../../Utility/GalaxyWeaver/GalaxyWeaver.h"

namespace PaintsNow {
	class LeavesFlute : public TReflected<LeavesFlute, IScript::Library>, public ISyncObject, public IFrame::Callback {
	public:
		LeavesFlute(bool nogui, Interfaces& interfaces, const TWrapper<IArchive*, IStreamBase&, size_t>& subArchiveCreator, const String& defMount, uint32_t threadCount, uint32_t warpCount);
		~LeavesFlute() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		Interfaces& GetInterfaces() const;
		Kernel& GetKernel();

		// Script interfaces
		void Require(IScript::Request& request) override;

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
		/// Call library
		/// </summary>
		/// <param name="handle"> library handle </param>
		/// <param name="entry"> library entry function name </param>
		/// <param name="args"> [optional] arguments </param>
		void RequestCallLibrary(IScript::Request& request, size_t handle, const String& entry, IScript::Request::Arguments args);

		/// <summary>
		/// Free library
		/// </summary>
		/// <param name="handle"> library handle </param>
		/// <returns> true if successfully free </returns>
		bool RequestFreeLibrary(IScript::Request& request, size_t handle);

	public:
		void EnterMainLoop();
		void EnterStdinLoop();
		void BeginConsole();
		void EndConsole();
		bool ConsoleProc(IThread::Thread* thread, size_t index);
		bool ProcessCommand(const String& command);
		void OverrideSetupProc(const TWrapper<void, LeavesFlute&>& handler);
		void OverrideConsoleProc(const TWrapper<void, LeavesFlute&>& handler);
		void Execute(const String& file, const std::vector<String>& params);
		void Setup();
		void EnableRawPrint(bool rawPrint);

	public:
		bool OnRender() override;
		void OnWindowSize(const IFrame::EventSize&) override;
		void OnMouse(const IFrame::EventMouse& mouse) override;
		void OnKeyboard(const IFrame::EventKeyboard& keyboard) override;
		void OnConsoleOutput(const String& text);
		void Print(const String& text);

	protected:
		Interfaces& interfaces;
		std::vector<IScript::Library*> modules;

	public:
		PurpleTrail purpleTrail;
		BridgeSunset bridgeSunset;
		EchoLegend echoLegend;
		SnowyStream snowyStream;
		MythForest mythForest;
		HeartVioliner heartVioliner;
		Remembery remembery;
		GalaxyWeaver galaxyWeaver;

	protected:
		TWrapper<void, LeavesFlute&> setupHandler;
		TWrapper<void, LeavesFlute&> consoleHandler;
		IThread::Thread* consoleThread;
		IScript::Request::Ref listenConsole;
		String newAppTitle;
		String appTitle;
		bool rawPrint;
	};
}

