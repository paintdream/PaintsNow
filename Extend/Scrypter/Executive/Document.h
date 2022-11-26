// Document.h
// PaintDream (paintdream@paintdream.com)
// 2022-9-27
//

#pragma once

namespace PaintsNow
{
	struct ParameterData final : public TReflected<ParameterData, IReflectObjectComplex>
	{
		TObject<IReflect>& operator () (IReflect& reflect) override;

		String name;
		String description;
		String type;
		String value;
	};

	struct RecordData
	{
		StringView object;
		StringView description;
		StringView cookie;
	};

	struct LogData
	{
		StringView text;
	};

	struct RuntimeData
	{
		RuntimeData();
		RuntimeData(rvalue<RuntimeData> rhs);
		RuntimeData(const RuntimeData& rhs);

		void Clear();
		StringView CacheStringView(StringView view);

		TQueueList<uint8_t> memoryCache;
		TQueueList<RecordData> recordDataItems;
		TQueueList<LogData> logDataItems;
	};

	struct ProfileData
	{
		ProfileData();

		String status;
		String detail;
		float ratio;
	};

	class Document final : public TReflected<Document, WarpTiny>
	{
	public:
		enum STATUS
		{
			DOCUMENT_IDLE,
			DOCUMENT_EXECUTING,
			DOCUMENT_EXTERNAL_LOCKING,
			DOCUMENT_ERROR
		};

		Document(Kernel& kernel, IArchive& archive, Worker& mainWorker, const TWrapper<void, Document*>& onUpdate);
		~Document() override;

		bool InvokeGetOperations();
		bool InvokeUploadParameters();
		bool InvokeDownloadParameters();
		bool InvokeMain();
		bool InvokeReload();
		bool InvokeOperation(size_t operationIndex, rvalue<std::vector<const RecordData*> > recordData);

		void WriteRecord(StringView path, StringView description, StringView cookie);
		void WriteLog(StringView content);
		void UpdateProfile(StringView status, StringView detail, float ratio);

		const String& GetPluginPath() const;
		void SetPluginPath(const String& path);
		size_t GetParametersVersion() const;
		const std::vector<RuntimeData>& GetRuntimeData() const;
		std::vector<ParameterData>& GetParameters();
		const std::vector<String>& GetOperations() const;
		const ProfileData& GetProfile() const;
		void ClearError();
		void Clear();

		bool AcquireLock();
		void ReleaseLock();
		void PreSerialize(bool save);
		void PostSerialize(bool save);
		void PrepareCall();
		void CompleteCall();

		STATUS GetStatus() const;
		TShared<Document> PushDocument();
		void PopDocument(const TShared<Document>& doc);
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		String LoadScriptText(const String& filePath);
		RuntimeData& GetCurrentRuntimeData();
		void OnUpdate();

	protected:
		void RoutineGetOperations();
		void RoutineUploadParameters();
		void RoutineDownloadParameters();
		void RoutineMain();
		void RoutineReload();
		void RoutineOperation(size_t operationIndex, const std::vector<const RecordData*>& recordData);

	protected:
		Kernel& kernel;
		IArchive& archive;
		Worker& mainWorker;
		std::atomic<size_t> status;
		size_t parametersVersion;
		std::vector<ParameterData> parameters;
		std::vector<RuntimeData> runtimeData;
		IScript::Request::Ref commandController;
		TWrapper<void, Document*> onUpdate;
		std::atomic<size_t> callCounter;
		std::vector<String> operations;
		String pluginPath;
		ProfileData profileData;
	};

}