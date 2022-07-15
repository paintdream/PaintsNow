#include "ZArchive7Z.h"
#include "../../../../Core/System/MemoryStream.h"

using namespace PaintsNow;

// Charset settings. From LZMA SDK
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

static int Buf_EnsureSize(CBuf* dest, size_t size) {
	if (dest->size >= size)
		return 1;
	Buf_Free(dest, &g_Alloc);
	return Buf_Create(dest, size, &g_Alloc);
}

#if !defined(_WIN32)

static Byte kUtf8Limits[5] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

static Bool Utf16_To_Utf8(Byte* dest, size_t* destLen, const UInt16* src, size_t srcLen) {
	size_t destPos = 0, srcPos = 0;
	for (;;) {
		unsigned numAdds;
		UInt32 value;
		if (srcPos == srcLen) {
			*destLen = destPos;
			return True;
		}
		value = src[srcPos++];
		if (value < 0x80) {
			if (dest)
				dest[destPos] = (char)value;
			destPos++;
			continue;
		}
		if (value >= 0xD800 && value < 0xE000) {
			UInt32 c2;
			if (value >= 0xDC00 || srcPos == srcLen)
				break;
			c2 = src[srcPos++];
			if (c2 < 0xDC00 || c2 >= 0xE000)
				break;
			value = (((value - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
		}
		for (numAdds = 1; numAdds < 5; numAdds++)
			if (value < (((UInt32)1) << (numAdds * 5 + 6)))
				break;
		if (dest)
			dest[destPos] = (char)(kUtf8Limits[numAdds - 1] + (value >> (6 * numAdds)));
		destPos++;
		do {
			numAdds--;
			if (dest)
				dest[destPos] = (char)(0x80 + ((value >> (6 * numAdds)) & 0x3F));
			destPos++;
		} while (numAdds != 0);
	}
	*destLen = destPos;
	return False;
}

static SRes Utf16_To_Utf8Buf(CBuf* dest, const UInt16* src, size_t srcLen) {
	size_t destLen = 0;
	Bool res;
	Utf16_To_Utf8(nullptr, &destLen, src, srcLen);
	destLen += 1;
	if (!Buf_EnsureSize(dest, destLen))
		return SZ_ERROR_MEM;
	res = Utf16_To_Utf8(dest->data, &destLen, src, srcLen);
	dest->data[destLen] = 0;
	return res ? SZ_OK : SZ_ERROR_FAIL;
}

#endif

static SRes Utf16_To_Char(CBuf* buf, const UInt16* s
#if defined(_WIN32) || defined(WIN32)
	, UINT codePage
#endif
) {
	unsigned len = 0;
	for (len = 0; s[len] != 0; len++);

#if defined(_WIN32) || defined(WIN32)
	{
		unsigned size = len * 3 + 100;
		if (!Buf_EnsureSize(buf, size))
			return SZ_ERROR_MEM;
		{
			buf->data[0] = 0;
			if (len != 0) {
				char defaultChar = '_';
				BOOL defUsed;
				unsigned numChars = 0;
				numChars = WideCharToMultiByte(codePage, 0, (LPCWCH)s, len, (char*)buf->data, size, &defaultChar, &defUsed);
				if (numChars == 0 || numChars >= size)
					return SZ_ERROR_FAIL;
				buf->data[numChars] = 0;
			}
			return SZ_OK;
		}
	}
#else
	return Utf16_To_Utf8Buf(buf, s, len);
#endif
}

// Prepare callbacks for C
extern "C" SRes StreamRead(void* p, void* buf, size_t * size) {
	CFileInStream* is = (CFileInStream*)p;
	ZArchive7Z& z = **(ZArchive7Z**)(&is->file);
	IStreamBase& stream = z.GetStream();

	return stream.Read(buf, *size) ? SZ_OK : SZ_ERROR_FAIL;
}

extern "C" SRes StreamSeek(void* p, Int64 * buf, ESzSeek origin) {
	CFileInStream* is = (CFileInStream*)p;
	ZArchive7Z& z = **(ZArchive7Z**)(&is->file);
	IStreamBase& stream = z.GetStream();
	int64_t pos = z.GetPos();

	IStreamBase::SEEK_OPTION option = IStreamBase::CUR;
	switch (origin) {
		case SZ_SEEK_CUR:
			pos += (int64_t)*buf;
			option = IStreamBase::CUR;
			break;
		case SZ_SEEK_SET:
			pos = 0;
			option = IStreamBase::BEGIN;
			break;
		case SZ_SEEK_END:
			pos = z.GetLength();
			option = IStreamBase::END;
			break;
	}

	assert((uint64_t)*buf < (uint64_t)0x100000000);
	bool ret = stream.Seek(option, (long)*buf);
	*buf = pos;
	z.SetPos(pos);
	return ret ? SZ_OK : SZ_ERROR_FAIL;
}

bool ZArchive7Z::Mount(const String& prefix, IArchive* baseArchive) {
	return false;
}

bool ZArchive7Z::Unmount(const String& prefix, IArchive* baseArchive) {
	return false;
}

String ZArchive7Z::GetFullPath(const String& path) const {
	return String(); // not supported
}

bool ZArchive7Z::Exists(const String& path) const {
	if (path.empty()) return false;
	if (!(const_cast<ZArchive7Z*>(this))->Open()) return false;

	return mapPathToID.find(path) != mapPathToID.end();
}

bool ZArchive7Z::Notify(const void* key, const String& path, const TWrapper<void, const String&, size_t>& handler) {
	return false;
}

ZArchive7Z::ZArchive7Z(IStreamBase& s, size_t len) : stream(s), pos(0), size(len), opened(false) {
	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;

	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;

	archiveStream.s.Read = StreamRead;
	archiveStream.s.Seek = StreamSeek;
	*(ZArchive7Z**)&archiveStream.file = this;

	LookToRead_CreateVTable(&lookStream, False);
	lookStream.realStream = &archiveStream.s;
	LookToRead_Init(&lookStream);
	CrcGenerateTable();

	SzArEx_Init(&db);
}

IStreamBase& ZArchive7Z::GetStream() {
	return stream;
}

int64_t ZArchive7Z::GetPos() const {
	return pos;
}

void ZArchive7Z::SetPos(int64_t s) {
	pos = s;
}

int64_t ZArchive7Z::GetLength() const {
	return size;
}

ZArchive7Z::~ZArchive7Z() {
	SzArEx_Free(&db, &allocImp);
}

inline bool ZArchive7Z::Open() {
	if (!opened) {
		if (SzArEx_Open(&db, &lookStream.s, &allocImp, &allocTempImp) != SZ_OK) {
			return false; // cannot open
		}

		UInt16* temp = nullptr;
		size_t tempSize = 0;

		for (UInt32 i = 0; i < db.NumFiles; i++) {
			size_t len;
			int isDir = SzArEx_IsDir(&db, i);
			len = SzArEx_GetFileNameUtf16(&db, i, nullptr);
			if (len > tempSize) {
				SzFree(nullptr, temp);
				tempSize = len;
				temp = (UInt16*)SzAlloc(nullptr, tempSize * sizeof(temp[0]));
				if (temp == nullptr) {
					// TODO: raise memory alloc error
					break;
				}
			}

			SzArEx_GetFileNameUtf16(&db, i, temp);

			// Convert file name
			CBuf buf;
			SRes res;

			Buf_Init(&buf);
			res = Utf16_To_Char(&buf, temp
#if defined(_WIN32) || defined(WIN32)
				, CP_OEMCP
#endif
			);
			if (res == SZ_OK) {
				String path = (const char*)buf.data;
				if (isDir) path += '/';

				mapPathToID[path] = i;
			}

			Buf_Free(&buf, &g_Alloc);
		}

		SzFree(nullptr, temp);

		opened = true;
	}

	return opened;
}

IStreamBase* ZArchive7Z::Open(const String& uri, bool write, uint64_t& length, uint64_t* lastModifiedTime) {
	if (uri.empty()) return nullptr;
	if (!Open()) return nullptr;
	if (uri[uri.size() - 1] == '/') return nullptr;

	std::unordered_map<String, uint32_t>::const_iterator p = mapPathToID.find(uri);
	if (p == mapPathToID.end()) return nullptr;

	UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
	Byte* outBuffer = nullptr; /* it must be 0 before first call for each new archive. */
	size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */
	size_t offset = 0;
	size_t outSizeProcessed = 0;

	SRes res = SzArEx_Extract(&db, &lookStream.s, (*p).second, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp, &allocTempImp);

	if (res != SZ_OK) return nullptr;

	if (lastModifiedTime != 0) {
		lastModifiedTime = 0; // Not supported.
	}

	MemoryStream* ms = new MemoryStream(outSizeProcessed);
	ms->Write(outBuffer + offset, outSizeProcessed);
	ms->Seek(IStreamBase::BEGIN, 0);
	length = outSizeProcessed;
	IAlloc_Free(&allocImp, outBuffer);
	return ms;
}

void ZArchive7Z::Query(const String& uri, const TWrapper<void, const String&>& wrapper) const {
	ZArchive7Z* z = const_cast<ZArchive7Z*>(this);
	if (!z->Open()) return;

	for (std::unordered_map<String, uint32_t>::const_iterator p = mapPathToID.begin(); p != mapPathToID.end(); ++p) {
		if (uri.empty() || uri.compare(0, uri.length(), (*p).first) == 0) {
			wrapper((*p).first);
		}
	}
}

bool ZArchive7Z::IsReadOnly() const {
	return true;
}

bool ZArchive7Z::Delete(const String& uri) {
	assert(false); // do not support this operation
	return false;
}

// Unit test
#include "../../../../Core/Driver/Archive/Dirent/ZArchiveDirent.h"
#include "../../../../Core/Driver/Thread/Pthread/ZThreadPthread.h"

struct PrintCallback {
	void Print(const String& path) {
		printf("%s\n", path.c_str());
	}
};

int ZArchive7Z::main(int argc, char* argv[]) {
	if (argc >= 2) {
		ZThreadPthread thread;
		ZArchiveDirent dir(thread, "");

		uint64_t len;
		IStreamBase* base = dir.Open(argv[1], false, len);
		if (base != nullptr) {
			ZArchive7Z z(*base, verify_cast<size_t>(len));
			PrintCallback pb;
			z.Query("", Wrap(&pb, &PrintCallback::Print));
			base->Destroy();
		}
	}

	return 0;
}
