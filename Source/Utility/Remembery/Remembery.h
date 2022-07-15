// Remembery.h
// PaintDream (paintdream@paintdream.com)
// 2015-6-15
//

#pragma once
#include "Hive.h"
#include "../BridgeSunset/BridgeSunset.h"

namespace PaintsNow {
	class Remembery : public TReflected<Remembery, IScript::Library> {
	public:
		Remembery(IThread& threadApi, IArchive& archive, IDatabase& databaseFactory, BridgeSunset& bridgeSunset);
		~Remembery() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create or open a database
		/// </summary>
		/// <param name="path"> database path </param>
		/// <param name="username"> optional username </param>
		/// <param name="password"> optional password </param>
		/// <returns> Hive object </returns>
		TShared<Hive> RequestNewDatabase(IScript::Request& request, const String& path, const String& username, const String& password, bool createOnNonExist);

		/// <summary>
		/// Execute a SQL query
		/// </summary>
		/// <param name="hive"> Hive object returned from NewDatabase() </param>
		/// <param name="sql"> sql statement </param>
		/// <param name="honeyData"> additional data (for insertion) </param>
		/// <returns> A delayed Honey object that contains the query result </returns>
		TShared<Honey> RequestExecute(IScript::Request& request, IScript::Delegate<Hive> hive, const String& sql, HoneyData& honeyData);

		/// <summary>
		/// Get data from current position and step to next lines
		/// </summary>
		/// <param name="honey"> Honey object returned from Execute() </param>
		/// <param name="count"> request count of lines, 0 for all lines at once</param>
		/// <returns> An array of line data </returns>
		void RequestStep(IScript::Request& request, IScript::Delegate<Honey> honey, uint32_t count);

		/// <summary>
		/// Execute database shell
		/// </summary>
		/// <param name="args"> Arguments for shell </param>
		/// <returns> shell returned value </returns>
		int RequestShell(IScript::Request& request, const std::vector<String>& args, const String& outputFile);

	private:
		BridgeSunset& bridgeSunset;
		IDatabase& databaseFactory;
		IArchive& archive;
	};
}

