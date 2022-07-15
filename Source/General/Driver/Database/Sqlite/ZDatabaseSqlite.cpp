#include "ZDatabaseSqlite.h"
#include "../../../../Core/Interface/IStreamBase.h"
#include "../../../../General/Misc/DynamicObject.h"

extern "C" {
	#include "Core/sqlite3.h"
	int SQLITE_CDECL sqlite3_shell(int argc, char** argv, FILE* fp);
}

using namespace PaintsNow;

// MetaData

class DatabaseSqliteImpl : public IDatabase::Database {
public:
	sqlite3* handle;
	DynamicUniqueAllocator uniqueAllocator;
	String tmpFileName;
};

ZDatabaseSqlite::ZDatabaseSqlite() {
	sqlite3_initialize();
}

ZDatabaseSqlite::~ZDatabaseSqlite() {
	sqlite3_shutdown();
}

IDatabase::Database* ZDatabaseSqlite::Connect(IArchive& archive, const String& target, const String& username, const String& password, bool createOnNonExist) {
	sqlite3* handle;
	String fullPath;
	String tmpFileName;
	if (target == ":memory:") {
		fullPath = target;
	} else {
		fullPath = archive.GetFullPath(target);

		// test if fullPath can be opened
		FILE* fp = fopen(fullPath.c_str(), "rb");
		if (fp != NULL) {
			fclose(fp);

			if (SQLITE_OK == sqlite3_open_v2(fullPath.c_str(), &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, nullptr)) {
				DatabaseSqliteImpl* impl = new DatabaseSqliteImpl();
				impl->handle = handle;
				return impl;
			} else {
				if (handle != nullptr) {
					sqlite3_close(handle);
				}

				return nullptr;
			}
		} else if (archive.Exists(target)) {
			// try to extract
			uint64_t length;
			IStreamBase* f = archive.Open(target, false, length);
			if (f == nullptr) return nullptr;

			char tmpName[L_tmpnam];
			tmpnam(tmpName);
			tmpFileName = tmpName;

			FILE* fp = fopen(tmpFileName.c_str(), "wb");
			if (fp != nullptr) {
				uint8_t* buffer = new uint8_t[verify_cast<size_t>(length)];
				size_t s = verify_cast<size_t>(length);
				if (f->Read(buffer, s)) {
					fwrite(buffer, 1, s, fp);
				}

				fclose(fp);
			}

			f->Destroy();
			fullPath = tmpFileName;
		}
	}

	if (createOnNonExist) {
		if (SQLITE_OK == sqlite3_open_v2(fullPath.c_str(), &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, nullptr)) {
			DatabaseSqliteImpl* impl = new DatabaseSqliteImpl();
			impl->handle = handle;
			impl->tmpFileName = std::move(tmpFileName);
			tmpFileName = "";
			return impl;
		} else {
			if (handle != nullptr) {
				sqlite3_close(handle);
			}
		}
	}

	if (!tmpFileName.empty()) {
		remove(tmpFileName.c_str());
	}

	return nullptr;
}

void ZDatabaseSqlite::Close(Database* database) {
	DatabaseSqliteImpl* impl = static_cast<DatabaseSqliteImpl*>(database);
	if (impl->handle != nullptr) {
		sqlite3_close(impl->handle);
	}

	if (!impl->tmpFileName.empty()) {
		remove(impl->tmpFileName.c_str());
	}

	delete impl;
}

static void StringCreator(void* buffer) {
	new (buffer) String();
}

static void StringDeletor(void* buffer) {
	reinterpret_cast<String*>(buffer)->~String();
}

static void StringAssigner(void* dst, const void* src) {
	*reinterpret_cast<String*>(dst) = *reinterpret_cast<const String*>(src);
}

class QueryMetaData : public TReflected<QueryMetaData, IDatabase::MetaData> {
public:
	QueryMetaData(DynamicUniqueAllocator& allocator, sqlite3* h, sqlite3_stmt* s) : uniqueAllocator(allocator), handle(h), dynamicObject(nullptr), stmt(s), count(0), inited(true), finished(false) {
		int status = sqlite3_step(stmt);
		if (status == SQLITE_ROW || status == SQLITE_DONE) {
			count = sqlite3_column_count(stmt);
			std::vector<DynamicInfo::Field> fields(count);

			for (int i = 0; i < count; i++) {
				const char* name = sqlite3_column_name(stmt, i);
				if (name == nullptr) name = "";
				int t = sqlite3_value_type(sqlite3_column_value(stmt, i));
				// int t = sqlite3_column_type(stmt, i);

				DynamicInfo::Field& field = fields[i];
				field.name = name;

				static DynamicInfo::MemController mc = {
					StringCreator, StringDeletor, StringAssigner
				};

				switch (t) {
					case SQLITE_INTEGER:
						field.type = UniqueType<int>::Get();
						field.controller = nullptr;
						sets.emplace_back(&QueryMetaData::SetValueInt);
						break;
					case SQLITE_FLOAT:
						field.type = UniqueType<double>::Get();
						field.controller = nullptr;
						sets.emplace_back(&QueryMetaData::SetValueFloat);
						break;
					case SQLITE_NULL:
						field.type = UniqueType<Void>::Get();
						field.controller = nullptr;
						sets.emplace_back(&QueryMetaData::SetValueNull);
						break;
					case SQLITE_TEXT:
					default:
						field.type = UniqueType<String>::Get();
						field.controller = &mc;
						sets.emplace_back(&QueryMetaData::SetValueText);
						break;
				}
			}

			DynamicInfo* info = uniqueAllocator.AllocFromDescriptor("QueryMetaDataInstance", fields);
			dynamicObject = static_cast<DynamicObject*>(info->Create());
		} else {
			fprintf(stderr, "\nerror %s\n", sqlite3_errmsg(handle));
		}
	}

