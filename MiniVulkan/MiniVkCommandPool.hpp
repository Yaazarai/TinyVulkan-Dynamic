#pragma once
#ifndef MINIVULKAN_MINIVKCOMMANDPOOL
#define MINIVULKAN_MINIVKCOMMANDPOOL
	#include "./MiniVk.hpp"

	namespace MINIVULKAN_NAMESPACE {
		class MiniVkCommandPool : public MiniVkObject {
		private:
			MiniVkRenderDevice& renderDevice;
			VkCommandPool commandPool;
			std::vector<VkCommandBuffer> commandBuffers;
		public:
			void Disposable() {
				vkDeviceWaitIdle(renderDevice.logicalDevice);
				vkDestroyCommandPool(renderDevice.logicalDevice, commandPool, nullptr);
			}
			
			MiniVkCommandPool(MiniVkRenderDevice& renderDevice, size_t bufferCount = 1) : renderDevice(renderDevice) {
				onDispose += MiniVkCallback<>(this, &MiniVkCommandPool::Disposable);
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
					renderDevice.physicalDevice, renderDevice.presentationSurface
				).graphicsFamily.value();

				if (vkCreateCommandPool(renderDevice.logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create command pool!");
			}

			void CreateCommandBuffers(size_t bufferCount = 1) {
				commandBuffers.resize(bufferCount);

				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = commandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

				if (vkAllocateCommandBuffers(renderDevice.logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to allocate command buffers!");
			}
		};
	}
#endif