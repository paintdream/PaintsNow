// File.h
// PaintDream (paintdream@paintdream.com)
// 2022-9-27
//

#pragma once

namespace PaintsNow
{
	class Document;
	class File final : public TReflected<File, WarpTiny>
	{
	public:
		File(Document* document, HANDLE file, HANDLE iocpHandle, uint32_t bufferSize);
		~File() override;
		HANDLE GetHandle() const;
		const char* GetBuffer() const;
		char* GetBuffer();
		uint64_t GetSize() const;
		uint32_t GetBufferSize() const;
		OVERLAPPED* GetOverlapped();
		bool Close();
		bool Acquire();
		bool Release();

		static File* FromOverlapped(OVERLAPPED* overlapped);
		IScript::Request::Ref GetCallback() const;
		void SetCallback(const IScript::Request::Ref& callback);
		Document* GetDocument() const;

	protected:
		File(const File& file);

		TShared<Document> document;
		HANDLE fileHandle;
		HANDLE iocpHandle;
		OVERLAPPED overlapped;
		IScript::Request::Ref callback;
		String buffer;
	};
}