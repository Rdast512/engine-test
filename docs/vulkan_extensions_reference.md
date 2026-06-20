# Vulkan Extensions Reference

## Scope

This document covers:

- Every non-commented device extension listed in [src/core/vk_device.hpp](../src/core/vk_device.hpp).
- Extension rows that are currently probed or intentionally disabled in the feature/property chains for future migration.
- Instance extension context used by [src/core/vk_device.cpp](../src/core/vk_device.cpp) during instance creation.
- Feature and property structures chained through `VkPhysicalDeviceFeatures2` and `VkPhysicalDeviceProperties2`.
- Promoted-to-core extensions whose features are enabled through core Vulkan 1.1–1.4 structs.

This document does not treat commented-out device extensions as active; they are documented in the Disabled section for completeness.

Every extension row below includes a source link to the official Vulkan registry man page.

## Terminology

- **Extension**: A named Vulkan capability package (e.g. [`VK_KHR_ray_query`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_ray_query.html)) exposed via `vkEnumerateInstanceExtensionProperties` or `vkEnumerateDeviceExtensionProperties`.
- **Extension dependency**: A required extension or Vulkan core version needed before another extension is valid.
- **Feature struct**: A `VkPhysicalDevice*Features*` structure chained into `VkPhysicalDeviceFeatures2` and passed to `vkCreateDevice`.
- **Property struct**: A `VkPhysicalDevice*Properties*` structure chained into `VkPhysicalDeviceProperties2` to query limits and behavior.
- **Promoted-to-core**: Extension functionality moved into a Vulkan core version. Per the Vulkan spec, promoted extensions must NOT be re-enabled as device extensions when the target core version is requested.
- **pNext chain**: Linked list of structures attached through `pNext` to query/enable features and properties.
- **Runtime capability gating**: Deciding code paths at runtime based on queried feature bits and limits.
- **Fallback path**: Alternate implementation path used when an optional feature is unavailable.

Canonical terminology references:

- [VkExtensionProperties](https://docs.vulkan.org/refpages/latest/refpages/source/VkExtensionProperties.html)
- [VkPhysicalDeviceFeatures2](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFeatures2.html)
- [VkPhysicalDeviceProperties2](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceProperties2.html)
- [VK_KHR_get_physical_device_properties2](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_get_physical_device_properties2.html)

## Repository Integration Anchors

- Extension list source: [src/core/vk_device.hpp](../src/core/vk_device.hpp)
- Instance/device setup and feature chain: [src/core/vk_device.cpp](../src/core/vk_device.cpp)
- Capability cache: [src/core/types.hpp](../src/core/types.hpp)
- Descriptor integration: [src/core/vk_descriptors.cpp](../src/core/vk_descriptors.cpp)
- Pipeline integration: [src/render/vk_pipeline.cpp](../src/render/vk_pipeline.cpp)
- Renderer integration: [src/render/vk_renderer.cpp](../src/render/vk_renderer.cpp)

---

## Instance Extensions

| Extension                                                                                                       | Source                                                                                                                                                                                                                                                                                                                                                                                                                   | Purpose                                                   | Status                                                              |
| --------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------------- | ------------------------------------------------------------------- |
| `VK_KHR_surface` + platform surface (`VK_KHR_win32_surface` / `VK_KHR_xlib_surface` / `VK_KHR_wayland_surface`) | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_surface.html) / [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_win32_surface.html) / [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_xlib_surface.html) / [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_wayland_surface.html) | Window-system surface creation                            | Enabled — queried from SDL, appended to instance create info        |
| `VK_EXT_debug_utils`                                                                                            | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_debug_utils.html)                                                                                                                                                                                                                                                                                                                      | Validation/debug names and labels                         | Enabled when validation layers are on; wires debug messenger        |
| `VK_KHR_display`                                                                                                | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_display.html)                                                                                                                                                                                                                                                                                                                          | Display object model for advanced surface/swapchain paths | Enabled with swapchain maintenance and display capability workflows |
| `VK_KHR_surface_maintenance1`                                                                                   | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_surface_maintenance1.html)                                                                                                                                                                                                                                                                                                             | Surface behavior fixes and feature interactions           | Enabled with extended swapchain maintenance stack                   |
| `VK_KHR_get_display_properties2`                                                                                | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_get_display_properties2.html)                                                                                                                                                                                                                                                                                                          | Extended display property queries                         | Enabled when querying advanced display/surface capability metadata  |
| `VK_KHR_get_surface_capabilities2`                                                                              | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_get_surface_capabilities2.html)                                                                                                                                                                                                                                                                                                        | Extended surface capability query API                     | Enabled — companion for present-timing and maintenance queries      |

