/* PaintDream Open Document */

#pragma once
#ifndef __STD_TYPES__
#define __STD_TYPES__
#ifdef _MSC_VER
#if (_MSC_VER <= 1200)
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
#else
#include <stdint.h>
#endif
#else
#include <stdint.h>
#endif
#endif

#define MAX_FIELDNAME_LENGTH 256
#define MAX_TYPENAME_LENGTH 510
#define MAX_META_LENGTH (MAX_TYPENAME_LENGTH - 1)

typedef struct tagPodStream PodStream;
typedef uint32_t PodSize;

/* stream callbacks */
typedef int (*PodWriter)(const PodStream* stream, const void* base, PodSize size, void* context);
typedef int (*PodReader)(const PodStream* stream, void* base, PodSize size, void* context);
typedef PodSize (*PodLocater)(const PodStream* stream, void* context);
typedef int (*PodSeeker)(const PodStream* stream, uint8_t direct, PodSize step, void* context);

struct tagPod;
typedef uint32_t Checksum;

typedef struct tagPodField {
	Checksum type;
	uint8_t isArray;
	uint8_t isInteger;
	uint8_t isDynamic;
	uint8_t nameLength;
	uint8_t name[MAX_FIELDNAME_LENGTH];
} PodField;

typedef struct tagPodList {
	struct tagPodList* front;
	struct tagPodList* next;
	struct tagPod* node;
	PodSize offset;
	PodField field;
} PodList;

/* data processing handlers */
typedef void* (*PodLocateHandler)(void* locateContext, void** iterator, PodSize* count, void* base, char* dynamicMeta, uint32_t* metaLength, const struct tagPod** dynamicType, void* context);
typedef void* (*PodIterateHandler)(void* locateContext, void** iterator, void* context);

typedef struct tagPod {
	Checksum type;
	uint32_t fieldCount;
	PodList* subList;
	PodLocateHandler locateHandler;
	PodIterateHandler iterateHandler;
	void* locateContext;
	uint8_t id[MAX_TYPENAME_LENGTH];
} Pod;

typedef struct tagPodNode {
	struct tagPodNode* next;
	Pod* pod;
} PodNode;

struct tagPodRoot {
	PodNode* head;
};

typedef struct tagPodRoot PodRoot;

/* stream structure defination, you can put your custom handler here */
typedef struct tagPodStream {
	PodWriter writer;
	PodReader reader;
	PodSeeker seeker;
	PodLocater locater;
} PodStream;

/* return status difination */
#define POD_FROM_STREAM 0
#define POD_TO_STREAM 1
#define POD_SUCCESS 0
#define POD_ERROR_FORMAT 1
#define POD_ERROR_STREAM 2
#define POD_ERROR_MEMORY 3
#define POD_TYPE_NOT_FOUND 4
#define POD_STREAM_END 5
#define POD_MAX_TYPE_SIZE 0x100
#define POD_TYPE(size) ((Pod*)(size_t)size)
#define POD_MAX_TYPE POD_TYPE(POD_MAX_TYPE_SIZE)

/* some useful macros */
#define POD_OFFSET(f, m) (size_t)(&((f*)0)->m)
#define POD_FIELD_TYPE(f, m) POD_TYPE(sizeof(((f*)0)->m))
#define PodQuickInsert(pod, type, isInteger, field) \
	PodInsert(pod, (const uint8_t*)#field, POD_OFFSET(type, field), 0, isInteger, 0, POD_FIELD_TYPE(type, field));

/* call it before other Pod* functions */
void PodInit();
/* call it when pod is no longer used */
void PodUninit();
int PodIsPlain(const Pod* p);

/* create pod type tree root */
PodRoot* PodCreateRoot();
void PodDeleteRoot(PodRoot* root);

/* create a type and insert it to specified type tree */
Pod* PodCreate(PodRoot* root, const uint8_t* id);

/* insert a field to specified type */
void PodInsert(Pod* type, const uint8_t* name, PodSize offset, uint8_t isArray, uint8_t isInteger, uint8_t isDynamic, Pod* subType);

/* set data processing handlers for specified type */
void PodSetHandler(Pod* type, PodLocateHandler locateHandler, PodIterateHandler iterateHandler, void* locateContext);

/* delete specfied type */
void PodDelete(Pod* type);

/*
Pod* PodDuplicate(const Pod* from);
*/

/* synchronize type trees from ref to root */
int PodSyncRoot(PodRoot* root, const PodRoot* ref);

/* write data block */
int PodWriteData(const Pod* type, const PodStream* stream, const void* base, void* context);
/* write data spec information */
int PodWriteSpec(const Pod* type, const PodStream* stream, void* context);
/* write all data spec information from root */
int PodWriteSpecRoot(const PodRoot* root, const PodStream* stream, void* context);

/* read data block */
int PodParseSpec(PodRoot* root, const PodStream* stream, void* context);
/* read all data spec information to root */
int PodParseSpecRoot(PodRoot* root, const PodStream* stream, void* context);
/* read data spec information */
int PodParseData(const PodRoot* root, const PodStream* stream, void* base, void* context);

/* stock file streams */
int PodWriterFile(const PodStream* stream, const void* base, PodSize size, void* context);
int PodReaderFile(const PodStream* stream, void* base, PodSize size, void* context);
PodSize PodLocaterFile(const PodStream* stream, void* context);
int PodSeekerFile(const PodStream* stream, uint8_t direct, PodSize step, void* context);

const PodStream* PodGetStockStreamFile();
