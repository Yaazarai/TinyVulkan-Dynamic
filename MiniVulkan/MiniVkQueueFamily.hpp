#pragma once
#ifndef MINIVK_MINIVKQUEUEFAMILIES
#define MINIVK_MINIVKQUEUEFAMILIES
	#include "./MiniVk.hpp"
	
	namespace MINIVULKAN_NS {
		class MiniVkQueueFamily : public MiniVkObject {
		public:
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			// void Disposable() { /* Not dynamic, no dispose implementation. */ }
			~MiniVkQueueFamily() { /* Not dynamic, no dispose implementation. */ }
			MiniVkQueueFamily() { /* onDispose += callback<>(this, &MiniVkQueueFamily::Disposable); */ }

			/// <summary>Copy constructor.</summary>
			MiniVkQueueFamily(const MiniVkQueueFamily& qf) {
				graphicsFamily = qf.graphicsFamily;
				presentFamily = qf.presentFamily;
			}

			bool IsComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }

			/// <summary>Returns info about the VkPhysicalDevice graphics/present queue families. If no surface provided, auto checks for Win32 surface support.</summary>
			inline static MiniVkQueueFamily FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface = nullptr) {
				MiniVkQueueFamily indices;

				uint32_t queueFamilyCount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

				std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
				vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

				int i = 0;
				for (const auto& queueFamily : queueFamilies) {
					if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
						indices.graphicsFamily = i;

					VkBool32 presentSupport = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

					if (presentSupport)
						indices.presentFamily = i;

					if (indices.IsComplete()) break;
					i++;
				}

				return indices;
			}
		};
	}
#endif