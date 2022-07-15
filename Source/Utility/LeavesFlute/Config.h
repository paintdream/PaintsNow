// Config.h
// PaintDream (paintdream@paintdream.com)
// 2016-1-1
//

#pragma once
#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IType.h"
#include "../../Core/Interface/IDevice.h"
#include "../../Core/Template/TProxy.h"
#include <string>
#include <map>
#include <list>

namespace PaintsNow {
	class Config {
	public:
		~Config();
		void RegisterFactory(const String& factoryEntry, const String& name, const TWrapper<IDevice*>& factoryBase);
		void UnregisterFactory(const String& factoryEntry, const String& name);

		class Entry {
		public:
			String name;
			TWrapper<IDevice*> factoryBase;
		};

		const std::list<Entry>& GetEntry(const String& factoryEntry) const;

	private:
		std::map<String, std::list<Entry> > mapFactories;
	};
}
