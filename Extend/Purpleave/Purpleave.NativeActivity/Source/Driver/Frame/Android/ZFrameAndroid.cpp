#include "ZFrameAndroid.h"
#include <jni.h>
#include <malloc.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <android/log.h>

using namespace PaintsNow;

template <typename T, T instance, typename C, typename... Args>
auto WrapContextCallback(void (C::*)(Args...)) -> void (*)(ANativeActivity*, Args...) {
	return +[](ANativeActivity* activity, Args... args) {
		(reinterpret_cast<C*>(activity->instance)->*instance)(std::move(args)...);
	};
}

template <typename T, T instance, typename C, typename R, typename... Args>
auto WrapContextCallback(R (C::*)(Args...)) -> R (*)(ANativeActivity*, Args...) {
	return +[](ANativeActivity* activity, Args... args) {
		return (reinterpret_cast<C*>(activity->instance)->*instance)(std::move(args)...);
	};
}

#define WRAP_CONTEXT_CALLBACK(f) \
	WrapContextCallback<decltype(f), f>(f)

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "threaded_app", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "threaded_app", __VA_ARGS__))

/* For debug builds, always enable the debug traces in this library */
#ifndef NDEBUG
#  define LOGV(...)  ((void)__android_log_print(ANDROID_LOG_VERBOSE, "threaded_app", __VA_ARGS__))
#else
#  define LOGV(...)  ((void)0)
#endif

void ZFrameAndroid::FreeSavedState() {
	pthread_mutex_lock(&mutex);
	if (savedState != nullptr) {
		free(savedState);
		savedState = nullptr;
		savedStateSize = 0;
	}
	pthread_mutex_unlock(&mutex);
}

int8_t ZFrameAndroid::ReadCommand() {
	int8_t cmd;
	if (read(msgread, &cmd, sizeof(cmd)) == sizeof(cmd)) {
		switch (cmd) {
		case APP_CMD_SAVE_STATE:
			FreeSavedState();
			break;
		}
		return cmd;
	} else {
		LOGE("No data on command pipe!");
	}
	return -1;
}

void ZFrameAndroid::PrintCurrentConfig() {
	char lang[2], country[2];
	AConfiguration_getLanguage(config, lang);
	AConfiguration_getCountry(config, country);

	LOGV("Config: mcc=%d mnc=%d lang=%c%c cnt=%c%c orien=%d touch=%d dens=%d "
		"keys=%d nav=%d keysHid=%d navHid=%d sdk=%d size=%d long=%d "
		"modetype=%d modenight=%d",
		AConfiguration_getMcc(config),
		AConfiguration_getMnc(config),
		lang[0], lang[1], country[0], country[1],
		AConfiguration_getOrientation(config),
		AConfiguration_getTouchscreen(config),
		AConfiguration_getDensity(config),
		AConfiguration_getKeyboard(config),
		AConfiguration_getNavigation(config),
		AConfiguration_getKeysHidden(config),
		AConfiguration_getNavHidden(config),
		AConfiguration_getSdkVersion(config),
		AConfiguration_getScreenSize(config),
		AConfiguration_getScreenLong(config),
		AConfiguration_getUiModeType(config),
		AConfiguration_getUiModeNight(config));
}

