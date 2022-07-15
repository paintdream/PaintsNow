#include "Config.h"

using namespace PaintsNow;

Config::~Config() {}

void Config::RegisterFactory(const String& factoryEntry, const String& name, const TWrapper<IDevice*>& factoryBase) {
	Entry entry;
	entry.name = name;
	entry.factoryBase = factoryBase;

	mapFactories[factoryEntry].emplace_back(std::move(entry));
	// printf("Register new factory<%s> : %s\n", factoryEntry.c_str(), name.c_str());
}

void Config::UnregisterFactory(const String& factoryEntry, const String& name) {
	std::list<Entry> entries = mapFactories[factoryEntry];
	for (std::list<Entry>::iterator p = entries.begin(); p != entries.end(); ++p) {
		if ((*p).name == name) {
			entries.erase(p);
			break;
		}
	}
}

const std::list<Config::Entry>& Config::GetEntry(const String& factoryEntry) const {
	const std::map<String, std::list<Entry> >::const_iterator p = mapFactories.find(factoryEntry);
	if (p != mapFactories.end()) {
		return p->second;
	} else {
		static const std::list<Config::Entry> dummy;
		return dummy;
	}
}