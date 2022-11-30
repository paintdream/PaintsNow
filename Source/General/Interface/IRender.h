// IRender.h - Basic render interface
// PaintDream (paintdream@paintdream.com)
// 2019-9-19
//

#pragma once
#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IReflect.h"
#include "../../Core/Interface/IType.h"
#include "../../Core/Template/TProxy.h"
#include "../../Core/Template/TBuffer.h"
#include "../../Core/Interface/IDevice.h"
#include <string>
#include <map>

namespace PaintsNow {
	class IShader;
	class IRender;
	class pure_interface IRender : public IDevice {
	public:
		~IRender() override;
		class Device {};
		class Queue;
		class Resource {
		public:
			enum Type {
				RESOURCE_UNKNOWN = 0,
				RESOURCE_TEXTURE,
				RESOURCE_BUFFER,
				RESOURCE_SHADER,
				RESOURCE_RENDERSTATE,
				RESOURCE_RENDERTARGET,
				RESOURCE_DRAWCALL,
				RESOURCE_QUICK_DRAWCALL,
				RESOURCE_EVENT,
				RESOURCE_TRANSFER,
			};

			struct Description {
				enum Format {
					UNSIGNED_BYTE,
					UNSIGNED_SHORT, 
					HALF,
					FLOAT,
					// Following are not available in texture
					UNSIGNED_INT,
					END
				};
			};

			class BufferDescription : public Description {
			public:
				enum Usage {
					INDEX, VERTEX, INSTANCED, CONSTANT, UNIFORM, STORAGE
				};

				BufferDescription() {}

#if !defined(_MSC_VER) || _MSC_VER > 1200
				BufferDescription(BufferDescription&& rhs) = default;
				BufferDescription(const BufferDescription& rhs) = default;
				BufferDescription& operator = (const BufferDescription& rhs) = default;
#endif
				BufferDescription& operator = (rvalue<BufferDescription> rhs) {
					BufferDescription& source = rhs;
					Bytes temp;
					temp = std::move(source.data);
					*this = source;
					data = std::move(temp);

					return *this;
				}

				struct State {
					State() : usage(VERTEX), format(FLOAT), dynamic(0), component(4), stride(0), offset(0), length(0) {}
					uint32_t usage : 4;
					uint32_t format : 3;
					uint32_t dynamic : 1;
					uint8_t component;
					uint16_t stride;
					uint32_t offset;
					uint32_t length;
				};

				State state;
				Bytes data;
			};

			class TextureDescription : public Description {
			public:
				enum Dimension {
					TEXTURE_1D, TEXTURE_2D, TEXTURE_2D_CUBE, TEXTURE_3D
				};

				enum Layout {
					R, RG, RGB, RGBA, DEPTH, STENCIL, DEPTH_STENCIL, RGB10PACK, EMPTY, END
				};

				enum Sample {
					POINT, LINEAR, ANSOTRIPIC, TRILINEAR
				};

				enum Media {
					TEXTURE, // SRV
					RENDER_BUFFER, // RenderBuffer can only be used as color/depth/stencil attachment and render buffer fetch.
					RENDER_FETCH,
				};

				enum Mip {
					NOMIP, AUTOMIP, SPECMIP
				};

				enum Compress {
					NONE, BPTC, ASTC, DXT
				};

				enum Address {
					REPEAT, CLAMP, MIRROR_REPEAT, MIRROR_CLAMP
				};

				enum Block {
					BLOCK_4X4,
					BLOCK_5X4,
					BLOCK_5X5,
					BLOCK_6X5,
					BLOCK_6X6,
					BLOCK_8X5,
					BLOCK_8X6,
					BLOCK_10X5,
					BLOCK_10X6,
					BLOCK_8X8,
					BLOCK_10X8,
					BLOCK_10X10,
					BLOCK_12X10,
					BLOCK_12X12,
					BLOCK_END
				};

				TextureDescription() : dimension(0, 0, 0), frameBarrierIndex(0) {}

#if !defined(_MSC_VER) || _MSC_VER > 1200
				TextureDescription(TextureDescription&& rhs) = default;
				TextureDescription(const TextureDescription& rhs) = default;
				TextureDescription& operator = (const TextureDescription& rhs) = default;
#endif
				TextureDescription& operator = (rvalue<TextureDescription> rhs) {
					TextureDescription& source = rhs;
					Bytes temp;
					temp = std::move(source.data);
					*this = source;
					data = std::move(temp);

					return *this;
				}

