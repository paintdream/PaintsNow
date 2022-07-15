// ToolkitWin32.h
// PaintDream (paintdream@paintdream.com)
// 2021-3-1
//

#pragma once

#ifdef _WIN32

#include "../../../../Utility/LeavesFlute/LeavesFlute.h"

namespace PaintsNow {
	class ToolkitWin32 : public TReflected<ToolkitWin32, IScript::Library> {
	public:
		ToolkitWin32();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		~ToolkitWin32() override;

		void ScriptInitialize(IScript::Request& request) override;
		void ScriptUninitialize(IScript::Request& request) override;
		void HandleMessage(LeavesFlute& leavesFlute, uint32_t message, uint64_t wParam, uint64_t lParam);
		uint32_t GetMainThreadID() const;

	protected:
		/// <summary>
		/// Retrieve system info
		/// </summary>
		/// <returns/> system info dict </returns>
		void RequestGetSystemInfo(IScript::Request& request);

		/// <summary>
		/// Get id of main thread.
		/// </summary>
		/// <param name="request"></param>
		/// <returns></returns>
		uint32_t RequestGetMainThreadID(IScript::Request& request);

		/// <summary>
		/// Listen win32 message
		/// </summary>
		/// <param name="callback"> message callback </param>
		void RequestListenMessage(IScript::Request& request, IScript::Request::Ref callback);

		/// <summary>
		/// Exit service worker
		/// </summary>
		/// <param name="request"></param>
		void RequestExit(IScript::Request& request);

		/// <summary>
		/// Post message to thread
		/// </summary>
		/// <param name="thread"> thread id </param>
		void RequestPostThreadMessage(IScript::Request& request, uint64_t thread, uint32_t msg, uint64_t wParam, uint64_t lParam);

		/// <summary>
		/// Create win32 process, returning handle
		/// </summary>
		/// <param name="path"> executable path </param>
		/// <param name="currentPath"> current folder path </param>
		/// <param name="parameter"> optional configs </param>
		/// <returns> a list with { win32 process handle, main thread id } </returns>
		std::pair<uint64_t, uint64_t> RequestCreateProcess(IScript::Request& request, const String& path, const String& parameter, const String& currentPath);

		/// <summary>
		/// Get path of host program
		/// </summary>
		/// <returns> full path of host program </returns>
		String RequestGetHostPath(IScript::Request& request);

		/// <summary>
		/// Close win32 handle
		/// </summary>
		/// <param name="object"> object handle </param>
		void RequestCloseHandle(IScript::Request& request, uint64_t object);

		/// <summary>
		/// Wait for single object (process/thread/event)
		/// </summary>
		/// <param name="object"> object handle </param>
		/// <param name="timeout"> timeout </param>
		/// <returns> the return value of WaitForSingleObject </returns>
		uint32_t RequestWaitForSingleObject(IScript::Request& request, uint64_t object, uint64_t timeout);

		/// <summary>
		/// Terminate a process
		/// </summary>
		/// <param name="process"> process handle </param>
		/// <param name="exitCode"> exit code </param>
		/// <returns> true if successfully terminated </returns>
		bool RequestTerminateProcess(IScript::Request& request, uint64_t process, uint32_t exitCode);

	protected:
		IScript::Request::Ref messageListener;
		uint32_t mainThreadID;
	};
}

#endif
