#pragma once
#ifndef MINIVK_MINIVKSHADERSTAGE
#define MINIVK_MINIVKSHADERSTAGE
	#include "./MiniVk.hpp"

	namespace MINIVULKAN_NS {
		class MiniVkShaderStages : public MiniVkObject {
		private:
			MiniVkInstance& mvkLayer;

		public:
			std::vector<VkShaderModule> shaderModules;
			std::vector<std::tuple<VkShaderStageFlagBits, std::string>> shaders;
			std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfo;

			void Disposable() {
				vkDeviceWaitIdle(mvkLayer.logicalDevice);
				for(auto shaderModule : shaderModules)
					vkDestroyShaderModule(mvkLayer.logicalDevice, shaderModule, nullptr);
			}

			MiniVkShaderStages(MiniVkInstance& mvkLayer, const std::vector<std::tuple<VkShaderStageFlagBits, std::string>> shaders) : mvkLayer(mvkLayer), shaders(shaders) {
				onDispose += std::callback<>(this, &MiniVkShaderStages::Disposable);

				for (size_t i = 0; i < shaders.size(); i++) {
					auto shaderCode = ReadFile(std::get<1>(shaders[i]));
					auto shaderModule = CreateShaderModule(shaderCode);
					shaderModules.push_back(shaderModule);
					shaderCreateInfo.push_back(CreateShaderInfo(std::get<1>(shaders[i]), shaderModule, std::get<0>(shaders[i])));
				}
			}

			MiniVkShaderStages operator=(const MiniVkShaderStages& shader) = delete;

			VkShaderModule CreateShaderModule(std::vector<char> shaderCode) {
				VkShaderModuleCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				createInfo.pNext = nullptr;
				createInfo.flags = 0;
				createInfo.codeSize = shaderCode.size();
				createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

				VkShaderModule shaderModule;
				if (vkCreateShaderModule(mvkLayer.logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create shader module!");

				return shaderModule;
			}

			VkPipelineShaderStageCreateInfo CreateShaderInfo(const std::string& path, VkShaderModule shaderModule, VkShaderStageFlagBits stageFlagBits) {
				VkPipelineShaderStageCreateInfo shaderStageInfo{};
				shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStageInfo.stage = stageFlagBits;
				shaderStageInfo.module = shaderModule;
				shaderStageInfo.pName = "main";

				#ifdef _DEBUG
				std::cout << "MiniVulkan: Loading Shader @ " << path << std::endl;
				#endif

				return shaderStageInfo;
			}

			std::vector<char> ReadFile(const std::string& path) {
				std::ifstream file(path, std::ios::ate | std::ios::binary);

				if (!file.is_open())
					throw std::runtime_error("MiniVulkan: Failed to Read File: " + path);

				size_t fsize = static_cast<size_t>(file.tellg());
				std::vector<char> buffer(fsize);
				file.seekg(0);
				file.read(buffer.data(), fsize);
				file.close();
				return buffer;
			}
		};
	}
#endif