#include "CmdLine.h"
#include <fstream>

using namespace PaintsNow;

CmdLine::~CmdLine() {}

void CmdLine::Process(int argc, char* argv[]) {
	bool skipDefaultConfig = false;
	for (int k = 1; k < argc; k++) {
		if (strstr(argv[k], "-Config=") != nullptr) {
			skipDefaultConfig = true;
			break;
		}
	}

	if (!skipDefaultConfig) {
		ProcessConfig("default.config");
	}

	for (int i = 1; i < argc; i++) {
		ProcessOne(argv[i]);
	}
}

void CmdLine::ProcessOne(const String& str) {
	switch (str[0]) {
		case '-':
			ProcessOption(str);
			break;
		default:
			ProcessPackage(str);
			break;
	}
}

void CmdLine::ProcessConfig(const String& path) {
	std::ifstream config;
	config.open(path.c_str(), std::ios::in);
	std::string item;
	while (config.good()) {
		config >> item;
		ProcessOption(item.c_str());
	}
}

void CmdLine::ProcessOption(const String& str) {
	// find '='
	String::size_type pos = str.find('=');
	if (pos != String::npos) {
		// valid config
		String key = str.substr(2, pos - 2);
		String value = str.substr(pos + 1);
		String::size_type param = value.find('#');
		Option option;
		if (param != String::npos) {
			option.name = value.substr(0, param);
			option.param = value.substr(param + 1);
		} else {
			option.name = value;
		}

		if (key == "Config") {
			ProcessConfig(value);
		} else if (key == "LoadModule") {
			// load library
			moduleList.emplace_back(std::move(option));
		} else if (key[0] == 'I') {
			// set factory options
			factoryMap[key] = std::move(option);
		} else {
			configMap[key] = std::move(option);
		}
	}
}

const std::map<String, CmdLine::Option>& CmdLine::GetFactoryMap() const {
	return factoryMap;
}

const std::map<String, CmdLine::Option>& CmdLine::GetConfigMap() const {
	return configMap;
}

const std::list<CmdLine::Option>& CmdLine::GetModuleList() const {
	return moduleList;
}

const std::list<String>& CmdLine::GetPackageList() const {
	return packageList;
}

void CmdLine::ProcessPackage(const String& str) {
	packageList.emplace_back(str);
}