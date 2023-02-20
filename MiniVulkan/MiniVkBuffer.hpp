#ifndef MINIVULKAN_MINIVKBUFFER
#define MINIVULKAN_MINIVKBUFFER
	#include "./MiniVK.hpp"

	namespace MINIVULKAN_NAMESPACE {
		/*
			ABVOUT BUFFERS & IMAGES:
				When Creating Buffers:
					Buffers must be initialized with a VkDviceSize, which is the size of the data in BYTES, not the
					size of the data container (number of items). This same principle applies to stagging buffer data.

				There are 3 types of GPU dedicated memory buffers:
					Vertex:		Allows you to send mesh triangle data to the GPU.
					Index:		Allws you to send mapped indices for vertex buffers to the GPU.
					Uniform:	Allows you to send data to shaders using uniforms.
						* Push Constants are an alternative that do not require buffers, simply use: vkCmdPushConstants(...).

				The last buffer type is a CPU memory buffer for transfering data from the CPU to the GPU:
					Staging:	Staging CPU data for transfer to the GPU.

				Render images are for rendering sprites or textures on the GPU (similar to the swap chain, but handled manually).
					The default image layout is: VK_IMAGE_LAYOUT_UNDEFINED
					To render to shaders you must change/transition the layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
					Once the layout is set for transfering you can write data to the image from CPU memory to GPU memory.
					Finally for use in shaders you need to change the layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
		*/

		enum class MiniVkBufferType {
			VKVMA_BUFFER_TYPE_VERTEX,	/// For passing mesh/triangle vertices for rendering to shaders.
			VKVMA_BUFFER_TYPE_INDEX,	/// For indexing Vertex information in Vertex Buffers.
			VKVMA_BUFFER_TYPE_UNIFORM,	/// For passing uniform/shader variable data to shaders (not used with push descriptors).
			VKVMA_BUFFER_TYPE_STAGING,	/// For tranfering CPU bound buffer data to the GPU.
			VKVMA_BUFFER_TYPE_INDIRECT	/// For writing VkIndirectCommand's to a buffer for Indirect drawing.
		};

		class MiniVkBuffer : public MiniVkObject {
		private:
			void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags) {
				VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
				bufCreateInfo.size = size;
				bufCreateInfo.usage = usage;

				VmaAllocationCreateInfo allocCreateInfo = {};
				allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
				allocCreateInfo.flags = flags;
				
				if (vmaCreateBuffer(vmAlloc.GetAllocator(), &bufCreateInfo, &allocCreateInfo, &buffer, &memory, &description) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Could not allocate memory for MiniVkBuffer!");
			}
		
		public:
			MiniVkRenderDevice& renderDevice;
			MiniVkVMAllocator& vmAlloc;

			VkBuffer buffer = VK_NULL_HANDLE;
			VmaAllocation memory = VK_NULL_HANDLE;
			VmaAllocationInfo description;
			VkDeviceSize size;

			void Disposable() {
				vmaDestroyBuffer(vmAlloc.GetAllocator(), buffer, memory);
			}

			MiniVkBuffer(MiniVkRenderDevice& renderDevice, MiniVkVMAllocator& vmAlloc, VkDeviceSize dataSize, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags)
			: renderDevice(renderDevice), vmAlloc(vmAlloc), size(dataSize) {
				onDispose += MiniVkCallback<>(this, &MiniVkBuffer::Disposable);
				CreateBuffer(size, usage, flags);
			}

			MiniVkBuffer(MiniVkRenderDevice& renderDevice, MiniVkVMAllocator& vmAlloc, VkDeviceSize dataSize, MiniVkBufferType type)
			: renderDevice(renderDevice), vmAlloc(vmAlloc), size(dataSize) {
				onDispose += MiniVkCallback<>(this, &MiniVkBuffer::Disposable);

				switch (type) {
					case MiniVkBufferType::VKVMA_BUFFER_TYPE_VERTEX:
					CreateBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
					break;
					case MiniVkBufferType::VKVMA_BUFFER_TYPE_INDEX:
					CreateBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
					break;
					case MiniVkBufferType::VKVMA_BUFFER_TYPE_UNIFORM:
					CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
					break;
					case MiniVkBufferType::VKVMA_BUFFER_TYPE_INDIRECT:
					CreateBuffer(size, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
					break;
					case MiniVkBufferType::VKVMA_BUFFER_TYPE_STAGING:
					default:
					CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
					break;
				}
			}

			void StageBufferData(VkQueue graphicsQueue, VkCommandPool commandPool, void* data, VkDeviceSize dataSize, VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
				MiniVkBuffer stagingBuffer = MiniVkBuffer(renderDevice, vmAlloc, dataSize, MiniVkBufferType::VKVMA_BUFFER_TYPE_STAGING);
				memcpy(stagingBuffer.description.pMappedData, data, (size_t)dataSize);
				TransferBufferCmd(graphicsQueue, commandPool, stagingBuffer, size, srcOffset, dstOffset);
				stagingBuffer.Dispose();
			}

			void TransferBufferCmd(VkQueue graphicsQueue, VkCommandPool commandPool, MiniVkBuffer& srcBuffer, VkDeviceSize dataSize, VkDeviceSize srceOffset = 0, VkDeviceSize destOffset = 0) {
				VkCommandBuffer commandBuffer = BeginTransferCmd(graphicsQueue, commandPool);

				VkBufferCopy copyRegion{};
				copyRegion.srcOffset = srceOffset;
				copyRegion.dstOffset = destOffset;
				copyRegion.size = dataSize;
				vkCmdCopyBuffer(commandBuffer, srcBuffer.buffer, buffer, 1, &copyRegion);

				EndTransferCmd(graphicsQueue, commandPool, commandBuffer);
			}

			VkCommandBuffer BeginTransferCmd(VkQueue graphicsQueue, VkCommandPool commandPool) {
				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandPool = commandPool;
				allocInfo.commandBufferCount = 1;

				VkCommandBuffer commandBuffer;
				vkAllocateCommandBuffers(renderDevice.logicalDevice, &allocInfo, &commandBuffer);

				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				vkBeginCommandBuffer(commandBuffer, &beginInfo);
				return commandBuffer;
			}

			void EndTransferCmd(VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
				vkEndCommandBuffer(commandBuffer);

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffer;

				vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
				vkQueueWaitIdle(graphicsQueue);
				vkFreeCommandBuffers(renderDevice.logicalDevice, commandPool, 1, &commandBuffer);
			}
		};
	}
#endif