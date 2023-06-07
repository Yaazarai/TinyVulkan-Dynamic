#pragma once
#ifndef TINYVK_TINYVKWINDOW
#define TINYVK_TINYVKWINDOW
	#include "./TinyVK.hpp"

	typedef void (*GLFWgamepadbuttonfun)(const GLFWgamepadstate* gamepad, int32_t gpad, int32_t button, int32_t action);
	typedef void (*GLFWgamepadaxisfun)(const GLFWgamepadstate* gamepad, int32_t gpad, int32_t axisXID, float_t axisX, int32_t axisYID, float_t axisY);
	typedef void (*GLFWgamepadtriggerfun)(const GLFWgamepadstate* gamepad, int32_t gpad, int32_t axisXID, float_t axis);
	GLFWAPI GLFWgamepadbuttonfun glfwGamepadButtonCallback = NULL;
	GLFWAPI GLFWgamepadaxisfun glfwGamepadAxisCallback = NULL;
	GLFWAPI GLFWgamepadtriggerfun glfwGamepadTriggerCallback = NULL;
	GLFWAPI void glfwSetGamepadButtonCallback(GLFWgamepadbuttonfun callback) { glfwGamepadButtonCallback = callback; }
	GLFWAPI void glfwSetGamepadAxisCallback(GLFWgamepadaxisfun callback) { glfwGamepadAxisCallback = callback; }
	GLFWAPI void glfwSetGamepadTriggerCallback(GLFWgamepadtriggerfun callback) { glfwGamepadTriggerCallback = callback; }
	GLFWAPI GLFWgamepadstate glfwGamepadCache[GLFW_JOYSTICK_LAST + 1]{};
	GLFWAPI float_t glfwRoundfd(float_t value, float_t precision) { return std::round(value * precision) / precision; }
	GLFWAPI void glfwPollGamepads() {
		for (int32_t i = 0; i <= GLFW_JOYSTICK_LAST; i++) {
			if (glfwJoystickPresent(i) == GLFW_FALSE) continue;

			GLFWgamepadstate gpstate;
			glfwGetGamepadState(i, &gpstate);

			for(int32_t b = 0; b <= GLFW_GAMEPAD_BUTTON_LAST; b++) {
				if (gpstate.buttons[b] != glfwGamepadCache[i].buttons[b]) {
					glfwGamepadCache[i].buttons[b] = gpstate.buttons[b];
					glfwGamepadButtonCallback(&gpstate, i, b, gpstate.buttons[b]);
				}
			}

			if (glfwRoundfd(gpstate.axes[GLFW_GAMEPAD_AXIS_LEFT_X], 1000) != glfwRoundfd(glfwGamepadCache[i].axes[GLFW_GAMEPAD_AXIS_LEFT_X], 1000)
				|| glfwRoundfd(gpstate.axes[GLFW_GAMEPAD_AXIS_LEFT_Y], 1000) != glfwRoundfd(glfwGamepadCache[i].axes[GLFW_GAMEPAD_AXIS_LEFT_Y], 1000)) {
				glfwGamepadCache[i].axes[GLFW_GAMEPAD_AXIS_LEFT_X] = gpstate.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
				glfwGamepadCache[i].axes[GLFW_GAMEPAD_AXIS_LEFT_Y] = gpstate.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];

				if (glfwGamepadAxisCallback != NULL)
					glfwGamepadAxisCallback(&gpstate, i, GLFW_GAMEPAD_AXIS_LEFT_X, gpstate.axes[GLFW_GAMEPAD_AXIS_LEFT_X], GLFW_GAMEPAD_AXIS_LEFT_Y, gpstate.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);
			}

			if (glfwRoundfd(gpstate.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], 1000) != glfwRoundfd(glfwGamepadCache[i].axes[GLFW_GAMEPAD_AXIS_RIGHT_X], 1000)
				|| glfwRoundfd(gpstate.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y], 1000) != glfwRoundfd(glfwGamepadCache[i].axes[GLFW_GAMEPAD_AXIS_RIGHT_Y], 1000)) {
				glfwGamepadCache[i].axes[GLFW_GAMEPAD_AXIS_RIGHT_X] = gpstate.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
				glfwGamepadCache[i].axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] = gpstate.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
				
				if (glfwGamepadAxisCallback != NULL)
					glfwGamepadAxisCallback(&gpstate, i, GLFW_GAMEPAD_AXIS_RIGHT_X, gpstate.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], GLFW_GAMEPAD_AXIS_RIGHT_Y, gpstate.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);
			}

			if (glfwRoundfd(gpstate.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER], 1000) != glfwRoundfd(glfwGamepadCache[i].axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER], 1000)) {
				glfwGamepadCache[i].axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] = gpstate.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
				
				if (glfwGamepadTriggerCallback != NULL)
					glfwGamepadTriggerCallback(&gpstate, i, GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, gpstate.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]);
			}

			if (glfwRoundfd(gpstate.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER], 1000) != glfwRoundfd(glfwGamepadCache[i].axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER], 1000)) {
				glfwGamepadCache[i].axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = gpstate.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
				
				if (glfwGamepadTriggerCallback != NULL)
					glfwGamepadTriggerCallback(&gpstate, i, GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, gpstate.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]);
			}
		}
	}

	namespace TINYVULKAN_NAMESPACE {
		/*
			tinyvulkan::Window is the GLFW window handler for TinyVulkan that will link to
			and initialize GLFW and Vulkan to create your game/application window.
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
					for TinyVulkan, more specifically the class MvkLayer to call them for the Vulkan API.

			NOTE: That this class is marked with a defaulted template for GLFW. You can override this template
				and then override/redefine ALL virtual methods/constructors/destructors
				[Constructor, Destructor, CreateWindow, ShouldClose, GetHandle, RunMain] to use a different window system from GLFW.

				This template is largely redundant and not used as it is simply there to provide the default type
				for explicitly defining the API, in this case GLFW. You can remove it and change T to GLFWwindow or change the
				window API entirely as long as you expose [public] these functions:
					CreateWindowSurface(), GetFrameBufferSize() and GetRequiredExtensions().

			Keyboard Inputs:
				You can #define the macro below if you do NOT want keys to repeat their press/release event states when they are first pressed/released:
					TINYVK_WINDOW_NOTRIGGER_REPEAT_INITIAL_STATE

				If the above macro is defined the press/release events will lag behind one-frame as they won't trigger until the next frame for consecutive
				repeats of the same key event.

				///
				/// TINYVK_NOPOLLING_GAMEPADS
				///		#define the above if you do NOT want the window to poll gamepad states/events.
				///
		*/
		
		/// <summary>GLFW window handler for TinyVulkan that will link to and initialize GLFW and Vulkan to create your game/application window</summary>
		class TinyVkWindow : public disposable {
		private:
			bool hwndResizable;
			int hwndWidth, hwndHeight, hwndXpos, hwndYpos;
			std::string title;
			GLFWwindow* hwndWindow;

			invokable<GLFWwindow*, int, int> frameBufferResized;
			invokable<GLFWwindow*, int, int> windowPositionMoved;
			inline static invokable<GLFWwindow*, int, int> onWindowResized;
			inline static invokable<GLFWwindow*, int, int> onWindowPositionMoved;

			/// <summary>GLFWwindow unique pointer constructor.</summary>
			virtual GLFWwindow* InitiateWindow(std::string title, int width, int height, bool resizable = true, bool transparentFramebuffer = false) {
				glfwInit();
				glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
				glfwWindowHint(GLFW_RESIZABLE, (resizable) ? GLFW_TRUE : GLFW_FALSE);
				glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, (transparentFramebuffer) ? GLFW_TRUE : GLFW_FALSE);

				if (!glfwVulkanSupported())
					throw new std::runtime_error("TinyVulkan: GLFW implementation could not locate Vulkan loader.");

				hwndResizable = resizable;
				hwndWidth = width;
				hwndHeight = height;
				return glfwCreateWindow(hwndWidth, hwndHeight, title.c_str(), nullptr, nullptr);
			}

			/// <summary>Generates an event for window framebuffer resizing.</summary>
			inline static void OnFrameBufferNotifyReSizeCallback(GLFWwindow* hwnd, int width, int height) {
				onResizeFrameBuffer.invoke(hwnd, width, height);
				onWindowResized.invoke(hwnd, width, height);
			}

			/// <summary>Generates an event for window position moved.</summary>
			inline static void OnWindowPositionCallback(GLFWwindow* hwnd, int xpos, int ypos) {
				onWindowPositionMoved.invoke(hwnd, xpos, ypos);
			}
			
		public:
			/// <summary>Calls the disposable interface dispose event.</summary>
			~TinyVkWindow() { this->Dispose(); }

			/// <summary>Disposable function for disposable class interface and window resource cleanup.</summary>
			void Disposable(bool waitIdle) {
				glfwDestroyWindow(hwndWindow);
				glfwTerminate();
			}

			/// <summary>Initiialize managed GLFW Window and Vulkan API. Initialize GLFW window unique_ptr.</summary>
			TinyVkWindow(std::string title, int width, int height, bool resizable, bool transparentFramebuffer = false, bool hasMinSize = false, int minWidth = 200, int minHeight = 200) {
				onDispose.hook(callback<bool>([this](bool forceDispose){this->Disposable(forceDispose); }));
				onWindowResized.hook(callback<GLFWwindow*, int, int>([this](GLFWwindow* hwnd, int width, int height) { if (hwnd != hwndWindow) return; hwndWidth = width; hwndHeight = height; }));
				onWindowPositionMoved.hook(callback<GLFWwindow*, int, int>([this](GLFWwindow* hwnd, int xpos, int ypos) { if (hwnd != hwndWindow) return; hwndXpos = xpos; hwndYpos = ypos; }));

				hwndWindow = InitiateWindow(title, width, height, resizable, transparentFramebuffer);
				glfwSetWindowUserPointer(hwndWindow, this);
				glfwSetFramebufferSizeCallback(hwndWindow, TinyVkWindow::OnFrameBufferNotifyReSizeCallback);
				glfwSetWindowPosCallback(hwndWindow, TinyVkWindow::OnWindowPositionCallback);

				if (hasMinSize) glfwSetWindowSizeLimits(hwndWindow, minWidth, minHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);

				InitGLFWInput();
			}

			/// <summary>Remove default copy constructor.</summary>
			TinyVkWindow(const TinyVkWindow&) = delete;

			/// <summary>Remove default copy destructor.</summary>
			TinyVkWindow& operator=(const TinyVkWindow&) = delete;

			/// <summary>Invokable callback to respond to Vulkan API when the active frame buffer is resized.</suimmary>
			inline static invokable<GLFWwindow*, int, int> onResizeFrameBuffer;

			/// <summary>Pass to render engine for swapchain resizing.</summary>
			void OnFrameBufferReSizeCallback(int& width, int& height) {
				width = 0;
				height = 0;

				while (width <= 0 || height <= 0)
					glfwGetFramebufferSize(hwndWindow, &width, &height);

				hwndWidth = width;
				hwndHeight = height;
			}

			/// <summary>[overridable] Checks if the GLFW window should close.</summary>
			virtual bool ShouldClose() { return glfwWindowShouldClose(hwndWindow) == GLFW_TRUE; }

			/// <summary>[overridable] Returns the active GLFW window handle.</summary>
			virtual GLFWwindow* GetHandle() { return hwndWindow; }

			/// <summary>[overridable] Returns [BOOL] should close and polls input events (optional).</summary>
			virtual bool ShouldClosePollEvents() {
				bool shouldClose = glfwWindowShouldClose(hwndWindow) == GLFW_TRUE;
				glfwPollEvents();
				#ifdef TINYVK_ALLOWS_POLLING_GAMEPADS
				glfwPollGamepads();
				#endif
				return shouldClose;
			}

			/// <summary>[overridable] Returns [BOOL] should close and polls input events (optional).</summary>
			virtual bool ShouldCloseWaitEvents() {
				bool shouldClose = ShouldClose();
				glfwWaitEvents();
				#ifdef TINYVK_ALLOWS_POLLING_GAMEPADS
				glfwPollGamepads();
				#endif
				return shouldClose;
			}

			/// <summary>[overridable] Sets the callback pointer for the window.</summary>
			virtual void SetCallbackPointer(void* data) { glfwSetWindowUserPointer(hwndWindow, data); }

			/// <summary>[overridable] Gets the callback pointer for the window.</summary>
			virtual void* GetCallbackPointer() { return glfwGetWindowUserPointer(hwndWindow); }

			/// <summary>[overridable] Creates a Vulkan surface for this GLFW window.</summary>
			virtual VkSurfaceKHR CreateWindowSurface(VkInstance instance) {
				VkSurfaceKHR wndSurface;
				if (glfwCreateWindowSurface(instance, hwndWindow, nullptr, &wndSurface) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to create GLFW Window Surface!");
				return wndSurface;
			}

			/// <summary>[overridable] Gets the GLFW window handle.</summary>
			virtual GLFWwindow* GetWindowHandle() { return hwndWindow; }

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
			virtual int GetWidth() { return std::max(hwndWidth, 1); }

			/// <summary>[overridable] Returns the window's framebuffer height.</summary>
			virtual int GetHeight() { return std::max(hwndHeight, 1); }

			virtual int GetXpos() { return hwndXpos; }

			virtual int GetYpos() { return hwndYpos; }

			/// <summary>Executes functions in the main window loop (w/ ref to bool to exit loop as needed).</summary>
			invokable<std::atomic_bool&> onWhileMain;

			/// <summary>[overridable] Executes the main window loop.</summary>
			virtual void WhileMain(const bool waitOrPollEvents = true) {
				std::atomic_bool shouldRun = true;

				while (shouldRun) {
					onWhileMain.invoke(shouldRun);
					shouldRun = (waitOrPollEvents)? !ShouldCloseWaitEvents() : !ShouldClosePollEvents();
				}
			}

			#pragma region GLFW GAMEPAD API

		private:
			inline static std::unordered_map<TinyVkKeyboardButtons, TinyVkInputEvents> cachedKeyboardStates;
			inline static TinyVkInputEvents cachedMouseStates[static_cast<int32_t>(TinyVkMouseButtons::BUTTON_LAST) + 1];

			struct TinyVkGamepadStruct { int32_t id; TinyVkInputEvents buttons[GLFW_JOYSTICK_LAST + 1]; };
			inline static TinyVkGamepadStruct cachedGamepadStates[GLFW_JOYSTICK_LAST + 1];

			inline static invokable<GLFWwindow*, TinyVkKeyboardButtons, TinyVkInputEvents, TinyVkModKeyBits> KeyboardButton;
			inline static invokable<GLFWwindow*, TinyVkMouseButtons, TinyVkInputEvents, TinyVkModKeyBits> MouseButton;
			inline static invokable<GLFWwindow*, double_t, double_t> MouseMoved;
			inline static invokable<GLFWwindow*, double_t, double_t> MouseScrolled;
			inline static invokable<GLFWwindow*, bool> MouseEntered;
			inline static invokable<TinyVkGamepads, bool> GamepadConnection;
			inline static invokable<TinyVkGamepads, TinyVkGamepadButtons, TinyVkInputEvents> GamepadButton;
			inline static invokable<TinyVkGamepads, TinyVkGamepadAxis, float_t, TinyVkGamepadAxis, float_t> GamepadAxis;
			inline static invokable<TinyVkGamepads, TinyVkGamepadAxis, float_t> GamepadTrigger;

			inline static void KeyboardButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t scancode, int32_t mods) {
				KeyboardButton.invoke(window, static_cast<TinyVkKeyboardButtons>(button), static_cast<TinyVkInputEvents>(action), static_cast<TinyVkModKeyBits>(mods));
			}
			inline static void MouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t mods) {
				MouseButton.invoke(window, static_cast<TinyVkMouseButtons>(button), static_cast<TinyVkInputEvents>(action), static_cast<TinyVkModKeyBits>(mods));
			}
			inline static void MouseMovedCallback(GLFWwindow* window, double_t xpos, double_t ypos) {
				MouseMoved.invoke(window, xpos, ypos);
			}
			inline static void MouseScrolledCallback(GLFWwindow* window, double_t xoffset, double_t yoffset) {
				MouseScrolled.invoke(window, xoffset, yoffset);
			}
			inline static void MouseEnteredCallback(GLFWwindow* window, int32_t entered) {
				MouseEntered.invoke(window, entered);
			}
			inline static void GamepadConnectionCallback(int32_t gpad, int32_t connected) {
				GamepadConnection.invoke(static_cast<TinyVkGamepads>(gpad), connected == GLFW_CONNECTED);
			}
			inline static void GamepadButtonCallback(const GLFWgamepadstate* gpstate, int32_t gpad, int32_t button, int32_t action) {
				GamepadButton.invoke(static_cast<TinyVkGamepads>(gpad), static_cast<TinyVkGamepadButtons>(button), static_cast<TinyVkInputEvents>(action));
			}
			inline static void GamepadAxisCallback(const GLFWgamepadstate* gpstate, int32_t gpad, int32_t axisXID, float_t axisX, int32_t axisYID, float_t axisY) {
				GamepadAxis.invoke(static_cast<TinyVkGamepads>(gpad), static_cast<TinyVkGamepadAxis>(axisXID), axisX, static_cast<TinyVkGamepadAxis>(axisYID), axisY);
			}
			inline static void GamepadTriggerCallback(const GLFWgamepadstate* gpstate, int32_t gpad, int32_t axisID, float_t axis) {
				GamepadTrigger.invoke(static_cast<TinyVkGamepads>(gpad), static_cast<TinyVkGamepadAxis>(axisID), axis);
			}

			void KeyboardButtonHandler(GLFWwindow* wnd, TinyVkKeyboardButtons button, TinyVkInputEvents action, TinyVkModKeyBits modKeys) {
				if (wnd != hwndWindow) return;

				TinyVkInputEvents cachedAction = action;
				auto cached = cachedKeyboardStates.find(button);
				if (cached != cachedKeyboardStates.end())
					cachedAction = cached->second;

				onKeyboardButtonChanged.invoke(button, modKeys, action, cachedAction);
			}
			void MouseButtonHandler(GLFWwindow* wnd, TinyVkMouseButtons button, TinyVkInputEvents action, TinyVkModKeyBits modKeys) {
				if (wnd != hwndWindow) return;

				TinyVkInputEvents cachedAction = cachedMouseStates[static_cast<int32_t>(button)];
				onMouseButtonChanged.invoke(button, modKeys, action, cachedAction);
			}
			void MouseMovedHandler(GLFWwindow* wnd, double_t xpos, double_t ypos) {
				if (wnd != hwndWindow) return;

				onMouseMoved.invoke(xpos, ypos);
			}
			void MouseScrolledHandler(GLFWwindow* wnd, double_t xoffset, double_t yoffset) {
				if (wnd != hwndWindow) return;

				onMouseScrolled.invoke(xoffset, yoffset);
			}
			void MouseEnteredHandler(GLFWwindow* wnd, bool entered) {
				if (wnd != hwndWindow) return;

				onMouseEntered.invoke(entered);
			}
			void GamepadConnectionHandler(TinyVkGamepads gpad, bool connected) {
				onGamepadConnectionChanged.invoke(gpad, connected);
			}
			void GamepadButtonHandler(TinyVkGamepads gpad, TinyVkGamepadButtons button, TinyVkInputEvents action) {
				TinyVkInputEvents cachedAction = cachedGamepadStates->buttons[static_cast<int32_t>(button)];
				cachedGamepadStates->buttons[static_cast<int32_t>(button)] = static_cast<TinyVkInputEvents>(action);
				onGamepadButtonChanged.invoke(gpad, button, action, cachedAction);
			}
			void GamepadAxisHandler(TinyVkGamepads gpad, TinyVkGamepadAxis axisXID, float_t axisX, TinyVkGamepadAxis axisYID, float_t axisY) {
				onGamepadAxisChanged.invoke(gpad, axisXID, axisX, axisY);
			}
			void GamepadTriggerHandler(TinyVkGamepads gpad, TinyVkGamepadAxis axisID, float_t axis) {
				onGamepadTriggerChanged.invoke(gpad, axisID, axis);
			}

			void InitGLFWInput() {
				glfwSetInputMode(hwndWindow, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
				glfwSetKeyCallback(hwndWindow, KeyboardButtonCallback);
				glfwSetMouseButtonCallback(hwndWindow, MouseButtonCallback);
				glfwSetCursorPosCallback(hwndWindow, MouseMovedCallback);
				glfwSetScrollCallback(hwndWindow, MouseScrolledCallback);
				glfwSetCursorEnterCallback(hwndWindow, MouseEnteredCallback);
				glfwSetJoystickCallback(GamepadConnectionCallback);
				glfwSetGamepadButtonCallback(GamepadButtonCallback);
				glfwSetGamepadAxisCallback(GamepadAxisCallback);
				glfwSetGamepadTriggerCallback(GamepadTriggerCallback);

				KeyboardButton.hook(callback<GLFWwindow*, TinyVkKeyboardButtons, TinyVkInputEvents, TinyVkModKeyBits>([this](GLFWwindow* window, TinyVkKeyboardButtons button, TinyVkInputEvents action, TinyVkModKeyBits mods) { this->KeyboardButtonHandler(window, button, action, mods); }));
				MouseButton.hook(callback<GLFWwindow*, TinyVkMouseButtons, TinyVkInputEvents, TinyVkModKeyBits>([this](GLFWwindow* window, TinyVkMouseButtons button, TinyVkInputEvents action, TinyVkModKeyBits mods) { this->MouseButtonHandler(window, button, action, mods); }));
				MouseMoved.hook(callback<GLFWwindow*, double_t, double_t>([this](GLFWwindow* window, double_t xpos, double_t ypos) { this->MouseMovedHandler(window, xpos, ypos); }));
				MouseScrolled.hook(callback<GLFWwindow*, double_t, double_t>([this](GLFWwindow* window, double_t xoffset, double_t yoffset) { this->MouseScrolledHandler(window, xoffset, yoffset); }));
				MouseEntered.hook(callback<GLFWwindow*, bool>([this](GLFWwindow* window, bool entered) { this->MouseEnteredHandler(window, entered); }));
				GamepadConnection.hook(callback<TinyVkGamepads, bool>([this](TinyVkGamepads gpad, bool connected) { this->GamepadConnectionHandler(gpad, connected); }));
				GamepadButton.hook(callback<TinyVkGamepads, TinyVkGamepadButtons, TinyVkInputEvents>([this](TinyVkGamepads gpad, TinyVkGamepadButtons button, TinyVkInputEvents action) { this->GamepadButtonHandler(gpad, button, action); }));
				GamepadAxis.hook(callback<TinyVkGamepads, TinyVkGamepadAxis, float_t, TinyVkGamepadAxis, float_t>([this](TinyVkGamepads gpad, TinyVkGamepadAxis axisXID, float_t axisX, TinyVkGamepadAxis axisYID, float_t axisY) { this->GamepadAxisHandler(gpad, axisXID, axisX, axisYID, axisY); }));
				GamepadTrigger.hook(callback<TinyVkGamepads, TinyVkGamepadAxis, float_t>([this](TinyVkGamepads gpad, TinyVkGamepadAxis axisID, float_t axis) { this->GamepadTriggerHandler(gpad, axisID, axis); }));

				for (int32_t i = 0; i < GLFW_JOYSTICK_LAST; i++) {
					if (glfwJoystickPresent(i))
						onGamepadInitializeConnection.invoke(static_cast<TinyVkGamepads>(i));
				}
			}
		public:
			/// <summary>Keyboard Event Button Changed Event: Key, Modifiers, Action, PreviousAction.</summary>
			invokable<TinyVkKeyboardButtons, TinyVkModKeyBits, TinyVkInputEvents, TinyVkInputEvents> onKeyboardButtonChanged;
			/// <summary>Mouse Event Button Changed Event: Button, Modifiers, Action, PreviousAction.</summary>
			invokable<TinyVkMouseButtons, TinyVkModKeyBits, TinyVkInputEvents, TinyVkInputEvents> onMouseButtonChanged;
			/// <summary>Mouse Event Moved: X-Axis, Y-Axis.</summary>
			invokable<double_t, double_t> onMouseMoved;
			/// <summary>Mosue Event Scrolled: Y-Axis.</summary>
			invokable<double_t, double_t> onMouseScrolled;
			/// <summary>Mouse Event Entered Window: True/False.</summary>
			invokable<bool> onMouseEntered;
			/// <summary>Gamepad Event Button Changed Event: GamepadID, Button, Action, PreviousAction.</summary>
			invokable<TinyVkGamepads, TinyVkGamepadButtons, TinyVkInputEvents, TinyVkInputEvents> onGamepadButtonChanged;
			/// <summary>Gamepad Event Axis CHanged: Gamepad ID, Axis ID, X-Axis, Y-Axis.</summary>
			invokable<TinyVkGamepads, TinyVkGamepadAxis, float_t, float_t> onGamepadAxisChanged;
			/// <summary>Gamepad Event Trigger Axis Changed: Gamepad ID, Axis ID, Y-Axis.</summary>
			invokable<TinyVkGamepads, TinyVkGamepadAxis, float_t> onGamepadTriggerChanged;
			/// <summary>Gamepad Event Initialized (Window Startup) Connected: True/False.</summary>
			inline static invokable<TinyVkGamepads> onGamepadInitializeConnection;
			/// <summary>Gamepad Event Connected: True/False.</summary>
			invokable<TinyVkGamepads, bool> onGamepadConnectionChanged;

			/// <summary>Gets the name of a GLFW gamepad.</summary>
			std::string GamepadGlfwGetName(TinyVkGamepads gpad) {
				const char* name = glfwGetJoystickName(static_cast<int32_t>(gpad));
				return (name != NULL) ? std::string(name) : std::string();
			}

			#pragma endregion
		};
	}
#endif