				void Cleanup() {
					data.Clear();
				}
				
				struct State {
					State() : type(TEXTURE_2D), format(UNSIGNED_BYTE), sample(LINEAR), 
						layout(RGBA), addressU(REPEAT), addressV(REPEAT), addressW(REPEAT),
						mip(NOMIP), media(TEXTURE), immutable(1), attachment(0), pcf(0), srgb(0),
						compress(NONE), block(BLOCK_4X4), reserved(0) {}

					inline bool operator == (const State& rhs) const {
						return memcmp(this, &rhs, sizeof(*this)) == 0;
					}

					inline bool operator != (const State& rhs) const {
						return memcmp(this, &rhs, sizeof(*this)) != 0;
					}
					
					inline bool operator < (const State& rhs) const {
						return memcmp(this, &rhs, sizeof(*this)) < 0;
					}

					uint32_t type : 3;
					uint32_t format : 3;
					uint32_t sample : 2;
					uint32_t layout : 4;
					uint32_t addressU : 2;
					uint32_t addressV : 2;
					uint32_t addressW : 2;
					uint32_t mip : 2;
					uint32_t media : 2;
					uint32_t immutable : 1;
					uint32_t attachment : 1;
					uint32_t pcf : 1;
					uint32_t srgb : 1;
					uint32_t compress : 2;
					uint32_t block : 4;
					uint32_t reserved;
				};

				Bytes data;
				UShort3 dimension; // width, height, depth
				uint16_t frameBarrierIndex;
				State state;
			};

			class ShaderDescription : public Description {
			public:
				enum Stage {
					GLOBAL, VERTEX, TESSELLATION_CONTROL, TESSELLATION_EVALUATION, GEOMETRY, FRAGMENT, COMPUTE, END
				};

				String name;
				void* context;
				void* instance;
				TWrapper<void, Resource*, ShaderDescription&, Stage, const String&, const String&> compileCallback;
				std::vector<std::pair<Stage, IShader* > > entries;
			};

			// Executable
			struct RenderStateDescription : public Description {
				RenderStateDescription() { memset(this, 0, sizeof(*this)); }
				enum Test {
					DISABLED, NEVER, LESS, EQUAL, LESS_EQUAL, GREATER, GREATER_EQUAL, ALWAYS
				};

				inline bool operator < (const RenderStateDescription& rhs) const {
					return memcmp(this, &rhs, sizeof(*this)) < 0;
				}

				inline bool operator == (const RenderStateDescription& rhs) const {
					return memcmp(this, &rhs, sizeof(*this)) == 0;
				}

				inline bool operator != (const RenderStateDescription& rhs) const {
					return memcmp(this, &rhs, sizeof(*this)) != 0;
				}

				uint32_t stencilReplacePass : 1;
				uint32_t stencilReplaceFail : 1;
				uint32_t stencilReplaceZFail : 1;
				uint32_t cullFrontFace : 1;
				uint32_t cull : 1;
				uint32_t fill : 1;
				uint32_t blend : 1;
				uint32_t colorWrite : 1;

				uint32_t depthTest : 3;
				uint32_t depthWrite : 1;
				uint32_t stencilTest : 3;
				uint32_t stencilWrite : 1;
				uint32_t stencilMask : 8;
				uint32_t stencilValue : 8;
			};

			class RenderTargetDescription : public Description {
			public:
				RenderTargetDescription() : range(UShort2(0, 0), UShort2(0, 0)), reserved(0) {}

				// attachments
				UShort2Pair range;
				UShort3 dimension; // width, height, depth
				short reserved;

				enum {
					DEFAULT,
					DISCARD,
					CLEAR
				};

				struct Storage {
					Storage() : loadOp(DEFAULT), storeOp(DEFAULT), mipLevel(0), layer(0), resource(nullptr), clearColor(0, 0, 0, 0) {}

					uint8_t loadOp : 4;
					uint8_t storeOp : 4;
					uint8_t mipLevel; // used for render to mip
					uint16_t layer; // used for render to 3d texture layer
					Resource* resource;
					Float4 clearColor;
				};

