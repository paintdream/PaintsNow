#include "stdafx.h"
using namespace PaintsNow;

static const uint64_t MAX_SCRIPT_FILE_SIZE = 64 * 1024 * 1024;
static const char* PLUGINPATH_PREFIX = "--[==[";
static const char* PLUGINPATH_POSTFIX = "]==]--";

Document::Document(Executive& e, uint32_t warp) : executive(e), mainWorker(*executive.GetWorkers()[warp]) {
	SetWarpIndex(warp);
}

Document::~Document()
{
	if (commandController)
	{
		assert(&mainWorker == &executive.GetCurrentWorker());
		IScript::Request& request = mainWorker.GetScript().GetDefaultRequest();
		request.DoLock();
		request.Dereference(commandController);
		request.UnLock();
	}
}

String Document::LoadScriptText(const String& filePath)
{
	uint64_t length = 0;
	IStreamBase* stream = executive.GetArchive().Open(filePath, false, length);
	String content;

	if (stream != nullptr)
	{
		if (length < MAX_SCRIPT_FILE_SIZE) // at most 64 MB
		{
			size_t len = verify_cast<size_t>(length);
			content.resize(len);
			stream->Read(const_cast<char*>(content.c_str()), len);
			stream->Destroy();
		}
	}

	return content;
}

bool Document::InvokeMain()
{
	uint32_t expected = DOCUMENT_IDLE;
	if (status.compare_exchange_strong(expected, DOCUMENT_EXECUTING, std::memory_order_relaxed))
	{
		executive.GetKernel().QueueRoutinePost(this, CreateTaskContextFree(Wrap(this, &Document::RoutineMain)));
		return true;
	}
	else
	{
		return false;
	}
}

bool Document::UpdateStatus(StringView stat, StringView detail, float ratio)
{
	uint32_t expected = DOCUMENT_EXECUTING;
	if (stat == "Complete")
	{
		return status.compare_exchange_strong(expected, DOCUMENT_IDLE);
	}
	else
	{
		return status.compare_exchange_strong(expected, DOCUMENT_ERROR);
	}
}

void Document::RoutineMain()
{
	if (commandController)
	{
		assert(&mainWorker == &executive.GetCurrentWorker());
		IScript::Request& request = *mainWorker.requestPool.AcquireSafe();
		request.DoLock();

		request.Push();
		request.Call(commandController, "main", this);
		request.Pop();

		request.UnLock();
		mainWorker.requestPool.ReleaseSafe(&request);
		// status.store(DOCUMENT_IDLE, std::memory_order_release);
	}
	else
	{
		status.store(DOCUMENT_ERROR, std::memory_order_release);
	}
}

bool Document::InvokeLoad()
{
	if (document.compare(0, sizeof(PLUGINPATH_PREFIX) - 1, PLUGINPATH_PREFIX) == 0)
	{
		size_t off = document.find(PLUGINPATH_POSTFIX, sizeof(PLUGINPATH_PREFIX));
		if (off != String::npos)
		{
			uint32_t expected = DOCUMENT_IDLE;
			if (status.compare_exchange_strong(expected, DOCUMENT_LOADING, std::memory_order_relaxed))
			{
				pluginPath = document.substr(sizeof(PLUGINPATH_PREFIX), off - sizeof(PLUGINPATH_PREFIX));
				executive.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &Document::RoutineLoad)));
				return true;
			}
		}
	}

	return false;
}

void Document::RoutineLoad()
{
	assert(!pluginPath.empty());
	String& text = LoadScriptText(pluginPath);
	assert(status.load(std::memory_order_acquire) == DOCUMENT_IDLE);
	if (!text.empty())
	{
		assert(&mainWorker == &executive.GetCurrentWorker());
		IScript::Request& request = mainWorker.GetScript().GetDefaultRequest();
		request.DoLock();

		if (commandController)
		{
			request.Dereference(commandController);
			commandController = IScript::Request::Ref();
		}

		IScript::Request::Ref procedure = request.Load(text);
		if (procedure)
		{
			request.Push();
			request.Call(procedure);
			request >> commandController;
			request.Dereference(procedure);
			request.Pop();

			if (commandController)
			{
				request.Push();
				request.Call(commandController, "load", document);
				request.Pop();
				request.UnLock();
				status.store(DOCUMENT_IDLE, std::memory_order_release);
			}
			else
			{
				request.UnLock();
				status.store(DOCUMENT_ERROR, std::memory_order_release);
			}
		}
		else
		{
			request.UnLock();
			status.store(DOCUMENT_ERROR, std::memory_order_release);
		}
	}
	else
	{
		status.store(DOCUMENT_ERROR, std::memory_order_release);
	}
}

