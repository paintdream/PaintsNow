// ZFrameAndroid.h
// PaintDream (paintdream@paintdream.com)
// 2021-05-01
//

#pragma once

#include "../../../../../../../Source/General/Interface/IFrame.h"
#include "../../../../../../../Source/Core/Template/TProxy.h"

/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <poll.h>
#include <pthread.h>
#include <sched.h>

#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>
#include <android/sensor.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

namespace PaintsNow {
	class ZFrameAndroid : public IFrame {
	public:
		ZFrameAndroid(ANativeActivity* activity, void* savedState, size_t savedStateSize);

		void SetCallback(Callback* callback) override;
		const Int2& GetWindowSize() const override;
		void SetWindowSize(const Int2& size) override;
		void SetWindowTitle(const String& title) override;
		void EnableVerticalSynchronization(bool enable) override;
		void ShowCursor(CURSOR cursor) override;
		void WarpCursor(const Int2& position) override;
		void EnterMainLoop() override;
		void ExitMainLoop() override;

		static void InvokeMain(const TWrapper<void>& wrapper);

	protected:
		Int2 windowSize;

	protected:
		enum {
			APP_CMD_INPUT_CHANGED,
			APP_CMD_INIT_WINDOW,
			APP_CMD_TERM_WINDOW,
			APP_CMD_WINDOW_RESIZED,
			APP_CMD_WINDOW_REDRAW_NEEDED,

			APP_CMD_CONTENT_RECT_CHANGED,

			APP_CMD_GAINED_FOCUS,

			APP_CMD_LOST_FOCUS,
			APP_CMD_CONFIG_CHANGED,
			APP_CMD_LOW_MEMORY,
			APP_CMD_START,
			APP_CMD_RESUME,

			APP_CMD_SAVE_STATE,
			APP_CMD_PAUSE,
			APP_CMD_STOP,
			APP_CMD_DESTROY,
		};

		enum {
			LOOPER_ID_MAIN = 1,
			LOOPER_ID_INPUT = 2,
			LOOPER_ID_USER = 3,
		};

		struct android_poll_source {
			// The identifier of this source.  May be LOOPER_ID_MAIN or
			// LOOPER_ID_INPUT.
			int32_t id;

			// Function to call to perform the standard processing of data from
			// this source.
			void (ZFrameAndroid::*process)(android_poll_source* source);
		};

		void PrintCurrentConfig();
		void FreeSavedState();
		int8_t ReadCommand();
		void PreExecuteCommand(int8_t cmd);
		void PostExecuteCommand(int8_t cmd);
		void ProcessInput(android_poll_source* source);
		void ProcessCommand(android_poll_source* source);
		void Main();
		void WriteCommand(int8_t cmd);
		void SetInput(AInputQueue* inputQueue);
		void SetWindow(ANativeWindow* window);
		void SetActivityState(int8_t cmd);

	protected:
		void OnDestroy();
		void OnStart();
		void OnResume();
		void* OnSaveInstanceState(size_t* outSize);
		void OnPause();
		void OnStop();
		void OnConfigurationChanged();
		void OnLowMemory();
		void OnWindowFocusChanged(int focused);
		void OnNativeWindowCreated(ANativeWindow* window);
		void OnNativeWindowDestroyed(ANativeWindow* window);
		void OnInputQueueCreated(AInputQueue* queue);
		void OnInputQueueDestroyed(AInputQueue* queue);

	protected:
		Callback* callback;

	protected:
		// The ANativeActivity object instance that this app is running in.
		ANativeActivity* activity;

		// The current configuration the app is running in.
		AConfiguration* config;

		// This is the last instance's saved state, as provided at creation time.
		// It is NULL if there was no state.  You can use this as you need; the
		// memory will remain around until you call android_app_exec_cmd() for
		// APP_CMD_RESUME, at which point it will be freed and savedState set to NULL.
		// These variables should only be changed when processing a APP_CMD_SAVE_STATE,
		// at which point they will be initialized to NULL and you can malloc your
		// state and place the information here.  In that case the memory will be
		// freed for you later.
		void* savedState;
		size_t savedStateSize;

		// The ALooper associated with the app's thread.
		ALooper* looper;

		// When non-NULL, this is the input queue from which the app will
		// receive user input events.
		AInputQueue* inputQueue;

		// When non-NULL, this is the window surface that the app can draw in.
		ANativeWindow* window;

		// Current content rectangle of the window; this is the area where the
		// window's content should be placed to be seen by the user.
		ARect contentRect;

		// Current state of the app's activity.  May be either APP_CMD_START,
		// APP_CMD_RESUME, APP_CMD_PAUSE, or APP_CMD_STOP; see below.
		int activityState;

		// This is non-zero when the application's NativeActivity is being
		// destroyed and waiting for the app thread to complete.
		int destroyRequested;

		// -------------------------------------------------
		// Below are "protected" implementation of the glue code.

		pthread_mutex_t mutex;
		pthread_cond_t cond;

		int msgread;
		int msgwrite;

		android_poll_source cmdPollSource;
		android_poll_source inputPollSource;
		int running;
		int stateSaved;
		int destroyed;
		int redrawNeeded;
		AInputQueue* pendingInputQueue;
		ANativeWindow* pendingWindow;
		ARect pendingContentRect;

	protected:
		struct saved_state {
			float angle;
			int32_t x;
			int32_t y;
		};

		int InitDisplay();
		void DrawFrame();
		void TerminateDisplay();
		int HandleInput(AInputEvent* event);
		void HandleCommand(int32_t cmd);

		ASensorManager* sensorManager;
		const ASensor* accelerometerSensor;
		ASensorEventQueue* sensorEventQueue;

		int animating;
		EGLDisplay display;
		EGLSurface surface;
		EGLContext context;
		saved_state state;
	};
}