---

## Device Extensions — Enabled (KHR)

| Extension                               | Source                                                                                                                 | Feature struct + flags                                                                                                                           | Property struct                                    | Usage                                                                              |
| --------------------------------------- | ---------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------ | -------------------------------------------------- | ---------------------------------------------------------------------------------- |
| `VK_KHR_swapchain`                      | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_swapchain.html)                      | No dedicated feature struct                                                                                                                      | Surface capability queries                         | Base present path in `vk_swapchain.cpp`, render-present cycle in `vk_renderer.cpp` |
| `VK_KHR_maintenance7`                   | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance7.html)                   | `PhysicalDeviceMaintenance7FeaturesKHR::maintenance7`                                                                                            | `PhysicalDeviceMaintenance7PropertiesKHR`          | Unlocks maintenance fixes and behaviors required by modern drivers                 |
| `VK_KHR_maintenance8`                   | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance8.html)                   | `PhysicalDeviceMaintenance8FeaturesKHR::maintenance8`                                                                                            | _(core limits)_                                    | API consistency and future-proofing                                                |
| `VK_KHR_maintenance9`                   | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance9.html)                   | `PhysicalDeviceMaintenance9FeaturesKHR::maintenance9`                                                                                            | `PhysicalDeviceMaintenance9PropertiesKHR`          | Modern Vulkan 1.4 behavior envelopes                                               |
| `VK_KHR_maintenance10`                  | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance10.html)                  | `PhysicalDeviceMaintenance10FeaturesKHR::maintenance10`                                                                                          | `PhysicalDeviceMaintenance10PropertiesKHR`         | Vulkan 1.4-class driver behavior                                                   |
| `VK_KHR_deferred_host_operations`       | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_deferred_host_operations.html)       | No dedicated feature struct                                                                                                                      | No dedicated property struct                       | Required dependency for `VK_KHR_acceleration_structure` (host-deferred build/copy) |
| `VK_KHR_acceleration_structure`         | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_acceleration_structure.html)         | `PhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure`                                                                          | `PhysicalDeviceAccelerationStructurePropertiesKHR` | Core ray tracing geometry foundation (BLAS/TLAS)                                   |
| `VK_KHR_ray_tracing_pipeline`           | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_ray_tracing_pipeline.html)           | `PhysicalDeviceRayTracingPipelineFeaturesKHR::rayTracingPipeline`, `::rayTracingPipelineTraceRaysIndirect`                                       | `PhysicalDeviceRayTracingPipelinePropertiesKHR`    | Pipeline/SBT tracing path; ray generation, hit, miss shader stages                 |
| `VK_KHR_fragment_shading_rate`          | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_fragment_shading_rate.html)          | `PhysicalDeviceFragmentShadingRateFeaturesKHR::pipelineFragmentShadingRate`, `::primitiveFragmentShadingRate`, `::attachmentFragmentShadingRate` | `PhysicalDeviceFragmentShadingRatePropertiesKHR`   | Variable-rate shading at render-pass/dynamic-state level                           |
| `VK_KHR_ray_query`                      | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_ray_query.html)                      | `PhysicalDeviceRayQueryFeaturesKHR::rayQuery`                                                                                                    | No dedicated property struct                       | Inline ray queries in raster/compute shaders for hybrid lighting and visibility    |
| `VK_KHR_swapchain_maintenance1`         | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_swapchain_maintenance1.html)         | `PhysicalDeviceSwapchainMaintenance1FeaturesKHR::swapchainMaintenance1`                                                                          | Surface capability queries                         | Extended swapchain behavior and maintenance control                                |
| `VK_KHR_ray_tracing_maintenance1`       | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_ray_tracing_maintenance1.html)       | `PhysicalDeviceRayTracingMaintenance1FeaturesKHR::rayTracingMaintenance1`, `::rayTracingPipelineTraceRaysIndirect2`                              | No dedicated property struct                       | Maintenance path for KHR ray tracing pipeline + indirect trace                     |
| `VK_KHR_present_id2`                    | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_present_id2.html)                    | No dedicated feature struct                                                                                                                      | Present metadata/query behavior                    | Dependency for `VK_EXT_present_timing`; queue present metadata tracking            |
| `VK_KHR_calibrated_timestamps`          | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_calibrated_timestamps.html)          | No dedicated feature struct                                                                                                                      | Time domain query behavior                         | Dependency for `VK_EXT_present_timing`; CPU/GPU/presentation timeline calibration  |
| `VK_KHR_present_mode_fifo_latest_ready` | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_present_mode_fifo_latest_ready.html) | `PhysicalDevicePresentModeFifoLatestReadyFeaturesKHR::presentModeFifoLatestReady`                                                                | No dedicated property struct                       | Optional low-latency FIFO present mode                                             |
| `VK_KHR_copy_memory_indirect`           | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_copy_memory_indirect.html)           | `PhysicalDeviceCopyMemoryIndirectFeaturesKHR::indirectMemoryCopy`, `::indirectMemoryToImageCopy`                                                 | No dedicated property struct                       | GPU-driven copy scheduling for streaming/resource upload                           |
| `VK_KHR_shader_untyped_pointers`        | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_shader_untyped_pointers.html)        | `PhysicalDeviceShaderUntypedPointersFeaturesKHR::shaderUntypedPointers`                                                                          | No dedicated property struct                       | Required for shader-side heap mapping and BDA-oriented SPIR-V paths                |

