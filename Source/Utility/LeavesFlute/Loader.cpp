#include "Loader.h"
// #include <vld.h>
#include "../LeavesFlute/Platform.h"

#if !defined(CMAKE_PAINTSNOW) || ADD_RENDER_OPENGL
#include "../../General/Driver/Render/OpenGL/ZRenderOpenGL.h"
#endif

#if (!defined(CMAKE_PAINTSNOW) || ADD_RENDER_VULKAN) && (!defined(_MSC_VER) || _MSC_VER > 1200)
#include "../../General/Driver/Render/Vulkan/ZRenderVulkan.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_NETWORK_LIBEVENT
#include "../../General/Driver/Network/LibEvent/ZNetworkLibEvent.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_NETWORK_KCP
#include "../../General/Driver/Network/KCP/ZNetworkKCP.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_IMAGE_FREEIMAGE
#include "../../General/Driver/Image/FreeImage/ZImageFreeImage.h"
#endif

#if defined(_WIN32) || defined(WIN32)
#include <Windows.h>
#if !defined(CMAKE_PAINTSNOW) || ADD_TIMER_TIMERQUEUE_BUILTIN
#include "../../General/Driver/Timer/WinTimerQueue/ZWinTimerQueue.h"
#endif
#else
#if !defined(CMAKE_PAINTSNOW) || ADD_TIMER_POSIX_BUILTIN
#include "../../General/Driver/Timer/PosixTimer/ZPosixTimer.h"
#endif
#endif

#if !defined(CMAKE_PAINTSNOW) || (ADD_FRAME_GLFW && !defined(CMAKE_ANDROID))
#include "../../General/Driver/Frame/GLFW/ZFrameGLFW.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_ARCHIVE_DIRENT_BUILTIN
#include "../../Core/Driver/Archive/Dirent/ZArchiveDirent.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_ARCHIVE_VIRTUAL_BUILTIN
#include "../../General/Driver/Archive/Virtual/ZArchiveVirtual.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_LZMA_BUILTIN
#include "../../General/Driver/Archive/7Z/ZArchive7Z.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_OPENAL
#include "../../General/Driver/Audio/OpenAL/ZAudioOpenAL.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_LAME
#include "../../General/Driver/Filter/LAME/ZFilterLAME.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FONT_FREETYPE
#include "../../General/Driver/Font/Freetype/ZFontFreetype.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_SCRIPT_LUA_BUILTIN
#include "../../Core/Driver/Script/Lua/ZScriptLua.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_THREAD_PTHREAD
#include "../../Core/Driver/Thread/Pthread/ZThreadPthread.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_DATABASE_SQLITE3_BUILTIN
#include "../../General/Driver/Database/Sqlite/ZDatabaseSqlite.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_RANDOM_LIBNOISE_BUILTIN
#include "../../General/Driver/Random/Libnoise/ZRandomLibnoisePerlin.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_POD_BUILTIN
#include "../../Core/Driver/Filter/Pod/ZFilterPod.h"
#endif

using namespace PaintsNow;

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_LZMA_BUILTIN
IArchive* Create7ZArchive(IStreamBase& streamBase, size_t length) {
	return new ZArchive7Z(streamBase, length);
}
#endif

class ZAudioDummy : public IAudio {
public:
	Buffer* CreateBuffer() override {
		static Buffer buffer;
		return &buffer;
	}

	void SetBufferStream(Buffer* buffer, Decoder& stream, bool online) override {}
	void DeleteBuffer(Buffer* buffer) override {}

	Source* CreateSource() override {
		static Source s;
		return &s;
	}

	void DeleteSource(Source* sourceHandle) override {}
	void SetSourcePosition(Source* sourceHandle, const Float3& position) override {}
	void SetSourceVolume(Source* sourceHandle, float volume) override {}
	TWrapper<size_t> SetSourceBuffer(Source* sourceHandle, const Buffer* buffer) override { return nullptr; }
	void SetListenerPosition(const Float3& position) override {}
	void Play(Source* sourceHandle) override {}
	void Pause(Source* sourceHandle) override {}
	void Rewind(Source* sourceHandle) override {}
	void Stop(Source* sourceHandle) override {}
};

