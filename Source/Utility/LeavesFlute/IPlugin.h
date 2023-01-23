// IPlugin.h
// PaintDream (paintdream@paintdream.com)
// 2022-6-15
//

#pragma once

namespace PaintsNow {
	class IPlugin {
	public:
		virtual ~IPlugin() {}
		virtual const char* GetVersionMajor() = 0;
		virtual const char* GetVersionMinor() = 0;
		virtual void* GetSymbolAddress(const char* name) = 0;

		// Task related
		class Task {
		public:
			virtual ~Task() {}
			virtual void Execute() = 0;
			virtual void Abort() = 0;
		};

		virtual unsigned int GetThreadCount() = 0;
		virtual unsigned int GetCurrentThreadIndex() = 0;
		// at least 16-bit aligned for size >= 16. 64 KB or larger is recommended
		virtual void* AllocateMemory(unsigned int size) = 0;
		virtual void FreeMemory(void* p, unsigned int size) = 0;

		virtual void QueueTask(Task* task, int priority = 0) = 0;
		virtual void QueueWarpTask(Task* task, unsigned int warp = ~(unsigned int)0) = 0;
		virtual unsigned int GetCurrentWarpIndex() = 0;
		virtual void SetWarpPriority(unsigned int warp, int priority) = 0;
		virtual unsigned int YieldCurrentWarp() = 0;
		virtual void SuspendWarp(unsigned int warp) = 0;
		virtual void ResumeWarp(unsigned int warp) = 0;
		virtual unsigned int AllocateWarpIndex() = 0;
		virtual void FreeWarpIndex(unsigned int warp) = 0;

		// Frame related
		class EngineCallback {
		public:
			virtual ~EngineCallback() {}

			virtual void OnRenderBegin() = 0;
			virtual void OnRenderEnd() = 0;
			virtual void OnEngineReset(bool exit) = 0;
		};

		virtual void RegisterEngineCallback(EngineCallback* frameCallback) = 0;
		virtual void UnregisterEngineCallback(EngineCallback* frameCallback) = 0;

		// Stream operations
		class Stream;
		enum SEEK_OPTION { BEGIN, CUR, END };

		virtual Stream* OpenStream(const char* path, bool openExisting, unsigned long& len) = 0;
		virtual void FlushStream(Stream* stream) = 0;
		virtual bool ReadStream(Stream* stream, void* p, unsigned long& len) = 0;
		virtual bool WriteStream(Stream* stream, const void* p, unsigned long& len) = 0;
		virtual bool SeekStream(Stream* stream, SEEK_OPTION option, long offset) = 0;
		virtual void CloseStream(Stream* stream) = 0;

		// Profiler
		virtual void PushProfilerSection(const char* text) = 0;
		virtual void PopProfilerSection() = 0;

		// Script
		typedef const char* (*ScriptHandler)(const char* requestData, unsigned long& len, void* context);
		typedef void (*ScriptHandlerResponse)(const char* responseData, unsigned long len, void* context);

		class Script;
		virtual Script* AllocateScript() = 0;
		virtual void RegisterScriptHandler(Script* script, const char* procedure, ScriptHandler scriptHandler, ScriptHandlerResponse response, void* context) = 0;
		virtual void UnregisterScriptHandler(Script* script, const char* procedure) = 0;
		virtual bool CallScript(Script* script, const char* procedure, const char* requestData, unsigned long len, ScriptHandlerResponse response, void* context) = 0;
		virtual void FreeScript(Script* script) = 0;
	};
}