---

## Device Extensions — Enabled (EXT)

| Extension                               | Source                                                                                                                 | Feature struct + flags                                                                                               | Property struct                                                         | Usage                                                                                                                                                   |
| --------------------------------------- | ---------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `VK_EXT_opacity_micromap`               | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_opacity_micromap.html)               | `PhysicalDeviceOpacityMicromapFeaturesEXT::micromap`                                                                 | `PhysicalDeviceOpacityMicromapPropertiesEXT`                            | Opacity micromaps for ray tracing (alpha-tested geometry acceleration)                                                                                  |
| `VK_EXT_memory_budget`                  | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_memory_budget.html)                  | No dedicated feature struct                                                                                          | `VkPhysicalDeviceMemoryBudgetPropertiesEXT` via memory properties chain | Allocator budget telemetry and eviction policy                                                                                                          |
| `VK_EXT_memory_priority`                | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_memory_priority.html)                | `PhysicalDeviceMemoryPriorityFeaturesEXT::memoryPriority`                                                            | No dedicated property struct                                            | Assign memory priority classes for critical resources                                                                                                   |
| `VK_EXT_memory_decompression`           | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_memory_decompression.html)           | `PhysicalDeviceMemoryDecompressionFeaturesEXT::memoryDecompression`                                                  | `PhysicalDeviceMemoryDecompressionPropertiesEXT`                        | Decompression-aware streaming/upload workflows                                                                                                          |
| `VK_EXT_descriptor_heap`                | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_descriptor_heap.html)                | `PhysicalDeviceDescriptorHeapFeaturesEXT::descriptorHeap` _(runtime-gated)_                                          | `PhysicalDeviceDescriptorHeapPropertiesEXT`                             | Current bindless path in `vk_descriptors.cpp`, `vk_pipeline.cpp`, `vk_renderer.cpp`; keep heap mappings explicit and legacy descriptor sets as fallback |
| `VK_EXT_blend_operation_advanced`       | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_blend_operation_advanced.html)       | `PhysicalDeviceBlendOperationAdvancedFeaturesEXT::advancedBlendCoherentOperations` = `false` _(explicitly disabled)_ | `PhysicalDeviceBlendOperationAdvancedPropertiesEXT`                     | Disabled unless advanced blend operations are needed                                                                                                    |
| `VK_EXT_mesh_shader`                    | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_mesh_shader.html)                    | `PhysicalDeviceMeshShaderFeaturesEXT::taskShader`, `::meshShader`                                                    | `PhysicalDeviceMeshShaderPropertiesEXT`                                 | Mesh/task shader pipelines and GPU-driven geometry path                                                                                                 |
| `VK_EXT_device_generated_commands`      | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_device_generated_commands.html)      | `PhysicalDeviceDeviceGeneratedCommandsFeaturesEXT::deviceGeneratedCommands`                                          | `PhysicalDeviceDeviceGeneratedCommandsPropertiesEXT`                    | GPU-generated draw/dispatch command streams                                                                                                             |
| `VK_EXT_multi_draw`                     | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_multi_draw.html)                     | `PhysicalDeviceMultiDrawFeaturesEXT::multiDraw`                                                                      | `PhysicalDeviceMultiDrawPropertiesEXT`                                  | Batch draw workloads; reduce CPU submission overhead                                                                                                    |
| `VK_EXT_pageable_device_local_memory`   | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_pageable_device_local_memory.html)   | `PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT::pageableDeviceLocalMemory`                                      | No dedicated property struct                                            | Memory budget/priority and residency management                                                                                                         |
| `VK_EXT_shader_object`                  | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_shader_object.html)                  | `PhysicalDeviceShaderObjectFeaturesEXT::shaderObject`                                                                | No dedicated property struct                                            | Optional future shader-object pipeline path; renderer still uses full pipeline objects                                                                  |
| `VK_EXT_present_timing`                 | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_present_timing.html)                 | `PhysicalDevicePresentTimingFeaturesEXT` _(in chain, all fields `false` — not enabled)_                              | `VkPresentTimingSurfaceCapabilitiesEXT` and related queries             | Frame pacing feedback; feature NOT enabled yet (extension + dependencies only)                                                                          |
| `VK_EXT_ray_tracing_invocation_reorder` | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_ray_tracing_invocation_reorder.html) | `PhysicalDeviceRayTracingInvocationReorderFeaturesEXT::rayTracingInvocationReorder`                                  | No dedicated property struct                                            | Improves ray traversal coherence in heavy RT scenes                                                                                                     |

