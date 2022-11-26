#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "Pod.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#define MAX_INTEGER_SIZE 32

#pragma pack(push, 4)

typedef struct {
	uint8_t magic[4]; /* .DAT */
	PodSize size;
	Checksum type;
	uint16_t idLength;
	uint8_t id[MAX_TYPENAME_LENGTH];
} DataHeader;

typedef struct {
	uint8_t magic[4];
	Checksum type;
	PodSize fieldCount;
	uint16_t idLength;
	uint8_t id[MAX_TYPENAME_LENGTH];
} PodHeader;

#pragma pack(pop)

static int IsStdEndian();
static void ReverseBytes(void* buffer, uint32_t size);
static void InsertPodList(Pod* type, PodList* pl);
static int ReadData(const PodList* p, const PodStream* stream, void* base, void* context);
// static void ReadSubData(const PodList* p, const PodStream* stream, void* pos, PodSize count, void* context);
static int WriteData(const PodList* p, const PodStream* stream, const void* base, void* context);
// static void WriteSubData(const PodList* p, const PodStream* stream, const void* pos, PodSize count, void* context);
static uint8_t GetSize(const PodList* p);
static int WriteStream(const PodStream* stream, const void* pos, PodSize size, int isInteger, void* context);
static int WriteDataHeaderStream(const PodStream* stream, const DataHeader* header, void* context);
static int WritePodFieldStream(const PodStream* stream, const PodField* field, void* context);
static int WriteHeaderStream(const PodStream* stream, const PodHeader* header, void* context);
static int ReadStream(const PodStream* stream, void* base, PodSize size, int isInteger, void* context);
static int ReadDataHeaderStream(const PodStream* stream, DataHeader* header, void* context);
static int ReadHeaderStream(const PodStream* stream, PodHeader* header, void* context);
static int ReadPodFieldStream(const PodStream* stream, PodField* field, void* context);

static int SeekStream(const PodStream* stream, uint8_t direct, PodSize step, void* context);
static PodSize TellStream(const PodStream* stream, void* context);
static void WritePodField(PodField* field, PodList* list);
static uint32_t GetCRC32(uint32_t org, const uint8_t* buffer, PodSize size);
static uint32_t Reflect(uint32_t ref, char ch);
static void InitCRC32();
static const Pod* QueryType(const PodRoot* pod, const Checksum* type, const Pod* ignore);
static int EqualType(const Checksum* lhs, const Checksum* rhs);
static void GetType(const Pod* p, Checksum* type);
static void InsertToRoot(PodRoot* root, Pod* p);
static const Pod* QueryID(const PodRoot* pod, const uint8_t* id);
static PodList* QueryName(PodList* head, PodList* end, const uint8_t* name);

static int IsStdEndian() {
	static const int a = 0x01;
	return *(const char*)&a == 0x01;
}

static void ReverseBytes(void* buf, uint32_t size) {
	uint32_t i = 0;
	char ch;
	char* buffer = (char*)buf;
	
	for (i = 0; i < size / 2; i++) {
		ch = buffer[i];
		buffer[i] = buffer[size - i - 1];
		buffer[size - i - 1] = ch;
	}
}

/* Internal functions */
static PodList* QueryName(PodList* head, PodList* end, const uint8_t* name) {
	PodList* p = head;
	if (p != end) {
		do {
			if (strncmp((const char*)p->field.name, (const char*)name, MAX_FIELDNAME_LENGTH) == 0)
				return p;
			p = p->next;
		} while (p != end);
	}

	return NULL;
}

static const Pod* QueryID(const PodRoot* pod, const uint8_t* id) {
	const PodNode* p = pod->head;
	while (p != NULL) {
		if (strncmp((const char*)p->pod->id, (const char*)id, MAX_TYPENAME_LENGTH) == 0)
			return p->pod;
		p = p->next;
	}

	return NULL;
}

static void InsertToRoot(PodRoot* root, Pod* p) {
	PodNode* d = (PodNode*)malloc(sizeof(PodNode));
	if (d != NULL) {
		d->pod = p;
		d->next = root->head;
		root->head = d;
	}
}

static int EqualType(const Checksum* lhs, const Checksum* rhs) {
	return *lhs == *rhs;
}

static void GetType(const Pod* p, Checksum* type) {
	if (PodIsPlain(p)) {
		*type = sizeof(const Pod*);
	} else {
		memcpy(type, &p->type, sizeof(Checksum));
	}
}

