/*
	minivulkan::Window is the GLFW window handler for MiniVulkan that will link to
	and initialize GLFW and Vulkan to create you game / application window.
		* Window::onRefresh is an event callback that is called when the window needs to be refreshed.
			This gets called when GLFW calls the GLFWwindowrefreshfun callback.
			This callback type is in Invokable.hpp and can be initialized like so:
				Window::onRefresh += callback<GLFWwindow*>(this, &Window::RefreshNeeded);
				void RefreshNeeded(GLFWwindow* hwnd) { ... }

		* GetHandle() returns the window handle for the application from GLFW.
			NOTE: this is wrapped w/ unique_ptr and does not need to be manually free'd with GLFWDestroyWindow.

		* ShouldClose() checks the GLFW close flag to see if the window should close.
			If the window should NOT close this will call glfwPollEvents();

		* ~Window() destructor.
			Calls onDestruct to execute destructor events.

		* InitiateWindow and TerminateWindow
			are automatically called when the GLFWwindow* window handle is cleaned up by std::unique_ptr<> hwndWindow.

		* CreateWindowSurface(), GetFrameBufferSize() and GetRequiredExtensions() must be puiblically exposed
			for MiniVulkan, more specifically the class MvkLayer to call them for the Vulkan API.

	NOTE: That this class is marked with a defaulted template for GLFW. You can override this template
		and then override/redefine ALL virtual methods/constructors/destructors
		[Constructor, Destructor, CreateWindow, ShouldClose, GetHandle, RunMain] to use a different window system from GLFW.

		This template is largely redundant and not used as it is simply there to provide the default type
		for explicitly defining the API, in this case GLFW. You can remove it and change T to GLFWwindow or change the
		window API entirely as long as you expose [public] these functions:
			CreateWindowSurface(), GetFrameBufferSize() and GetRequiredExtensions().
*/
#pragma once
#ifndef MINIVK_MINIVKWINDOW
#define MINIVK_MINIVKWINDOW
	#include "./MiniVulkan.hpp"

	namespace MINIVULKAN_NS {
		class MvkWindow : public MvkObject {
		private:
			bool hwndResizable;
			int hwndWidth, hwndHeight;
			std::string title;
			GLFWwindow* hwndWindow;

			/// <summary>GLFWwindow unique pointer constructor.</summary>
			virtual GLFWwindow* InitiateWindow(std::string title, int width, int height, bool resizable = true, bool transparentFramebuffer = false) {
				glfwInit();
				glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
				glfwWindowHint(GLFW_RESIZABLE, (resizable) ? GLFW_TRUE : GLFW_FALSE);
				glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
				
				hwndResizable = resizable;
				hwndWidth = width;
				hwndHeight = height;
				return glfwCreateWindow(hwndWidth, hwndHeight, title.c_str(), nullptr, nullptr);
			}

		public:
			/// <summary>Callback for GLFW Window refresh event.</summary>
			inline static void OnRefreshCallback(GLFWwindow* hwnd) { MvkWindow::onRefresh.invoke(hwnd); }

			/// <summary>Pass to render engine for swapchain resizing.</summary>
			void OnFrameBufferReSizeCallback(int& width, int& height) {
				width = 0;
				height = 0;

				while (width <= 0 || height <= 0)
					glfwGetFramebufferSize(hwndWindow, &width, &height);

				hwndWidth = width;
				hwndHeight = height;
			}

			/// <summary>[overridable] Pass to render engine for swapchain resizing.</summary>
			inline static void OnFrameBufferNotifyReSizeCallback(GLFWwindow* hwnd, int width, int height) { onResizeFrameBuffer.invoke(width, height); }

			// Invokable callback to respond to Vulkan API when the active frame buffer is resized.
			inline static std::invokable<int,int> onResizeFrameBuffer;
			// Invokable callback to respond to GLFWwindowrefreshfun: Window::onRefresh += callback<GLFWwindow*>(ClassInstance, &Class::Function);
			inline static std::invokable<GLFWwindow*> onRefresh;

			void Disposable() {
				glfwDestroyWindow(hwndWindow);
				glfwTerminate();
			}

			/// <summary>Initiialize managed GLFW Window and Vulkan API. Initialize GLFW window unique_ptr.</summary>
			MvkWindow(std::string title, int width, int height, bool resizable, bool transparentFramebuffer = false, bool hasMinSize = false, int minWidth = 200, int minHeight = 200) {
				onDispose += std::callback<>(this, &MvkWindow::Disposable);
				hwndWindow = InitiateWindow(title, width, height, resizable, transparentFramebuffer);
				glfwSetWindowUserPointer(hwndWindow, this);
				glfwSetWindowRefreshCallback(hwndWindow, MvkWindow::OnRefreshCallback);
				glfwSetFramebufferSizeCallback(hwndWindow, MvkWindow::OnFrameBufferNotifyReSizeCallback);
				if (hasMinSize) glfwSetWindowSizeLimits(hwndWindow, minWidth, minHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);
			}

			// Remove default copy constructor.
			MvkWindow(const MvkWindow&) = delete;
			// Remove default copy destructor.
			MvkWindow& operator=(const MvkWindow&) = delete;

			/// <summary>[overridable] Calls glfwPollEvents() then checks if the GLFW window should close.</summary>
			virtual bool ShouldClose() { return glfwWindowShouldClose(hwndWindow); }

			/// <summary>[overridable] Returns the active GLFW window handle.</summary>
			virtual GLFWwindow* GetHandle() { return hwndWindow; }

			/// <summary>[overridable] Polls for window user input events.</summary>
			virtual void PollWindowEvents() { glfwPollEvents(); }

			/// <summary>[overridable] Returns [BOOL] should close and polls input events (optional).</summary>
			virtual bool ShouldClosePollEvents() {
				bool shouldClose = glfwWindowShouldClose(hwndWindow) == GLFW_TRUE;
				PollWindowEvents();
				return shouldClose;
			}

			/// <summary>[overridable] Sets the callback pointer for the window.</summary>
			virtual void SetCallbackPointer(void* data) { glfwSetWindowUserPointer(hwndWindow, data); }

			/// <summary>[overridable] Gets the callback pointer for the window.</summary>
			virtual void* GetCallbackPointer() { return glfwGetWindowUserPointer(hwndWindow); }

			/// <summary>[overridable] Creates a Vulkan surface for this GLFW window.</summary>
			virtual VkSurfaceKHR CreateWindowSurface(VkInstance& instance) {
				VkSurfaceKHR wndSurface;
				if (glfwCreateWindowSurface(instance, hwndWindow, nullptr, &wndSurface) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create GLFW Window Surface!");
				return wndSurface;
			}

			/// <summary>[overridable] Gets the GLFW window handle.</summary>
			virtual GLFWwindow* GetWindowHandle() { return hwndWindow; }

			/// <summary>[overridable] Gets the required GLFW extensions.</summary>
			virtual std::vector<const char*> GetRequiredExtensions(bool enableValidationLayers) {
				return MvkWindow::QueryRequiredExtensions(enableValidationLayers);
			}

			/// <summary>[overridable] Gets the required GLFW extensions.</summary>
			static std::vector<const char*> QueryRequiredExtensions(bool enableValidationLayers) {
				glfwInit();

				uint32_t glfwExtensionCount = 0;
				const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
				std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

				if (enableValidationLayers)
					extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

				return extensions;
			}
			
			/// <summary>[overridable] Returns the window's framebuffer width.</summary>
			virtual int GetWidth() { return MAX(hwndWidth, 1); }

			/// <summary>[overridable] Returns the window's framebuffer height.</summary>
			virtual int GetHeight() { return MAX(hwndHeight, 1); }
		};
	}
#endif