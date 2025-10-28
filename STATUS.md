Project status as of 2025-10-27 (src directory only)

### src/core
- `src/core/Application.cpp` — `initWindow` currently throws on the successful `SDL_Init` path, so the app never creates its window; extensive TODO comments document major unimplemented systems (resource ownership, async work, caches, multi-threading, GLTF/KTX2, ImGui, instancing). Rendering still relies on the single-time command pattern noted in-file.
- `src/core/Application.h` — Contains duplicate `Utils.h` includes and a lingering TODO about replacing raw pointers with smart pointers; state mirrors the constructor usage seen in `Application.cpp`.
- `src/core/main.cpp` — Minimal wrapper that instantiates `Application` and reports exceptions to stderr; otherwise unchanged.
- `src/core/Types.h` — Declares a comprehensive `Texture` interface but only defines the convenience constructor at the end; the rest of the class lacks definitions, leaving the type unusable at link time.
- `src/core/Utils.cpp` — Empty translation unit; no helper implementations exist for the declarations expected from `Utils.h`.
- `src/core/Utils.h` — Aggregates wide-ranging platform, Vulkan, SDL, GLM, VMA, stb, and tinyobj includes; defines `Vertex`/`UniformBufferObject`, file IO helper, constants, and asset paths. Acts as a monolithic utility header with heavy dependencies.

### src/graphics
- `src/graphics/DescriptorManager.cpp` — Builds descriptor set layout, pool, and sets for per-frame uniform buffers plus a single combined image sampler; logic assumes `uniformBuffers` and sampler/image view lifetimes are externally managed.
- `src/graphics/DescriptorManager.h` — Exposes the manager with public members storing device references and descriptor objects; initialization deferred to `init()` as reflected in the cpp file.
- `src/graphics/Pipeline.cpp` — The pipeline setup is hard-wired to a single SPIR-V module (`shader.spv`) expected to provide both `vertMain` and `fragMain` entries; dynamic rendering path is configured without a render pass, aligning with the resource manager depth format.
- `src/graphics/Pipeline.h` — Holds references to the device, swap-chain parameters, descriptor layout, and resource manager; only `init()`/`createGraphicsPipeline()` are defined, matching the cpp implementation.
- `src/graphics/Renderer.cpp` — Stubbed constructor/destructor with no operational rendering code; class presently unused.
- `src/graphics/Renderer.h` — Declares placeholders for frame lifecycle functions and keeps commented dependency hints; member vectors for command buffers and sync primitives are unused without implementations.
- `src/graphics/SwapChain.cpp` — Swap-chain creation, image view setup, and recreation logic rely on VulkanContext state, but the referenced context members are never guaranteed valid due to how the header binds references.
- `src/graphics/SwapChain.h` — Stores raw pointers to window/context/resource manager and immediately binds reference members (`physicalDevice`, `surface`, `device`, `queueFamilyIndices`) using the `context` pointer before the constructor assigns it, leading to undefined behaviour during object construction.
- `src/graphics/VulkanContext.cpp` — Initializes Vulkan core objects, queries SDL-required extensions, prints device info, and manages queues; the file still contains a duplicate `vk::raii::Instance` construction (one before and one inside the try/catch) and depends on validation layer availability. Transfer queue discovery may leave `transferIndex` at `UINT32_MAX`.
- `src/graphics/VulkanContext.h` — Declares the context interface with RAII handles and queue indices; validation layers list is global, and required device extensions include maintenance v7/v8.

### src/resources
- `src/resources/AssetsLoader.cpp` — Immediately loads the default OBJ model on construction and fills vertex/index buffers; error handling throws on load failure. Declared helper functions (`processVertexData`, `loadMaterials`) remain unused and undefined.
- `src/resources/AssetsLoader.h` — Exposes the loader with getters for vertex/index data and stubs for the unimplemented helpers noted above.
- `src/resources/ResourceManager.cpp` — Owns command pools/buffers, sync objects, GPU memory resources, and swap-chain-dependent images. Current logic assumes a dedicated transfer queue: `createCommandPool` and `copyBuffer` attempt to use `transferIndex`/`transferCommandBuffer[0]` even when the context falls back to the graphics queue, which will dereference an empty vector or invalid index. Sync objects are rebuilt on swap-chain image count updates.
- `src/resources/ResourceManager.h` — Public members expose Vulkan handles, buffers, and mapped uniform pointers; constants and helper signatures align with the cpp file. The header documents default color subresource usage.
- `src/resources/TextureManager.cpp` — Initializes texture resources from a PNG on disk, reuses the resource manager’s staging buffer (overwriting shared state), and generates mipmaps via the resource manager helpers. Assumes linear blitting support and that command buffers are available in slot zero.
- `src/resources/TextureManager.h` — Keeps references back to resource manager buffers/queues and stores Vulkan handles for texture image/sampler/view; lifecycle tied to `init()` implementation.