static const Pod* QueryType(const PodRoot* pod,  const Checksum* type, const Pod* ignore) {
	Checksum cs = 0;
	const PodNode* p = pod->head;
	while (p != NULL) {
		if (p->pod != ignore) {
			GetType(p->pod, &cs);
			if (EqualType(&cs, type)) {
				return p->pod;
			}
		}

		p = p->next;
	}

	return NULL;
}

uint32_t crc32_table[256];

static uint32_t Reflect(uint32_t ref, char ch) {
	int i;
	uint32_t value = 0;
	for (i = 1; i < (ch + 1); i++) {
		if (ref & 1)
			value |= 1 << (ch - i);
		ref >>= 1;
	}
	return value;
}

static void InitCRC32() {
	uint32_t ulPolynomial = 0x04c11db7;
	uint32_t i, j;

	assert(sizeof(uint32_t) == 4);
	for (i = 0; i <= 0xFF; i++) {
		crc32_table[i] = Reflect(i, 8) << 24;
		for (j = 0; j < 8; j++) {
			crc32_table[i] = (crc32_table[i] << 1) ^ (crc32_table[i] & (1 << 31) ? ulPolynomial : 0);
		}
		crc32_table[i] = Reflect(crc32_table[i], 32);
	}
}

static uint32_t GetCRC32(uint32_t org, const uint8_t* buffer, PodSize size) {
	uint32_t crc = org ^ 0xffffffff;
	while (size-- != 0) {
		crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ *buffer++];
	}

	crc = crc ^ 0xffffffff;
	if (crc < POD_MAX_TYPE_SIZE) {
		crc |= 0x80000000;
	}

	return crc;
}

static void InsertPodList(Pod* type, PodList* pl) {
	PodField field;
	PodList* q = type->subList;
	/* Checksum old = type->type; */
	pl->front = q;

	if (q == NULL) {
		pl->front = pl->next = pl;
		/* printf("Insert Pod: %s\n", pl->field.name); */
		type->subList = pl;
	} else {
		q->front->next = pl;
		pl->front = q->front;
		pl->next = q;
		q->front = pl;
	}

	type->fieldCount++;
	WritePodField(&field, pl);

	type->type = GetCRC32(type->type, (const uint8_t*)&field, POD_OFFSET(PodField, name) + field.nameLength);
}

static void WritePodField(PodField* field, PodList* list) {
	memset(field, 0, sizeof(PodField));
	field->type = PodIsPlain(list->node) ? *(Checksum*)&list->node : list->node->type;
	field->isArray = list->field.isArray;
	field->isInteger = list->field.isInteger;
	field->isDynamic = list->field.isDynamic;
	strncpy((char*)field->name, (const char*)list->field.name, MAX_FIELDNAME_LENGTH);
	((char*)field->name)[MAX_FIELDNAME_LENGTH - 1] = '\0';
	field->nameLength = (uint16_t)strlen((const char*)field->name) + 1;
}

int PodIsPlain(const Pod* p) {
	return p < POD_MAX_TYPE;
}

static uint8_t GetSize(const PodList* p) {
	assert(p != NULL);
	assert(PodIsPlain(p->node));
	return *(uint8_t*)&p->node;
}

#define CLEAN_ITERATOR(t, iterator, context) \
	{	void* pos; \
		if (iterator != NULL) while ((pos = t->iterateHandler(t->locateContext, &iterator, context)) != NULL); }