void ZFrameAndroid::PreExecuteCommand(int8_t cmd) {
	switch (cmd) {
	case APP_CMD_INPUT_CHANGED:
		LOGV("APP_CMD_INPUT_CHANGED\n");
		pthread_mutex_lock(&mutex);
		if (inputQueue != nullptr) {
			AInputQueue_detachLooper(inputQueue);
		}
		inputQueue = pendingInputQueue;
		if (inputQueue != nullptr) {
			LOGV("Attaching input queue to looper");
			AInputQueue_attachLooper(inputQueue,
				looper, LOOPER_ID_INPUT, nullptr,
				&inputPollSource);
		}
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
		break;

	case APP_CMD_INIT_WINDOW:
		LOGV("APP_CMD_INIT_WINDOW\n");
		pthread_mutex_lock(&mutex);
		window = pendingWindow;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
		break;

	case APP_CMD_TERM_WINDOW:
		LOGV("APP_CMD_TERM_WINDOW\n");
		pthread_cond_broadcast(&cond);
		break;

	case APP_CMD_RESUME:
	case APP_CMD_START:
	case APP_CMD_PAUSE:
	case APP_CMD_STOP:
		LOGV("activityState=%d\n", cmd);
		pthread_mutex_lock(&mutex);
		activityState = cmd;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
		break;

	case APP_CMD_CONFIG_CHANGED:
		LOGV("APP_CMD_CONFIG_CHANGED\n");
		AConfiguration_fromAssetManager(config,
			activity->assetManager);
		PrintCurrentConfig();
		break;

	case APP_CMD_DESTROY:
		LOGV("APP_CMD_DESTROY\n");
		destroyRequested = 1;
		break;
	}
}

void ZFrameAndroid::PostExecuteCommand(int8_t cmd) {
	switch (cmd) {
	case APP_CMD_TERM_WINDOW:
		LOGV("APP_CMD_TERM_WINDOW\n");
		pthread_mutex_lock(&mutex);
		window = nullptr;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
		break;

	case APP_CMD_SAVE_STATE:
		LOGV("APP_CMD_SAVE_STATE\n");
		pthread_mutex_lock(&mutex);
		stateSaved = 1;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
		break;

	case APP_CMD_RESUME:
		FreeSavedState();
		break;
	}
}

void ZFrameAndroid::ProcessInput(android_poll_source* source) {
	AInputEvent* event = nullptr;
	while (AInputQueue_getEvent(inputQueue, &event) >= 0) {
		LOGV("New input event: type=%d\n", AInputEvent_getType(event));
		if (AInputQueue_preDispatchEvent(inputQueue, event)) {
			continue;
		}

		int32_t handled = 0;

		HandleInput(event);
		AInputQueue_finishEvent(inputQueue, event, handled);
	}
}

void ZFrameAndroid::ProcessCommand(android_poll_source* source) {
	int8_t cmd = ReadCommand();
	PreExecuteCommand(cmd);
	HandleCommand(cmd);
	PostExecuteCommand(cmd);
}

void ZFrameAndroid::WriteCommand(int8_t cmd) {
	if (write(msgwrite, &cmd, sizeof(cmd)) != sizeof(cmd)) {
		LOGE("Failure writing android_app cmd: %s\n", strerror(errno));
	}
}

void ZFrameAndroid::SetInput(AInputQueue* inputQueue) {
	pthread_mutex_lock(&mutex);
	pendingInputQueue = inputQueue;
	WriteCommand(APP_CMD_INPUT_CHANGED);
	while (inputQueue != pendingInputQueue) {
		pthread_cond_wait(&cond, &mutex);
	}
	pthread_mutex_unlock(&mutex);
}

void ZFrameAndroid::SetWindow(ANativeWindow* window) {
	pthread_mutex_lock(&mutex);
	if (pendingWindow != nullptr) {
		WriteCommand(APP_CMD_TERM_WINDOW);
	}

	pendingWindow = window;
	if (window != nullptr) {
		WriteCommand(APP_CMD_INIT_WINDOW);
	}

	while (window != pendingWindow) {
		pthread_cond_wait(&cond, &mutex);
	}
	pthread_mutex_unlock(&mutex);
}

void ZFrameAndroid::SetActivityState(int8_t cmd) {
	pthread_mutex_lock(&mutex);
	WriteCommand(cmd);
	while (activityState != cmd) {
		pthread_cond_wait(&cond, &mutex);
	}
	pthread_mutex_unlock(&mutex);
}

