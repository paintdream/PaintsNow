#include "Loader.h"
// #include <vld.h>
#include "../LeavesFlute/Platform.h"

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
	bool NextDeviceFrame(Device* device) override { return true; }
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

	// Resource
	struct_aligned(8) ResourceImpl : public IRender::Resource {};
	Resource* CreateResource(Device* device, Resource::Type resourceType) override {
		static ResourceImpl r;
		static int counter = 0;
		return &r + counter++;
	}
	void SetupBarrier(Queue* queue, Barrier* barrier) override {}
	const void* GetResourceDeviceHandle(Resource* resource) override { return nullptr; }
	void UploadResource(Queue* queue, Resource* resource, Resource::Description* description) override {}
	void RequestDownloadResource(Queue* queue, Resource* resource, Resource::Description* description) override {}
	void CompleteDownloadResource(Queue* queue, Resource* resource) override {}
	void ExecuteResource(Queue* queue, Resource* resource) override {}
	void DeleteResource(Queue* queue, Resource* resource) override {}
	void SetResourceNotation(Resource* lhs, const String& note) override {}
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
struct DelayedReference {
	DelayedReference(T*& p) : ptr(p) {}

	operator T& () const {
		return *ptr;
	}

	T*& ptr;
};

template <class T>
struct DelayedValue {
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
	uint32_t warpCount = 0; // let mythforest decide
	const uint32_t maxWarpCount = 1 << WarpTiny::WARP_BITS;
	const uint32_t maxThreadCount = maxWarpCount >> 1;
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

	TWrapper<IFrame*> frameFactoryDummy = WrapFactory(UniqueType<ZFrameDummy>());
	config.RegisterFactory("IFrame", "ZFrameDummy", frameFactoryDummy);

	frameFactory = frameFactoryDummy;

	TWrapper<IRender*> renderFactoryDummy = WrapFactory(UniqueType<ZRenderDummy>());
	config.RegisterFactory("IRender", "ZRenderDummy", renderFactoryDummy);

	renderFactory = renderFactoryDummy;

	TWrapper<IAudio*> audioFactoryDummy = WrapFactory(UniqueType<ZAudioDummy>());
	config.RegisterFactory("IAudio", "ZAudioDummy", audioFactoryDummy);

	audioFactory = audioFactoryDummy;

	TWrapper<IFilterBase*> decoderFactoryDummy = WrapFactory(UniqueType<ZDecoderDummy>());
	config.RegisterFactory("IFilterBase::Audio", "ZDecoderDummy", decoderFactoryDummy);

	audioFilterFactory = decoderFactoryDummy;

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

	TWrapper<INetwork*> networkFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_NETWORK_LIBEVENT
	networkFactory = WrapFactory(UniqueType<ZNetworkLibEvent>(), delayThread);
	config.RegisterFactory("INetwork", "ZNetworkLibEvent", networkFactory);
#endif

	tunnelFactory = networkFactory;

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

	TWrapper<IAudio*> audioFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_OPENAL
	audioFactory = WrapFactory(UniqueType<ZAudioOpenAL>());
	config.RegisterFactory("IAudio", "ZAudioOpenAL", audioFactory);
#endif

	TWrapper<IArchive*> archiveFactory;
	TWrapper<IArchive*> rootArchiveFactory;

#if !defined(CMAKE_PAINTSNOW) || ADD_ARCHIVE_DIRENT_BUILTIN
	rootArchiveFactory = WrapFactory(UniqueType<ZArchiveDirent>());
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
	SetFactory((TWrapper<IDevice*>&)networkFactory, "INetwork", factoryMap);
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
		frame = frameFactory();
		IScript* script = scriptFactory();
		IRender* render = renderFactory();
		ITimer* timer = timerFactory();
		IImage* image = imageFactory();
		INetwork* network = networkFactory();
		ITunnel* tunnel = tunnelFactory();
		IAudio* audio = audioFactory();
		IArchive* rootArchive = rootArchiveFactory();
		IArchive* archive = archiveFactory();
		IRandom* random = randomFactory();
		IDatabase* database = databaseFactory();
		IFilterBase* assetFilter = assetFilterFactory();
		IFilterBase* audioFilter = audioFilterFactory();
		IFontBase* font = fontFactory();

		// Mount root archives
		archive->Mount("", rootArchive);

		{
			Interfaces interfaces(*archive, *audio, *database, *assetFilter, *audioFilter, *font, *frame, *image, *network, *random, *render, *script, *thread, *timer, *tunnel);
			LeavesFlute leavesFlute(nogui, interfaces, subArchiveFactory, mount, threadCount, warpCount);
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

		font->ReleaseDevice();
		assetFilter->ReleaseDevice();
		audioFilter->ReleaseDevice();
		database->ReleaseDevice();
		random->ReleaseDevice();
		tunnel->ReleaseDevice();
		image->ReleaseDevice();
		network->ReleaseDevice();
		script->ReleaseDevice();
		timer->ReleaseDevice();
		render->ReleaseDevice();
		audio->ReleaseDevice();
		archive->ReleaseDevice();
		rootArchive->ReleaseDevice();

		frame->ReleaseDevice();
	}

	delete thread;
}

#ifdef _WIN32
#include <Windows.h>
#ifndef _DEBUG
extern "C" void _except_handler4_common() {}
#endif
#endif

Loader::Loader() : leavesFlute(nullptr) {
#ifdef _WIN32
	::CoInitialize(nullptr);
#endif
}

Loader::~Loader() {
#ifdef _WIN32
	::CoUninitialize();
#endif
}