static int ReadData(const PodList* p, const PodStream* stream, void* base, void* context) {
	void* iterator;
	void* pos;
	uint8_t* buf;
	PodSize count = 0, size = 0;
	uint32_t metaLength = 0;
	char metaData[MAX_META_LENGTH + 1];
	const PodList* org = p;
	const Pod* t = NULL;
	int status = POD_SUCCESS;

	if (p == NULL) {
		/* empty object */
		return POD_SUCCESS;
	}
 
	do {
		if (p->offset != (PodSize)-1) {
			buf = (uint8_t*)base + p->offset;
			if (PodIsPlain(p->node)) {
				assert(!p->field.isArray);
				if ((status = ReadStream(stream, buf, GetSize(p), p->field.isInteger, context)) != POD_SUCCESS)
					return status;
			} else {
				/* printf("Read %s at offset %p\n", p->field.name, (uint32_t)TellStream(stream, context)); */
				iterator = NULL;
				if ((status = ReadStream(stream, &size, sizeof(size), 1, context)) != POD_SUCCESS)
					return status;
				/* read count */
				count = 1;
				if (p->field.isArray) {
					if ((status = ReadStream(stream, &count, sizeof(count), 1, context)) != POD_SUCCESS)
						return status;
				}

				if (count != 0) {
					if (p->field.isDynamic) {
						/* read meta data */
						if ((status = ReadStream(stream, &metaLength, sizeof(metaLength), 1, context)) != POD_SUCCESS)
							return status;

						if (metaLength > MAX_META_LENGTH)
							return POD_TYPE_NOT_FOUND;

						if ((status = ReadStream(stream, metaData, metaLength, 0, context)) != POD_SUCCESS)
							return status;

						metaData[metaLength] = '\0';
					}

					t = p->node;
					pos = t->locateHandler(t->locateContext, &iterator, &count, buf, p->field.isDynamic ? metaData : NULL, p->field.isDynamic ? &metaLength : NULL, &t, context);
					if (t == NULL) {
						assert(iterator == NULL);
						return POD_ERROR_FORMAT;
					}

					if (pos != NULL && t->subList != NULL) {
						if (PodIsPlain(t->subList->node) &&
							((t->iterateHandler != NULL && !t->subList->field.isDynamic) || (IsStdEndian() || !t->subList->field.isInteger))) {
							/* accelerate plain data */
							if ((status = ReadStream(stream, (char*)pos + t->subList->offset, count * GetSize(t->subList), 0, context)) != POD_SUCCESS) {
								CLEAN_ITERATOR(t, iterator, context);
								return status;
							}
						} else {
							// assert(count == 1);
							if ((status = ReadData(t->subList, stream, pos, context)) != POD_SUCCESS) {
								CLEAN_ITERATOR(t, iterator, context);
								return status;
							}
						}
					} else if (iterator != NULL) {
						assert(t->iterateHandler != NULL);
						while ((pos = t->iterateHandler(t->locateContext, &iterator, context)) != NULL) {
							if ((status = ReadData(t->subList, stream, pos, context)) != POD_SUCCESS) {
								CLEAN_ITERATOR(t, iterator, context);
								return status;
							}
						}
					}

					CLEAN_ITERATOR(t, iterator, context);
				}
			}
		} else { /* Not found ! */
			if (PodIsPlain(p->node)) {
				if ((status = SeekStream(stream, 1, GetSize(p), context)) != POD_SUCCESS)
					return status;
			} else {
				if ((status = ReadStream(stream, &size, sizeof(size), 1, context)) != POD_SUCCESS)
					return status;

				if ((status = SeekStream(stream, 1, size, context)) != POD_SUCCESS)
					return status;
			}
		}

		p = p->next;
	} while (p != org);

	return POD_SUCCESS;
}

static int SeekStream(const PodStream* stream, uint8_t direct, PodSize step, void* context) {
	return stream->seeker(stream, direct, step, context);
}

static int ReadStream(const PodStream* stream, void* base, PodSize size, int isInteger, void* context) {
	int status = stream->reader(stream, base, size, context);
	if (status == POD_SUCCESS) {
		if (!IsStdEndian() && isInteger) {
			ReverseBytes((char*)base, (uint32_t)size);
		}
	}

	return status;
}

static int WriteStream(const PodStream* stream, const void* pos, PodSize size, int isInteger, void* context) {
	char buffer[MAX_INTEGER_SIZE];
	PodSize i;

	if (!IsStdEndian() && isInteger) {
		assert(size < MAX_INTEGER_SIZE);
		for (i = 0; i < size; i++) {
			buffer[i] = ((const char*)pos)[size - i - 1];
		}

		return stream->writer(stream, buffer, size, context);
	} else {
		return stream->writer(stream, pos, size, context);
	}
}

static int WriteDataHeaderStream(const PodStream* stream, const DataHeader* header, void* context) {
	DataHeader temp;
	if (!IsStdEndian()) {
		memcpy(&temp, header, sizeof(DataHeader));
		ReverseBytes(&temp.size, sizeof(temp.size));
		ReverseBytes(&temp.type, sizeof(temp.type));
		ReverseBytes(&temp.idLength, sizeof(temp.idLength));

		return WriteStream(stream, &temp, POD_OFFSET(DataHeader, id) + header->idLength, 0, context);
	} else {
		return WriteStream(stream, header, POD_OFFSET(DataHeader, id) + header->idLength, 0, context);
	}
}

static int WritePodFieldStream(const PodStream* stream, const PodField* field, void* context) {
	PodField temp;
	if (!IsStdEndian()) {
		memcpy(&temp, field, sizeof(PodField));
		ReverseBytes(&temp.type, sizeof(temp.type));
		ReverseBytes(&temp.nameLength, sizeof(temp.nameLength));
		return WriteStream(stream, &temp, POD_OFFSET(PodField, name) + field->nameLength, 0, context);
	} else {
		return WriteStream(stream, field, POD_OFFSET(PodField, name) + field->nameLength, 0, context);
	}
}