void ZFrameAndroid::OnDestroy() {
	LOGV("Destroy: %p\n", activity);
	pthread_mutex_lock(&mutex);
	WriteCommand(APP_CMD_DESTROY);
	while (!destroyed) {
		pthread_cond_wait(&cond, &mutex);
	}
	pthread_mutex_unlock(&mutex);

	close(msgread);
	close(msgwrite);
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
	// free(android_app);
}

void ZFrameAndroid::OnStart() {
	LOGV("Start: %p\n", activity);
	SetActivityState(APP_CMD_START);
}

void ZFrameAndroid::OnResume() {
	LOGV("Resume: %p\n", activity);
	SetActivityState(APP_CMD_RESUME);
}

void* ZFrameAndroid::OnSaveInstanceState(size_t* outLen) {
	void* savedState = nullptr;

	LOGV("SaveInstanceState: %p\n", activity);
	pthread_mutex_lock(&mutex);
	stateSaved = 0;
	WriteCommand(APP_CMD_SAVE_STATE);
	while (!stateSaved) {
		pthread_cond_wait(&cond, &mutex);
	}

	if (savedState != nullptr) {
		savedState = savedState;
		*outLen = savedStateSize;
		savedState = nullptr;
		savedStateSize = 0;
	}

	pthread_mutex_unlock(&mutex);

	return savedState;
}

void ZFrameAndroid::OnPause() {
	LOGV("Pause: %p\n", activity);
	SetActivityState(APP_CMD_PAUSE);
}

void ZFrameAndroid::OnStop() {
	LOGV("Stop: %p\n", activity);
	SetActivityState(APP_CMD_STOP);
}

void ZFrameAndroid::OnConfigurationChanged() {
	struct android_app* android_app = (struct android_app*)activity->instance;
	LOGV("ConfigurationChanged: %p\n", activity);
	WriteCommand(APP_CMD_CONFIG_CHANGED);
}

void ZFrameAndroid::OnLowMemory() {
	LOGV("LowMemory: %p\n", activity);
	WriteCommand(APP_CMD_LOW_MEMORY);
}

void ZFrameAndroid::OnWindowFocusChanged(int focused) {
	LOGV("WindowFocusChanged: %p -- %d\n", activity, focused);
	WriteCommand(focused ? APP_CMD_GAINED_FOCUS : APP_CMD_LOST_FOCUS);
}

void ZFrameAndroid::OnNativeWindowCreated(ANativeWindow* window) {
	LOGV("NativeWindowCreated: %p -- %p\n", activity, window);
	SetWindow(window);
}

void ZFrameAndroid::OnNativeWindowDestroyed(ANativeWindow* window) {
	LOGV("NativeWindowDestroyed: %p -- %p\n", activity, window);
	SetWindow(nullptr);
}

void ZFrameAndroid::OnInputQueueCreated(AInputQueue* queue) {
	LOGV("InputQueueCreated: %p -- %p\n", activity, queue);
	SetInput(queue);
}

void ZFrameAndroid::OnInputQueueDestroyed(AInputQueue* queue) {
	LOGV("InputQueueDestroyed: %p -- %p\n", activity, queue);
	SetInput(nullptr);
}

int ZFrameAndroid::InitDisplay() {
	// initialize OpenGL ES and EGL

	/*
	* Here specify the attributes of the desired configuration.
	* Below, we select an EGLConfig with at least 8 bits per color
	* component compatible with on-screen windows
	*/
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};
	EGLint w, h, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);

	/* Here, the application chooses the configuration it desires. In this
	* sample, we have a very simplified selection process, where we pick
	* the first EGLConfig that matches our criteria */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	* guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	* As soon as we picked a EGLConfig, we can safely reconfigure the
	* ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, window, nullptr);
	context = eglCreateContext(display, config, nullptr, nullptr);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGE("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	this->display = display;
	this->context = context;
	this->surface = surface;
	this->windowSize = Int2(w, h);
	this->state.angle = 0;

	callback->OnWindowSize(IFrame::EventSize(windowSize));

	// Initialize GL state.
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	// glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);

	return 0;
}

