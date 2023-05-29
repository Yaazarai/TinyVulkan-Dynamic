#pragma once
#ifndef TINYVK_TINYVKMEMALLOC
#define TINYVK_TINYVKMEMALLOC
	#include "./TinyVK.hpp"
	
	namespace TINYVULKAN_NAMESPACE {
		class TinyVkVMAllocator : public disposable {
		public:
			VmaAllocator memoryAllocator = VK_NULL_HANDLE;
			TinyVkRenderDevice& renderDevice;

			~TinyVkVMAllocator() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(renderDevice.logicalDevice); 
				vmaDestroyAllocator(memoryAllocator);
			}

			TinyVkVMAllocator(TinyVkInstance& mvkInstance, TinyVkRenderDevice& renderDevice) : renderDevice(renderDevice) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));

				VmaAllocatorCreateInfo allocatorCreateInfo = {};
				allocatorCreateInfo.vulkanApiVersion = TVK_RENDERER_VERSION;
				allocatorCreateInfo.physicalDevice = renderDevice.physicalDevice;
				allocatorCreateInfo.device = renderDevice.logicalDevice;
				allocatorCreateInfo.instance = mvkInstance.instance;
				vmaCreateAllocator(&allocatorCreateInfo, &memoryAllocator);
			}

			TinyVkVMAllocator operator=(const TinyVkVMAllocator& memAlloc) = delete;

			VmaAllocator GetAllocator() { return memoryAllocator; }
		};
	}
#endif