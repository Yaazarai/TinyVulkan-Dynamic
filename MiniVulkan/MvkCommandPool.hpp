#pragma once
#ifndef MINIVK_MINIVKCOMMANDPOOL
#define MINIVK_MINIVKCOMMANDPOOL
	#include "./MiniVulkan.hpp"

	namespace MINIVULKAN_NS {
		class MvkCommandPool : public MvkObject {
		private:
			MvkInstance& mvkInstance;
			VkCommandPool commandPool;
			std::vector<VkCommandBuffer> commandBuffers;
		public:
			void Disposable() {
				vkDeviceWaitIdle(mvkInstance.logicalDevice);
				vkDestroyCommandPool(mvkInstance.logicalDevice, commandPool, nullptr);
			}
			
			MvkCommandPool(MvkInstance& mvkInstance, size_t bufferCount = 1) : mvkInstance(mvkInstance) {
				onDispose += std::callback<>(this, &MvkCommandPool::Disposable);
				CreateCommandPool();
				CreateCommandBuffers(bufferCount+1);
			}

			MvkCommandPool operator=(const MvkCommandPool& cmdPool) = delete;

			VkCommandPool& GetPool() { return commandPool; }
			std::vector<VkCommandBuffer>& GetBuffers() { return commandBuffers; }
			size_t GetBufferCount() { return commandBuffers.size(); }

			void CreateCommandPool() {
				VkCommandPoolCreateInfo poolInfo{};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				poolInfo.queueFamilyIndex = MvkQueueFamily::FindQueueFamilies(
					mvkInstance.physicalDevice, mvkInstance.presentationSurface
				).graphicsFamily.value();

				if (vkCreateCommandPool(mvkInstance.logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create command pool!");
			}

			void CreateCommandBuffers(size_t bufferCount = 1) {
				commandBuffers.resize(bufferCount);

				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = commandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

				if (vkAllocateCommandBuffers(mvkInstance.logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to allocate command buffers!");
			}
		};
	}
#endif