	int GetColumnCount() const {
		return count;
	}

	void SetValueNull(int i) {

	}

	void SetValueText(int i) {
		const char* text = (const char*)sqlite3_column_text(stmt, i);
		if (text != nullptr) {
			String s = text;
			dynamicObject->Set(dynamicObject->GetDynamicInfo()->fields[i], &s);
		}
	}

	void SetValueFloat(int i) {
		double value = sqlite3_column_double(stmt, i);
		dynamicObject->Set(dynamicObject->GetDynamicInfo()->fields[i], &value);
	}

	void SetValueInt(int i) {
		int value = sqlite3_column_int(stmt, i);
		dynamicObject->Set(dynamicObject->GetDynamicInfo()->fields[i], &value);
	}

	~QueryMetaData() override {
		if (dynamicObject != nullptr)
			dynamicObject->Destroy();
		sqlite3_finalize(stmt);
	}

	IIterator* New() const override {
		assert(false);
		return nullptr;
	}

	void Attach(void* base) override {
		assert(false);
	}

	void Initialize(size_t count) override {
		assert(false);
	}

	Unique GetElementUnique() const override {
		assert(false);
		return Unique();
	}

	Unique GetElementReferenceUnique() const override {
		assert(false);
		return Unique();
	}

	bool IsElementBasicObject() const override {
		return true;
	}

	size_t GetTotalCount() const override {
		assert(false);
		return 0;
	}

	void* GetHost() const override {
		return nullptr;
	}

	void* Get() override {
		return dynamicObject;
	}

	const IReflectObject& GetElementPrototype() const override {
		return *dynamicObject;
	}

	bool Next() override {
		if (count == 0 || finished) return false;

		if (inited || sqlite3_step(stmt) == SQLITE_ROW) {
			inited = false;
			for (int i = 0; i < count; i++) {
				(this->*sets[i])(i);
			}

			return true;
		} else {
			finished = true;
			return false;
		}
	}

	bool IsLayoutLinear() const override {
		return false;
	}

	bool IsLayoutPinned() const override {
		return true;
	}

	virtual const String& GetInternalName() const {
		static String nullName;
		return nullName;
	}

private:
	DynamicUniqueAllocator& uniqueAllocator;
	sqlite3* handle;
	sqlite3_stmt* stmt;
	typedef void (QueryMetaData::* Set)(int i);
	std::vector<Set> sets;
	DynamicObject* dynamicObject;

	int count;
	bool inited;
	bool finished;
};

class MapperSqlite : public IReflect {
public:
	MapperSqlite(sqlite3_stmt* s, const std::vector<String>& n) : IReflect(true, false), stmt(s), names(n), counter(0) {}
	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		singleton Unique intType = UniqueType<int>::Get();
		singleton Unique strType = UniqueType<String>::Get();
		singleton Unique cstrType = UniqueType<const char*>::Get();
		singleton Unique doubleType = UniqueType<double>::Get();

		size_t offset = (const char*)ptr - (const char*)base;
		if (typeID == intType) {
			setters.emplace_back(std::make_pair(&MapperSqlite::SetValueInt, offset));
		} else if (typeID == strType) {
			setters.emplace_back(std::make_pair(&MapperSqlite::SetValueString, offset));
		} else if (typeID == cstrType) {
			setters.emplace_back(std::make_pair(&MapperSqlite::SetValueText, offset));
		} else if (typeID == doubleType) {
			setters.emplace_back(std::make_pair(&MapperSqlite::SetValueFloat, offset));
		} else {
			assert(false);
		}

		counter++;
	}
	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

	void SetValueString(int i, const void* base) {
		sqlite3_bind_text(stmt, i, ((const String*)base)->c_str(), (int)((const String*)base)->length(), SQLITE_TRANSIENT);
	}

	void SetValueText(int i, const void* base) {
		sqlite3_bind_text(stmt, i, (const char*)base, (int)strlen((const char*)base), SQLITE_TRANSIENT);
	}

	void SetValueFloat(int i, const void* base) {
		sqlite3_bind_double(stmt, i, *(const double*)base);
	}

	void SetValueInt(int i, const void* base) {
		sqlite3_bind_int(stmt, i, *(const int*)base);
	}

	sqlite3_stmt* stmt;
	const std::vector<String>& names;
	typedef void (MapperSqlite::* Set)(int i, const void* base);
	std::vector<std::pair<Set, size_t> > setters;
	int counter;
};