				Storage depthStorage;
				Storage stencilStorage;
				std::vector<Storage> colorStorages;
			};
			
			class DrawCallDescription : public Description {
			public:
				struct BufferRange {
#ifdef _DEBUG
					BufferRange() { memset(this, 0xcc, sizeof(*this)); }
#endif
					Resource* buffer;
					uint32_t offset;
					uint32_t length;
					uint16_t component;
					uint16_t type;
				};

				DrawCallDescription() : shaderResource(nullptr), instanceCounts(0, 0, 0) {}

#if !defined(_MSC_VER) || _MSC_VER > 1200
				DrawCallDescription(DrawCallDescription&& rhs) = default;
				DrawCallDescription(const DrawCallDescription& rhs) = default;
				DrawCallDescription& operator = (const DrawCallDescription& rhs) = default;
#endif
				DrawCallDescription& operator = (rvalue<DrawCallDescription> rhs) {
					DrawCallDescription& source = rhs;
					std::vector<BufferRange> tempBuffer;
					std::swap(tempBuffer, source.bufferResources);
					std::vector<Resource*> tempTexture;
					std::swap(tempTexture, source.textureResources);
					*this = source;
					std::swap(bufferResources, tempBuffer);
					std::swap(textureResources, tempTexture);

					return *this;
				}

				void Cleanup() {
					instanceCounts = UInt3(0, 0, 0);
					shaderResource = nullptr;
					textureResources.clear();
					bufferResources.clear();
				}

				Resource* shaderResource;
				UInt3 instanceCounts; // y/z for compute shaders
				
				BufferRange indexBufferResource;
				std::vector<BufferRange> bufferResources;
				std::vector<Resource*> textureResources;
			};

			// frequently used, design as trivially constructible, copyable and destructible
			struct QuickDrawCallDescription : public Description {
				// blob layout
				// Resource* shaderResource;
				// Resource* textureResources[textureCount];
				// BufferRange indexBufferResource;
				// BufferRange bufferResources[bufferCount];

				typedef DrawCallDescription::BufferRange BufferRange;

				inline Resource*& GetShader() { return *reinterpret_cast<Resource**>(blobData); }
				inline Resource** GetTextures() { return reinterpret_cast<Resource**>(reinterpret_cast<char*>(blobData) + sizeof(Resource*)); }
				inline BufferRange* GetIndexBuffer() { return reinterpret_cast<BufferRange*>(reinterpret_cast<char*>(blobData) + sizeof(Resource*) * (textureCount + 1)); }
				inline BufferRange* GetBuffers() { return reinterpret_cast<BufferRange*>(reinterpret_cast<char*>(blobData) + sizeof(Resource*) * (textureCount + 1) + sizeof(BufferRange)); }
				inline uint32_t GetSize() const { return sizeof(Resource*) * (textureCount + 1) + sizeof(BufferRange) * (bufferCount + 1); }

				void* blobData;
				uint8_t textureCount;
				uint8_t bufferCount;
				uint16_t instanceCount;
			};

			class EventDescription : public Description {
			public:
				EventDescription() : counter(0) {}
				size_t counter;
				TWrapper<void, IRender&, Queue*, Resource*, bool> eventCallback;
			};

			class TransferDescription : public Description {
			public:
				Resource* source;
				Resource* target;
				uint16_t copyColor : 1;
				uint16_t copyDepth : 1;
				uint16_t copyStencil : 1;
				uint16_t sample : 2;
				uint16_t reserved : 3;
				uint8_t sourceMipLevel;
				uint8_t targetMipLevel;
				uint16_t sourceIndex;
				uint16_t targetIndex;
				uint16_t sourceBaseArrayLayer;
				uint16_t targetBaseArrayLayer;
				uint16_t sourceLayerCount;
				uint16_t targetLayerCount;
				Int2Pair sourceRegion;
				Int2Pair targetRegion;
			};
		};

		class Queue : public Resource {};