static int WriteHeaderStream(const PodStream* stream, const PodHeader* header, void* context) {
	PodHeader temp;
	if (!IsStdEndian()) {
		memcpy(&temp, header, sizeof(PodHeader));
		ReverseBytes(&temp.fieldCount, sizeof(temp.fieldCount));
		ReverseBytes(&temp.type, sizeof(temp.type));
		ReverseBytes(&temp.idLength, sizeof(temp.idLength));
		return WriteStream(stream, &temp, POD_OFFSET(PodHeader, id) + header->idLength, 0, context);
	} else {
		return WriteStream(stream, header, POD_OFFSET(PodHeader, id) + header->idLength, 0, context);
	}
}

static int ReadDataHeaderStream(const PodStream* stream, DataHeader* header, void* context) {
	int status = ReadStream(stream, header, POD_OFFSET(DataHeader, id), 0, context);
	if (status == POD_SUCCESS) {
		if (memcmp(header->magic, ".DAT", 4) != 0) {
			if ((status = SeekStream(stream, 0, POD_OFFSET(DataHeader, id), context)) != POD_SUCCESS)
				return status;
			return POD_ERROR_FORMAT;
		}

		if (!IsStdEndian())
			ReverseBytes(&header->idLength, sizeof(header->idLength));

		if ((status = ReadStream(stream, header->id, header->idLength, 0, context)) != POD_SUCCESS) {
			return status;
		}

		if (!IsStdEndian()) {
			ReverseBytes(&header->size, sizeof(header->size));
			ReverseBytes(&header->type, sizeof(header->type));
		}

		return POD_SUCCESS;
	} else {
		return status;
	}
}

static int ReadHeaderStream(const PodStream* stream, PodHeader* header, void* context) {
	int status = ReadStream(stream, header, POD_OFFSET(PodHeader, id), 0, context);
	if (status == POD_SUCCESS) {
		if (memcmp(header->magic, ".POD", 4) != 0) {
			SeekStream(stream, 0, POD_OFFSET(PodHeader, id), context);
			return POD_ERROR_FORMAT;
		}

		if (!IsStdEndian())
			ReverseBytes(&header->idLength, sizeof(header->idLength));

		assert(header->idLength <= MAX_TYPENAME_LENGTH);
		if ((status = ReadStream(stream, header->id, header->idLength, 0, context)) != POD_SUCCESS) {
			return status;
		}

		if (!IsStdEndian()) {
			ReverseBytes(&header->fieldCount, sizeof(header->fieldCount));
			ReverseBytes(&header->type, sizeof(header->type));
		}

		return POD_SUCCESS;
	} else {
		return status;
	}
}

static int ReadPodFieldStream(const PodStream* stream, PodField* field, void* context) {
	int status = ReadStream(stream, field, POD_OFFSET(PodField, name), 0, context);
	if (status == POD_SUCCESS) {
		if (!IsStdEndian())
			ReverseBytes(&field->nameLength, sizeof(field->nameLength));

		if ((status = ReadStream(stream, field->name, field->nameLength, 0, context)) != POD_SUCCESS) {
			return status;
		}

		assert(field->nameLength != 0);
		if (!IsStdEndian())
			ReverseBytes(&field->type, sizeof(field->type));
		return POD_SUCCESS;
	} else {
		return status;
	}
}

static PodSize TellStream(const PodStream* stream, void* context) {
	return stream->locater(stream, context);
}

