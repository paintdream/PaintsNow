// Interfaces.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-4
//

#pragma once
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "IRender.h"
#include "IAudio.h"
#include "IDatabase.h"
#include "IDebugger.h"
#include "IFontBase.h"
#include "IFrame.h"
#include "IImage.h"
#include "INetwork.h"
#include "IRandom.h"
#include "../../Core/Interface/IArchive.h"
#include "../../Core/Interface/IFilterBase.h"
#include "../../Core/Interface/IScript.h"
#include "../../Core/Interface/IThread.h"
#include "ITimer.h"
#include "ITunnel.h"

namespace PaintsNow {

	class Interfaces {
	public:
		Interfaces(IArchive& archive, IAudio& audio, IDatabase& database, IFilterBase& assetFilterBase, IFilterBase& audioFilterBase,
			IFontBase& fontBase, IFrame& frame, IImage& image, INetwork& standardNetwork, INetwork& quickNetwork, IRandom& random, IRender& render,
			IScript& script, IThread& thread, ITimer& timer);

		IArchive& archive;
		IAudio& audio;
		IDatabase& database;
		IFilterBase& assetFilterBase;
		IFilterBase& audioFilterBase;
		IFontBase& fontBase;
		IFrame& frame;
		IImage& image;
		INetwork& standardNetwork;
		INetwork& quickNetwork;
		IRandom& random;
		IRender& render;
		IScript& script;
		IThread& thread;
		ITimer& timer;
	};
}

