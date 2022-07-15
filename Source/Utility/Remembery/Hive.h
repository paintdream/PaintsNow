// Hive.h
// PaintDream (paintdream@paintdream.com)
// 2015-12-31
//

#pragma once
#include "../../Core/System/Kernel.h"
#include "Honey.h"

namespace PaintsNow {
	class Hive : public TReflected<Hive, WarpTiny> {
	public:
		Hive(IDatabase& base, IDatabase::Database* database);
		~Hive() override;

		TObject<IReflect>& operator () (IReflect& reflect) override;
		TShared<Honey> Execute(const String& sql, HoneyData& honeyData);

	private:
		IDatabase& base;
		IDatabase::Database* database;
	};
}

