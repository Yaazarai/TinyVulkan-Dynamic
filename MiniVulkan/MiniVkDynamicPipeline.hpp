#pragma once
#ifndef MINIVK_MINIVKDYNAMICPIPELINE
#define MINIVK_MINIVKDYNAMICPIPELINE
	#include "./MiniVk.hpp"

	namespace MINIVULKAN_NS {
		#define VKCOMP_RGBA VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		#define VKCOMP_BGRA VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT

		class MiniVkDynamicPipeline : public MiniVkObject {
		private:
			MiniVkInstance& mvkLayer;
		public:
			/// GRAPHICS_PIPELINE ///
			VkVertexInputBindingDescription vertexBindingDescription;
			std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescrition;

			std::vector<VkPushConstantRange> pushConstantRanges;
			MiniVkShaderStages& shaderStages;
			VkPipelineDynamicStateCreateInfo dynamicState;
			VkPipelineLayout pipelineLayout;
			VkPipeline graphicsPipeline;
			VkFormat imageFormat;
			VkColorComponentFlags colorComponentFlags = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			VkPrimitiveTopology vertexTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			/// QUEUE_FAMILIES ///
			VkQueue graphicsQueue;
			VkQueue presentQueue;

			void Disposable() {
				vkDeviceWaitIdle(mvkLayer.logicalDevice);
				vkDestroyPipeline(mvkLayer.logicalDevice, graphicsPipeline, nullptr);
				vkDestroyPipelineLayout(mvkLayer.logicalDevice, pipelineLayout, nullptr);
			}

			MiniVkDynamicPipeline(MiniVkInstance& mvkLayer, VkFormat imageFormat, MiniVkShaderStages& shaderStages, VkVertexInputBindingDescription vertexBindingDescription,
			std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescrition, const std::vector<VkPushConstantRange>& pushConstantRanges, VkColorComponentFlags colorComponentFlags = VKCOMP_RGBA, VkPrimitiveTopology vertexTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
			: mvkLayer(mvkLayer), shaderStages(shaderStages), vertexBindingDescription(vertexBindingDescription), vertexAttributeDescrition(vertexAttributeDescrition),
			imageFormat(imageFormat), colorComponentFlags(colorComponentFlags), vertexTopology(vertexTopology), pushConstantRanges(pushConstantRanges) {
				onDispose += std::callback<>(this, &MiniVkDynamicPipeline::Disposable);

				MiniVkQueueFamily indices = MiniVkQueueFamily::FindQueueFamilies(mvkLayer.physicalDevice, mvkLayer.presentationSurface);
				vkGetDeviceQueue(mvkLayer.logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
				vkGetDeviceQueue(mvkLayer.logicalDevice, indices.presentFamily.value(), 0, &presentQueue);

				CreateGraphicsPipeline();
			}

			MiniVkDynamicPipeline operator=(const MiniVkDynamicPipeline& pipeline) = delete;

			/// <summary>Creates the graphics rendering pipeline: Automatically called on constructor call.</summary>
			void CreateGraphicsPipeline() {
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				/////////// This section specifies that MiniVkVertex provides the vertex layout description ///////////
				auto bindingDescription = vertexBindingDescription;
				auto attributeDescriptions = vertexAttributeDescrition;

				VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
				vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputInfo.vertexBindingDescriptionCount = 1;
				vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
				vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
				vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

				pipelineLayoutInfo.pushConstantRangeCount = 0;
				uint32_t pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
				if (pushConstantRangeCount > 0) {
					pipelineLayoutInfo.pushConstantRangeCount = pushConstantRangeCount;
					pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
				}

				if (vkCreatePipelineLayout(mvkLayer.logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create graphics pipeline layout!");
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
				inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				inputAssembly.topology = vertexTopology;
				inputAssembly.primitiveRestartEnable = VK_FALSE;

				VkPipelineViewportStateCreateInfo viewportState{};
				viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewportState.viewportCount = 1;
				viewportState.scissorCount = 1;
				viewportState.flags = 0;

				VkPipelineRasterizationStateCreateInfo rasterizer{};
				rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rasterizer.depthClampEnable = VK_FALSE;
				rasterizer.rasterizerDiscardEnable = VK_FALSE;
				rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
				rasterizer.lineWidth = 1.0f;
				rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
				rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
				rasterizer.depthBiasEnable = VK_FALSE;

				VkPipelineMultisampleStateCreateInfo multisampling{};
				multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampling.sampleShadingEnable = VK_FALSE;
				multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

				VkPipelineColorBlendAttachmentState colorBlendAttachment{};
				colorBlendAttachment.colorWriteMask = colorComponentFlags;
				colorBlendAttachment.blendEnable = VK_FALSE;

				VkPipelineColorBlendStateCreateInfo colorBlending{};
				colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				colorBlending.logicOpEnable = VK_FALSE;
				colorBlending.logicOp = VK_LOGIC_OP_COPY;
				colorBlending.attachmentCount = 1;
				colorBlending.pAttachments = &colorBlendAttachment;
				colorBlending.blendConstants[0] = 0.0f;
				colorBlending.blendConstants[1] = 0.0f;
				colorBlending.blendConstants[2] = 0.0f;
				colorBlending.blendConstants[3] = 0.0f;

				std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
				dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamicState.flags = 0;
				dynamicState.pDynamicStates = dynamicStateEnables.data();
				dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

				VkPipelineRenderingCreateInfoKHR renderingCreateInfo{};
				renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
				renderingCreateInfo.colorAttachmentCount = 1;
				renderingCreateInfo.pColorAttachmentFormats = &imageFormat;
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				VkGraphicsPipelineCreateInfo pipelineInfo{};
				pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipelineInfo.stageCount = 2;
				pipelineInfo.pStages = shaderStages.shaderStages.data();
				pipelineInfo.pVertexInputState = &vertexInputInfo;
				pipelineInfo.pInputAssemblyState = &inputAssembly;
				pipelineInfo.pViewportState = &viewportState;
				pipelineInfo.pRasterizationState = &rasterizer;
				pipelineInfo.pMultisampleState = &multisampling;
				pipelineInfo.pColorBlendState = &colorBlending;

				pipelineInfo.pDynamicState = &dynamicState;
				pipelineInfo.pNext = &renderingCreateInfo;

				pipelineInfo.layout = pipelineLayout;
				pipelineInfo.renderPass = nullptr;
				pipelineInfo.subpass = 0;
				pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
				pipelineInfo.basePipelineIndex = -1; // Optional

				if (vkCreateGraphicsPipelines(mvkLayer.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create graphics pipeline!");
				///////////////////////////////////////////////////////////////////////////////////////////////////////
			}

			inline static VkPushConstantRange SelectPushConstantRange(uint32_t pushConstantRangeSize = 0, VkShaderStageFlags shaderStages = VK_SHADER_STAGE_ALL_GRAPHICS) {
				VkPushConstantRange pushConstantRange{};
				pushConstantRange.stageFlags = shaderStages;
				pushConstantRange.offset = 0;
				pushConstantRange.size = pushConstantRangeSize;
				return pushConstantRange;
			}
		};
	}
#endif