#pragma once
#ifndef MINIVK_MINIVKMEMALLOC
#define MINIVK_MINIVKMEMALLOC
	#include "./MiniVulkan.hpp"
	
	namespace MINIVULKAN_NS {
		class MvkMemAlloc : public MvkObject {
		private:
			MvkInstance& mvkInstance;
		public:
			VmaAllocator memoryAllocator = VK_NULL_HANDLE;

			void Disposable() {
				vmaDestroyAllocator(memoryAllocator);
			}

			MvkMemAlloc(MvkInstance& mvkInstance) : mvkInstance(mvkInstance) {
				onDispose += std::callback<>(this, &MvkMemAlloc::Disposable);

				VmaAllocatorCreateInfo allocatorCreateInfo = {};
				allocatorCreateInfo.vulkanApiVersion = MVK_API_VERSION;
				allocatorCreateInfo.physicalDevice = mvkInstance.physicalDevice;
				allocatorCreateInfo.device = mvkInstance.logicalDevice;
				allocatorCreateInfo.instance = mvkInstance.instance;

				vmaCreateAllocator(&allocatorCreateInfo, &memoryAllocator);
			}

			MvkMemAlloc operator=(const MvkMemAlloc& memAlloc) = delete;

			VmaAllocator GetAllocator() { return memoryAllocator; }
		};
	}
#endif