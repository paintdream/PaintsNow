// CmdLine.h
// PaintDream (paintdream@paintdream.com)
// 2016-1-1
//

#pragma once
#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IType.h"
#include <string>
#include <map>
#include <list>

namespace PaintsNow {
	class CmdLine {
	public:
		virtual ~CmdLine();

		void Process(int argc, char* argv[]);
		class Option {
		public:
			String name;
			String param;
		};
		const std::map<String, Option>& GetFactoryMap() const;
		const std::map<String, Option>& GetConfigMap() const;
		const std::list<Option>& GetModuleList() const;
		const std::list<String>& GetPackageList() const;

	private:
		void ProcessConfig(const String& path);
		void ProcessOne(const String& str);
		void ProcessOption(const String& str);
		void ProcessPackage(const String& str);
		// void ProcessFactory(const String& str);
		// void ProcessParam(const String& str);

		std::map<String, Option> factoryMap;
		std::map<String, Option> configMap;
		std::list<Option> moduleList;
		std::list<String> packageList;
	};
}