void ZFrameAndroid::DrawFrame() {
	if (display == nullptr) {
		// No display.
		return;
	}

	// Just fill the screen with a color.
	glClearColor(((float)state.x) / windowSize.x(), state.angle,
		((float)state.y) / windowSize.y(), 1);
	glClear(GL_COLOR_BUFFER_BIT);

	eglSwapBuffers(display, surface);
}

void ZFrameAndroid::TerminateDisplay() {
	if (display != EGL_NO_DISPLAY) {
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (context != EGL_NO_CONTEXT) {
			eglDestroyContext(display, context);
		}
		if (surface != EGL_NO_SURFACE) {
			eglDestroySurface(display, surface);
		}
		eglTerminate(display);
	}
	animating = 0;
	display = EGL_NO_DISPLAY;
	context = EGL_NO_CONTEXT;
	surface = EGL_NO_SURFACE;
}

/**
* Process the next input event.
*/
int32_t ZFrameAndroid::HandleInput(AInputEvent* event) {
	int32_t type = AInputEvent_getType(event);
	int32_t source = AInputEvent_getSource(event);
	int32_t action = AMotionEvent_getAction(event);

	switch (type) {
	case AINPUT_EVENT_TYPE_KEY:
	{
		int32_t action = AKeyEvent_getAction(event);
		int32_t keyCode = AKeyEvent_getKeyCode(event);

		IFrame::EventKeyboard keyboard;
		keyboard.keyCode = 0;

		if (action == AKEY_STATE_UP) {
			keyboard.keyCode |= IFrame::EventKeyboard::KEY_POP;
		}

		if (keyCode >= AKEYCODE_0 && keyCode <= AKEYCODE_9) {
			keyboard.keyCode = '0' + keyCode - AKEYCODE_0;
		} else if (keyCode >= AKEYCODE_A && keyCode <= AKEYCODE_Z) {
			keyboard.keyCode = 'A' + keyCode - AKEYCODE_A;
		} else if (keyCode >= AKEYCODE_F1 && keyCode <= AKEYCODE_F12) {
			keyboard.keyCode = IFrame::EventKeyboard::KEY_F1 + keyCode - AKEYCODE_F1;
		} else if (keyCode >= AKEYCODE_NUMPAD_0 && keyCode <= AKEYCODE_NUMPAD_9) {
			keyboard.keyCode = IFrame::EventKeyboard::KEY_KP_0 + keyCode - AKEYCODE_NUMPAD_0;
		} else {
			switch (keyCode) {
#define MAP_KEY(from, to) case AKEYCODE_##from: keyboard.keyCode = IFrame::EventKeyboard::KEY_##to; break
				MAP_KEY(CTRL_LEFT, LEFT_CONTROL);
				MAP_KEY(CTRL_RIGHT, RIGHT_CONTROL);
				MAP_KEY(ALT_LEFT, LEFT_ALT);
				MAP_KEY(ALT_RIGHT, RIGHT_ALT);
				MAP_KEY(SHIFT_LEFT, LEFT_SHIFT);
				MAP_KEY(SHIFT_RIGHT, RIGHT_SHIFT);
				MAP_KEY(ENTER, ENTER);
				MAP_KEY(TAB, TAB);
				MAP_KEY(BACK, BACKSPACE);
				MAP_KEY(INSERT, INSERT);
				MAP_KEY(DEL, DELETE);
				MAP_KEY(HOME, HOME);
				MAP_KEY(DPAD_UP, UP);
				MAP_KEY(DPAD_LEFT, LEFT);
				MAP_KEY(DPAD_RIGHT, RIGHT);
				MAP_KEY(DPAD_DOWN, DOWN);
				MAP_KEY(PAGE_UP, PAGE_UP);
				MAP_KEY(PAGE_DOWN, PAGE_DOWN);
				MAP_KEY(NUMPAD_DIVIDE, KP_DIVIDE);
				MAP_KEY(NUMPAD_MULTIPLY, KP_MULTIPLY);
				MAP_KEY(NUMPAD_SUBTRACT, KP_SUBTRACT);
				MAP_KEY(NUMPAD_ADD, KP_ADD);
				MAP_KEY(NUMPAD_ENTER, KP_ENTER);
				MAP_KEY(MENU, MENU);
			}
		}

		callback->OnKeyboard(keyboard);
		break;
	}
	case AINPUT_EVENT_TYPE_MOTION:
	{
		size_t count = AMotionEvent_getPointerCount(event);
		for (size_t i = 0; i < count; i++) {
			float x = AMotionEvent_getX(event, i);
			float y = AMotionEvent_getY(event, i);

			switch (source) {
			case AINPUT_SOURCE_TOUCHSCREEN:
			{
				switch (action) {
				case AMOTION_EVENT_ACTION_MOVE:
					callback->OnMouse(IFrame::EventMouse(true, true, true, false, Short2(x, y), verify_cast<uint16_t>(i)));
					break;
				case AMOTION_EVENT_ACTION_DOWN:
					callback->OnMouse(IFrame::EventMouse(true, false, true, false, Short2(x, y), verify_cast<uint16_t>(i)));
					break;
				case AMOTION_EVENT_ACTION_UP:
					callback->OnMouse(IFrame::EventMouse(false, false, true, false, Short2(x, y), verify_cast<uint16_t>(i)));
					break;
				}
			}
			}
		}

		break;
	}
	}

	return 0;
}

