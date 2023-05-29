#pragma once
#ifndef TINYVK_TINYVKCOMMANDPOOL
#define TINYVK_TINYVKCOMMANDPOOL
	#include "./TinyVk.hpp"

	namespace TINYVULKAN_NAMESPACE {
		class TinyVkCommandPool : public disposable {
		private:
			TinyVkRenderDevice& renderDevice;
			VkCommandPool commandPool;
			size_t bufferCount;
			
			std::vector<VkCommandBuffer> commandBuffers;
			std::vector<bool> rentQueue;
		public:
			~TinyVkCommandPool() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(renderDevice.logicalDevice);

				vkDestroyCommandPool(renderDevice.logicalDevice, commandPool, nullptr);
			}
			
			TinyVkCommandPool(TinyVkRenderDevice& renderDevice, size_t bufferCount = static_cast<size_t>(TinyVkBufferingMode::QUADRUPLE)) : renderDevice(renderDevice), bufferCount(bufferCount) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));

				CreateCommandPool();
				CreateCommandBuffers(bufferCount+1);
			}

			TinyVkCommandPool operator=(const TinyVkCommandPool& cmdPool) = delete;

			VkCommandPool& GetPool() { return commandPool; }
			std::vector<VkCommandBuffer>& GetBuffers() { return commandBuffers; }
			size_t GetBufferCount() { return commandBuffers.size(); }

			void CreateCommandPool() {
				VkCommandPoolCreateInfo poolInfo{};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				poolInfo.queueFamilyIndex = TinyVkQueueFamily::FindQueueFamilies(
					renderDevice.physicalDevice, renderDevice.presentationSurface
				).graphicsFamily.value();

				if (vkCreateCommandPool(renderDevice.logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to create command pool!");
			}

			void CreateCommandBuffers(size_t bufferCount = 1) {
				commandBuffers.resize(bufferCount);
				rentQueue.resize(bufferCount);

				for (int32_t i = 0; i < bufferCount; i++) {
					commandBuffers[i] = nullptr;
					rentQueue[i] = false;
				}

				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = commandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

				if (vkAllocateCommandBuffers(renderDevice.logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to allocate command buffers!");
			}

			bool HasBuffers() {
				for (int32_t i = 0; i < rentQueue.size(); i++)
					if (rentQueue[i] == false) return true;

				return false;
			}

			int32_t HasBuffersCount() {
				int32_t hasSum = 0;

				for (int32_t i = 0; i < rentQueue.size(); i++)
					hasSum += (rentQueue[i] == false)?1:0;

				return hasSum;
			}

			std::pair<VkCommandBuffer,int32_t> LeaseBuffer() {
				for (int32_t i = 0; i < rentQueue.size(); i++)
					if (rentQueue[i] == false) {
						rentQueue[i] = true;
						vkResetCommandBuffer(commandBuffers[i], 0);
						return std::pair(commandBuffers[i], i);
					}

				throw std::runtime_error("TinyVulkan: VKCommandPool is full and cannot lease any more VkCommandBuffers! MaxSize: " + std::to_string(bufferCount));
				return std::pair<VkCommandBuffer,int32_t>(nullptr,-1);
			}

			void ReturnBuffer(std::pair<VkCommandBuffer, int32_t> bufferIndexPair) {
				if (bufferIndexPair.second < 0 || bufferIndexPair .second >= rentQueue.size())
					throw std::runtime_error("TinyVulkan: Failed to return command buffer!");

				rentQueue[bufferIndexPair.second] = false;
			}
		};
	}
#endif