static int WriteData(const PodList* p, const PodStream* stream, const void* base, void* context) {
	void* iterator;
	void* pos;
	uint8_t* buf;
	PodSize count = 0, size = 0;
	uint32_t metaLength = 0;
	char metaData[MAX_META_LENGTH + 1];
	const PodList* org = p;
	const Pod* t = NULL;
	int status = POD_SUCCESS;

	if (p == NULL) {
		/* empty object */
		return POD_SUCCESS;
	}

	do {
		if (p->offset != (PodSize)-1) { // -1 is reserved for future purpose
			buf = (uint8_t*)base + p->offset;
			if (PodIsPlain(p->node)) {
				assert(!p->field.isArray);
				if ((status = WriteStream(stream, buf, GetSize(p), p->field.isInteger, context)) != POD_SUCCESS)
					return status;
			} else {
				/* printf("Write %s at offset %p\n", p->field.name, (uint32_t)TellStream(stream, context)); */
				iterator = NULL;
				t = p->node;
				metaLength = MAX_META_LENGTH;

				count = -1;
				size = 0; /* preserve space for real size */
				if ((status = WriteStream(stream, &size, sizeof(size), 1, context)) != POD_SUCCESS) {
					CLEAN_ITERATOR(t, iterator, context);
					return status;
				}

				size = TellStream(stream, context);
				if (p->field.isArray) {
					/* count will be rewritten soon */
					if ((status = WriteStream(stream, &count, sizeof(count), 1, context)) != POD_SUCCESS) {
						CLEAN_ITERATOR(t, iterator, context);
						return status;
					}
				}

				/* read count and metas */
				pos = p->node->locateHandler(p->node->locateContext, &iterator, &count, buf, metaData, &metaLength, &t, context);
				if (t == NULL) {
					assert(iterator == NULL);
					return POD_TYPE_NOT_FOUND;
				}

				if (count != 0) {
					if (p->field.isDynamic) { /* notice here we use p [maybe pointer], not t*/
						if ((status = WriteStream(stream, &metaLength, sizeof(metaLength), 1, context)) != POD_SUCCESS) {
							CLEAN_ITERATOR(t, iterator, context);
							return status;
						}

						if ((status = WriteStream(stream, metaData, metaLength, 0, context)) != POD_SUCCESS) {
							CLEAN_ITERATOR(t, iterator, context);
							return status;
						}
					}

					if (t->subList != NULL) {
						if (pos != NULL) {
							if (PodIsPlain(t->subList->node) &&
								((t->iterateHandler != NULL && !t->subList->field.isDynamic) || (IsStdEndian() || !t->subList->field.isInteger))) {
								/* accelerate plain data */
								if ((status = WriteStream(stream, (char*)pos + t->subList->offset, count * GetSize(t->subList), 0, context)) != POD_SUCCESS) {
									CLEAN_ITERATOR(t, iterator, context);
									return status;
								}
							} else {
								// assert(count == 1);
								if ((status = WriteData(t->subList, stream, pos, context)) != POD_SUCCESS) {
									CLEAN_ITERATOR(t, iterator, context);
									return status;
								}
							}
						} else if (iterator != NULL) {
							assert(t->iterateHandler != NULL);
							while ((pos = t->iterateHandler(t->locateContext, &iterator, context)) != NULL) {
								if ((status = WriteData(t->subList, stream, pos, context)) != POD_SUCCESS) {
									CLEAN_ITERATOR(t, iterator, context);
									return status;
								}
							}
						}
					}
				}

				size = TellStream(stream, context) - size;
				if ((status = SeekStream(stream, 0, size + sizeof(size), context)) != POD_SUCCESS) {
					CLEAN_ITERATOR(t, iterator, context);
					return status;
				}

				if ((status = WriteStream(stream, &size, sizeof(size), 1, context)) != POD_SUCCESS) {
					CLEAN_ITERATOR(t, iterator, context);
					return status;
				}

				if (p->field.isArray) {
					/* rewritten */
					if ((status = WriteStream(stream, &count, sizeof(count), 1, context)) != POD_SUCCESS) {
						CLEAN_ITERATOR(t, iterator, context);
						return status;
					}

					SeekStream(stream, 1, size - sizeof(count), context);
				} else {
					SeekStream(stream, 1, size, context);
				}

				CLEAN_ITERATOR(t, iterator, context);
			}
		}

		p = p->next;
	} while (p != org);

	return POD_SUCCESS;
}

/* Basic functions */

PodRoot* PodCreateRoot() {
	PodRoot* root = (PodRoot*)malloc(sizeof(PodRoot));
	if (root != NULL) {
		memset(root, 0, sizeof(PodRoot));
	}

	return root;
}

void PodDeleteRoot(PodRoot* root) {
	PodNode* node = root->head;
	while (node != NULL) {
		node = node->next;
		PodDelete(root->head->pod);
		free(root->head);
		root->head = node;
	}

	free(root);
}

Pod* PodCreate(PodRoot* root, const uint8_t* id) {
	Pod* pod = (Pod*)malloc(sizeof(Pod));

	if (pod != NULL) {
		memset(pod, 0, sizeof(Pod));
		pod->type = 0x6174;
		strncpy((char*)pod->id, (const char*)id, MAX_TYPENAME_LENGTH);
		InsertToRoot(root, pod);
	}

	return pod;
}

