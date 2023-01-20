#pragma once
#ifndef MINIVK_MINIVKMEMALLOC
#define MINIVK_MINIVKMEMALLOC
	#include "./MiniVk.hpp"
	
	namespace MINIVULKAN_NS {
		class MiniVkMemAlloc : public MiniVkObject {
		private:
			MiniVkInstance& mvkLayer;
		public:
			VmaAllocator memoryAllocator = VK_NULL_HANDLE;

			void Disposable() {
				vmaDestroyAllocator(memoryAllocator);
			}

			MiniVkMemAlloc(MiniVkInstance& mvkLayer) : mvkLayer(mvkLayer) {
				onDispose += std::callback<>(this, &MiniVkMemAlloc::Disposable);

				VmaAllocatorCreateInfo allocatorCreateInfo = {};
				allocatorCreateInfo.vulkanApiVersion = MVK_API_VERSION;
				allocatorCreateInfo.physicalDevice = mvkLayer.physicalDevice;
				allocatorCreateInfo.device = mvkLayer.logicalDevice;
				allocatorCreateInfo.instance = mvkLayer.instance;

				vmaCreateAllocator(&allocatorCreateInfo, &memoryAllocator);
			}

			MiniVkMemAlloc operator=(const MiniVkMemAlloc& memAlloc) = delete;

			VmaAllocator GetAllocator() { return memoryAllocator; }
		};
	}
#endif