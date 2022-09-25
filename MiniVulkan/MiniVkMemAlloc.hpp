#pragma once
#ifndef MINIVK_MINIVKMEMALLOC
#define MINIVK_MINIVKMEMALLOC
	#include "./MiniVk.hpp"
	
	namespace MINIVULKAN_NS {
		struct MiniVkVertex {
			glm::vec2 pos;
			glm::vec3 color;

			static VkVertexInputBindingDescription GetBindingDescription() {
				VkVertexInputBindingDescription bindingDescription{};
				bindingDescription.binding = 0;
				bindingDescription.stride = sizeof(MiniVkVertex);
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
				return bindingDescription;
			}

			static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() {
				std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
				attributeDescriptions[0].binding = 0;
				attributeDescriptions[0].location = 0;
				attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
				attributeDescriptions[0].offset = offsetof(MiniVkVertex, pos);

				attributeDescriptions[1].binding = 0;
				attributeDescriptions[1].location = 1;
				attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[1].offset = offsetof(MiniVkVertex, color);
				return attributeDescriptions;
			}
		};

		struct MiniVkIndexer {
			std::vector<MiniVkVertex> vertices;
			std::vector<uint32_t> indices;
		};

		struct MiniVkUniform {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
		};
		
		enum class MiniVkMemoryType { VKVMA_MEMORY_TYPE_IMAGE, VKVMA_MEMORY_TYPE_BUFFER, VKVMA_MEMORY_TYPE_NONE };

		struct MiniVkMemoryStrct {
		public:
			size_t hash = ((size_t)(this)) ^ (&typeid(MiniVkMemoryStrct))->hash_code();
			MiniVkMemoryType type = MiniVkMemoryType::VKVMA_MEMORY_TYPE_NONE;
			VkImage image = VK_NULL_HANDLE;
			VkBuffer buffer = VK_NULL_HANDLE;
			VmaAllocation memory = VK_NULL_HANDLE;
			VmaAllocationInfo description;
			VkDeviceSize size;
			VkResult result;

			bool operator==(MiniVkMemoryStrct const& memStrct) { return (*this).hash == memStrct.hash; }
		};

		class MiniVkMemAlloc : public MiniVkObject {
		private:
			MiniVkInstance& mvkLayer;
		public:
			VmaAllocator memoryAllocator = VK_NULL_HANDLE;
			std::vector<MiniVkMemoryStrct*> allocations;

			void Disposable() {
				while (!allocations.empty())
					DestroyMemory(allocations.front());

				vmaDestroyAllocator(memoryAllocator);
			}

			MiniVkMemAlloc(MiniVkInstance& mvkLayer) : mvkLayer(mvkLayer) {
				onDispose += std::callback<>(this, &MiniVkMemAlloc::Disposable);
			
				VmaVulkanFunctions vulkanFunctions = {};
				vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
				vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

				VmaAllocatorCreateInfo allocatorCreateInfo = {};
				allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
				allocatorCreateInfo.physicalDevice = mvkLayer.physicalDevice;
				allocatorCreateInfo.device = mvkLayer.logicalDevice;
				allocatorCreateInfo.instance = mvkLayer.instance;
				allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

				vmaCreateAllocator(&allocatorCreateInfo, &memoryAllocator);
			}

			MiniVkMemAlloc operator=(const MiniVkMemAlloc& memAlloc) = delete;

			void DestroyMemory(MiniVkMemoryStrct* memory) {
				auto iter = std::find(allocations.begin(), allocations.end(), memory);
				if (iter != allocations.end()) {
					switch (memory->type) {
						case MiniVkMemoryType::VKVMA_MEMORY_TYPE_BUFFER:
						vmaDestroyBuffer(memoryAllocator, memory->buffer, memory->memory);
						break;
						case MiniVkMemoryType::VKVMA_MEMORY_TYPE_IMAGE:
						vmaDestroyImage(memoryAllocator, memory->image, memory->memory);
						break;
						case MiniVkMemoryType::VKVMA_MEMORY_TYPE_NONE: break;
					}

					allocations.erase(iter);
					delete memory;
				}
			}

			MiniVkMemoryStrct* CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags) {
				MiniVkMemoryStrct* memory = new MiniVkMemoryStrct();
				memory->type = MiniVkMemoryType::VKVMA_MEMORY_TYPE_BUFFER;
				allocations.push_back(memory);

				VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
				bufCreateInfo.size = size;
				bufCreateInfo.usage = usage;
				
				VmaAllocationCreateInfo allocCreateInfo = {};
				allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
				allocCreateInfo.flags = flags;

				memory->size = size;
				memory->result = vmaCreateBuffer(memoryAllocator, &bufCreateInfo, &allocCreateInfo, &memory->buffer, &memory->memory, &memory->description);
				return allocations.back();
			}

			MiniVkMemoryStrct* CreateImage(VkDeviceSize width, VkDeviceSize height, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED) {
				MiniVkMemoryStrct* memory = new MiniVkMemoryStrct();
				memory->type = MiniVkMemoryType::VKVMA_MEMORY_TYPE_IMAGE;
				allocations.push_back(memory);

				VkImageCreateInfo imgCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
				imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
				imgCreateInfo.extent.width = static_cast<uint32_t>(width);
				imgCreateInfo.extent.height = static_cast<uint32_t>(height);
				imgCreateInfo.extent.depth = 1;
				imgCreateInfo.mipLevels = 1;
				imgCreateInfo.arrayLayers = 1;
				imgCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
				imgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imgCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

				VmaAllocationCreateInfo allocCreateInfo = {};
				allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
				allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
				allocCreateInfo.priority = 1.0f;

				memory->result = vmaCreateImage(memoryAllocator, &imgCreateInfo, &allocCreateInfo, &memory->image, &memory->memory, nullptr);
				return memory;
			}

			template<typename T>
			MiniVkMemoryStrct* CreateVertexBuffer(std::vector<T> data) {
				return CreateBuffer(data.size() * sizeof(T), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
			}

			template<typename T>
			MiniVkMemoryStrct* CreateIndexBuffer(std::vector<T> data) {
				return CreateBuffer(data.size() * sizeof(T), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
			}

			template<typename T>
			MiniVkMemoryStrct* CreateUniformBuffer(T uniformBuff) {
				return CreateBuffer(sizeof(uniformBuff), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
			}

			MiniVkMemoryStrct* CreateStagingBuffer(VkDeviceSize size) {
				return CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
					VMA_ALLOCATION_CREATE_MAPPED_BIT);
			}

			void StageMemoryData(VkQueue graphicsQueue, VkCommandPool commandPool, MiniVkMemoryStrct* memory, void* data, VkDeviceSize dataSize, VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
				MiniVkMemoryStrct* stagingBuffer = CreateStagingBuffer(memory->size);
				memcpy(stagingBuffer->description.pMappedData, data, (size_t)dataSize);
				MiniVkMemAlloc::Copy(mvkLayer, graphicsQueue, commandPool, *stagingBuffer, *memory, memory->size);
				DestroyMemory(stagingBuffer);
			}

			inline static void Copy(MiniVkInstance& mvkLayer, VkQueue graphicsQueue, VkCommandPool commandPool, MiniVkMemoryStrct srcBuffer, MiniVkMemoryStrct dstBuffer, VkDeviceSize size, VkDeviceSize srceOffset = 0, VkDeviceSize destOffset = 0) {
				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandPool = commandPool;
				allocInfo.commandBufferCount = 1;

				VkCommandBuffer commandBuffer;
				vkAllocateCommandBuffers(mvkLayer.logicalDevice, &allocInfo, &commandBuffer);

				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				vkBeginCommandBuffer(commandBuffer, &beginInfo);
				VkBufferCopy copyRegion{};
				copyRegion.srcOffset = srceOffset; // Optional
				copyRegion.dstOffset = destOffset; // Optional
				copyRegion.size = size;
				vkCmdCopyBuffer(commandBuffer, srcBuffer.buffer, dstBuffer.buffer, 1, &copyRegion);
				vkEndCommandBuffer(commandBuffer);

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffer;

				vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
				vkQueueWaitIdle(graphicsQueue);
				vkFreeCommandBuffers(mvkLayer.logicalDevice, commandPool, 1, &commandBuffer);
			}
		};
	}
#endif