void PodDelete(Pod* pod) {
	PodList* p = NULL, *q = NULL;
	assert(pod != NULL);

	p = pod->subList;
	if (p != NULL) {
		do {
			q = p;
			p = p->next;
			free(q);
		} while (p != pod->subList);
	}
	free(pod);
}

void PodInsert(Pod* type, const uint8_t* name, PodSize offset, uint8_t isArray, uint8_t isInteger, uint8_t isDynamic, Pod* subType) {
	PodList* pl;
	assert(type != NULL);
	pl = (PodList*)malloc(sizeof(PodList));

	/*
	if (!PodIsPlain(subType)) {
		assert(subType->locateHandler != NULL);
	}*/
	if (pl != NULL) {
		memset(pl, 0, sizeof(PodList));
		pl->offset = offset;
		pl->field.isArray = isArray;
		pl->field.isInteger = isInteger;
		pl->field.isDynamic = isDynamic;
		pl->field.nameLength = (uint8_t)strlen((const char*)name);
		pl->node = subType;
		strncpy((char*)pl->field.name, (const char*)name, MAX_FIELDNAME_LENGTH);
		((char*)pl->field.name)[MAX_FIELDNAME_LENGTH - 1] = '\0';
		InsertPodList(type, pl);
	}
}

void PodSetHandler(Pod* type, PodLocateHandler locateHandler, PodIterateHandler iterateHandler, void* locateContext) {
	type->locateHandler = locateHandler;
	type->iterateHandler = iterateHandler;
	type->locateContext = locateContext;
}

Pod* PodDuplicate(const Pod* from) {
	PodList* pl, *q;
	Pod* w = (Pod*)malloc(sizeof(Pod));
	if (w != NULL) {
		memcpy(w, from, sizeof(Pod));
		w->subList = NULL;
		/* copy field list */
		q = w->subList;
		if (q != NULL) {
			do {
				pl = (PodList*)malloc(sizeof(PodList));
				memcpy(pl, q, sizeof(PodList));
				if (!PodIsPlain(pl->node)) {
					pl->node = PodDuplicate(q->node);
				}

				InsertPodList(w, pl);
				q = q->next;
			} while (q != w->subList);
		}
	}

	return w;
}

int PodWriteData(const Pod* type, const PodStream* stream, const void* base, void* context) {
	PodSize size;
	DataHeader header;
	int status = POD_SUCCESS;
	/* write temp header */
	memset(&header, 0, sizeof(header));
	memcpy(header.magic, ".DAT", 4);
	header.type = type->type;
	strncpy((char*)header.id, (const char*)type->id, MAX_TYPENAME_LENGTH);
	header.idLength = (uint16_t)strlen((const char*)header.id) + 1;
	if ((status = WriteDataHeaderStream(stream, &header, context)) != POD_SUCCESS)
		return status;
	size = TellStream(stream, context);

	if ((status = WriteData(type->subList, stream, base, context)) != POD_SUCCESS)
		return status;

	size = TellStream(stream, context) - size;
	header.size = size;
	if ((status = SeekStream(stream, 0, size + POD_OFFSET(DataHeader, id) + header.idLength, context)) != POD_SUCCESS)
		return status;

	if ((status = WriteDataHeaderStream(stream, &header, context)) != POD_SUCCESS)
		return status;

	if ((status = SeekStream(stream, 1, size, context)) != POD_SUCCESS)
		return status;

	return POD_SUCCESS;
}

int PodWriteSpecRoot(const PodRoot* root, const PodStream* stream, void* context) {
	/* make reverse listed list */
	int status = POD_SUCCESS;
	PodNode* p = root->head;
	while (p != NULL) {
		if (status == POD_SUCCESS) {
			status = PodWriteSpec(p->pod, stream, context);
		}

		p = p->next;
	}

	return status;
}

int PodWriteSpec(const Pod* type, const PodStream* stream, void* context) {
	// PodSize fc = 0;
	// uint32_t crc32 = 0;
	PodHeader header;
	PodField field;
	PodList* p;
	int status;
	header.type = type->type;
	memcpy(header.magic, ".POD", 4);
	header.fieldCount = type->fieldCount;
	strncpy((char*)header.id, (const char*)type->id, MAX_TYPENAME_LENGTH);
	header.idLength = (uint16_t)strlen((const char*)header.id) + 1;
	if ((status = WriteHeaderStream(stream, &header, context)) != POD_SUCCESS)
		return status;

	p = type->subList;
	if (p != NULL) {
		do {
			WritePodField(&field, p);
			if ((status = WritePodFieldStream(stream, &field, context)) != POD_SUCCESS)
				return status;
			p = p->next;
		} while (p != type->subList);
	}

	return POD_SUCCESS;
}

