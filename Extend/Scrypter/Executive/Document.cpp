#include "stdafx.h"
using namespace PaintsNow;

static const char* emptyString = "";

TObject<IReflect>& ParameterData::operator () (IReflect& reflect)
{
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty())
	{
		ReflectProperty(name)[ScriptVariable];
		ReflectProperty(description)[ScriptVariable];
		ReflectProperty(type)[ScriptVariable];
		ReflectProperty(value)[ScriptVariable];
	}

	return *this;
}

RuntimeData::RuntimeData()
{
}

// not copying
RuntimeData::RuntimeData(rvalue<RuntimeData> rhs)
{}

RuntimeData::RuntimeData(const RuntimeData& rhs)
{}

void RuntimeData::Clear()
{
	memoryCache.Clear();
	recordDataItems.Clear();
	logDataItems.Clear();
}

StringView RuntimeData::CacheStringView(StringView view)
{
	if (view.size() != 0)
	{
		uint8_t* buffer = memoryCache.Allocate(verify_cast<uint32_t>(view.size()), 1);
		memcpy(buffer, view.data(), view.size());
		return StringView(reinterpret_cast<const char*>(buffer), view.size());
	}
	else
	{
		return StringView(emptyString, 0);
	}
}

Document::Document(Kernel& ker, IArchive& ar, Worker& worker, const TWrapper<void, Document*>& update) : kernel(ker), archive(ar), mainWorker(worker), onUpdate(update), parametersVersion(0)
{
	assert(onUpdate);
	callCounter.store(0, std::memory_order_relaxed);
	runtimeData.resize(kernel.GetWarpCount());
}

Document::~Document()
{
	if (commandController)
	{
		IScript::Request& request = mainWorker.GetScript().GetDefaultRequest();
		request.DoLock();
		request.Dereference(commandController);
		request.UnLock();
	}
}

const ProfileData& Document::GetProfile() const
{
	return profileData;
}

String Document::LoadScriptText(const String& filePath)
{
	uint64_t length = 0;
	IStreamBase* stream = archive.Open(filePath, false, length);
	String content;

	if (stream != nullptr)
	{
		size_t len = verify_cast<size_t>(length);
		content.resize(len);
		stream->Read(const_cast<char*>(content.c_str()), len);
		stream->Destroy();
	}

	return content;
}

TShared<Document> Document::PushDocument()
{
	TShared<Document> ret = mainWorker.currentDocument;
	mainWorker.currentDocument = this;

	return ret;
}

void Document::PopDocument(const TShared<Document>& doc)
{
	mainWorker.currentDocument = doc;
}

void Document::UpdateProfile(StringView stat, StringView detail, float ratio)
{
	profileData.status = stat;
	profileData.detail = detail;
	profileData.ratio = ratio;
}

bool Document::InvokeReload()
{
	size_t expected = DOCUMENT_IDLE;
	if (status.compare_exchange_strong(expected, DOCUMENT_EXECUTING, std::memory_order_relaxed))
	{
		kernel.QueueRoutine(this, CreateTaskContextFree(Wrap(this, &Document::RoutineReload)));
		return true;
	}
	else
	{
		return false;
	}
}

void Document::RoutineReload()
{
	if (commandController)
	{
		IScript::Request& request = mainWorker.GetScript().GetDefaultRequest();
		request.DoLock();
		request.Dereference(commandController);
		request.UnLock();
	}

	String code = LoadScriptText(pluginPath);
	if (code.size() != 0)
	{
		TShared<Document> lastDocument = PushDocument();
		IScript::Request& request = mainWorker.GetScript().GetDefaultRequest();
		request.DoLock();
		IScript::Request::Ref ref = request.Load(code);
		if (ref)
		{
			request.Push();
			request.Call(ref);
			request >> commandController;
			request.Dereference(ref);
			request.Pop();
		}

		request.UnLock();
		PopDocument(lastDocument);

		// synchronize parameters
		if (commandController)
		{
			PrepareCall();
			do
			{
				if (status.load(std::memory_order_relaxed) != DOCUMENT_EXECUTING)
					break;
				RoutineUploadParameters();

				if (status.load(std::memory_order_relaxed) != DOCUMENT_EXECUTING)
					break;
				RoutineDownloadParameters();

				if (status.load(std::memory_order_relaxed) != DOCUMENT_EXECUTING)
					break;
				RoutineGetOperations();
			}
			while (false);

			CompleteCall();
			return;
		}
	}

	status.store(DOCUMENT_ERROR, std::memory_order_release);
}

