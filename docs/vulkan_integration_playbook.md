# Vulkan Extension Integration Playbook

## Goal

This playbook describes how Vulkan extensions should be integrated in this project at system level. It is intentionally architecture-first: not every path is fully implemented yet, but each path describes how it should be wired.

Use this with [docs/vulkan_extensions_reference.md](./vulkan_extensions_reference.md).

## Terminology Quick Reference

- Capability negotiation: Enumerating extension support and querying feature/property chains before `vkCreateDevice`.
- Capability cache: Storing queried limits/properties in a central struct for runtime use.
- Feature gating: Enabling code paths only when the required feature bits are true.
- Dual-path renderer: Keeping both legacy descriptor-set path and bindless descriptor-heap path.
- Optional vendor path: Extension path that must gracefully fall back on unsupported hardware.

## Integration Flow

### 1. Instance Bring-up

Implementation anchor: [src/core/vk_device.cpp](../src/core/vk_device.cpp)

1. Query SDL-required WSI instance extensions.
2. Verify every required extension is available before creating the instance.
3. Append debug and display/surface maintenance instance extensions used by the project.
4. Create the instance and optional debug messenger.

### 2. Physical Device Selection

Implementation anchor: [src/core/vk_device.cpp](../src/core/vk_device.cpp)

1. Enumerate physical devices.
2. Score candidates (device type, baseline feature requirements).
3. Reject devices that fail mandatory baseline support.
4. Select best candidate and cache queue family capabilities.

### 3. Device Feature and Property Chains

Implementation anchors:

- [src/core/vk_device.cpp](../src/core/vk_device.cpp)
- [src/core/types.hpp](../src/core/types.hpp)

Target behavior:

1. Query all needed property structs via `VkPhysicalDeviceProperties2` chain.
2. Build a complete `VkPhysicalDeviceFeatures2` chain for all enabled extensions.
3. Keep extension list, feature chain, and property chain aligned.
4. Resolve runtime mode flags from queried capabilities (for example descriptor mode selection).

### 4. Descriptor Architecture

Implementation anchors:

- [src/core/vk_descriptors.cpp](../src/core/vk_descriptors.cpp)
- [src/render/vk_pipeline.cpp](../src/render/vk_pipeline.cpp)
- [src/render/vk_renderer.cpp](../src/render/vk_renderer.cpp)

Target behavior:

1. Prefer descriptor heaps when `VK_EXT_descriptor_heap` feature support is present.
2. Fall back to legacy descriptor sets when descriptor heap mode is unavailable.
3. Ensure heap buffers are host-mappable before writing descriptors.
4. Use pipeline creation flags compatible with descriptor heap mode.
5. Keep push-data layout stable and shader mapping explicit when using heap indexing.

### 5. Shader Compilation and SPIR-V Capabilities

Implementation anchor: [CMakeLists.txt](../CMakeLists.txt)

Target behavior:

1. Compile shader targets with capabilities matching enabled extension paths.
2. Keep shader capability flags synchronized with device feature chain.
3. Add explicit extension directives in shaders where required for non-uniform and bindless indexing.
4. Keep descriptor path assumptions consistent between shaders and C++ binding model.

### 6. Pipeline and Draw Submission

Implementation anchors:

- [src/render/vk_pipeline.cpp](../src/render/vk_pipeline.cpp)
- [src/render/vk_renderer.cpp](../src/render/vk_renderer.cpp)

Target behavior:

1. Build graphics pipelines with dynamic rendering support.
2. Bind descriptor resources according to selected descriptor mode.
3. Keep push constants or push data stable across shader stages.
4. Add GPU-driven draw paths incrementally (`VK_EXT_multi_draw`, `VK_EXT_device_generated_commands`).

### 7. Swapchain and Presentation Timing

Implementation anchors:

- [src/core/vk_swapchain.cpp](../src/core/vk_swapchain.cpp)
- [src/render/vk_renderer.cpp](../src/render/vk_renderer.cpp)

Target behavior:

1. Keep base swapchain path with robust out-of-date/suboptimal handling.
2. Add optional present timing path (`VK_EXT_present_timing`) with `VK_KHR_present_id2` and `VK_KHR_calibrated_timestamps`.
3. Add FIFO latest-ready present mode policy where supported.
4. Expose timing telemetry for frame pacing and debugging.

### 8. Memory Strategy

Implementation anchors:

- [src/core/vk_allocator.hpp](../src/core/vk_allocator.hpp)
- [src/core/vk_descriptors.cpp](../src/core/vk_descriptors.cpp)

Target behavior:

1. Integrate memory budget polling and residency decisions.
2. Use memory priorities for critical resources.
3. Add pageable local memory policy hooks.
4. Introduce decompression-aware resource streaming where beneficial.

### 9. Ray Tracing and Advanced Geometry

Primary extension stack:

- `VK_KHR_acceleration_structure`
- `VK_KHR_ray_tracing_pipeline`
- `VK_KHR_ray_query`
- `VK_KHR_ray_tracing_maintenance1`
- `VK_EXT_ray_tracing_invocation_reorder`
- `VK_NV_cluster_acceleration_structure`

Target behavior:

1. Build and manage BLAS/TLAS resources and scratch memory.
2. Add shader binding table management for ray tracing pipelines.
3. Integrate inline ray queries for hybrid passes where cheaper than full RT dispatch.
4. Keep vendor-specific geometry acceleration optional and capability-gated.

## Placeholder Sections (Requested)

The following sections are intentionally short placeholders. They are future integration stubs and not implementation instructions yet.

### NVIDIA RTXGI (placeholder)

Status: TODO

- Intent: Integrate DDGI-style global illumination on top of KHR ray tracing stack.
- Entry points:
  - New module under `src/raytracing/rtxgi`.
  - Volume update pass integrated into renderer frame graph.
  - Probe lighting resources managed by allocator and descriptor path.
- Reference: [RTXGI-DDGI repository](https://github.com/NVIDIAGameWorks/RTXGI-DDGI)

### NVIDIA RTXDI (placeholder)

Status: TODO

- Intent: Add ReSTIR direct/indirect light sampling in ray-traced lighting stages.
- Entry points:
  - New module under `src/raytracing/rtxdi`.
  - Light reservoir buffers and temporal history resources.
  - Optional denoiser stage after RTXDI outputs.
- Reference: [RTXDI repository](https://github.com/NVIDIA-RTX/RTXDI)

### NVIDIA RTX Mega Geometry (placeholder)

Status: TODO

- Intent: Add very-large-geometry ray tracing acceleration integration path.
- Entry points:
  - New module under `src/raytracing/rtx_mega_geometry`.
  - Geometry streaming and acceleration structure update policy.
  - Integration with vendor-specific acceleration extension path.
- Source status: Official stable public technical reference link pending validation in this workspace.

### NVIDIA RTXMU (placeholder)

Status: TODO

- Intent: Reduce acceleration-structure memory footprint with compaction/suballocation utility.
- Entry points:
  - AS memory manager layer under `src/raytracing/rtxmu`.
  - BLAS/TLAS build-compaction lifecycle integration.
  - Residency and garbage collection hooks.
- Reference: [RTXMU repository](https://github.com/NVIDIA-RTX/RTXMU)

### NVIDIA DLSS Upscale (placeholder)

Status: TODO

- Intent: Add optional vendor upscaler path after main render and before final presentation.
- Entry points:
  - New module under `src/postprocessing/upscale_dlss`.
  - Motion vector, depth, exposure, and reactive-mask feed-in path.
  - Swapchain output and UI composition compatibility checks.
- Reference: [NVIDIA DLSS developer page](https://developer.nvidia.com/rtx/dlss)

### AMD FSR Upscale (placeholder)

Status: TODO

- Intent: Add cross-vendor temporal upscaling and optional frame-generation path.
- Entry points:
  - New module under `src/postprocessing/upscale_fsr`.
  - Motion vectors, depth, and mask generation integration.
  - Quality mode and frame pacing configuration in renderer settings.
- Reference: [AMD FidelityFX Super Resolution](https://gpuopen.com/fidelityfx-super-resolution-3/)

## Suggested Rollout Order

1. Stabilize extension-feature-property synchronization.
2. Complete descriptor architecture path parity (heap and legacy).
3. Add present timing telemetry and frame pacing controls.
4. Land baseline KHR ray tracing path.
5. Integrate placeholder technologies one by one behind feature flags.

## Maintenance Rules

1. Any change in extension list or feature chain must update this playbook and [docs/vulkan_extensions_reference.md](./vulkan_extensions_reference.md).
2. Any new optional vendor path must have explicit fallback behavior documented.
3. Any shader capability flag change must be reflected in both CMake shader compile settings and device feature enablement.