bool Document::InvokeSave()
{
	uint32_t expected = DOCUMENT_IDLE;
	if (status.compare_exchange_strong(expected, DOCUMENT_SAVING, std::memory_order_relaxed))
	{
		executive.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &Document::RoutineSave)));
		return true;
	}
	else
	{
		return false;
	}
}

void Document::RoutineSave()
{
	if (commandController)
	{
		Worker& worker = executive.GetCurrentWorker();
		IScript::Request& request = worker.GetScript().GetDefaultRequest();

		request.DoLock();
		request.Push();
		String text;
		request.Call(commandController, "save", this);
		request >> text;
		request.Pop();
		request.UnLock();

		if (!text.empty())
		{
			document = String(PLUGINPATH_PREFIX) + pluginPath + PLUGINPATH_POSTFIX + text;
			status.store(DOCUMENT_IDLE, std::memory_order_release);
		}
		else
		{
			status.store(DOCUMENT_ERROR, std::memory_order_release);
		}
	}
	else
	{
		status.store(DOCUMENT_ERROR, std::memory_order_release);
	}
}

bool Document::InvokeUploadParams()
{
	uint32_t expected = DOCUMENT_IDLE;
	if (status.compare_exchange_strong(expected, DOCUMENT_UPLOAD_PARAMS, std::memory_order_relaxed))
	{
		executive.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &Document::RoutineUploadParams)));
		return true;
	}
	else
	{
		return false;
	}
}

void Document::RoutineUploadParams()
{
	Worker& worker = executive.GetCurrentWorker();
	IScript::Request& request = worker.GetScript().GetDefaultRequest();

	request.DoLock();
	request.Push();

	request << "upload";
	request << this;
	request << beginarray;
	for (size_t i = 0; i < parameters.size(); i++)
	{
		ParameterData& param = parameters[i];
		request << begintable << key("name") << param.name
			<< key("caption") << param.caption
			<< key("type") << param.type
			<< key("value") << param.value << endtable;
	}
	request << endarray;

	request.Call(commandController);
	request.Pop();
	request.UnLock();

	status.store(DOCUMENT_IDLE, std::memory_order_release);
}

bool Document::InvokeDownloadParams()
{
	uint32_t expected = DOCUMENT_IDLE;
	if (status.compare_exchange_strong(expected, DOCUMENT_DOWNLOAD_PARAMS, std::memory_order_relaxed))
	{
		executive.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &Document::RoutineDownloadParams)));
		return true;
	}
	else
	{
		return false;
	}
}

void Document::RoutineDownloadParams()
{
	Worker& worker = executive.GetCurrentWorker();
	IScript::Request& request = worker.GetScript().GetDefaultRequest();

	parameters.clear();
	request.DoLock();
	request.Push();
	IScript::Request::ArrayStart as;
	request.Call(commandController, "download", this);

	request >> as;
	parameters.resize(as.count);
	for (size_t i = 0; i < as.count; i++)
	{
		ParameterData& param = parameters[i];
		request >> begintable << key("name") >> param.name
			<< key("caption") >> param.caption
			<< key("type") >> param.type
			<< key("value") >> param.value << endtable;
	}
	request << endarray;

	request.Pop();
	request.UnLock();

	status.store(DOCUMENT_IDLE, std::memory_order_release);
}

const String& Document::GetDocument() const
{
	assert(GetStatus() == DOCUMENT_IDLE);
	return document;
}

void Document::SetDocument(const String& c)
{
	assert(GetStatus() == DOCUMENT_IDLE);
	document = c;
}

const String& Document::GetPluginPath() const
{
	assert(GetStatus() == DOCUMENT_IDLE);
	return pluginPath;
}

void Document::SetPluginPath(const String& path)
{
	assert(GetStatus() == DOCUMENT_IDLE);
	pluginPath = path;
}

const std::vector<ParameterData>& Document::GetParameters() const
{
	assert(GetStatus() == DOCUMENT_IDLE);
	return parameters;
}

void Document::SetParameters(rvalue<std::vector<ParameterData> > p)
{
	assert(GetStatus() == DOCUMENT_IDLE);
	parameters = std::move(p);
}

void Document::ClearError()
{
	uint32_t expected = DOCUMENT_ERROR;
	status.compare_exchange_strong(expected, DOCUMENT_IDLE, std::memory_order_release);
}

Document::STATUS Document::GetStatus() const
{
	return (Document::STATUS)status.load(std::memory_order_acquire);
}

void Document::WriteOutput(StringView path, StringView content)
{
	RuntimeData& slot = runtimeData[executive.GetKernel().GetCurrentWarpIndex()];
	RecordData record;
	record.path = slot.recordDataCache.New(path.size());
	record.path.Import(0, reinterpret_cast<const uint8_t*>(path.data()), path.size());
	record.content = slot.recordDataCache.New(content.size());
	record.content.Import(0, reinterpret_cast<const uint8_t*>(content.data()), content.size());

	slot.recordData.Push(record);
}