void ZFrameAndroid::HandleCommand(int32_t cmd) {
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
		// The system has asked us to save our current state.  Do so.
		savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)savedState) = state;
		savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		// The window is being shown, get it ready.
		if (window != nullptr) {
			InitDisplay();
			DrawFrame();
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// The window is being hidden or closed, clean it up.
		TerminateDisplay();
		break;
	case APP_CMD_GAINED_FOCUS:
		// When our app gains focus, we start monitoring the accelerometer.
		if (accelerometerSensor != nullptr) {
			ASensorEventQueue_enableSensor(sensorEventQueue,
				accelerometerSensor);
			// We'd like to get 60 events per second (in microseconds).
			ASensorEventQueue_setEventRate(sensorEventQueue,
				accelerometerSensor, (1000L / 60) * 1000);
		}
		break;
	case APP_CMD_LOST_FOCUS:
		// When our app loses focus, we stop monitoring the accelerometer.
		// This is to avoid consuming battery while not being used.
		if (accelerometerSensor != nullptr) {
			ASensorEventQueue_disableSensor(sensorEventQueue,
				accelerometerSensor);
		}
		// Also stop animating.
		animating = 0;
		DrawFrame();
		break;
	}
}

void ZFrameAndroid::Main() {
	// Prepare to monitor accelerometer
	sensorManager = ASensorManager_getInstance();
	accelerometerSensor = ASensorManager_getDefaultSensor(sensorManager,
		ASENSOR_TYPE_ACCELEROMETER);
	sensorEventQueue = ASensorManager_createEventQueue(sensorManager,
		looper, LOOPER_ID_USER, nullptr, nullptr);

	if (savedState != nullptr) {
		// We are starting with a previous saved state; restore from it.
		state = *(struct saved_state*)savedState;
	}

	animating = 1;

	// loop waiting for stuff to do.

	while (1) {
		// Read all pending events.
		int ident;
		int events;
		android_poll_source* source;

		// If not animating, we will block forever waiting for events.
		// If animating, we loop until all events are read, then continue
		// to draw the next frame of animation.
		while ((ident = ALooper_pollAll(animating ? 0 : -1, nullptr, &events,
			(void**)&source)) >= 0) {

			// Process this event.
			if (source != nullptr) {
				void (ZFrameAndroid::*pfn)(android_poll_source*) = source->process;
				(this->*pfn)(source);
			}

			// If a sensor has data, process it now.
			if (ident == LOOPER_ID_USER) {
				if (accelerometerSensor != nullptr) {
					ASensorEvent event;
					while (ASensorEventQueue_getEvents(sensorEventQueue,
						&event, 1) > 0) {
						LOGI("accelerometer: x=%f y=%f z=%f",
							event.acceleration.x, event.acceleration.y,
							event.acceleration.z);
					}
				}
			}

			// Check if we are exiting.
			if (destroyRequested != 0) {
				TerminateDisplay();
				return;
			}
		}

		if (animating) {
			// Done with events; draw next animation frame.
			state.angle += .01f;
			if (state.angle > 1) {
				state.angle = 0;
			}

			// Drawing is throttled to the screen update rate, so there
			// is no need to do timing here.
			DrawFrame();
		}

		callback->OnRender();
	}
}

	
ZFrameAndroid::ZFrameAndroid(ANativeActivity* v, void* s, size_t ss) : activity(v), savedState(s), savedStateSize(ss) {
	LOGV("Creating: %p\n", activity);
	activity->callbacks->onDestroy = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnDestroy);
	activity->callbacks->onStart = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnStart);
	activity->callbacks->onResume = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnResume);
	activity->callbacks->onSaveInstanceState = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnSaveInstanceState);
	activity->callbacks->onPause = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnPause);
	activity->callbacks->onStop = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnStop);
	activity->callbacks->onConfigurationChanged = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnConfigurationChanged);
	activity->callbacks->onLowMemory = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnLowMemory);
	activity->callbacks->onWindowFocusChanged = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnWindowFocusChanged);
	activity->callbacks->onNativeWindowCreated = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnNativeWindowCreated);
	activity->callbacks->onNativeWindowDestroyed = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnNativeWindowDestroyed);
	activity->callbacks->onInputQueueCreated = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnInputQueueCreated);
	activity->callbacks->onInputQueueDestroyed = WRAP_CONTEXT_CALLBACK(&ZFrameAndroid::OnInputQueueDestroyed);

	activity->instance = this;
}

