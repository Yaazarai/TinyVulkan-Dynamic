#pragma once
#ifndef TINYVK_TINYVKGRAPHICSRENDERER
#define TINYVK_TINYVKGRAPHICSRENDERER
	#include "./TinyVK.hpp"

	namespace TINYVULKAN_NAMESPACE {
		/// 
		/// RENDERING PARADIGM:
		///		TinyVkImageRenderer is for rendering directly to a VkImage render target for offscreen rendering.
		///		Call RenderExecute(mutex[optional], preRecordedCommandBuffer[optiona]) to render to the image.
		///			You may pass a pre-recorded command buffer ready to go, or retrieve a command buffer from
		///			the underlying command pool queue and build your command buffer with a render event (onRenderEvent).
		///				onRenderEvent += callback<VkCommandBuffer>(...);
		///				
		///			If you use a render event, the command buffer will be returned to the command pool queue after execution.
		///		
		///		TinyVkSwapChainRenderer is for rendering directly to the SwapChain for onscreen rendering.
		///		Call RenderExecute(mutex[optional]) to render to the swap chain image.
		///			All swap chain rendering is done via render events and does not accept pre-recorded command buffers.
		///			The command buffer will be returned to the command pool queue after execution.
		///	
		///	Why? This rendering paradigm allows the swap chain to effectively manage and synchronize its own resources
		///	minimizing screen rendering errors or validation layer errors.
		/// 
		/// Both the TinyVkImageRenderer and TinyVkSwapChainRenderer contain their own managed depthImages which will
		/// be optionally created and utilized if their underlying graphics pipeline supports depth testing.
		/// 

		class TinyVkRendererInterface {
		public:
			/// <summary>Alias call for easy-calls to: vkCmdBindVertexBuffers + vkCmdBindIndexBuffer.</summary>
			inline static void CmdBindGeometry(VkCommandBuffer cmdBuffer, const VkBuffer* vertexBuffers, const VkBuffer indexBuffer, const VkDeviceSize* offsets, const VkDeviceSize indexOffset = 0, uint32_t binding = 0, uint32_t bindingCount = 1) {
				vkCmdBindVertexBuffers(cmdBuffer, binding, bindingCount, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, indexOffset, VK_INDEX_TYPE_UINT32);
			}

			/// <summary>Alias call for: vkCmdBindVertexBuffers.</summary>
			inline static void CmdBindGeometry(VkCommandBuffer cmdBuffer, const VkBuffer* vertexBuffers, const VkDeviceSize* offsets, uint32_t binding = 0, uint32_t bindingCount = 1) {
				vkCmdBindVertexBuffers(cmdBuffer, binding, bindingCount, vertexBuffers, offsets);
			}

			/// <summary>Alias call for: vkCmdBindIndexBuffers.</summary>
			inline static void CmdBindGeometry(VkCommandBuffer cmdBuffer, const VkBuffer indexBuffer, const VkDeviceSize indexOffset = 0, uint32_t binding = 0, uint32_t bindingCount = 1) {
				vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, indexOffset, VK_INDEX_TYPE_UINT32);
			}

			/// <summary>Alias call for: vkCmdBindVertexBuffers2.</summary>
			inline static void CmdBindGeometry(VkCommandBuffer cmdBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* vertexBuffers, const VkDeviceSize* vbufferOffsets, const VkDeviceSize* vbufferSizes, const VkDeviceSize* vbufferStrides = nullptr) {
					vkCmdBindVertexBuffers2(cmdBuffer, firstBinding, bindingCount, vertexBuffers, vbufferOffsets, vbufferSizes, vbufferStrides);
			}

			/// <summary>Alias call for vkCmdDraw (isIndexed = false) and vkCmdDrawIndexed (isIndexed = true).</summary>
			inline static void CmdDrawGeometry(VkCommandBuffer cmdBuffer, bool isIndexed = false, uint32_t instanceCount = 1, uint32_t firstInstance = 0, uint32_t vertexCount = 0, uint32_t vertexOffset = 0, uint32_t firstIndex = 0) {
				switch (isIndexed) {
					case true:
					vkCmdDrawIndexed(cmdBuffer, vertexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
					break;
					case false:
					vkCmdDraw(cmdBuffer, vertexCount, instanceCount, vertexOffset, firstInstance);
					break;
				}
			}

			/// <summary>Alias call for: vkCmdDrawIndexedIndirect.</summary>
			inline static void CmdDrawGeometryIndirect(VkCommandBuffer cmdBuffer, const VkBuffer drawParamBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
				vkCmdDrawIndexedIndirect(cmdBuffer, drawParamBuffer, offset, drawCount, stride);
			}

			/// <summary>Alias call for: vkCmdDrawIndexedIndirectCount.</summary>
			inline static void CmdDrawGeometryIndirect(VkCommandBuffer cmdBuffer, const VkBuffer drawParamBuffer, VkDeviceSize offset, const VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t drawCount, uint32_t maxDrawCount, uint32_t stride) {
				vkCmdDrawIndexedIndirectCount(cmdBuffer, drawParamBuffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
			}
		};

		/// <summary>Offscreen Rendering (Render-To-Texture Model): Render to VkImage.</summary>
		class TinyVkImageRenderer : public TinyVkRendererInterface, disposable {
		private:
			TinyVkImage* optionalDepthImage;
			TinyVkImage* renderTarget;
			TinyVkCommandPool* commandPool;

		public:
			TinyVkRenderDevice& renderDevice;
			TinyVkVMAllocator& vmAlloc;
			TinyVkGraphicsPipeline& graphicsPipeline;

			/// Invokable Render Events: (executed in TinyVkImageRenderer::RenderExecute()
			invokable<TinyVkCommandPool&> onRenderEvents;

			~TinyVkImageRenderer() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(renderDevice.logicalDevice);

				commandPool->Dispose();
				delete commandPool;

				if (graphicsPipeline.DepthTestingIsEnabled()) {
					optionalDepthImage->Dispose();
					delete optionalDepthImage;
				}
			}

			/// <summary>Creates a headless renderer specifically for performing render commands on a TinyVkImage (VkImage).</summary>
			TinyVkImageRenderer(TinyVkRenderDevice& renderDevice, TinyVkVMAllocator& vmAlloc, TinyVkImage* renderTarget, TinyVkGraphicsPipeline& graphicsPipeline, size_t cmdpoolbuffercount = 32ULL)
			: renderDevice(renderDevice), vmAlloc(vmAlloc), graphicsPipeline(graphicsPipeline), renderTarget(renderTarget) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));

				commandPool = new TinyVkCommandPool(renderDevice, cmdpoolbuffercount + 1);

				optionalDepthImage = nullptr;
				if (graphicsPipeline.DepthTestingIsEnabled())
					optionalDepthImage = new TinyVkImage(renderDevice, graphicsPipeline, *commandPool, vmAlloc, renderTarget->width, renderTarget->height, true, graphicsPipeline.QueryDepthFormat(), TINYVK_DEPTHSTENCIL_ATTACHMENT_OPTIMAL, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_ASPECT_DEPTH_BIT);
			}

			TinyVkImageRenderer operator=(const TinyVkImageRenderer& imageRenderer) = delete;

			/// <summary>Sets the target image/texture for the TinyVkImageRenderer.</summary>
			void SetRenderTarget(TinyVkImage* renderTarget, bool waitOldTarget = true) {
				if (this->renderTarget != nullptr && waitOldTarget) {
					vkWaitForFences(renderDevice.logicalDevice, 1, &renderTarget->imageWaitable, VK_TRUE, UINT64_MAX);
					vkResetFences(renderDevice.logicalDevice, 1, &renderTarget->imageWaitable);
				}

				this->renderTarget = renderTarget;
			}

			/// <summary>Begins recording render commands to the provided command buffer.</summary>
			void BeginRecordCmdBuffer(const VkClearValue clearColor, const VkClearValue depthStencil, VkCommandBuffer commandBuffer) {
				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
				beginInfo.pInheritanceInfo = nullptr; // Optional

				if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to record [begin] to command buffer!");

				const VkImageMemoryBarrier memory_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = renderTarget->image,
					.subresourceRange = {
					  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					  .baseMipLevel = 0,
					  .levelCount = 1,
					  .baseArrayLayer = 0,
					  .layerCount = 1,
					},
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &memory_barrier);

				VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
				colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
				colorAttachmentInfo.imageView = renderTarget->imageView;
				colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachmentInfo.clearValue = clearColor;

				VkRenderingInfoKHR dynamicRenderInfo{};
				dynamicRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;

				VkRect2D renderAreaKHR{};
				renderAreaKHR.extent = { static_cast<uint32_t>(renderTarget->width), static_cast<uint32_t>(renderTarget->height) };
				renderAreaKHR.offset = { 0,0 };
				dynamicRenderInfo.renderArea = renderAreaKHR;
				dynamicRenderInfo.layerCount = 1;
				dynamicRenderInfo.colorAttachmentCount = 1;
				dynamicRenderInfo.pColorAttachments = &colorAttachmentInfo;

				VkRenderingAttachmentInfoKHR depthStencilAttachmentInfo{};
				if (graphicsPipeline.DepthTestingIsEnabled()) {
					const VkImageMemoryBarrier depth_memory_barrier{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.image = optionalDepthImage->image,
						.subresourceRange = {
						  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
						  .baseMipLevel = 0,
						  .levelCount = 1,
						  .baseArrayLayer = 0,
						  .layerCount = 1,
						},
					};
					
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &depth_memory_barrier);

					depthStencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
					depthStencilAttachmentInfo.imageView = optionalDepthImage->imageView;
					depthStencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					depthStencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					depthStencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					depthStencilAttachmentInfo.clearValue = depthStencil;
					dynamicRenderInfo.pDepthAttachment = &depthStencilAttachmentInfo;
				}

				VkViewport dynamicViewportKHR{};
				dynamicViewportKHR.x = 0;
				dynamicViewportKHR.y = 0;
				dynamicViewportKHR.width = static_cast<float>(renderTarget->width);
				dynamicViewportKHR.height = static_cast<float>(renderTarget->height);
				dynamicViewportKHR.minDepth = 0.0f;
				dynamicViewportKHR.maxDepth = 1.0f;
				vkCmdSetViewport(commandBuffer, 0, 1, &dynamicViewportKHR);
				vkCmdSetScissor(commandBuffer, 0, 1, &renderAreaKHR);

				if (vkCmdBeginRenderingEKHR(renderDevice.instance.instance, commandBuffer, &dynamicRenderInfo) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to record [begin] to rendering!");
				
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.graphicsPipeline);
			}

			/// <summary>Ends recording render commands to the provided command buffer.</summary>
			void EndRecordCmdBuffer(const VkClearValue clearColor, const VkClearValue depthStencil, VkCommandBuffer commandBuffer) {
				if (vkCmdEndRenderingEKHR(renderDevice.instance.instance, commandBuffer) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to record [end] to rendering!");

				const VkImageMemoryBarrier image_memory_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = renderTarget->image,
					.subresourceRange = {
					  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					  .baseMipLevel = 0,
					  .levelCount = 1,
					  .baseArrayLayer = 0,
					  .layerCount = 1,
					}
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &image_memory_barrier);

				if (graphicsPipeline.DepthTestingIsEnabled()) {
					const VkImageMemoryBarrier depth_memory_barrier{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.image = optionalDepthImage->image,
						.subresourceRange = {
						  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
						  .baseMipLevel = 0,
						  .levelCount = 1,
						  .baseArrayLayer = 0,
						  .layerCount = 1,
						},
					};

					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &depth_memory_barrier);
				}

				if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to record [end] to command buffer!");
			}

			/// <summary>Records Push Descriptors to the command buffer.</summary>
			VkResult PushDescriptorSet(VkCommandBuffer cmdBuffer, std::vector<VkWriteDescriptorSet> writeDescriptorSets) {
				return vkCmdPushDescriptorSetEKHR(renderDevice.instance.instance, cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout,
					0, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data());
			}

			/// <summary>Records Push Constants to the command buffer.</summary>
			void PushConstants(VkCommandBuffer cmdBuffer, VkShaderStageFlagBits shaderFlags, uint32_t byteSize, const void* pValues) {
				vkCmdPushConstants(cmdBuffer, graphicsPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, byteSize, pValues);
			}
			
			/// <summary>Executes the registered onRenderEvents and renders them to the target image/texture.</summary>
			void RenderExecute(VkCommandBuffer preRecordedCmdBuffer = nullptr) {
				timed_guard imageLock(renderTarget->image_lock);
				if (!imageLock.Acquired()) return;
				
				if (renderTarget == nullptr)
					throw new std::runtime_error("TinyVulkan: RenderTarget for TinyVkImageRenderer is not set [nullptr]!");

				vkWaitForFences(renderDevice.logicalDevice, 1, &renderTarget->imageWaitable, VK_TRUE, UINT64_MAX);
				vkResetFences(renderDevice.logicalDevice, 1, &renderTarget->imageWaitable);
				
				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				if (graphicsPipeline.DepthTestingIsEnabled()) {
					TinyVkImage* depthImage = optionalDepthImage;
					if (depthImage->width != renderTarget->width || depthImage->height != renderTarget->height) {
						depthImage->Disposable(false);
						depthImage->ReCreateImage(renderTarget->width, renderTarget->height, depthImage->isDepthImage, graphicsPipeline.QueryDepthFormat(), TINYVK_DEPTHSTENCIL_ATTACHMENT_OPTIMAL, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_ASPECT_DEPTH_BIT);
					}
				}
				
				vkResetCommandPool(renderDevice.logicalDevice, commandPool->GetPool(), 0);
				onRenderEvents.invoke(*commandPool);
				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				std::vector<VkCommandBuffer> commandBuffers;
				for (size_t i = 0; i < commandPool->rentQueue.size(); i++) {
					if (commandPool->rentQueue[i] != false)
						commandBuffers.push_back(commandPool->commandBuffers[i]);
				}
				submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
				submitInfo.pCommandBuffers = commandBuffers.data();

				if (vkQueueSubmit(graphicsPipeline.graphicsQueue, 1, &submitInfo, renderTarget->imageWaitable) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to submit draw command buffer!");
			}
		};

		/// <summary>Onscreen Rendering (Render/Present-To-Screen Model): Render to SwapChain.</summary>
		class TinyVkSwapChainRenderer : public TinyVkRendererInterface, disposable {
		private:
			std::vector<VkSemaphore> imageAvailableSemaphores;
			std::vector<VkSemaphore> renderFinishedSemaphores;
			std::vector<VkFence> inFlightFences;
			std::vector<TinyVkImage*> optionalDepthImages;
			std::vector<TinyVkCommandPool*> commandPools;
			std::vector<VkExtent2D> frameRenderSizes;
			TinyVkCommandPool* depthImagePool;
			uint32_t currentSyncFrame = 0; // Current Synchronized Frame (Ordered).
			uint32_t currentSwapFrame = 0; // Current SwapChain Image Frame (Out of Order).

			void CreateImageSyncObjects() {
				size_t count = static_cast<size_t>(swapChain.bufferingMode);
				imageAvailableSemaphores.resize(count);
				renderFinishedSemaphores.resize(count);
				inFlightFences.resize(count);

				VkSemaphoreCreateInfo semaphoreInfo{};
				semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				VkFenceCreateInfo fenceInfo{};
				fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

				for (size_t i = 0; i < swapChain.images.size(); i++) {
					if (vkCreateSemaphore(renderDevice.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
						vkCreateSemaphore(renderDevice.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
						vkCreateFence(renderDevice.logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
						throw std::runtime_error("TinyVulkan: Failed to create synchronization objects for a frame!");
				}
			}

			VkResult RendererAcquireImage() {
				vkWaitForFences(renderDevice.logicalDevice, 1, &inFlightFences[currentSyncFrame], VK_TRUE, UINT64_MAX);
				VkResult result = swapChain.AcquireNextImage(imageAvailableSemaphores[currentSyncFrame], VK_NULL_HANDLE, currentSwapFrame);
				vkResetFences(renderDevice.logicalDevice, 1, &inFlightFences[currentSyncFrame]);
				return result;
			}

			VkResult RendererExecuteEvents() {
				commandPools[currentSyncFrame]->ReturnAllBuffers(true);

				if (graphicsPipeline.DepthTestingIsEnabled()) {
					TinyVkImage* depthImage = optionalDepthImages[currentSyncFrame];
					if (depthImage->width != swapChain.imageExtent.width || depthImage->height != swapChain.imageExtent.height) {
						depthImage->Disposable(false);
						depthImage->ReCreateImage(swapChain.imageExtent.width, swapChain.imageExtent.height, depthImage->isDepthImage, graphicsPipeline.QueryDepthFormat(), TINYVK_DEPTHSTENCIL_ATTACHMENT_OPTIMAL, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_ASPECT_DEPTH_BIT);
					}
				}

				frameRenderSizes[currentSyncFrame] = swapChain.imageExtent;
				commandPools[currentSyncFrame]->ReturnAllBuffers(true);
				onRenderEvents.invoke(*commandPools[currentSyncFrame]);
				return VK_SUCCESS;
			}

			VkResult RendererSubmitPresent() {
				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

				VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentSyncFrame] };
				VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = waitSemaphores;
				submitInfo.pWaitDstStageMask = waitStages;
				
				std::vector<VkCommandBuffer> commandBuffers;
				for (size_t i = 0; i < commandPools[currentSyncFrame]->rentQueue.size(); i++) {
					if (commandPools[currentSyncFrame]->rentQueue[i] != false)
						commandBuffers.push_back(commandPools[currentSyncFrame]->commandBuffers[i]);
				}
				submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
				submitInfo.pCommandBuffers = commandBuffers.data();

				VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentSyncFrame] };
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = signalSemaphores;

				if (vkQueueSubmit(graphicsPipeline.graphicsQueue, 1, &submitInfo, inFlightFences[currentSyncFrame]) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to submit draw command buffer!");

				VkPresentInfoKHR presentInfo{};
				presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				presentInfo.waitSemaphoreCount = 1;
				presentInfo.pWaitSemaphores = signalSemaphores;

				VkSwapchainKHR swapChainList[]{ swapChain.swapChain };
				presentInfo.swapchainCount = 1;
				presentInfo.pSwapchains = swapChainList;
				presentInfo.pImageIndices = &currentSwapFrame;

				currentSyncFrame = (currentSyncFrame + 1) % static_cast<size_t>(swapChain.images.size());
				return vkQueuePresentKHR(graphicsPipeline.presentQueue, &presentInfo);
			}

			void RenderSwapChain() {
				if (!swapChain.presentable) return;

				VkResult result = VK_SUCCESS;
				if ((result = RendererAcquireImage()) == VK_SUCCESS)
					if ((result = RendererExecuteEvents()) == VK_SUCCESS)
						result = RendererSubmitPresent();

				if (result == VK_ERROR_OUT_OF_DATE_KHR) {
					commandPools[currentSyncFrame]->ReturnAllBuffers(true);
					swapChain.presentable = false;
					currentSyncFrame = 0;
				} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
					throw std::runtime_error("TinyVulkan: Failed to acquire swap chain image or submit to draw queue!");
			}
		public:
			TinyVkRenderDevice& renderDevice;
			TinyVkVMAllocator& memAlloc;
			TinyVkSwapChain& swapChain;
			TinyVkGraphicsPipeline& graphicsPipeline;

			/// Invokable Render Events: (executed in TinyVkSwapChainRenderer::RenderExecute()
			invokable<TinyVkCommandPool&> onRenderEvents;

			~TinyVkSwapChainRenderer() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(renderDevice.logicalDevice);

				if (graphicsPipeline.DepthTestingIsEnabled()) {
					for(size_t i = 0; i < static_cast<size_t>(swapChain.bufferingMode); i++) {
						optionalDepthImages[i]->Dispose();
						delete optionalDepthImages[i];
					}

					depthImagePool->Dispose();
					delete depthImagePool;
				}

				for (TinyVkCommandPool* cmdPool : commandPools) {
					cmdPool->Dispose();
					delete cmdPool;
				}

				for (size_t i = 0; i < inFlightFences.size(); i++) {
					vkDestroySemaphore(renderDevice.logicalDevice, imageAvailableSemaphores[i], nullptr);
					vkDestroySemaphore(renderDevice.logicalDevice, renderFinishedSemaphores[i], nullptr);
					vkDestroyFence(renderDevice.logicalDevice, inFlightFences[i], nullptr);
				}
			}

			/// <summary>Creates a renderer specifically for performing render commands on a TinyVkSwapChain (VkSwapChain) to present to the window.</summary>
			TinyVkSwapChainRenderer(TinyVkRenderDevice& renderDevice, TinyVkVMAllocator& memAlloc, TinyVkSwapChain& swapChain, TinyVkGraphicsPipeline& graphicsPipeline, size_t cmdpoolbuffercount = 32ULL)
				: renderDevice(renderDevice), memAlloc(memAlloc), swapChain(swapChain), graphicsPipeline(graphicsPipeline) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));
				swapChain.onResizeFrameBuffer.hook(callback<int, int>([this](int, int){ this->RenderSwapChain(); }));

				for(size_t i = 0; i < static_cast<size_t>(swapChain.bufferingMode); i++) {
					commandPools.push_back(new TinyVkCommandPool(renderDevice, cmdpoolbuffercount + 1));
					frameRenderSizes.push_back({ swapChain.imageExtent.width, swapChain.imageExtent.height });
				}

				if (graphicsPipeline.DepthTestingIsEnabled()) {
					depthImagePool = new TinyVkCommandPool(renderDevice, static_cast<size_t>(swapChain.bufferingMode) + 1);
					
					for(size_t i = 0; i < static_cast<size_t>(swapChain.bufferingMode); i++)
						optionalDepthImages.push_back(new TinyVkImage(renderDevice, graphicsPipeline, *depthImagePool, memAlloc, swapChain.imageExtent.width, swapChain.imageExtent.height, true, graphicsPipeline.QueryDepthFormat(), TINYVK_DEPTHSTENCIL_ATTACHMENT_OPTIMAL, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_ASPECT_DEPTH_BIT));
				}

				CreateImageSyncObjects();
			}

			TinyVkSwapChainRenderer operator=(const TinyVkSwapChainRenderer& swapRenderer) = delete;

			/// <summary>Returns the current resource synchronized frame index.</summary>
			size_t GetSyncronizedFrameIndex() { return currentSyncFrame; }

			/// <summary>Begins recording render commands to the provided command buffer.</summary>
			void BeginRecordCmdBuffer(VkCommandBuffer commandBuffer, const VkClearValue clearColor, const VkClearValue depthStencil) {
				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
				beginInfo.pInheritanceInfo = nullptr; // Optional

				if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to record [begin] to command buffer!");

				const VkImageMemoryBarrier swapchain_memory_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.image = swapChain.images[currentSwapFrame],
					.subresourceRange = {
					  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					  .baseMipLevel = 0,
					  .levelCount = 1,
					  .baseArrayLayer = 0,
					  .layerCount = 1,
					},
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &swapchain_memory_barrier);

				VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
				colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
				colorAttachmentInfo.imageView = swapChain.imageViews[currentSwapFrame];
				colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachmentInfo.clearValue = clearColor;

				VkRenderingInfoKHR dynamicRenderInfo{};
				dynamicRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;

				VkRect2D renderAreaKHR{};
				renderAreaKHR.extent = frameRenderSizes[currentSyncFrame];
				renderAreaKHR.offset = { 0,0 };
				dynamicRenderInfo.renderArea = renderAreaKHR;
				dynamicRenderInfo.layerCount = 1;
				dynamicRenderInfo.colorAttachmentCount = 1;
				dynamicRenderInfo.pColorAttachments = &colorAttachmentInfo;
				
				VkRenderingAttachmentInfoKHR depthStencilAttachmentInfo{};
				if (graphicsPipeline.DepthTestingIsEnabled()) {
					const VkImageMemoryBarrier depth_memory_barrier{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.image = optionalDepthImages[currentSyncFrame]->image,
						.subresourceRange = {
						  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
						  .baseMipLevel = 0,
						  .levelCount = 1,
						  .baseArrayLayer = 0,
						  .layerCount = 1,
						},
					};

					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &depth_memory_barrier);

					depthStencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
					depthStencilAttachmentInfo.imageView = optionalDepthImages[currentSyncFrame]->imageView;
					depthStencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					depthStencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					depthStencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					depthStencilAttachmentInfo.clearValue = depthStencil;
					dynamicRenderInfo.pDepthAttachment = &depthStencilAttachmentInfo;
				}

				VkViewport dynamicViewportKHR{};
				dynamicViewportKHR.x = 0;
				dynamicViewportKHR.y = 0;
				dynamicViewportKHR.width = static_cast<float>(frameRenderSizes[currentSyncFrame].width);
				dynamicViewportKHR.height = static_cast<float>(frameRenderSizes[currentSyncFrame].height);
				dynamicViewportKHR.minDepth = 0.0f;
				dynamicViewportKHR.maxDepth = 1.0f;

				vkCmdSetViewport(commandBuffer, 0, 1, &dynamicViewportKHR);
				vkCmdSetScissor(commandBuffer, 0, 1, &renderAreaKHR);

				if (vkCmdBeginRenderingEKHR(renderDevice.instance.instance, commandBuffer, &dynamicRenderInfo) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to record [begin] to rendering!");
				
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.graphicsPipeline);
			}

			/// <summary>Ends recording render commands to the provided command buffer.</summary>
			void EndRecordCmdBuffer(VkCommandBuffer commandBuffer, const VkClearValue clearColor, const VkClearValue depthStencil) {
				if (vkCmdEndRenderingEKHR(renderDevice.instance.instance, commandBuffer) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to record [end] to rendering!");

				const VkImageMemoryBarrier swapchain_memory_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					.image = swapChain.images[currentSwapFrame],
					.subresourceRange = {
					  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					  .baseMipLevel = 0,
					  .levelCount = 1,
					  .baseArrayLayer = 0,
					  .layerCount = 1,
					}
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &swapchain_memory_barrier);

				if (graphicsPipeline.DepthTestingIsEnabled()) {
					const VkImageMemoryBarrier depth_memory_barrier{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.image = optionalDepthImages[currentSyncFrame]->image,
						.subresourceRange = {
						  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
						  .baseMipLevel = 0,
						  .levelCount = 1,
						  .baseArrayLayer = 0,
						  .layerCount = 1,
						},
					};

					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &depth_memory_barrier);
				}

				if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to record [end] to command buffer!");
			}

			/// <summary>Records Push Descriptors to the command buffer.</summary>
			VkResult PushDescriptorSet(VkCommandBuffer cmdBuffer, std::vector<VkWriteDescriptorSet> writeDescriptorSets) {
				return vkCmdPushDescriptorSetEKHR(renderDevice.instance.instance, cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout,
					0, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data());
			}

			/// <summary>Records Push Constants to the command buffer.</summary>
			void PushConstants(VkCommandBuffer cmdBuffer, VkShaderStageFlagBits shaderFlags, uint32_t byteSize, const void* pValues) {
				vkCmdPushConstants(cmdBuffer, graphicsPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, byteSize, pValues);
			}

			/// <summary>Executes the registered onRenderEvents and presents them to the SwapChain(Window).</summary>
			void RenderExecute() {
				timed_guard swapChainLock(swapChain.swapChainMutex);
				if (!swapChainLock.Acquired()) return;
				RenderSwapChain();
			}
		};
	}
#endif