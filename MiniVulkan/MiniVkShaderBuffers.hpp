#pragma once
#ifndef MINIVK_MINIVKSHADEBRUFFERS
#define MINIVK_MINIVKSHADEBRUFFERS
	#include "./MiniVk.hpp"
	
	namespace MINIVULKAN_NS {
		/// MINIVKBUFFER:
		///		This is a HOST_VISIBLE buffer that must be staged for shader(s).
		/// 
		/// HOST (HOST PC MACHINE -> CPU)
		/// DEVICE (PHYSICAL RENDER DEVICE -> GPU)
		///		HOST_VISIBLE means that the CPU can access GPU allocated memory (SLOW).
		///		DEVICE_LOCAL means only the GPU can access GPU allocated memory (FAST).
		/// 
		/// STAGE: (staging)
		///		The process of writing to HOST_VISIBLE memory and copying that memory
		///		to DEVICE_LOCAL memory for shaders to read from for efficiency.
		/// 
		/// COPY:
		///		Requires using the GraphicsQueue and CommandPool associated to copy
		///		HOST_VISIBLE buffer memory to DEVICE_LOCAL memory by submitting a
		///		memory transfer command to the GPU.
		
		class MiniVkBuffer : public MiniVkObject {
		protected:
			MiniVkInstance& mvkLayer;

			uint32_t QueryMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
				VkPhysicalDeviceMemoryProperties memProperties;
				vkGetPhysicalDeviceMemoryProperties(mvkLayer.physicalDevice, &memProperties);

				for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
					if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
						return i;

				throw std::runtime_error("MiniVulkan: Failed to find suitable memory type for vertex buffer!");
			}

			void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
				VkBufferCreateInfo bufferInfo{};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = size;
				bufferInfo.usage = usage;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				if (vkCreateBuffer(mvkLayer.logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create [MiniVkBuffer] buffer!");

				VkMemoryRequirements memRequirements;
				vkGetBufferMemoryRequirements(mvkLayer.logicalDevice, buffer, &memRequirements);

				VkMemoryAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memRequirements.size;
				allocInfo.memoryTypeIndex = QueryMemoryType(memRequirements.memoryTypeBits, properties);

				if (vkAllocateMemory(mvkLayer.logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to allocate [MiniVkBuffer] buffer memory!");

				vkBindBufferMemory(mvkLayer.logicalDevice, buffer, bufferMemory, 0);
			}
		public:
			VkDeviceSize size;
			VkBuffer buffer;
			VkDeviceMemory memory;

			void Disposable() {
				vkDeviceWaitIdle(mvkLayer.logicalDevice);
				vkDestroyBuffer(mvkLayer.logicalDevice, buffer, nullptr);
				vkFreeMemory(mvkLayer.logicalDevice, memory, nullptr);
			}

			MiniVkBuffer(MiniVkInstance& mvkLayer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) : mvkLayer(mvkLayer), size(size) {
				onDispose += std::callback<>(this, &MiniVkBuffer::Disposable);
				CreateBuffer(size, usage, properties, buffer, memory);
			}

			void Stage(VkQueue graphicsQueue, VkCommandPool commandPool, void* data, VkDeviceSize dataSize) {
				MiniVkBuffer stagingBuffer(mvkLayer, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

				void* _data;
				vkMapMemory(mvkLayer.logicalDevice, stagingBuffer.memory, 0, dataSize, 0, &_data);
				memcpy(_data, data, (size_t)dataSize);
				vkUnmapMemory(mvkLayer.logicalDevice, stagingBuffer.memory);

				MiniVkBuffer::Copy(mvkLayer, graphicsQueue, commandPool, stagingBuffer.buffer, buffer, size);
			}

			inline static void Copy(MiniVkInstance& mvkLayer, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srceOffset = 0, VkDeviceSize destOffset = 0) {
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
				vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
				vkEndCommandBuffer(commandBuffer);

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffer;

				vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
				vkQueueWaitIdle(graphicsQueue);
				vkFreeCommandBuffers(mvkLayer.logicalDevice, commandPool, 1, &commandBuffer);
			}

			template<typename T>
			void Fill(std::vector<T>& data) {
				void* bufferData = nullptr;
				vkMapMemory(mvkLayer.logicalDevice, memory, 0, size, 0, &bufferData);
				memcpy(bufferData, data.data(), (size_t)size);
				vkUnmapMemory(mvkLayer.logicalDevice, memory);
			}

			void Fill(void* data) {
				void* bufferData = nullptr;
				vkMapMemory(mvkLayer.logicalDevice, memory, 0, size, 0, &data);
				memcpy(bufferData, data, (size_t)size);
				vkUnmapMemory(mvkLayer.logicalDevice, memory);
			}

			MiniVkBuffer operator=(const MiniVkBuffer buff) = delete;
		};

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

		/// ABOUT VERTEX BUFFERS:
		///		Vertex buffers consist of two things:
		///			RAM Vector vertex memory.
		///			GPU Device local buffer memory.
		///		
		///		The vertex info in the vector will NOT be copied into the GPU host visible buffer
		///		memory on creation. You must stage the vertex buffer using the graphics pipeline
		///     prior to rendering to copy the vertex vector to the actual vertex buffer on the GPU.

		class MiniVkVertexBuffer : public MiniVkBuffer {
		public:
			std::vector<MiniVkVertex>& vertices;

			MiniVkVertexBuffer(MiniVkInstance& mvkLayer, std::vector<MiniVkVertex> &vertices) : vertices(vertices),
				MiniVkBuffer(mvkLayer, sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {}

			void StageV(VkQueue graphicsQueue, VkCommandPool commandPool) {
				MiniVkBuffer::Stage(graphicsQueue, commandPool, vertices.data(), sizeof(MiniVkVertex) * vertices.size());
			}

			MiniVkVertexBuffer operator=(const MiniVkVertexBuffer& vbuff) = delete;
		};

		/// ABOUT INDEX BUFFERS:
		///		Index Buffers are buffers which contain indexes that map to vertices in a vertex buffer
		///		so that the renderer can reference the same vertex several times to save memory for
		///		triangles with overlapping vertices.

		class MiniVkIndexBuffer : public MiniVkBuffer {
		public:
			MiniVkVertexBuffer& vertexBuffer;
			std::vector<uint32_t> indices;

			MiniVkIndexBuffer(MiniVkInstance& mvkLayer, MiniVkVertexBuffer& vertexBuffer, const std::vector<uint32_t>& indices) : vertexBuffer(vertexBuffer), indices(indices),
				MiniVkBuffer(mvkLayer, sizeof(uint32_t)* indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {}

			void StageI(VkQueue graphicsQueue, VkCommandPool commandPool) {
				MiniVkBuffer::Stage(graphicsQueue, commandPool, indices.data(), sizeof(uint32_t) * indices.size());
			}

			MiniVkIndexBuffer operator=(const MiniVkIndexBuffer ibuff) = delete;
		};

		/// ABOUT UNIFORM BUFFERS (vulkan descriptor sets)
		///		Uniform Buffers allow you to send information to a shader.
		///		By default this should always include the Model/View/Projection matrices.
		
		template<typename UniformStruct>
		class MiniVkUniformBuffer : public MiniVkBuffer {
		public:
			UniformStruct set;

			MiniVkUniformBuffer(MiniVkInstance& mvkLayer, VkDescriptorSetLayout setLayout, UniformStruct set) : set(set),
				MiniVkBuffer(mvkLayer, sizeof(UniformStruct), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT) {}

			void StageU(VkQueue graphicsQueue, VkCommandPool commandPool) {
				MiniVkBuffer::Stage(graphicsQueue, commandPool, &set, sizeof(UniformStruct));
			}

			MiniVkUniformBuffer operator=(const MiniVkUniformBuffer& ubuff) = delete;
		};

		/// ABOUT TEXTURE BUFFERS:
		///		Textures Images need to be loaded into GPU memory, this buffer allows that.
		
		class MiniVkTextureBuffer : public MiniVkBuffer {
		public:
			std::vector<unsigned char> rgbaPixels;

			MiniVkTextureBuffer(MiniVkInstance& mvkLayer, std::vector<unsigned char> rgbaPixels) :
				MiniVkBuffer(mvkLayer, rgbaPixels.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), rgbaPixels(rgbaPixels) {}

			void StageT(VkQueue graphicsQueue, VkCommandPool commandPool) {
				MiniVkBuffer::Stage(graphicsQueue, commandPool, rgbaPixels.data(), rgbaPixels.size());
			}

			MiniVkTextureBuffer operator=(const MiniVkTextureBuffer& tbuff) = delete;
		};
	}
#endif