class ZDecoderDummy : public NoFilter {};

class ZFrameDummy : public IFrame {
public:
	ZFrameDummy() {}
	void SetCallback(Callback* callback) override {}
	const Int2& GetWindowSize() const override {
		static Int2 size;
		return size;
	}

	void SetWindowSize(const Int2& size) override {}
	void SetWindowTitle(const String& title) override {}
	void EnableVerticalSynchronization(bool enable) override {}

	virtual void OnMouse(const EventMouse& mouse) {}
	virtual void OnKeyboard(const EventKeyboard& keyboard) {}
	virtual void OnRender() {}
	virtual void OnWindowSize(const Int2& newSize) {}
	void EnterMainLoop() override {}
	void ExitMainLoop() override {}
	void ShowCursor(CURSOR cursor) override {}
	void WarpCursor(const Int2& position) override {}
};

class ZRenderDummy : public IRender {
public:
	std::vector<String> EnumerateDevices() override { return std::vector<String>(); }

	struct_aligned(8) DeviceImpl : public IRender::Device {};
	Device* CreateDevice(const String& description) override {
		static DeviceImpl device;
		return &device;
	}
	Int2 GetDeviceResolution(Device* device) override { return Int2(640, 480); }
	void SetDeviceResolution(Device* device, const Int2& resolution) override {}
	bool PreDeviceFrame(Device* device) override { return true; }
	void PostDeviceFrame(Device* device) override { }
	void DeleteDevice(Device* device) override {}

	// Queue
	struct_aligned(8) QueueImpl : public IRender::Queue {};
	Device* GetQueueDevice(Queue* queue) override { return nullptr; }
	Queue* CreateQueue(Device* device, uint32_t flag) override {
		static QueueImpl q;
		return &q; // Make asserts happy
	}

	size_t GetProfile(Device* device, const String&) override { return 0; }
	void SubmitQueues(Queue** queue, uint32_t count, SubmitOption option) override {}
	void DeleteQueue(Queue* queue) override {}
	void FlushQueue(Queue* queue) override {}
	bool IsQueueEmpty(Queue* queue) override { return false; }

	// Resource
	struct_aligned(8) ResourceImpl : public IRender::Resource {
		Resource::Type type;
		Resource::Description* description;
	};

	Resource* CreateResource(Device* device, Resource::Type resourceType, Queue* optionalHostQueue) override {
		ResourceImpl* impl = new ResourceImpl();
		impl->type = resourceType;
		impl->description = nullptr;
		return impl;
	}

	void SetupBarrier(Queue* queue, Barrier* barrier) override {}
	Resource::Description* MapResource(Queue* queue, Resource* resource, uint32_t mapFlags) override {
		ResourceImpl* impl = static_cast<ResourceImpl*>(resource);
		assert(impl->description == nullptr);
		switch (impl->type) {
			case Resource::RESOURCE_UNKNOWN:
				assert(false);
				break;
			case Resource::RESOURCE_TEXTURE:
				return impl->description = new Resource::TextureDescription();
			case Resource::RESOURCE_BUFFER:
				return impl->description = new Resource::BufferDescription();
			case Resource::RESOURCE_SHADER:
				return impl->description = new Resource::ShaderDescription();
			case Resource::RESOURCE_RENDERSTATE:
				return impl->description = new Resource::RenderStateDescription();
			case Resource::RESOURCE_RENDERTARGET:
				return impl->description = new Resource::RenderTargetDescription();
			case Resource::RESOURCE_DRAWCALL:
				return impl->description = new Resource::DrawCallDescription();
			case Resource::RESOURCE_EVENT:
				return impl->description = new Resource::EventDescription();
			default:
				assert(false);
		}

		return nullptr;
	}

