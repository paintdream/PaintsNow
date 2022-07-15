// Document.h
// PaintDream (paintdream@paintdream.com)
// 2022-9-27
//

#pragma once

namespace PaintsNow
{
	class Executive;
	struct ParameterData
	{
		String name;
		String caption;
		String type;
		String value;
	};

	struct RecordData
	{
		Bytes path;
		Bytes content;
	};

	struct RuntimeData
	{
		BytesCache recordDataCache;
		TQueueList<RecordData> recordData;
	};

	class Document : public TReflected<Document, WarpTiny>
	{
	public:
		enum STATUS
		{
			DOCUMENT_IDLE,
			DOCUMENT_CREATING,
			DOCUMENT_LOADING,
			DOCUMENT_SAVING,
			DOCUMENT_EXECUTING,
			DOCUMENT_UPLOAD_PARAMS,
			DOCUMENT_DOWNLOAD_PARAMS,
			DOCUMENT_ERROR
		};

		Document(Executive& executive, uint32_t warp);
		~Document() override;

		bool InvokeLoad();
		bool InvokeSave();
		bool InvokeUploadParams();
		bool InvokeDownloadParams();
		bool InvokeMain();
		void WriteOutput(StringView path, StringView content);

		const String& GetPluginPath() const;
		void SetPluginPath(const String& path);
		const String& GetDocument() const;
		void SetDocument(const String& content);
		const std::vector<ParameterData>& GetParameters() const;
		void SetParameters(rvalue<std::vector<ParameterData> >);
		void ClearError();

		STATUS GetStatus() const;
		bool UpdateStatus(StringView status, StringView detail, float ratio);

	protected:
		String LoadScriptText(const String& filePath);

	protected:
		void RoutineLoad();
		void RoutineSave();
		void RoutineUploadParams();
		void RoutineDownloadParams();
		void RoutineMain();

	protected:
		Executive& executive;
		Worker& mainWorker;
		std::atomic<size_t> status;
		std::vector<ParameterData> parameters;
		std::vector<RuntimeData> runtimeData;
		IScript::Request::Ref commandController;
		String pluginPath;
		String document;
	};

}