---

## Device Extension — Enabled (NV)

| Extension                              | Source                                                                                                                | Feature struct + flags      | Property struct                                 | Usage                                                                                      |
| -------------------------------------- | --------------------------------------------------------------------------------------------------------------------- | --------------------------- | ----------------------------------------------- | ------------------------------------------------------------------------------------------ |
| `VK_NV_cluster_acceleration_structure` | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_NV_cluster_acceleration_structure.html) | No dedicated feature struct | No dedicated property struct _(extension only)_ | Vendor-specific acceleration structure path; optional behind runtime vendor/feature gating |

---

## Promoted-to-Core Extensions (Enabled via Core Feature Structs)

These extensions are promoted into Vulkan 1.1–1.4 and must NOT appear in the device extension list.
Their functionality is enabled through the core `VkPhysicalDeviceVulkan1xFeatures` structs.

The source column below points to the original extension man page.

### Vulkan 1.1 Core (always available at 1.4)

| Original extension                | Source                                                                                                           | Core coverage                                             |
| --------------------------------- | ---------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------- |
| `VK_KHR_maintenance1`             | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance1.html)             | Implicit; always available                                |
| `VK_KHR_maintenance2`             | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance2.html)             | Implicit; always available                                |
| `VK_KHR_maintenance3`             | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance3.html)             | Implicit + `PhysicalDeviceMaintenance3Properties` queried |
| `VK_KHR_bind_memory2`             | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_bind_memory2.html)             | Implicit; always available                                |
| `VK_KHR_get_memory_requirements2` | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_get_memory_requirements2.html) | Implicit; always available                                |

