#pragma once
#ifndef MINIVULKAN_MINIVKMEMALLOC
#define MINIVULKAN_MINIVKMEMALLOC
	#include "./MiniVK.hpp"
	
	namespace MINIVULKAN_NAMESPACE {
		class MiniVkVMAllocator : public std::disposable {
		public:
			VmaAllocator memoryAllocator = VK_NULL_HANDLE;
			MiniVkRenderDevice& renderDevice;

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(renderDevice.logicalDevice); 
				vmaDestroyAllocator(memoryAllocator);
			}

			MiniVkVMAllocator(MiniVkInstance& mvkInstance, MiniVkRenderDevice& renderDevice) : renderDevice(renderDevice) {
				onDispose.hook(callback<bool>(this, &MiniVkVMAllocator::Disposable));
				VmaAllocatorCreateInfo allocatorCreateInfo = {};
				allocatorCreateInfo.vulkanApiVersion = MVK_RENDERER_VERSION;
				allocatorCreateInfo.physicalDevice = renderDevice.physicalDevice;
				allocatorCreateInfo.device = renderDevice.logicalDevice;
				allocatorCreateInfo.instance = mvkInstance.instance;
				vmaCreateAllocator(&allocatorCreateInfo, &memoryAllocator);
			}

			MiniVkVMAllocator operator=(const MiniVkVMAllocator& memAlloc) = delete;

			VmaAllocator GetAllocator() { return memoryAllocator; }
		};
	}
#endif