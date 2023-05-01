#pragma once
#ifndef MINIVULKAN_MINIVKSHADERSTAGE
#define MINIVULKAN_MINIVKSHADERSTAGE
	#include "./MiniVK.hpp"

	namespace MINIVULKAN_NAMESPACE {
		class MiniVkShaderStages : public disposable {
		private:
			MiniVkRenderDevice& renderDevice;

		public:
			std::vector<VkShaderModule> shaderModules;
			std::vector<std::tuple<VkShaderStageFlagBits, std::string>> shaders;
			std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfo;

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(renderDevice.logicalDevice);

				for(auto shaderModule : shaderModules)
					vkDestroyShaderModule(renderDevice.logicalDevice, shaderModule, nullptr);
			}

			MiniVkShaderStages(MiniVkRenderDevice& renderDevice, const std::vector<std::tuple<VkShaderStageFlagBits, std::string>> shaders) : renderDevice(renderDevice), shaders(shaders) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));

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
				if (vkCreateShaderModule(renderDevice.logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
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