### Vulkan 1.2 Core

| Original extension             | Source                                                                                                        | Core feature flag(s)                                                                                                                                                                                                                                 | Property struct                                 |
| ------------------------------ | ------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------- |
| `VK_KHR_buffer_device_address` | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_buffer_device_address.html) | `Vulkan12Features::bufferDeviceAddress`                                                                                                                                                                                                              | _(core)_                                        |
| `VK_KHR_depth_stencil_resolve` | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_depth_stencil_resolve.html) | Implicit; always available in 1.2+                                                                                                                                                                                                                   | `PhysicalDeviceDepthStencilResolveProperties`   |
| `VK_KHR_spirv_1_4`             | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_spirv_1_4.html)             | Implicit; always available in 1.2+                                                                                                                                                                                                                   | _(N/A — SPIR-V capability)_                     |
| `VK_KHR_vulkan_memory_model`   | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_vulkan_memory_model.html)   | `Vulkan12Features::vulkanMemoryModel`, `::vulkanMemoryModelDeviceScope`                                                                                                                                                                              | _(core)_                                        |
| `VK_EXT_descriptor_indexing`   | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_descriptor_indexing.html)   | `Vulkan12Features::descriptorIndexing`, `::shaderSampledImageArrayNonUniformIndexing`, `::shaderStorageBufferArrayNonUniformIndexing`, `::descriptorBindingPartiallyBound`, `::descriptorBindingVariableDescriptorCount`, `::runtimeDescriptorArray` | `PhysicalDeviceDescriptorIndexingPropertiesEXT` |

### Vulkan 1.3 Core

| Original extension                    | Source                                                                                                               | Core feature flag(s)                           | Property struct                                     |
| ------------------------------------- | -------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------- | --------------------------------------------------- |
| `VK_KHR_copy_commands2`               | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_copy_commands2.html)               | Implicit; always available in 1.3+             | _(core)_                                            |
| `VK_EXT_extended_dynamic_state`       | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_extended_dynamic_state.html)       | Implicit; always available in 1.3+             | _(core)_                                            |
| `VK_EXT_extended_dynamic_state2`      | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_extended_dynamic_state2.html)      | Implicit; always available in 1.3+             | _(core)_                                            |
| `VK_KHR_load_store_op_none`           | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_load_store_op_none.html)           | Implicit; always available in 1.3+             | _(core)_                                            |
| `VK_KHR_map_memory2`                  | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_map_memory2.html)                  | Implicit; always available in 1.3+             | _(core)_                                            |
| `VK_EXT_tooling_info`                 | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_tooling_info.html)                 | Implicit; always available in 1.3+             | _(queried via `vkGetPhysicalDeviceToolProperties`)_ |
| `VK_KHR_maintenance4`                 | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance4.html)                 | `Vulkan13Features::maintenance4`               | `PhysicalDeviceMaintenance4Properties`              |
| `VK_EXT_texture_compression_astc_hdr` | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_texture_compression_astc_hdr.html) | `Vulkan13Features::textureCompressionASTC_HDR` | _(core — format support query)_                     |

### Vulkan 1.4 Core

| Original extension       | Source                                                                                                  | Core feature flag(s)                    | Property struct                            |
| ------------------------ | ------------------------------------------------------------------------------------------------------- | --------------------------------------- | ------------------------------------------ |
| `VK_KHR_maintenance5`    | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance5.html)    | `Vulkan14Features::maintenance5`        | `PhysicalDeviceMaintenance5Properties`     |
| `VK_KHR_maintenance6`    | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance6.html)    | `Vulkan14Features::maintenance6`        | `PhysicalDeviceMaintenance6Properties`     |
| `VK_KHR_global_priority` | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_global_priority.html) | `Vulkan14Features::globalPriorityQuery` | _(core)_                                   |
| `VK_EXT_host_image_copy` | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_host_image_copy.html) | `Vulkan14Features::hostImageCopy`       | `PhysicalDeviceHostImageCopyPropertiesEXT` |

