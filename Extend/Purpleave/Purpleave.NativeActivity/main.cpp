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
#include <jni.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android/native_activity.h>
#include "Source/Driver/Frame/Android/ZFrameAndroid.h"
#include "../../../Source/Utility/LeavesFlute/Loader.h"

using namespace PaintsNow;

extern "C" void ANativeActivity_onCreate(ANativeActivity * activity, void* savedState, size_t savedStateSize) {
	ZFrameAndroid::InvokeMain(WrapClosure([=]() {
		// Register frame factory
		Loader loader;
		loader.GetConfig().RegisterFactory("IFrame", "ZFrameAndroid", WrapFactory(UniqueType<ZFrameAndroid>(), activity, savedState, savedStateSize));

		// Inject frame factory
		CmdLine cmdLine;
		char* initArgs[] = {
			"LeavesWing",
			"--Graphic=true",
			"--IFrame=ZFrameAndroid"
		};

		cmdLine.Process(sizeof(initArgs) / sizeof(initArgs[0]), initArgs);

		// Now lets start
		loader.Run(cmdLine);
	}));
}