bool Document::InvokeMain()
{
	size_t expected = DOCUMENT_IDLE;
	if (status.compare_exchange_strong(expected, DOCUMENT_EXECUTING, std::memory_order_relaxed))
	{
		kernel.QueueRoutine(this, CreateTaskContextFree(Wrap(this, &Document::RoutineMain)));
		return true;
	}
	else
	{
		return false;
	}
}

void Document::RoutineMain()
{
	if (commandController)
	{
		profileData.ratio = 0;

		PrepareCall();
		TShared<Document> lastDocument = PushDocument();
		IScript::Request& request = *mainWorker.requestPool.AcquireSafe();
		request.DoLock();

		request.Push();
		request.Call(commandController, "Main");
		request.Pop();

		request.UnLock();
		mainWorker.requestPool.ReleaseSafe(&request);
		PopDocument(lastDocument);
		CompleteCall();
	}
	else
	{
		status.store(DOCUMENT_ERROR, std::memory_order_release);
	}
}

bool Document::InvokeGetOperations()
{
	size_t expected = DOCUMENT_IDLE;
	if (status.compare_exchange_strong(expected, DOCUMENT_EXECUTING, std::memory_order_relaxed))
	{
		kernel.QueueRoutine(this, CreateTaskContextFree(Wrap(this, &Document::RoutineGetOperations)));
		return true;
	}
	else
	{
		return false;
	}
}

void Document::RoutineGetOperations()
{
	if (commandController)
	{
		PrepareCall();
		TShared<Document> lastDocument = PushDocument();
		IScript::Request& request = mainWorker.GetScript().GetDefaultRequest();

		request.DoLock();
		request.Push();
		request.Call(commandController, "GetOperations");
		request >> operations;
		request.Pop();
		request.UnLock();

		PopDocument(lastDocument);
		CompleteCall();
	}
	else
	{
		status.store(DOCUMENT_ERROR, std::memory_order_release);
	}
}


bool Document::InvokeOperation(size_t operationIndex, rvalue<std::vector<const RecordData*> > recordData)
{
	size_t expected = DOCUMENT_IDLE;
	if (status.compare_exchange_strong(expected, DOCUMENT_EXECUTING, std::memory_order_relaxed))
	{
		kernel.QueueRoutine(this, CreateTaskContextFree(Wrap(this, &Document::RoutineOperation), operationIndex, std::move(recordData)));
		return true;
	}
	else
	{
		return false;
	}
}

void Document::RoutineOperation(size_t operationIndex, const std::vector<const RecordData*>& recordData)
{
	if (commandController && operationIndex < operations.size())
	{
		PrepareCall();

		TShared<Document> lastDocument = PushDocument();
		IScript::Request& request = mainWorker.GetScript().GetDefaultRequest();

		request.DoLock();
		for (size_t i = 0; i < recordData.size(); i++)
		{
			const RecordData* d = recordData[i];
			request.Push();
			request.Call(commandController, operations[operationIndex], d->object, d->description, d->cookie);
			request.Pop();
		}
		request.UnLock();

		PopDocument(lastDocument);
		CompleteCall();
	}
	else
	{
		status.store(DOCUMENT_ERROR, std::memory_order_release);
	}
}

bool Document::InvokeUploadParameters()
{
	size_t expected = DOCUMENT_IDLE;
	if (status.compare_exchange_strong(expected, DOCUMENT_EXECUTING, std::memory_order_relaxed))
	{
		kernel.QueueRoutine(this, CreateTaskContextFree(Wrap(this, &Document::RoutineUploadParameters)));
		return true;
	}
	else
	{
		return false;
	}
}

void Document::RoutineUploadParameters()
{
	if (commandController)
	{
		PrepareCall();
		TShared<Document> lastDocument = PushDocument();
		IScript::Request& request = mainWorker.GetScript().GetDefaultRequest();

		request.DoLock();
		request.Push();
		request.Call(commandController, "UploadParameters", parameters);
		request.Pop();
		request.UnLock();

		PopDocument(lastDocument);
		CompleteCall();
	}
	else
	{
		status.store(DOCUMENT_ERROR, std::memory_order_release);
	}
}