	const void* GetResourceDeviceHandle(IRender::Resource* resource) override {
		return nullptr;
	}

	void UnmapResource(Queue* queue, Resource* resource, uint32_t mapFlags) override {
		ResourceImpl* impl = static_cast<ResourceImpl*>(resource);
		IRender::Resource::Description* description = impl->description;
		impl->description = nullptr;
		switch (impl->type) {
			case Resource::RESOURCE_UNKNOWN:
				assert(false);
				break;
			case Resource::RESOURCE_TEXTURE:
				delete static_cast<Resource::TextureDescription*>(description);
				break;
			case Resource::RESOURCE_BUFFER:
				delete static_cast<Resource::BufferDescription*>(description);
				break;
			case Resource::RESOURCE_SHADER:
			{
				IRender::Resource::ShaderDescription* desc = static_cast<IRender::Resource::ShaderDescription*>(description);
				if (desc->compileCallback) {
					for (size_t i = 0; i < desc->entries.size(); i++) {
						std::pair<IRender::Resource::ShaderDescription::Stage, IShader*>& p = desc->entries[i];
						desc->compileCallback(impl, *desc, p.first, "", "");
					}

					desc->compileCallback(impl, *desc, IRender::Resource::ShaderDescription::END, "", "");
				}

				delete desc;
				break;
			}
			case Resource::RESOURCE_RENDERSTATE:
				delete static_cast<Resource::RenderStateDescription*>(description);
				break;
			case Resource::RESOURCE_RENDERTARGET:
				delete static_cast<Resource::RenderTargetDescription*>(description);
				break;
			case Resource::RESOURCE_DRAWCALL:
				delete static_cast<Resource::DrawCallDescription*>(description);
				break;
			case Resource::RESOURCE_EVENT:
			{
				IRender::Resource::EventDescription* desc = static_cast<IRender::Resource::EventDescription*>(description);
				description = nullptr;
				if (desc->eventCallback) {
					desc->eventCallback(*this, queue, impl, true);
				}

				delete desc;
				break;
			}
			default:
				assert(false);
				break;
		}
	}

	void ExecuteResource(Queue* queue, Resource* resource) override {}
	void DeleteResource(Queue* queue, Resource* resource) override {
		ResourceImpl* impl = static_cast<ResourceImpl*>(resource);
		assert(impl->description == nullptr);
		delete impl;
	}

	void SetResourceNote(Resource* lhs, const String& note) override {}
};

void Loader::SetFactory(TWrapper<IDevice*>& ptr, const String& key, const std::map<String, CmdLine::Option>& factoryMap) {
	const std::map<String, CmdLine::Option>::const_iterator p = factoryMap.find(key);
	if (p != factoryMap.end()) {
		const String& impl = p->second.name;
		const std::list<Config::Entry>& entries = config.GetEntry(key);
		for (std::list<Config::Entry>::const_iterator q = entries.begin(); q != entries.end(); ++q) {
			// printf("Scanning [%s] = %s (%s)\n", key.c_str(), (*q).name.c_str(), param.c_str());
			if ((ptr == (*q).factoryBase && impl.empty()) || (*q).name == impl) {
				ptr = (*q).factoryBase;
				break;
			}
		}
	}

	if (!ptr) {
		fprintf(stderr, "Couldn't find factory for %s\n", key.c_str());
		exit(-1);
	}
}

template <class T>
class DelayedReference {
public:
	DelayedReference(T*& p) : ptr(p) {}

	operator T& () const {
		return *ptr;
	}

	T*& ptr;
};

template <class T>
class DelayedValue {
public:
	DelayedValue(T& p) : value(p) {}

	operator T& () const {
		return const_cast<T&>(value);
	}

	T& value;
};

Config& Loader::GetConfig() {
	return config;
}

LeavesFlute*& Loader::GetLeavesFluteReference() {
	return leavesFlute;
}