void ZFrameAndroid::SetCallback(Callback* cb) {
	callback = cb;
}

// logic size, not android native buffer size
const Int2& ZFrameAndroid::GetWindowSize() const {
	return windowSize;
}

void ZFrameAndroid::SetWindowSize(const Int2& size) {
	windowSize = size;
}

void ZFrameAndroid::SetWindowTitle(const String& title) {
	// not implemented
}

void ZFrameAndroid::EnableVerticalSynchronization(bool enable) {
	eglSwapInterval(display, enable ? 1 : 0);
}

void ZFrameAndroid::ShowCursor(CURSOR cursor) {
	// not implemented
}

void ZFrameAndroid::WarpCursor(const Int2& position) {
	// not implemented
}

void ZFrameAndroid::EnterMainLoop() {
	pthread_mutex_init(&mutex, nullptr);
	pthread_cond_init(&cond, nullptr);

	if (savedState != nullptr) {
		savedState = malloc(savedStateSize);
		savedStateSize = savedStateSize;
		memcpy(savedState, savedState, savedStateSize);
	}

	int msgpipe[2];
	if (pipe(msgpipe)) {
		LOGE("could not create pipe: %s", strerror(errno));
		return;
	}

	msgread = msgpipe[0];
	msgwrite = msgpipe[1];

	config = AConfiguration_new();
	AConfiguration_fromAssetManager(config, activity->assetManager);

	PrintCurrentConfig();

	cmdPollSource.id = LOOPER_ID_MAIN;
	cmdPollSource.process = &ZFrameAndroid::ProcessCommand;
	inputPollSource.id = LOOPER_ID_INPUT;
	inputPollSource.process = &ZFrameAndroid::ProcessInput;

	ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
	ALooper_addFd(looper, msgread, LOOPER_ID_MAIN, ALOOPER_EVENT_INPUT, nullptr,
		&cmdPollSource);
	this->looper = looper;

	pthread_mutex_lock(&mutex);
	running = 1;
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);

	Main();

	LOGV("android_app_destroy!");
	FreeSavedState();
	pthread_mutex_lock(&mutex);
	if (inputQueue != nullptr) {
		AInputQueue_detachLooper(inputQueue);
	}
	AConfiguration_delete(config);
	destroyed = 1;
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);

	// Can't touch android_app object after this.
}