bool Document::InvokeDownloadParameters()
{
	size_t expected = DOCUMENT_IDLE;
	if (status.compare_exchange_strong(expected, DOCUMENT_EXECUTING, std::memory_order_relaxed))
	{
		kernel.QueueRoutine(this, CreateTaskContextFree(Wrap(this, &Document::RoutineDownloadParameters)));
		return true;
	}
	else
	{
		return false;
	}
}

void Document::RoutineDownloadParameters()
{
	if (commandController)
	{
		PrepareCall();
		TShared<Document> lastDocument = PushDocument();
		IScript::Request& request = mainWorker.GetScript().GetDefaultRequest();

		parameters.clear();
		request.DoLock();
		request.Push();
		request.Call(commandController, "DownloadParameters");
		request >> parameters;
		request.Pop();
		request.UnLock();
		parametersVersion++;

		PopDocument(lastDocument);
		CompleteCall();
	}
	else
	{
		status.store(DOCUMENT_ERROR, std::memory_order_release);
	}
}

const String& Document::GetPluginPath() const
{
	// assert(GetStatus() == DOCUMENT_IDLE);
	return pluginPath;
}

void Document::SetPluginPath(const String& path)
{
	assert(GetStatus() == DOCUMENT_EXTERNAL_LOCKING);
	pluginPath = path;
}

std::vector<ParameterData>& Document::GetParameters()
{
	assert(GetStatus() == DOCUMENT_EXTERNAL_LOCKING);
	return parameters;
}

const std::vector<String>& Document::GetOperations() const
{
	return operations;
}

const std::vector<RuntimeData>& Document::GetRuntimeData() const
{
	return runtimeData;
}

void Document::ClearError()
{
	size_t expected = DOCUMENT_ERROR;
	status.compare_exchange_strong(expected, DOCUMENT_IDLE, std::memory_order_release);
}

void Document::Clear()
{
	assert(status.load(std::memory_order_acquire) == DOCUMENT_EXTERNAL_LOCKING);
	// parameters.clear();
	// runtimeData.clear();
	for (size_t i = 0; i < runtimeData.size(); i++)
	{
		runtimeData[i].Clear();
	}

	OnUpdate();
}

Document::STATUS Document::GetStatus() const
{
	return (Document::STATUS)status.load(std::memory_order_acquire);
}

RuntimeData& Document::GetCurrentRuntimeData()
{
	return runtimeData[kernel.GetCurrentWarpIndex()];
}

void Document::WriteRecord(StringView object, StringView description, StringView cookie)
{
	RuntimeData& currentRuntimeData = GetCurrentRuntimeData();
	RecordData record;
	record.object = currentRuntimeData.CacheStringView(object);
	record.description = currentRuntimeData.CacheStringView(description);
	record.cookie = currentRuntimeData.CacheStringView(cookie);
	currentRuntimeData.recordDataItems.Push(std::move(record));

	OnUpdate();
}

void Document::WriteLog(StringView content)
{
	RuntimeData& currentRuntimeData = GetCurrentRuntimeData();
	LogData logData;
	logData.text = currentRuntimeData.CacheStringView(content);
	currentRuntimeData.logDataItems.Push(std::move(logData));

	OnUpdate();
}

void Document::OnUpdate()
{
	onUpdate(this);
}

size_t Document::GetParametersVersion() const
{
	return parametersVersion;
}

bool Document::AcquireLock()
{
	size_t expected = DOCUMENT_IDLE;
	return status.compare_exchange_strong(expected, DOCUMENT_EXTERNAL_LOCKING, std::memory_order_relaxed);
}

void Document::ReleaseLock()
{
	assert(status.load(std::memory_order_acquire) == DOCUMENT_EXTERNAL_LOCKING);
	status.store(DOCUMENT_IDLE, std::memory_order_release);
}

void Document::PreSerialize(bool save)
{
	if (!save)
	{
		ClearError();
		if (AcquireLock())
		{
			Clear();
			ReleaseLock();
		}
	}
}

