#pragma once
#ifndef MINIVK_MINIVKCOMMANDPOOL
#define MINIVK_MINIVKCOMMANDPOOL
	#include "./MiniVk.hpp"

	namespace MINIVULKAN_NS {
		class MiniVkCommandPool : public MiniVkObject {
		private:
			MiniVkInstance& mvkLayer;
			VkCommandPool commandPool;
			std::vector<VkCommandBuffer> commandBuffers;
		public:
			void Disposable() {
				vkDeviceWaitIdle(mvkLayer.logicalDevice);
				vkDestroyCommandPool(mvkLayer.logicalDevice, commandPool, nullptr);
			}
			
			MiniVkCommandPool(MiniVkInstance& mvkLayer, size_t bufferCount = 1) : mvkLayer(mvkLayer) {
				onDispose += std::callback<>(this, &MiniVkCommandPool::Disposable);
				CreateCommandPool();
				CreateCommandBuffers(bufferCount+1);
			}

			MiniVkCommandPool operator=(const MiniVkCommandPool& cmdPool) = delete;

			VkCommandPool& GetPool() { return commandPool; }
			std::vector<VkCommandBuffer>& GetBuffers() { return commandBuffers; }
			size_t GetBufferCount() { return commandBuffers.size(); }

			void CreateCommandPool() {
				VkCommandPoolCreateInfo poolInfo{};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				poolInfo.queueFamilyIndex = MiniVkQueueFamily::FindQueueFamilies(
					mvkLayer.physicalDevice, mvkLayer.presentationSurface
				).graphicsFamily.value();

				if (vkCreateCommandPool(mvkLayer.logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create command pool!");
			}

			void CreateCommandBuffers(size_t bufferCount = 1) {
				commandBuffers.resize(bufferCount);

				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = commandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

				if (vkAllocateCommandBuffers(mvkLayer.logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to allocate command buffers!");
			}
		};
	}
#endif