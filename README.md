# MiniVulkan-Dynamic
Vulkan Graphics render engine built using Dynamic Rendering and Push Constants/Descriptors.

VULKAN DEVICE EXTENSIONS: `VK_KHR_SWAPCHAIN_EXTENSION_NAME`, `VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME`,  `VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME`, `VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME`, `VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME`.

LIST OF FEATURES:
1. Push Descriptors & Push Constants
2. Dynamic Viewports & Scissors
3. 2D Projection Matrix (Can be easily swapped to 3D if needed).
4. Multi-Draw Indirect Rendering commands enabled for batch rendering.
5. Easy Invokable Events & Function callbacks subscription model (like C#).
   <br/> https://github.com/FatalSleep/Event-Callback
6. VMA implementation for memory management & allocation.
   <br/> https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
7. Implementation for Image and Buffer staging on GPU (for passing CPU bound images, verftex buffers, etc. to the GPU).
8. Easy creation of Vulkan stages for the entire render pipeline.
9. Strict Order of Dependency Design (abstracted for each stage of Vulkan to be as independent as possible from other stages).
10. Multiple Screen Buffering Modes: Double, Triple, Quadruple.
11. Thread-Pool implementation.
12. Window transparency using GLFW `transparentFramebuffer`.
13. Headless Rendering via `MiniVkImageRenderer` for rendering outside the swapchain (`MiniVkSwapChainRenderer`).

The `source.cpp` example loads QOI images (an example image is provided). Please see the official QOI repo for the full source code and any provided examples or commandline tools for dealing with QOI images (alternative to .png): https://qoiformat.org


### ABOUT MINIVULKAN-DYNAMIC

MiniVulkan-Dynamic (MV-D) is a simple to understand and use Vulkan render engine that relies on dynamic rendering instead of purpose built render-passes. This allows for rapid development with low-maintenance overhead. On top of this MV-D relies on utilizing Push Constants and Push Descriptors for all render data being passed too the GPU, this simplifies and removes maintenance overhead for managing descriptor sets and pools. The core philosophy behind MV-D ordered-minimal-dependency between systems. Meaning that each subsytem implementation (see below or `MiniVK.hpp` for include order) contains only the required subsytems before it in order to operate. With that in mind this helps to keep the MV-D library clean, understandable and easy to modify.

The entire MV-D library relies on two very important support headers `invokable.hpp` (see link above) and `disposable.hpp`. **Invokable** allows you to create multiple event-style functions (`callbacks<A...>`) which can all be hooked into an event (`invokable<A..>`) with the same arguments and executed all at once, this can be done with static, lambda and member-functions. **Disposable** (requires invokable) is a parent-class with an invokable event `onDispose<bool>` for hooking in events for cleaning up dynamic memory resources when either the disposable object goes out of scope and its destructor is called or when manually called with `Dispose(default = TRUE)`--note that the event `onDispose` allows for passing in a `BOOL` for checking if extra work should be done--in the case of MV-D if Vulkan's Render Device `WaitIdle` should be called to wait for the GPU to be finished with resources before disposing.

Finally MiniVulkan-Dynamic implements GLFW for its window back-end. All of the GLFW required function calls are ONLY within this header library and as such should allow you to overwrite or ignore the implementation with ease if you want to use something like SDL.

### MINIVULKAN-DYNAMIC API

`invokable.hpp`: For creating callable functions that can be hooked to invokable events for a hook/unhook (subscribe/unsubscribe) style execution system.
```C++
// Callback Constructors / Operators / Functions:
callback<A...> mycallback(&static_function) // static.
callback<A...> mycallback([](){cout << "Lambda Call" << endl;}) // lambda.
callback<A...> mycallback(&myobj, &static_function) // member.
bool operator == (const callback<A...>& cb) // Compares two callback objects by their FUNCTION hash.
bool operator != (const callback<A...>& cb) // Compares two callback objects by their FUNCTION hash.
size_t hash_code() // Get the FUNCTION hash code for the callback.
callback<A..>& invoke(A... args) // Executes the callback with the appropriate arguments then returns itself (function-chaining).

// Invokable Constructors / Operators / Functions: (callback<A...> MUST have the same argument types)
invokable<A...>& hook(const callback<A...> cb) // Hooks callback<A..> to this event.
invokable<A...>& unhook(const callback<A...> cb) // Unhooks callback<A..> from this event.
invokable<A...>& rehook(const callback<A...> cb) // Hooks callback<A..> to this event and unhooks ALL other callbacks.
invokable<A...>& empty() // Unhooks ALL callback<A...> from this event.
invokable<A...>& invoke(A... args) // Invokaes ALL callback<A...> hooked to this event with the arguments passed.
/*
** NOTE: If you have pointers/references that callbacks can modify, changes occur in order the callbacks were hooked to invokable.
*/
```

`disposable.hpp` parent class-interface for creating disposable objects with dynamic memory resources that need cleanup.
```C++
#define DISPOSABLE_BOOL_DEFAULT true // can be re-defined if the default needs to be changed to false.
~disposable() // Calls Dispose() on out-of-scope with onDispose<bool> set to TRUE by default.
disposable() // Default constructor
bool IsDisposed() // Returns TRUE/FALSE if the object has had Dispose() called on it.
DisposeOrdered(std::vector<disposable> objlist, bool descending) // Disposes vector of disposables (order default descending).
```

`MiniVkThreadPool.hpp` basic thread pool implementation (included, but not used). Has a queue of `callback<atomic_bool&>` that the thread pool will execute as each thread in the pool is free to execute a new task. Each `callback<atomic_bool&>` MUST check for the atomic_bool to see if it should continue working or cease and exit.
```C++
MiniVkThreadPool(const uint32_t potentialThreads, bool start) // Spawns N threads on the thread pool if start is true.
size_t GetSize() // Returns the number of threads in the pool.
void ClearCallbacks(size_t timeout) // Removes all callbacks from the threadpool queue.
void AwaitClosePool(size_t timeout) // Attempt close thread pool, if timeout (milliseconds) exceeded pool will not close.
void TrySpawnThreads(const uint32_t potentialThreads, bool start, size_t timeout) // Spawns N threads on the thread pool.
bool TryPushCallback(callback<std::atomic_bool&> cb, size_t timeout) // Attempts to queue up a new callback.
/*
** NOTE: Thread pool will ATTEMPT to spawn required thread count, but will default count to std::thread::hardware_concurrency().
** NOTE: timeout parameter for all functions defaults to 100 milliseconds (sze_t timeout = 100).
** NOTE: If you instantiate MiniVkThreadPool with startWorking = false, you'll need to call trySpawnThread() manually.
*/
```

`MiniVkWindow.hpp`
`MiniVkSupporters.hpp`
`MiniVkQueueFamily.hpp`
`MiniVkInstance.hpp`
`MiniVkRenderDevice.hpp`
`MiniVkSwapChain.hpp`
`MiniVkCommandPool.hpp`
`MiniVkVMAllocator.hpp`
`MiniVkBuffer.hpp`
`MiniVkImage.hpp`
`MiniVkShaderStages.hpp`
`MiniVkDynamicPipeline.hpp`
`MiniVkDynamicRenderer.hpp`
`MiniVkVertexMath.hpp`