void Document::PostSerialize(bool save)
{
	if (!save)
	{
		// in case loading from another computer's save
		size_t warpCount = kernel.GetWarpCount();
		if (runtimeData.size() < warpCount)
		{
			runtimeData.resize(warpCount);
		}

		InvokeReload();
	}
}

void Document::PrepareCall()
{
	callCounter.fetch_add(1, std::memory_order_acquire);
}

void Document::CompleteCall()
{
	if (callCounter.fetch_sub(1, std::memory_order_relaxed) == 1)
	{
		if (status.load(std::memory_order_acquire) == DOCUMENT_EXECUTING)
		{
			status.store(DOCUMENT_IDLE, std::memory_order_release);
			OnUpdate();
		}
	}
}

class MetaRuntimeDataPersist final : public TReflected<MetaRuntimeDataPersist, MetaStreamPersist>
{
public:
	IReflectObject* Clone() const override
	{
		return new MetaRuntimeDataPersist();
	}

	template <class T, class D>
	inline const MetaRuntimeDataPersist& FilterField(T* t, D* d) const
	{
		return *this; // do nothing
	}

	template <class T, class D>
	struct RealType
	{
		typedef MetaRuntimeDataPersist Type;
	};

	typedef MetaRuntimeDataPersist Type;

	static StringView ReadStringView(IStreamBase& streamBase, TQueueList<uint8_t>& memoryCache)
	{
		uint32_t len = 0;
		streamBase >> len;
		if (len != 0)
		{
			uint8_t* buffer = memoryCache.Allocate(len, 1);
			size_t s = len;
			streamBase.Read(buffer, s);
			return StringView(reinterpret_cast<const char*>(buffer), len);
		}
		else
		{
			return StringView(emptyString, 0);
		}
	}

	bool Read(IStreamBase& streamBase, void* ptr) const override
	{
		RuntimeData& runtimeData = *reinterpret_cast<RuntimeData*>(ptr);
		uint32_t recordDataCount;
		streamBase >> recordDataCount;

		for (uint32_t i = 0; i < recordDataCount; i++)
		{
			RecordData recordData;
			recordData.object = ReadStringView(streamBase, runtimeData.memoryCache);
			recordData.description = ReadStringView(streamBase, runtimeData.memoryCache);
			recordData.cookie = ReadStringView(streamBase, runtimeData.memoryCache);

			runtimeData.recordDataItems.Push(std::move(recordData));
		}

		uint32_t logDataCount;
		streamBase >> logDataCount;
		for (uint32_t j = 0; j < logDataCount; j++)
		{
			LogData logData;
			logData.text = ReadStringView(streamBase, runtimeData.memoryCache);
			runtimeData.logDataItems.Push(std::move(logData));
		}

		return true;
	}

	static void WriteStringView(IStreamBase& streamBase, StringView view)
	{
		size_t len = view.size();
		streamBase << verify_cast<uint32_t>(len);
		streamBase.Write(view.data(), len);
	}

	bool Write(IStreamBase& streamBase, const void* ptr) const override
	{
		const RuntimeData& runtimeData = *reinterpret_cast<const RuntimeData*>(ptr);
		streamBase << (uint32_t)runtimeData.recordDataItems.Count();

		for (TQueueList<RecordData>::const_iterator it = runtimeData.recordDataItems.begin(); it != runtimeData.recordDataItems.end(); ++it)
		{
			WriteStringView(streamBase, it->object);
			WriteStringView(streamBase, it->description);
			WriteStringView(streamBase, it->cookie);
		}

		streamBase << (uint32_t)runtimeData.logDataItems.Count();
		for (TQueueList<LogData>::const_iterator ip = runtimeData.logDataItems.begin(); ip != runtimeData.logDataItems.end(); ++ip)
		{
			WriteStringView(streamBase, ip->text);
		}

		return true;
	}

	String GetUniqueName() const override
	{
		return GetUnique()->GetBriefName();
	}
};

TObject<IReflect>& Document::operator () (IReflect& reflect)
{
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty())
	{
		ReflectProperty(pluginPath);
		ReflectProperty(parameters);
		ReflectProperty(runtimeData)[MetaRuntimeDataPersist()];
	}

	return *this;
}

ProfileData::ProfileData() : ratio(0) {}
