#include "Hive.h"

using namespace PaintsNow;

Hive::Hive(IDatabase& db, IDatabase::Database* dbInst) : base(db), database(dbInst) {}

Hive::~Hive() {
	base.Close(database);
}

TShared<Honey> Hive::Execute(const String& sql, HoneyData& honey) {
	honey.Enter();
	IDatabase::MetaData* meta = base.Execute(database, sql, honey.GetTotalCount() == 0 ? nullptr : &honey);
	honey.Leave();
	return meta != nullptr ? TShared<Honey>::From(new Honey(meta)) : nullptr;
}

TObject<IReflect>& Hive::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	return *this;
}