int PodParseSpec(PodRoot* root, const PodStream* stream, void* context) {
	PodHeader header;
	PodField field;
	Pod* pod;
	Pod* p;
	PodSize i;
	int status = POD_SUCCESS;

	if ((status = ReadHeaderStream(stream, &header, context)) != POD_SUCCESS)
		return status;

	/*
	pod = (Pod*)QueryType(root, &header.type, NULL);
	if (pod != NULL)
		return POD_SUCCESS;*/

	/* create new pod */
	pod = PodCreate(root, header.id);
	if (pod == NULL)
		return POD_ERROR_MEMORY;

	for (i = 0; i < header.fieldCount; i++) {
		if ((status = ReadPodFieldStream(stream, &field, context)) != POD_SUCCESS)
			return status;

		if (field.type < POD_MAX_TYPE_SIZE) {
			p = POD_TYPE(field.type);
		} else {
			p = (Pod*)QueryType(root, &field.type, pod);
		}

		if (p != NULL) {
			PodInsert(pod, field.name, (PodSize)-1, field.isArray, field.isInteger, field.isDynamic, p);
		} else {
			/* sub field not exist */
		}
	}

	return POD_SUCCESS;
}

int PodSyncRoot(PodRoot* root, const PodRoot* ref) {
	PodNode* w;
	const Pod* qd;
	PodList* tp;
	PodList* t;

	w = root->head;
	while (w != NULL) {
		if (!PodIsPlain(w->pod)) {
			tp = w->pod->subList;
			qd = QueryID(ref, w->pod->id);
			if (qd != NULL) {
				/* compare list */
				PodList* tq = qd->subList;
				if (tp != NULL && tq == NULL) {
					/* not even match, clear all */
					do {
						t = tp;
						tp = tp->next;
						free(t);
					} while (tp != w->pod->subList);

					w->pod->subList = NULL;
				} else if (tp == NULL && tq != NULL) {
					/* insert dummy fields */
					do {
						PodInsert(w->pod, tq->field.name, (PodSize)-1, tq->field.isArray, tq->field.isInteger, tq->field.isDynamic, NULL); /* just null here, since we do not actually read from it. */
						tq = tq->next;
					} while (tq != qd->subList);
				} else if (tp != NULL && tq != NULL) {
					do {
						if (tq->field.nameLength == tp->field.nameLength && memcmp(tq->field.name, tp->field.name, tp->field.nameLength) == 0) { /* same */
							tp = tp->next;
							tq = tq->next;
						} else {
							t = QueryName(tp->next, w->pod->subList, tq->field.name);
							if (t == NULL) { /* data format is newer: unrecognized field, just padding it */
								PodList* v = (PodList*)malloc(sizeof(PodList));
								v->offset = (PodSize)-1;
								v->front = tp->front;
								v->next = tp;
								v->field = tp->field;
								tp->front->next = v;
								tp->front = v;
								tq = tq->next;
							} else {
								t = QueryName(tq->next, qd->subList, tp->field.name); /* data format is older ? */
								if (t == NULL) { /* not found, remove it */
									PodList* v = tp;

									if (v->front == v) { /* the only node */
										tp = w->pod->subList = NULL;
									} else {
										tp = tp->next;
										v->front->next = v->next;
										v->next->front = v->front;
										if (v == w->pod->subList) {
											w->pod->subList = v->front;
										}
									}

									free(v);
								} else {
									tp = tp->next;
									tq = t->next; /* skip unmatched fields */
								}
							}
						}
					} while (tp != w->pod->subList && tq != qd->subList);
				}
			}
		}

		w = w->next;
	}

	return POD_SUCCESS;
}

int PodParseData(const PodRoot* root, const PodStream* stream, void* base, void* context) {
	/* read .DAT */
	DataHeader header;
	const Pod* p;
	int status = POD_SUCCESS;

	if ((status = ReadDataHeaderStream(stream, &header, context)) != POD_SUCCESS)
		return status;

	/* search type */
	p = QueryType(root, &header.type, NULL);
	if (p == NULL) {
		p = QueryID(root, header.id);
	}

	if (p != NULL) {
		return ReadData(p->subList, stream, base, context);
	} else {
		return POD_TYPE_NOT_FOUND;
	}
}