class WriterSqlite : public IReflect {
public:
	WriterSqlite(MapperSqlite& r) : IReflect(true, false), reflect(r), i(0) {}

	void Property(IReflectObject& object, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		MapperSqlite::Set s = reflect.setters[i].first;
		(reflect.*s)(i, ptr);
		i++;
	}
	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

private:
	MapperSqlite& reflect;
	int i;
};

IDatabase::MetaData* ZDatabaseSqlite::Execute(Database* database, const String& statementTemplate, IDatabase::MetaData* postData) {
	DatabaseSqliteImpl* impl = static_cast<DatabaseSqliteImpl*>(database);
	sqlite3_stmt* stmt;
	// replace all {} variables statements with ? and records the order
	String converted = statementTemplate;
	bool instr = false;
	bool quote = false;
	bool inVar = false;
	String varname;
	std::vector<String> names;

	size_t j = 0;
	for (size_t i = 0; i < converted.size(); i++) {
		bool skip = false;
		switch (converted[i]) {
			case '\'':
				instr = !instr;
				break;
			case '"':
				quote = !quote;
				break;
			case '{':
				varname = "";
				inVar = true;
				skip = true;
				break;
			case '}':
				names.emplace_back(varname);
				inVar = false;
				skip = true;
				converted[j++] = '?';
				break;
			default:
				if (!instr && !quote && inVar) {
					varname += converted[i];
					skip = true;
				}
				break;
		}

		if (!skip) {
			converted[j++] = converted[i];
		}
	}

	converted[j++] = '\0';

	if (sqlite3_prepare_v2(impl->handle, converted.c_str(), -1, &stmt, 0) != SQLITE_OK) {
		fprintf(stderr, "\nerror %s\n", sqlite3_errmsg(impl->handle));
		return nullptr;
	} else {
		if (postData != nullptr) {
			// Test code
			/*
			sqlite3_bind_int(stmt, 1, 1);
			sqlite3_bind_text(stmt, 2, "Hi", 2, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt, 3, "My", 2, SQLITE_TRANSIENT);
			int e = sqlite3_step(stmt);
			fprintf(stderr, "\nerror %s\n", sqlite3_errmsg(impl->handle));*/

			// bind data
			const IReflectObject& prototype = postData->GetElementPrototype();
			MapperSqlite reflect(stmt, names);
			prototype(reflect);

			if (postData->IsLayoutPinned()) {
				std::vector<std::pair<MapperSqlite::Set, size_t> >& setters = reflect.setters;
				while (postData->Next()) {
					// Pinned data, fast write
					for (size_t i = 0; i < setters.size(); i++) {
						MapperSqlite::Set s = setters[i].first;
						(reflect.*s)((int)i + 1, (const char*)&prototype + setters[i].second);
					}

					if (sqlite3_step(stmt) != SQLITE_DONE) {
						fprintf(stderr, "\nerror %s\n", sqlite3_errmsg(impl->handle));
					}

					sqlite3_reset(stmt);
				}
			} else {
				assert(!postData->IsElementBasicObject());
				while (postData->Next()) {
					WriterSqlite writer(reflect);
					(*reinterpret_cast<IReflectObject*>(postData->Get()))(writer);
					if (sqlite3_step(stmt) != SQLITE_DONE) {
						fprintf(stderr, "\nerror %s\n", sqlite3_errmsg(impl->handle));
					}

					sqlite3_reset(stmt);
				}
			}

			sqlite3_finalize(stmt);
			return nullptr;
		} else {
			QueryMetaData* data = new QueryMetaData(impl->uniqueAllocator, impl->handle, stmt);
			if (data->GetColumnCount() == 0) {
				data->Destroy();
				return nullptr;
			} else {
				return data;
			}
		}
	}
}

int ZDatabaseSqlite::Shell(IArchive& archive, const std::vector<String>& args, const String& outputFile) {
	// replace environment variables
	std::vector<String> converted;
	std::vector<const char*> ptrs(args.size() + 1);
	ptrs[0] = "sqlite3";

	for (size_t i = 0; i < args.size(); i++) {
		const String& str = args[i];
		size_t pos = str.find("${");
		if (pos != String::npos) {
			size_t end = str.find("}$");
			if (end != String::npos) {
				converted.reserve(args.size());
				converted.push_back(str.substr(0, pos) + archive.GetFullPath(str.substr(pos + 2, end - pos - 2)) + str.substr(end + 2));
				ptrs[i + 1] = converted.back().c_str();
				continue;
			}
		}

		ptrs[i + 1] = str.c_str();
	}

	FILE* fp = stdout;
	if (!outputFile.empty()) {
		fp = fopen(archive.GetFullPath(outputFile).c_str(), "wb");
	}

	int ret = sqlite3_shell((int)ptrs.size(), (char**)&ptrs[0], fp);

	if (!outputFile.empty()) {
		fflush(fp);
		fclose(fp);
	}

	return ret;
}