void Loader::Run(const CmdLine& cmdLine) {
	// Load necessary modules
	const std::list<CmdLine::Option>& modules = cmdLine.GetModuleList();

	bool nogui = true;
	bool vsync = true;
	bool isVulkan = true;
	bool enableModuleLog = false;
	const std::map<String, CmdLine::Option>& configMap = cmdLine.GetConfigMap();
	String entry = "Entry";
	uint32_t threadCount;
#ifdef _WIN32
	// full speed
	SYSTEM_INFO systemInfo;
	::GetSystemInfo(&systemInfo);
	threadCount = (int32_t)systemInfo.dwNumberOfProcessors;
#else
	threadCount = std::thread::hardware_concurrency();
#endif

	String mount;
	uint32_t warpCount = 8; 
	const uint32_t maxWarpCount = 1 << WarpTiny::WARP_BITS;
	const uint32_t maxThreadCount = maxWarpCount >> 1;
	long threadBalancer = 8;
	DelayedReference<IThread> delayThread(thread);

	for (std::map<String, CmdLine::Option>::const_iterator p = configMap.begin(); p != configMap.end(); ++p) {
		if ((*p).first == "Graphic") {
			nogui = (*p).second.name != "true";
		} else if ((*p).first == "VSync") {
			vsync = (*p).second.name == "true";
		} else if ((*p).first == "Log") {
			enableModuleLog = (*p).second.name == "true";
		} else if ((*p).first == "Entry") {
			entry = (*p).second.name;
		} else if ((*p).first == "Mount") {
			mount = (*p).second.name;
		} else if ((*p).first == "Balancer") {
			threadBalancer = (int32_t)verify_cast<int32_t>(atoi((*p).second.name.c_str()));
		} else if ((*p).first == "Warp") {
			// According to Unit::WARP_INDEX
			warpCount = Math::Min(maxWarpCount, (uint32_t)verify_cast<uint32_t>(atoi((*p).second.name.c_str())));
		} else if ((*p).first == "Thread") {
			int32_t expectedThreadCount = (int32_t)verify_cast<int32_t>(atoi((*p).second.name.c_str()));
			if (expectedThreadCount <= 0) {
				threadCount += expectedThreadCount;
			} else {
				threadCount = expectedThreadCount;
			}
		}
	}

	// warpCount = Math::Max(warpCount, threadCount);

	const std::map<String, CmdLine::Option>& factoryMap = cmdLine.GetFactoryMap();

	if (enableModuleLog) {
		for (std::map<String, CmdLine::Option>::const_iterator i = factoryMap.begin(); i != factoryMap.end(); ++i) {
			printf("Load config [%s] = %s Param: %s\n", (*i).first.c_str(), (*i).second.name.c_str(), (*i).second.param.c_str());
			assert(factoryMap.find(i->first) != factoryMap.end());
			// printf("Compare result: %d\n", strcmp(i->first.c_str(), "IRender"));
		}
	}

	// assert(factoryMap.find(String("IArchive")) != factoryMap.end());
	// assert(factoryMap.find(String("IRender")) != factoryMap.end());

	// Run default settings
	TWrapper<IThread*> threadFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_THREAD_PTHREAD
	threadFactory = WrapFactory(UniqueType<ZThreadPthread>());
	config.RegisterFactory("IThread", "ZThreadPthread", threadFactory);
#endif

	TWrapper<IFrame*> frameFactory = WrapFactory(UniqueType<ZFrameDummy>());
	config.RegisterFactory("IFrame", "ZFrameDummy", frameFactory);

	TWrapper<IRender*> renderFactory = WrapFactory(UniqueType<ZRenderDummy>());
	config.RegisterFactory("IRender", "ZRenderDummy", renderFactory);

	TWrapper<IAudio*> audioFactory = WrapFactory(UniqueType<ZAudioDummy>());
	config.RegisterFactory("IAudio", "ZAudioDummy", audioFactory);

	TWrapper<IFilterBase*> decoderFactory = WrapFactory(UniqueType<ZDecoderDummy>());
	config.RegisterFactory("IFilterBase::Audio", "ZDecoderDummy", decoderFactory);

#if (!defined(CMAKE_PAINTSNOW) || ADD_RENDER_VULKAN) && (!defined(_MSC_VER) || _MSC_VER > 1200)
	GLFWwindow* window = nullptr;
	DelayedValue<GLFWwindow*> delayWindow(window);
	if (!nogui) {
		renderFactory = WrapFactory(UniqueType<ZRenderVulkan>(), delayWindow);
		config.RegisterFactory("IRender", "ZRenderVulkan", renderFactory);
	}
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_RENDER_OPENGL
	static TWrapper<IRender*> glFactory = WrapFactory(UniqueType<ZRenderOpenGL>());
	if (!nogui) {
		renderFactory = glFactory;
		config.RegisterFactory("IRender", "ZRenderOpenGL", renderFactory);
	}
#endif

#if !defined(CMAKE_PAINTSNOW) || (ADD_FRAME_GLFW && !defined(CMAKE_ANDROID))
	DelayedValue<bool> delayIsVulkan(isVulkan);
#if (!defined(CMAKE_PAINTSNOW) || ADD_RENDER_VULKAN) && (!defined(_MSC_VER) || _MSC_VER > 1200)
		GLFWwindow** ptr = &window;
#else
		GLFWwindow** ptr = nullptr;
#endif
	DelayedValue<GLFWwindow**> delayWindowPtr(ptr);
	if (!nogui) {
		frameFactory = WrapFactory(UniqueType<ZFrameGLFW>(), delayWindowPtr, delayIsVulkan);
		config.RegisterFactory("IFrame", "ZFrameGLFW", frameFactory);
	}
#endif

	TWrapper<IArchive*, IStreamBase&, size_t> subArchiveFactory;

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_LZMA_BUILTIN
	subArchiveFactory = Create7ZArchive;
	config.RegisterFactory("IArchive::Mount", "ZArchive7Z", *(const TWrapper<IDevice*>*)&subArchiveFactory);
#endif

	TWrapper<IScript*> scriptFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_SCRIPT_LUA_BUILTIN
	scriptFactory = WrapFactory(UniqueType<ZScriptLua>(), delayThread);
	config.RegisterFactory("IScript", "ZScriptLua", scriptFactory);
#endif

	TWrapper<INetwork*> standardNetworkFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_NETWORK_LIBEVENT
	standardNetworkFactory = WrapFactory(UniqueType<ZNetworkLibEvent>(), delayThread);
	config.RegisterFactory("INetwork::Standard", "ZNetworkLibEvent", standardNetworkFactory);
#endif

	TWrapper<INetwork*> quickNetworkFactory;

#if !defined(CMAKE_PAINTSNOW) || ADD_NETWORK_KCP
	quickNetworkFactory = WrapFactory(UniqueType<ZNetworkKCP>(), delayThread);
	config.RegisterFactory("INetwork::Quick", "ZNetworkKCP", quickNetworkFactory);
#endif


	TWrapper<IImage*> imageFactory;

#if !defined(CMAKE_PAINTSNOW) || ADD_IMAGE_FREEIMAGE
	imageFactory = WrapFactory(UniqueType<ZImageFreeImage>());
	config.RegisterFactory("IImage", "ZImageFreeImage", imageFactory);
#endif

	TWrapper<IRandom*> randomFactory;

#if !defined(CMAKE_PAINTSNOW) || ADD_RANDOM_LIBNOISE_BUILTIN
	randomFactory = WrapFactory(UniqueType<ZRandomLibnoisePerlin>());
	config.RegisterFactory("IRandom", "ZRandomLibnoisePerlin", randomFactory);
#endif

	TWrapper<IFontBase*> fontFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_FONT_FREETYPE
	fontFactory = WrapFactory(UniqueType<ZFontFreetype>());
	config.RegisterFactory("IFontBase", "ZFontFreetype", fontFactory);
#endif

	TWrapper<IFilterBase*> audioFilterFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_LAME
	audioFilterFactory = WrapFactory(UniqueType<ZFilterLAME>());
	config.RegisterFactory("IFilterBase::Audio", "ZDecoderLAME", audioFilterFactory);
#endif

	TWrapper<IDatabase*> databaseFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_DATABASE_SQLITE3_BUILTIN
	databaseFactory = WrapFactory(UniqueType<ZDatabaseSqlite>());
	config.RegisterFactory("IDatabase", "ZDatabaseSqlite", databaseFactory);
#endif

	TWrapper<ITimer*> timerFactory;
#if (defined(_WIN32) || defined(WIN32)) && ((!defined(CMAKE_PAINTSNOW) || ADD_TIMER_TIMERQUEUE_BUILTIN))
	timerFactory = WrapFactory(UniqueType<ZWinTimerQueue>());
	config.RegisterFactory("ITimer", "ZWinTimerQueue", timerFactory);
#endif

#if !(defined(_WIN32) || defined(WIN32)) && (!defined(CMAKE_PAINTSNOW) || ADD_TIMER_POSIX_BUILTIN)
	timerFactory = WrapFactory(UniqueType<ZPosixTimer>());
	config.RegisterFactory("ITimer", "ZPosixTimer", timerFactory);
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_OPENAL
	audioFactory = WrapFactory(UniqueType<ZAudioOpenAL>());
	config.RegisterFactory("IAudio", "ZAudioOpenAL", audioFactory);
#endif

	TWrapper<IArchive*> archiveFactory;
	TWrapper<IArchive*> rootArchiveFactory;

#if !defined(CMAKE_PAINTSNOW) || ADD_ARCHIVE_DIRENT_BUILTIN
	rootArchiveFactory = WrapFactory(UniqueType<ZArchiveDirent>(), delayThread);
	config.RegisterFactory("IArchive::Root", "ZArchiveDirent", rootArchiveFactory);
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_ARCHIVE_VIRTUAL_BUILTIN
	archiveFactory = WrapFactory(UniqueType<ZArchiveVirtual>());
	config.RegisterFactory("IArchive", "ZArchiveVirtual", archiveFactory);
#endif

	TWrapper<IFilterBase*> assetFilterFactory;

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_POD_BUILTIN
	assetFilterFactory = WrapFactory(UniqueType<ZFilterPod>());
	config.RegisterFactory("IFilterBase::Asset", "ZFilterPod", assetFilterFactory);
#else
	assetFilterFactory = WrapFactory(UniqueType<NoFilter>());
	config.RegisterFactory("IFilterBase::Asset", "NoFilter", assetFilterFactory);
#endif

	SetFactory((TWrapper<IDevice*>&)renderFactory, "IRender", factoryMap);
	SetFactory((TWrapper<IDevice*>&)frameFactory, "IFrame", factoryMap);
	SetFactory((TWrapper<IDevice*>&)threadFactory, "IThread", factoryMap);
	SetFactory((TWrapper<IDevice*>&)audioFactory, "IAudio", factoryMap);
	SetFactory((TWrapper<IDevice*>&)rootArchiveFactory, "IArchive::Root", factoryMap);
	SetFactory((TWrapper<IDevice*>&)archiveFactory, "IArchive", factoryMap);
	SetFactory((TWrapper<IDevice*>&)subArchiveFactory, "IArchive::Mount", factoryMap);
	SetFactory((TWrapper<IDevice*>&)scriptFactory, "IScript", factoryMap);
	SetFactory((TWrapper<IDevice*>&)standardNetworkFactory, "INetwork::Standard", factoryMap);
	SetFactory((TWrapper<IDevice*>&)quickNetworkFactory, "INetwork::Quick", factoryMap);
	SetFactory((TWrapper<IDevice*>&)randomFactory, "IRandom", factoryMap);
	SetFactory((TWrapper<IDevice*>&)timerFactory, "ITimer", factoryMap);
	SetFactory((TWrapper<IDevice*>&)imageFactory, "IImage", factoryMap);
	SetFactory((TWrapper<IDevice*>&)assetFilterFactory, "IFilterBase::Asset", factoryMap);
	SetFactory((TWrapper<IDevice*>&)fontFactory, "IFontBase", factoryMap);
	SetFactory((TWrapper<IDevice*>&)audioFilterFactory, "IFilterBase::Audio", factoryMap);
	SetFactory((TWrapper<IDevice*>&)databaseFactory, "IDatabase", factoryMap);

	if (!nogui) {
		// Must have render factory in GUI mode.
		assert(renderFactory);

#if !defined(CMAKE_PAINTSNOW) || (ADD_RENDER_OPENGL && (ADD_FRAME_GLFW && !defined(CMAKE_ANDROID)))
		// initialize for GLFW
		if (renderFactory.proxy.p == glFactory.proxy.p) {
			isVulkan = false; // fallback to opengl
		}
#endif
	}

	thread = threadFactory(); // precreate thread
	{
		IScript* script = scriptFactory();
		ITimer* timer = timerFactory();
		IImage* image = imageFactory();
		INetwork* standardNetwork = standardNetworkFactory();
		INetwork* quickNetwork = quickNetworkFactory();
		IAudio* audio = audioFactory();
		IArchive* rootArchive = rootArchiveFactory();
		IArchive* archive = archiveFactory();
		IRandom* random = randomFactory();
		IDatabase* database = databaseFactory();
		IFilterBase* assetFilter = assetFilterFactory();
		IFilterBase* audioFilter = audioFilterFactory();
		IFontBase* font = fontFactory();

		// Bypass GLFW/LibEvent compatibility issue.
		// Use LibEvent once before GLFW initialized to prevent memory leaks from LibEvent socket pair generation.
		ITunnel::Dispatcher* dispatcher = standardNetwork->OpenDispatcher();
		standardNetwork->CloseDispatcher(dispatcher);

		IFrame* frame = frameFactory();
		IRender* render = renderFactory();

		this->frame = frame;

		// Mount root archives
		archive->Mount("", rootArchive);

		{
			Interfaces interfaces(*archive, *audio, *database, *assetFilter, *audioFilter, *font, *frame, *image, *standardNetwork, *quickNetwork, *random, *render, *script, *thread, *timer);
			LeavesFlute leavesFlute(nogui, interfaces, subArchiveFactory, mount, threadCount, warpCount, threadBalancer);
			leavesFlute.OverrideConsoleProc(consoleHandler);
			leavesFlute.OverrideSetupProc(setupHandler);
			this->leavesFlute = &leavesFlute;

			std::vector<String> paramList;
			std::map<String, CmdLine::Option>::const_iterator scriptParam = factoryMap.find("IScript");
			if (scriptParam != factoryMap.end()) {
				paramList = Split(scriptParam->second.param);
			}

			leavesFlute.Setup();
			leavesFlute.Execute(entry, paramList);

			std::map<String, CmdLine::Option>::const_iterator quit = configMap.find("Quit");
			if (quit == configMap.end() || (*quit).second.name != "true") {
				if (nogui) {
					leavesFlute.EnterStdinLoop();
				} else {
					frame->EnableVerticalSynchronization(vsync);
					leavesFlute.EnterMainLoop();
				}
			}
		}

		render->ReleaseDevice();
		frame->ReleaseDevice();
		font->ReleaseDevice();
		audioFilter->ReleaseDevice();
		assetFilter->ReleaseDevice();
		database->ReleaseDevice();
		random->ReleaseDevice();
		archive->ReleaseDevice();
		rootArchive->ReleaseDevice();
		audio->ReleaseDevice();
		quickNetwork->ReleaseDevice();
		standardNetwork->ReleaseDevice();
		image->ReleaseDevice();
		timer->ReleaseDevice();
		script->ReleaseDevice();
	}

	delete thread;
}


Loader::Loader() : leavesFlute(nullptr) {}
Loader::~Loader() {}