int PodParseSpecRoot(PodRoot* root, const PodStream* stream, void* context) {
	int ret;
	do {
		ret = PodParseSpec(root, stream, context);
	} while (ret == POD_SUCCESS);

	return ret == POD_STREAM_END ? POD_SUCCESS : ret;
}

int PodWriterFile(const PodStream* stream, const void* base, PodSize size, void* context) {
	FILE* fp = *(FILE**)context;
	if (fwrite(base, (size_t)size, 1, fp) == 1) {
		return POD_SUCCESS;
	} else {
		return POD_ERROR_STREAM;
	}
}

int PodReaderFile(const PodStream* stream, void* base, PodSize size, void* context) {
	FILE* fp = *(FILE**)context;
	if (fread(base, (size_t)size, 1, fp) == 1) {
		return POD_SUCCESS;
	} else {
		return POD_ERROR_STREAM;
	}
}

int PodSeekerFile(const PodStream* stream, uint8_t direct, PodSize step, void* context) {
	FILE* fp = *(FILE**)context;
	if (fseek(fp, direct ? (long)step : -(long)step, SEEK_CUR) == 0) {
		return POD_SUCCESS;
	} else {
		return POD_ERROR_STREAM;
	}
}

PodSize PodLocaterFile(const PodStream* stream, void* context) {
	FILE* fp = *(FILE**)context;
	return (PodSize)ftell(fp);
}

PodStream stockFileStream;

const PodStream* PodGetStockStreamFile() {
	return &stockFileStream;
}

void PodInit() {
	InitCRC32();

	stockFileStream.reader = PodReaderFile;
	stockFileStream.writer = PodWriterFile;
	stockFileStream.seeker = PodSeekerFile;
	stockFileStream.locater = PodLocaterFile;
}

void PodUninit() {}

/* A sample */

typedef struct {
	int a;
	char* str;
	double c;
} Type;

typedef struct {
	char* str;
	double c;
	double x;
} NewType;

typedef struct {
	FILE* fp;
} Context;

static void* LocateHandlerString(void* locateContext, void** pointer, PodSize* count, void* base, char* metaData, uint32_t* metaLength, const Pod** dynamicType, void* context) {
	if (*count != (PodSize)-1) {
		*(char**)base = (char*)malloc((size_t)*count + 1);
	} else {
		*count = (PodSize)strlen(*(const char**)base);
	}

	return *(char**)base;
}

static void Write() {
	Context context;
	const PodStream* stream = PodGetStockStreamFile();
	FILE* fp;
	PodRoot* root;
	Pod* pod;
	Pod* strPod;
	Type t;
	t.a = 100;
	t.str = "Hello, world!";
	t.c = 20.0;

	root = PodCreateRoot();
	strPod = PodCreate(root, (const uint8_t*)"String");
	PodSetHandler(strPod, LocateHandlerString, NULL, NULL);
	pod = PodCreate(root, (const uint8_t*)"Type");
	PodQuickInsert(pod, Type, 1, a);
	PodInsert(pod, (const uint8_t*)"str", POD_OFFSET(Type, str), 1, 0, 0, strPod);
	PodQuickInsert(pod, Type, 0, c);

	fp = fopen("test.pod", "wb");
	context.fp = fp;
	PodWriteSpecRoot(root, stream, &context);
	PodWriteData(pod, stream, &t, &context);
	fclose(fp);
	PodDeleteRoot(root);
}

static void Read() {
	const PodStream* stream = PodGetStockStreamFile();
	Context context;
	FILE* fp;
	PodRoot* root;
	PodRoot* fileRoot;
	Pod* pod;
	Pod* strPod;
	NewType t;
	memset(&t, 0, sizeof(t));

	fileRoot = PodCreateRoot();
	root = PodCreateRoot();
	strPod = PodCreate(root, (const uint8_t*)"String");
	PodSetHandler(strPod, LocateHandlerString, NULL, NULL);
	pod = PodCreate(root, (const uint8_t*)"Type");
	PodInsert(pod, (const uint8_t*)"str", POD_OFFSET(NewType, str), 1, 0, 0, strPod);
	PodQuickInsert(pod, NewType, 0, c);
	PodQuickInsert(pod, NewType, 1, x);

	fp = fopen("test.pod", "rb");
	context.fp = fp;
	PodParseSpecRoot(fileRoot, stream, &context);
	PodSyncRoot(fileRoot, root);
	PodParseData(fileRoot, stream, &t, &context);
	fclose(fp);

	PodDeleteRoot(root);
	PodDeleteRoot(fileRoot);
}

int PodMain(void) {
	PodInit();
	Write();
	Read();
	PodUninit();
	return 0;
}
