#ifndef TINYVK_TINYVKBUFFER
#define TINYVK_TINYVKBUFFER
	#include "./TinyVK.hpp"

	namespace TINYVULKAN_NAMESPACE {
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

		enum class TinyVkBufferType {
			VKVMA_BUFFER_TYPE_VERTEX,	/// For passing mesh/triangle vertices for rendering to shaders.
			VKVMA_BUFFER_TYPE_INDEX,	/// For indexing Vertex information in Vertex Buffers.
			VKVMA_BUFFER_TYPE_UNIFORM,	/// For passing uniform/shader variable data to shaders.
			VKVMA_BUFFER_TYPE_STAGING,	/// For tranfering CPU bound buffer data to the GPU.
			VKVMA_BUFFER_TYPE_INDIRECT,	/// For writing VkIndirectCommand's to a buffer for Indirect drawing.
		};

		class TinyVkBuffer : public disposable {
		private:
			void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags) {
				VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
				bufCreateInfo.size = size;
				bufCreateInfo.usage = usage;

				VmaAllocationCreateInfo allocCreateInfo = {};
				allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
				allocCreateInfo.flags = flags;
				
				if (vmaCreateBuffer(vmAlloc.GetAllocator(), &bufCreateInfo, &allocCreateInfo, &buffer, &memory, &description) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Could not allocate memory for TinyVkBuffer!");
			}
		
		public:
			std::timed_mutex buffer_lock;

			TinyVkRenderDevice& renderDevice;
			TinyVkGraphicsPipeline& graphicsPipeline;
			TinyVkCommandPool& commandPool;
			TinyVkVMAllocator& vmAlloc;

			VkBuffer buffer = VK_NULL_HANDLE;
			VmaAllocation memory = VK_NULL_HANDLE;
			VmaAllocationInfo description;
			VkDeviceSize size;

			~TinyVkBuffer() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(renderDevice.logicalDevice);

				vmaDestroyBuffer(vmAlloc.GetAllocator(), buffer, memory);
			}

			TinyVkBuffer(TinyVkRenderDevice& renderDevice, TinyVkGraphicsPipeline& graphicsPipeline, TinyVkCommandPool& commandPool, TinyVkVMAllocator& vmAlloc, VkDeviceSize dataSize, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags)
			: renderDevice(renderDevice), graphicsPipeline(graphicsPipeline), commandPool(commandPool), vmAlloc(vmAlloc), size(dataSize) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));

				CreateBuffer(size, usage, flags);
			}

			TinyVkBuffer(TinyVkRenderDevice& renderDevice, TinyVkGraphicsPipeline& graphicsPipeline, TinyVkCommandPool& commandPool, TinyVkVMAllocator& vmAlloc, VkDeviceSize dataSize, TinyVkBufferType type)
			: renderDevice(renderDevice), graphicsPipeline(graphicsPipeline), commandPool(commandPool), vmAlloc(vmAlloc), size(dataSize) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));

				switch (type) {
					case TinyVkBufferType::VKVMA_BUFFER_TYPE_VERTEX:
					CreateBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
					break;
					case TinyVkBufferType::VKVMA_BUFFER_TYPE_INDEX:
					CreateBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
					break;
					case TinyVkBufferType::VKVMA_BUFFER_TYPE_UNIFORM:
					CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
					break;
					case TinyVkBufferType::VKVMA_BUFFER_TYPE_INDIRECT:
					CreateBuffer(size, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
					break;
					case TinyVkBufferType::VKVMA_BUFFER_TYPE_STAGING:
					default:
					CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
					break;
				}
			}

			/// <summary>Copies data from CPU accessible memory to GPU accessible memory.</summary>
			void StageBufferData(void* data, VkDeviceSize dataSize, VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
				TinyVkBuffer stagingBuffer = TinyVkBuffer(renderDevice, graphicsPipeline, commandPool, vmAlloc, dataSize, TinyVkBufferType::VKVMA_BUFFER_TYPE_STAGING);
				memcpy(stagingBuffer.description.pMappedData, data, (size_t)dataSize);
				TransferBufferCmd(stagingBuffer, size, srcOffset, dstOffset);
				stagingBuffer.Dispose();
			}

			/// <summary>Copies data from the source TinyVkBuffer into this TinyVkBuffer.</summary>
			void TransferBufferCmd(TinyVkBuffer& srcBuffer, VkDeviceSize dataSize, VkDeviceSize srceOffset = 0, VkDeviceSize destOffset = 0) {
				std::pair<VkCommandBuffer,int32_t> bufferIndexPair = BeginTransferCmd();

				VkBufferCopy copyRegion{};
				copyRegion.srcOffset = srceOffset;
				copyRegion.dstOffset = destOffset;
				copyRegion.size = dataSize;
				vkCmdCopyBuffer(bufferIndexPair.first, srcBuffer.buffer, buffer, 1, &copyRegion);

				EndTransferCmd(bufferIndexPair);
			}

			/// <summary>Begins a transfer command and returns the command buffer index pair used for the command allocated from a TinyVkCommandPool.</summary>
			std::pair<VkCommandBuffer, int32_t> BeginTransferCmd() {
				std::pair<VkCommandBuffer, int32_t> bufferIndexPair = commandPool.LeaseBuffer();
				
				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				vkBeginCommandBuffer(bufferIndexPair.first, &beginInfo);
				return bufferIndexPair;
			}

			/// <summary>Ends a transfer command and gives the leased/rented command buffer pair back to the TinyVkCommandPool.</summary>
			void EndTransferCmd(std::pair<VkCommandBuffer, int32_t> bufferIndexPair) {
				vkEndCommandBuffer(bufferIndexPair.first);

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &bufferIndexPair.first;

				vkQueueSubmit(graphicsPipeline.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
				vkQueueWaitIdle(graphicsPipeline.graphicsQueue);
				commandPool.ReturnBuffer(bufferIndexPair);
			}

			/// <summary>Creates the data descriptor that represents this buffer when passing into graphicspipeline.SelectWrite*Descriptor().</summary>
			VkDescriptorBufferInfo GetBufferDescriptor(VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE) { return { buffer, offset, range }; }
		};
	}
#endif