---

## Core Feature Flags (PhysicalDeviceFeatures2 + Vulkan1xFeatures)

These are enabled in addition to extension-specific feature structs.

| Struct                    | Flags                                                                                                                                                                                                                                                                                                                     |
| ------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `PhysicalDeviceFeatures2` | `geometryShader`, `sampleRateShading`, `multiDrawIndirect`, `samplerAnisotropy`, `shaderInt64`                                                                                                                                                                                                                            |
| `Vulkan11Features`        | `shaderDrawParameters`                                                                                                                                                                                                                                                                                                    |
| `Vulkan12Features`        | `drawIndirectCount`, `descriptorIndexing`, `shaderSampledImageArrayNonUniformIndexing`, `shaderStorageBufferArrayNonUniformIndexing`, `descriptorBindingPartiallyBound`, `descriptorBindingVariableDescriptorCount`, `runtimeDescriptorArray`, `bufferDeviceAddress`, `vulkanMemoryModel`, `vulkanMemoryModelDeviceScope` |
| `Vulkan13Features`        | `synchronization2`, `textureCompressionASTC_HDR`, `dynamicRendering`, `maintenance4`                                                                                                                                                                                                                                      |
| `Vulkan14Features`        | `globalPriorityQuery`, `dynamicRenderingLocalRead`, `maintenance5`, `maintenance6`, `hostImageCopy`                                                                                                                                                                                                                       |

---

## Commented-Out / Disabled Extensions

These appear commented-out in the extension list or are probe-only rows in the capability cache; they are intentionally not active on the baseline path.

| Extension                          | Source                                                                                                            | Status                                                                     |
| ---------------------------------- | ----------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------- |
| `VK_KHR_pipeline_binary`           | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_pipeline_binary.html)           | Commented out; feature struct in chain (`{}` init)                         |
| `VK_KHR_pipeline_library`          | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_pipeline_library.html)          | Commented out (dependency for `VK_EXT_graphics_pipeline_library`)          |
| `VK_EXT_graphics_pipeline_library` | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_graphics_pipeline_library.html) | Commented out; feature struct in chain (`{}` init)                         |
| `VK_EXT_extended_dynamic_state`    | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_extended_dynamic_state.html)    | Promoted to 1.3; feature struct in chain (`{}` init, no-op)                |
| `VK_EXT_texel_buffer_alignment`    | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_texel_buffer_alignment.html)    | Feature struct in chain (`{}` init — not enabled); properties queried      |
| `VK_EXT_descriptor_buffer`         | [`spec`](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_descriptor_buffer.html)         | Probe-only; properties cached in `types.hpp`, feature path not enabled yet |

---

## Recommended Integration Baseline

- Keep a single authoritative extension list in [src/core/vk_device.hpp](../src/core/vk_device.hpp).
- Keep feature-chain enablement and property-chain queries synchronized in [src/core/vk_device.cpp](../src/core/vk_device.cpp).
- If an extension is enabled only for dependency reasons, document that explicitly.
- If an extension is intended for future use, keep it behind runtime capability checks and clear fallback paths.
- Promoted extensions must NOT be added to the device extension list; enable their features through core structs.

## Maintenance Checklist

When editing the extension list or feature chain:

1. Update [src/core/vk_device.hpp](../src/core/vk_device.hpp).
2. Update feature and property chains in [src/core/vk_device.cpp](../src/core/vk_device.cpp).
3. Update this document and [docs/vulkan_integration_playbook.md](./vulkan_integration_playbook.md).
4. Verify commented-out and future/probed extensions are not described as active.
5. Verify promoted extensions are documented under the Promoted-to-Core section, not the device extension tables.