		enum AccessFlags { // map to vulkan!
			ACCESS_INDIRECT_COMMAND_READ_BIT = 0x00000001,
			ACCESS_INDEX_READ_BIT = 0x00000002,
			ACCESS_VERTEX_ATTRIBUTE_READ_BIT = 0x00000004,
			ACCESS_UNIFORM_READ_BIT = 0x00000008,
			ACCESS_INPUT_ATTACHMENT_READ_BIT = 0x00000010,
			ACCESS_SHADER_READ_BIT = 0x00000020,
			ACCESS_SHADER_WRITE_BIT = 0x00000040,
			ACCESS_COLOR_ATTACHMENT_READ_BIT = 0x00000080,
			ACCESS_COLOR_ATTACHMENT_WRITE_BIT = 0x00000100,
			ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT = 0x00000200,
			ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = 0x00000400,
			ACCESS_TRANSFER_READ_BIT = 0x00000800,
			ACCESS_TRANSFER_WRITE_BIT = 0x00001000,
			ACCESS_HOST_READ_BIT = 0x00002000,
			ACCESS_HOST_WRITE_BIT = 0x00004000,
			ACCESS_MEMORY_READ_BIT = 0x00008000,
			ACCESS_MEMORY_WRITE_BIT = 0x00010000,
			ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT = 0x02000000,
			ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT = 0x04000000,
			ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT = 0x08000000,
			ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT = 0x00100000,
			ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT = 0x00080000,
			ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR = 0x00200000,
			ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR = 0x00400000,
			ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV = 0x00800000,
			ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT = 0x01000000,
			ACCESS_COMMAND_PREPROCESS_READ_BIT_NV = 0x00020000,
			ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV = 0x00040000,
		};

		enum ImageLayout { // also copy from vulkan
			LAYOUT_UNDEFINED = 0,
			LAYOUT_GENERAL,
			LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			LAYOUT_TRANSFER_SRC_OPTIMAL,
			LAYOUT_TRANSFER_DST_OPTIMAL,
			LAYOUT_PREINITIALIZED,
			LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = 1000117000,
			LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = 1000117001,
			LAYOUT_DEPTH_ATTACHMENT_OPTIMAL = 1000241000,
			LAYOUT_DEPTH_READ_ONLY_OPTIMAL = 1000241001,
			LAYOUT_STENCIL_ATTACHMENT_OPTIMAL = 1000241002,
			LAYOUT_STENCIL_READ_ONLY_OPTIMAL = 1000241003,
			LAYOUT_PRESENT_SRC_KHR = 1000001002,
			LAYOUT_SHARED_PRESENT_KHR = 1000111000,
			LAYOUT_SHADING_RATE_OPTIMAL_NV = 1000164003,
			LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT = 1000218000,
			LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR = LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
			LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR = LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
			LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR = LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR = LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
			LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR = LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,
			LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR = LAYOUT_STENCIL_READ_ONLY_OPTIMAL,
			LAYOUT_MAX_ENUM = 0x7FFFFFFF
		};

		enum AspectFlagBits {
			ASPECT_COLOR_BIT = 0x00000001,
			ASPECT_DEPTH_BIT = 0x00000002,
			ASPECT_STENCIL_BIT = 0x00000004,
			ASPECT_METADATA_BIT = 0x00000008,
			ASPECT_PLANE_0_BIT = 0x00000010,
			ASPECT_PLANE_1_BIT = 0x00000020,
			ASPECT_PLANE_2_BIT = 0x00000040,
			ASPECT_MEMORY_PLANE_0_BIT_EXT = 0x00000080,
			ASPECT_MEMORY_PLANE_1_BIT_EXT = 0x00000100,
			ASPECT_MEMORY_PLANE_2_BIT_EXT = 0x00000200,
			ASPECT_MEMORY_PLANE_3_BIT_EXT = 0x00000400,
			ASPECT_PLANE_0_BIT_KHR = ASPECT_PLANE_0_BIT,
			ASPECT_PLANE_1_BIT_KHR = ASPECT_PLANE_1_BIT,
			ASPECT_PLANE_2_BIT_KHR = ASPECT_PLANE_2_BIT,
			ASPECT_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
		};

