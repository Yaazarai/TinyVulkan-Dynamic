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

		* RunMain() runs a loop until the program ShouldClose().
			This function calls three invokable callbacks for event execution that can be subscribed to:
				onEnterMain (just before main loop)
				onRunMain   (during main loop, after glfwPollEvents())
				onExitMain  (just after main loop)

		* ~Window() destructor.
			Calls onDestruct to execute destructor events.

		* InitiateWindow and TerminateWindow
			are automatically called when the GLFWwindow* window handle is cleaned up by std::unique_ptr<> hwndWindow.

		* CreateWindowSurface(), GetFrameBufferSize() and GetRequiredExtensions() must be puiblically exposed
			for MiniVulkan, more specifically the class MiniVkLayer to call them for the Vulkan API.

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
#include "./MiniVk.hpp"

namespace MINIVULKAN_NS {
	class MiniVkWindow : public MiniVkObject {
	private:
		bool hwndResizable;
		int hwndWidth, hwndHeight;
		std::string title;
		GLFWwindow* hwndWindow;

		/// <summary>GLFWwindow unique pointer constructor.</summary>
		virtual GLFWwindow* InitiateWindow(int width, int height, bool resizable, std::string title) {
			glfwInit();
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_RESIZABLE, (resizable) ? GLFW_TRUE : GLFW_FALSE);
			hwndResizable = resizable;
			hwndWidth = width;
			hwndHeight = height;
			return glfwCreateWindow(hwndWidth, hwndHeight, title.c_str(), nullptr, nullptr);
		}

	public:
		/// <summary>Callback for GLFW Window refresh event.</summary>
		inline static void OnRefreshCallback(GLFWwindow* hwnd) { MiniVkWindow::onRefresh.invoke(hwnd); }

		/// <summary>[overridable] Pass to render engine for swapchain resizing.</summary>
		void OnFrameBufferReSizeCallback() {
			int width = 0, height = 0;
			GLFWwindow* window = hwndWindow;
			glfwGetFramebufferSize(window, &width, &height);

			while (width == 0 || height == 0) {
				glfwGetFramebufferSize(window, &width, &height);
				glfwWaitEvents();
			}
		}

		/// <summary>[overridable] Pass to render engine for swapchain resizing.</summary>
		inline static void OnFrameBufferNotifyReSizeCallback(GLFWwindow* hwnd, int width, int height) { onResizeFrameBuffer.invoke(); }

		// Invokable callback to respond to Vulkan API when the active frame buffer is resized.
		inline static std::invokable<> onResizeFrameBuffer;
		// Invokable callback to respond to GLFWwindowrefreshfun: Window::onRefresh += callback<GLFWwindow*>(ClassInstance, &Class::Function);
		inline static std::invokable<GLFWwindow*> onRefresh;
		// Invokable callback that executes before the main loop executes.
		std::invokable<void*> onEnterMain;
		// Invokable callback that executes when the Window is running in it's main loop.
		std::invokable<void*> onRunMain;
		// Invokable callback that executes after the main loop closes.
		std::invokable<void*> onExitMain;

		void Disposable() {
			glfwDestroyWindow(hwndWindow);
			glfwTerminate();
		}

		/// <summary>[overridable] Initiialize managed GLFW Window and Vulkan API. Initialize GLFW window unique_ptr.</summary>
		MiniVkWindow(int width, int height, bool resizable, std::string title) {
			onDispose += std::callback<>(this, &MiniVkWindow::Disposable);
			hwndWindow = InitiateWindow(width, height, resizable, title);
			glfwSetWindowUserPointer(hwndWindow, this);
			glfwSetWindowRefreshCallback(hwndWindow, MiniVkWindow::OnRefreshCallback);
			glfwSetFramebufferSizeCallback(hwndWindow, MiniVkWindow::OnFrameBufferNotifyReSizeCallback);
		}

		// Remove default copy constructor.
		MiniVkWindow(const MiniVkWindow&) = delete;
		// Remove default copy destructor.
		MiniVkWindow& operator=(const MiniVkWindow&) = delete;

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

		virtual void SetCallbackPointer(void* data) { glfwSetWindowUserPointer(hwndWindow, data); }

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

		/// <summary>[overridable] Gets the GLFW window frame buffer size.</summary>
		virtual void GetFrameBufferSize(int& width, int& height) { glfwGetFramebufferSize(hwndWindow, &width, &height); }

		/// <summary>[overridable] Gets the required GLFW extensions.</summary>
		virtual std::vector<const char*> GetRequiredExtensions(bool enableValidationLayers) {
			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
			std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

			if (enableValidationLayers)
				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			return extensions;
		}
	};
}
#endif