void ZFrameAndroid::ExitMainLoop() {
	WriteCommand(APP_CMD_DESTROY);
}

void ZFrameAndroid::InvokeMain(const TWrapper<void>& callback) {
	pthread_t thread;

	struct Context {
		Context(const TWrapper<void>& w) : callback(w) {
			running.store(false, std::memory_order_release);
		}

		pthread_mutex_t mutex;
		pthread_cond_t cond;
		TWrapper<void> callback;
		std::atomic<bool> running;
	} context(callback);

	pthread_mutex_init(&context.mutex, nullptr);
	pthread_cond_init(&context.cond, nullptr);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread, &attr, +[](void* c) -> void*{
		Context* context = reinterpret_cast<Context*>(c);
		context->running.store(true, std::memory_order_release);
		pthread_cond_broadcast(&context->cond);
		context->callback();
		return nullptr;
	}, &context);

	// Wait for thread to start.
	pthread_mutex_lock(&context.mutex);
	while (!context.running.load(std::memory_order_acquire)) {
		pthread_cond_wait(&context.cond, &context.mutex);
	}
	pthread_mutex_unlock(&context.mutex);

	pthread_cond_destroy(&context.cond);
	pthread_mutex_destroy(&context.mutex);
}

// Compile for android

extern "C" int iswblank(
	wint_t c
) {
	assert(false);
	return isblank(c);
}

extern "C" void* aligned_alloc(size_t alignment, size_t size) {
	return memalign(alignment, size);
}

extern "C" size_t wcsrtombs(
	char* mbstr,
	const wchar_t** wcstr,
	size_t count,
	mbstate_t * mbstate
) {
	assert(false);
	return 0;
}

extern "C" size_t mbsrtowcs(wchar_t* dst, const char** src, size_t len, mbstate_t * ps) {
	assert(false);
	return 0;
}

extern "C" size_t wcsnrtombs(char* dest, const wchar_t** src,
	size_t nwc, size_t len, mbstate_t * ps) {
	assert(false);
	return 0;
}

extern "C" size_t mbsnrtowcs(wchar_t* dest, const char** src,
	size_t nms, size_t len, mbstate_t * ps) {
	assert(false);
	return 0;
}

extern "C" int mbtowc(
	wchar_t* wchar,
	const char* mbchar,
	size_t count
) {
	assert(false);
	return 0;
}

extern "C" long long wcstoll(
	const wchar_t* strSource,
	wchar_t** endptr,
	int base
) {
	assert(false);
	return 0;
}

extern "C" unsigned long long wcstoull(
	const wchar_t* strSource,
	wchar_t** endptr,
	int base
) {
	assert(false);
	return 0;
}

extern "C" float wcstof(const wchar_t* str, wchar_t** endptr) {
	assert(false);
	return 0;
}

extern "C" int accept4() {
	assert(false);
	return 0;
}

extern "C" int getrandom() {
	return rand();
}

extern "C" float strtof_l(
	const char* strSource,
	char** endptr,
	locale_t locale
)
{
	return strtof(strSource, endptr);
}

extern "C" double strtod_l(
	const char* strSource,
	char** endptr,
	locale_t locale
) {
	return strtod(strSource, endptr);
}