		enum PipelineStageBits {
			PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0x00000001,
			PIPELINE_STAGE_DRAW_INDIRECT_BIT = 0x00000002,
			PIPELINE_STAGE_VERTEX_INPUT_BIT = 0x00000004,
			PIPELINE_STAGE_VERTEX_SHADER_BIT = 0x00000008,
			PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT = 0x00000010,
			PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT = 0x00000020,
			PIPELINE_STAGE_GEOMETRY_SHADER_BIT = 0x00000040,
			PIPELINE_STAGE_FRAGMENT_SHADER_BIT = 0x00000080,
			PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT = 0x00000100,
			PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT = 0x00000200,
			PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = 0x00000400,
			PIPELINE_STAGE_COMPUTE_SHADER_BIT = 0x00000800,
			PIPELINE_STAGE_TRANSFER_BIT = 0x00001000,
			PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x00002000,
			PIPELINE_STAGE_HOST_BIT = 0x00004000,
			PIPELINE_STAGE_ALL_GRAPHICS_BIT = 0x00008000,
			PIPELINE_STAGE_ALL_COMMANDS_BIT = 0x00010000,
		};

		enum DependencyFlagBits {
			DEPENDENCY_DEFAULT = 0,
			DEPENDENCY_BY_REGION_BIT = 0x00000001,
			DEPENDENCY_VIEW_LOCAL_BIT = 0x00000002,
			DEPENDENCY_DEVICE_GROUP_BIT = 0x00000004,
		};

		class Barrier {
		public:
			Barrier() : resource(nullptr), srcAccessMask(ACCESS_MEMORY_READ_BIT), dstAccessMask(ACCESS_MEMORY_WRITE_BIT) {}

			Resource* resource;
			AccessFlags srcAccessMask;
			AccessFlags dstAccessMask;
			PipelineStageBits srcStageMask;
			PipelineStageBits dstStageMask;
			DependencyFlagBits dependencyMask;

			union {
				// buffer spec
				struct {
					size_t offset;
					size_t size;
				};

				// texture spec
				struct {
					ImageLayout oldLayout;
					ImageLayout newLayout;
					AspectFlagBits aspectMask;
					uint32_t baseMipLevel;
					uint32_t levelCount;
					uint32_t baseArrayLayer;
					uint32_t layerCount;
				};
			};
		};

		// The only API that requires calling on device thread.
		enum SubmitOption {
			SUBMIT_EXECUTE_ALL,
			SUBMIT_EXECUTE_CONSUME,
			SUBMIT_EXECUTE_REPEAT,
			SUBMIT_CLEAR_ALL
		};

		virtual void SubmitQueues(Queue** queues, uint32_t count, SubmitOption option) = 0;

		// Device
		virtual std::vector<String> EnumerateDevices() = 0;
		virtual Device* CreateDevice(const String& description) = 0;
		virtual size_t GetProfile(Device* device, const String& feature) = 0;
		virtual Int2 GetDeviceResolution(Device* device) = 0;
		virtual void SetDeviceResolution(Device* device, const Int2& resolution) = 0;
		virtual bool PreDeviceFrame(Device* device) = 0;
		virtual void PostDeviceFrame(Device* device) = 0;
		virtual void DeleteDevice(Device* device) = 0;

		// Queue
		enum QueueFlag {
			QUEUE_REPEATABLE = 1 << 0,
			QUEUE_MULTITHREAD = 1 << 1,
			QUEUE_SECONDARY = 1 << 2
		};

		virtual Queue* CreateQueue(Device* device, uint32_t flag = 0) = 0;
		virtual Device* GetQueueDevice(Queue* queue) = 0;
		virtual void DeleteQueue(Queue* queue) = 0;
		virtual void FlushQueue(Queue* queue) = 0;
		virtual bool IsQueueEmpty(Queue* queue) = 0;

		// Resource
		enum MapFlag {
			MAP_DATA_EXCHANGE = 1 << 0
		};

		virtual Resource* CreateResource(Device* device, Resource::Type resourceType, Queue* optionalHostQueue = nullptr) = 0;
		virtual const void* GetResourceDeviceHandle(Resource* resource) = 0;
		virtual void DeleteResource(Queue* queue, Resource* resource) = 0; // must delete resource on a queue
		virtual Resource::Description* MapResource(Queue* queue, Resource* resource, uint32_t mapFlag) = 0;
		virtual void UnmapResource(Queue* queue, Resource* resource, uint32_t mapFlag) = 0;
		virtual void SetupBarrier(Queue* queue, Barrier* barrier) = 0;
		virtual void ExecuteResource(Queue* queue, Resource* resource) = 0;
		virtual void SetResourceNote(Resource* lhs, const String& note) = 0;
	};
}

