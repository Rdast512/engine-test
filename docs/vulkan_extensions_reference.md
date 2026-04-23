# Vulkan Extensions Reference

## Scope

This document covers:

- Every non-commented device extension listed in [src/core/vk_device.hpp](../src/core/vk_device.hpp).
- Instance extension context used by [src/core/vk_device.cpp](../src/core/vk_device.cpp) during instance creation.
- Feature and property structures that should be chained through `VkPhysicalDeviceFeatures2` and `VkPhysicalDeviceProperties2`.

This document does not cover commented-out extensions in [src/core/vk_device.hpp](../src/core/vk_device.hpp).

## Terminology

- Extension: A named Vulkan capability package (for example `VK_KHR_ray_query`) exposed via `vkEnumerateInstanceExtensionProperties` or `vkEnumerateDeviceExtensionProperties`.
- Extension dependency: A required extension or Vulkan core version needed before another extension is valid.
- Feature struct: A `VkPhysicalDevice*Features*` structure chained into `VkPhysicalDeviceFeatures2` and then passed to `vkCreateDevice`.
- Property struct: A `VkPhysicalDevice*Properties*` structure chained into `VkPhysicalDeviceProperties2` to query limits and behavior.
- Promoted-to-core: Extension functionality moved into a Vulkan core version. You can still use extension names for compatibility.
- pNext chain: Linked list of structures attached through `pNext` to query/enable features and properties.
- Runtime capability gating: Deciding code paths at runtime based on queried feature bits and limits.
- Fallback path: Alternate implementation path used when an optional feature is unavailable.

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

## Instance Extension Context

| Extension | Purpose | How it should be integrated | Official docs |
|---|---|---|---|
| SDL WSI extension set (`VK_KHR_surface` + platform surface extension such as `VK_KHR_win32_surface`) | Window-system surface creation | Query from SDL, verify availability, append to instance create info before `vkCreateInstance` | [VK_KHR_surface](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_surface.html) |
| `VK_EXT_debug_utils` | Validation/debug names and labels | Enable at instance creation; wire debug messenger when validation layers are on; use object naming helpers globally | [VK_EXT_debug_utils](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_debug_utils.html) |
| `VK_KHR_display` | Display object model for advanced surface/swapchain paths | Enable when using swapchain maintenance and display capability workflows | [VK_KHR_display](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_display.html) |
| `VK_KHR_surface_maintenance1` | Surface behavior fixes and feature interactions | Enable with extended swapchain maintenance stack | [VK_KHR_surface_maintenance1](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_surface_maintenance1.html) |
| `VK_KHR_get_display_properties2` | Extended display property queries | Enable when querying advanced display/surface capability metadata | [VK_KHR_get_display_properties2](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_get_display_properties2.html) |
| `VK_KHR_get_surface_capabilities2` | Extended surface capability query API | Required companion for modern present-timing and maintenance queries | [VK_KHR_get_surface_capabilities2](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_get_surface_capabilities2.html) |

## Device Extensions: KHR

| Extension | Matching feature enablement | Matching property query | Usage and integration in this project | Official docs |
|---|---|---|---|---|
| `VK_KHR_swapchain` | No dedicated feature struct | Swapchain capabilities via surface queries | Base present path for [src/core/vk_swapchain.cpp](../src/core/vk_swapchain.cpp), render-present cycle in [src/render/vk_renderer.cpp](../src/render/vk_renderer.cpp) | [VK_KHR_swapchain](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_swapchain.html) |
| `VK_KHR_maintenance7` | `VkPhysicalDeviceMaintenance7FeaturesKHR::maintenance7` | `VkPhysicalDeviceMaintenance7PropertiesKHR` | Enable in feature chain to unlock maintenance fixes and behaviors used by modern drivers | [VK_KHR_maintenance7](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_maintenance7.html) |
| `VK_KHR_maintenance8` | `VkPhysicalDeviceMaintenance8FeaturesKHR::maintenance8` | Core limits/properties as applicable | Keep enabled for API consistency and future-proofing | [VK_KHR_maintenance8](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_maintenance8.html) |
| `VK_KHR_maintenance9` | `VkPhysicalDeviceMaintenance9FeaturesKHR::maintenance9` | `VkPhysicalDeviceMaintenance9PropertiesKHR` | Keep enabled in core feature chain for modern Vulkan 1.4 behavior envelopes | [VK_KHR_maintenance9](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_maintenance9.html) |
| `VK_KHR_maintenance10` | `VkPhysicalDeviceMaintenance10FeaturesKHR::maintenance10` | `VkPhysicalDeviceMaintenance10PropertiesKHR` | Keep enabled where Vulkan 1.4 class driver behavior is expected | [VK_KHR_maintenance10](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_maintenance10.html) |
| `VK_KHR_deferred_host_operations` | No dedicated feature struct | No dedicated property struct | Required dependency for acceleration structure workflows, mainly host-deferred build/copy operations | [VK_KHR_deferred_host_operations](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_deferred_host_operations.html) |
| `VK_KHR_acceleration_structure` | `VkPhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure` | `VkPhysicalDeviceAccelerationStructurePropertiesKHR` | Core ray tracing geometry acceleration foundation; should drive BLAS/TLAS build systems in a dedicated ray tracing module | [VK_KHR_acceleration_structure](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_acceleration_structure.html) |
| `VK_KHR_ray_tracing_pipeline` | `VkPhysicalDeviceRayTracingPipelineFeaturesKHR::rayTracingPipeline`, `rayTracingPipelineTraceRaysIndirect` | `VkPhysicalDeviceRayTracingPipelinePropertiesKHR` | Pipeline/SBT tracing path; should integrate with ray generation, hit, miss shader stages and SBT management | [VK_KHR_ray_tracing_pipeline](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_ray_tracing_pipeline.html) |
| `VK_KHR_fragment_shading_rate` | `VkPhysicalDeviceFragmentShadingRateFeaturesKHR::{pipelineFragmentShadingRate,primitiveFragmentShadingRate,attachmentFragmentShadingRate}` | `VkPhysicalDeviceFragmentShadingRatePropertiesKHR` | Variable-rate shading path; should integrate at render pass/dynamic-state and quality modes layer | [VK_KHR_fragment_shading_rate](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_fragment_shading_rate.html) |
| `VK_KHR_ray_query` | `VkPhysicalDeviceRayQueryFeaturesKHR::rayQuery` | No dedicated property struct | Inline ray query support for raster/compute shaders, useful for hybrid lighting and visibility | [VK_KHR_ray_query](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_ray_query.html) |
| `VK_KHR_swapchain_maintenance1` | `VkPhysicalDeviceSwapchainMaintenance1FeaturesKHR::swapchainMaintenance1` | Surface capability structs through KHR surface capability queries | Extend swapchain behavior and maintenance control paths | [VK_KHR_swapchain_maintenance1](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_swapchain_maintenance1.html) |
| `VK_KHR_ray_tracing_maintenance1` | `VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR::{rayTracingMaintenance1,rayTracingPipelineTraceRaysIndirect2}` | No dedicated property struct | Maintenance path for KHR ray tracing pipeline and indirect trace operations | [VK_KHR_ray_tracing_maintenance1](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_ray_tracing_maintenance1.html) |
| `VK_KHR_present_id2` | No dedicated feature struct | Present metadata/query behavior | Dependency for present-timing workflows; should be used with queue present metadata tracking | [VK_KHR_present_id2](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_present_id2.html) |
| `VK_KHR_calibrated_timestamps` | No dedicated feature struct | Time domain query behavior | Dependency for present timing and timeline calibration between CPU/GPU/presentation domains | [VK_KHR_calibrated_timestamps](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_calibrated_timestamps.html) |
| `VK_KHR_present_mode_fifo_latest_ready` | `VkPhysicalDevicePresentModeFifoLatestReadyFeaturesKHR::presentModeFifoLatestReady` | No dedicated property struct | Optional low-latency FIFO present mode strategy for smoother frame delivery | [VK_KHR_present_mode_fifo_latest_ready](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_present_mode_fifo_latest_ready.html) |
| `VK_KHR_copy_memory_indirect` | `VkPhysicalDeviceCopyMemoryIndirectFeaturesKHR::{indirectMemoryCopy,indirectMemoryToImageCopy}` | No dedicated property struct | GPU-driven copy scheduling for streaming/resource upload systems | [VK_KHR_copy_memory_indirect](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_copy_memory_indirect.html) |
| `VK_KHR_shader_untyped_pointers` | `VkPhysicalDeviceShaderUntypedPointersFeaturesKHR::shaderUntypedPointers` | No dedicated property struct | Required for current shader capability set and descriptor-heap/BDA-oriented SPIR-V path | [VK_KHR_shader_untyped_pointers](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_shader_untyped_pointers.html) |
| `VK_KHR_vulkan_memory_model` | Core-promoted equivalent fields in `VkPhysicalDeviceVulkan12Features::{vulkanMemoryModel,vulkanMemoryModelDeviceScope}` | No dedicated property struct | Memory-model consistency for advanced shader/buffer synchronization semantics | [VK_KHR_vulkan_memory_model](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_vulkan_memory_model.html) |

## Device Extensions: EXT

| Extension | Matching feature enablement | Matching property query | Usage and integration in this project | Official docs |
|---|---|---|---|---|
| `VK_EXT_memory_budget` | No dedicated feature struct | `VkPhysicalDeviceMemoryBudgetPropertiesEXT` through memory properties query chain | Should feed allocator budget telemetry and eviction policy decisions | [VK_EXT_memory_budget](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_memory_budget.html) |
| `VK_EXT_memory_priority` | `VkPhysicalDeviceMemoryPriorityFeaturesEXT::memoryPriority` | No dedicated property struct | Should let allocator assign memory priority classes for important resources | [VK_EXT_memory_priority](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_memory_priority.html) |
| `VK_EXT_memory_decompression` | `VkPhysicalDeviceMemoryDecompressionFeaturesEXT::memoryDecompression` | `VkPhysicalDeviceMemoryDecompressionPropertiesEXT` | Should be used for decompression-aware streaming/upload workflows | [VK_EXT_memory_decompression](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_memory_decompression.html) |
| `VK_EXT_descriptor_heap` | `VkPhysicalDeviceDescriptorHeapFeaturesEXT::descriptorHeap` (runtime-gated) | `VkPhysicalDeviceDescriptorHeapPropertiesEXT` | Core bindless path in [src/core/vk_descriptors.cpp](../src/core/vk_descriptors.cpp), [src/render/vk_pipeline.cpp](../src/render/vk_pipeline.cpp), [src/render/vk_renderer.cpp](../src/render/vk_renderer.cpp) | [VK_EXT_descriptor_heap](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_descriptor_heap.html) |
| `VK_EXT_blend_operation_advanced` | `VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT::advancedBlendCoherentOperations` | `VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT` | Keep disabled unless advanced blend operations are needed; enable coherency only with explicit blending strategy | [VK_EXT_blend_operation_advanced](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_blend_operation_advanced.html) |
| `VK_EXT_mesh_shader` | `VkPhysicalDeviceMeshShaderFeaturesEXT::{taskShader,meshShader}` | `VkPhysicalDeviceMeshShaderPropertiesEXT` | Should power mesh/task shader pipelines and optional GPU-driven geometry path | [VK_EXT_mesh_shader](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_mesh_shader.html) |
| `VK_EXT_device_generated_commands` | `VkPhysicalDeviceDeviceGeneratedCommandsFeaturesEXT::deviceGeneratedCommands` | `VkPhysicalDeviceDeviceGeneratedCommandsPropertiesEXT` | Should enable GPU-generated draw/dispatch command streams in renderer backend | [VK_EXT_device_generated_commands](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_device_generated_commands.html) |
| `VK_EXT_multi_draw` | `VkPhysicalDeviceMultiDrawFeaturesEXT::multiDraw` | `VkPhysicalDeviceMultiDrawPropertiesEXT` | Should batch draw workloads and reduce CPU submission overhead | [VK_EXT_multi_draw](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_multi_draw.html) |
| `VK_EXT_pageable_device_local_memory` | `VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT::pageableDeviceLocalMemory` | No dedicated property struct | Should integrate with memory budget/priority and residency management policy | [VK_EXT_pageable_device_local_memory](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_pageable_device_local_memory.html) |
| `VK_EXT_shader_object` | `VkPhysicalDeviceShaderObjectFeaturesEXT::shaderObject` | No dedicated property struct | Optional future path for shader-object based pipelines; current renderer still uses full graphics pipeline objects | [VK_EXT_shader_object](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_shader_object.html) |
| `VK_EXT_present_timing` | Should enable `VkPhysicalDevicePresentTimingFeaturesEXT::presentTiming` when timing feedback/scheduling is used | `VkPresentTimingSurfaceCapabilitiesEXT` and related query structs | Use for frame pacing feedback, target present times, and presentation telemetry (with KHR dependencies) | [VK_EXT_present_timing](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_present_timing.html) |
| `VK_EXT_ray_tracing_invocation_reorder` | `VkPhysicalDeviceRayTracingInvocationReorderFeaturesEXT::rayTracingInvocationReorder` | No dedicated property struct | Ray tracing optimization feature to improve ray traversal coherence in heavy RT scenes | [VK_EXT_ray_tracing_invocation_reorder](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_ray_tracing_invocation_reorder.html) |
| `VK_EXT_descriptor_buffer` | Should include `VkPhysicalDeviceDescriptorBufferFeaturesEXT` fields as needed (`descriptorBuffer`, optional capture replay bits) | `VkPhysicalDeviceDescriptorBufferPropertiesEXT` | Properties are already cached in [src/core/types.hpp](../src/core/types.hpp); full feature-enable path should be added when descriptor-buffer mode is used directly | [VK_EXT_descriptor_buffer](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_descriptor_buffer.html) |

## Device Extension: NV

| Extension | Matching feature enablement | Matching property query | Usage and integration in this project | Official docs |
|---|---|---|---|---|
| `VK_NV_cluster_acceleration_structure` | Should chain `VkPhysicalDeviceClusterAccelerationStructureFeaturesNV` when cluster AS path is enabled | `VkPhysicalDeviceClusterAccelerationStructurePropertiesNV` | Vendor-specific acceleration structure path for cluster workflows; keep optional behind runtime vendor/feature gating | [VK_NV_cluster_acceleration_structure](https://docs.vulkan.org/refpages/latest/refpages/source/VK_NV_cluster_acceleration_structure.html) |

## Recommended Integration Baseline

- Keep a single authoritative extension list in [src/core/vk_device.hpp](../src/core/vk_device.hpp).
- Keep feature-chain enablement and property-chain queries synchronized in [src/core/vk_device.cpp](../src/core/vk_device.cpp).
- If an extension is enabled only for dependency reasons, document that explicitly.
- If an extension is intended for future use, keep it behind runtime capability checks and clear fallback paths.

## Maintenance Checklist

When editing the extension list or feature chain:

1. Update [src/core/vk_device.hpp](../src/core/vk_device.hpp).
2. Update feature and property chains in [src/core/vk_device.cpp](../src/core/vk_device.cpp).
3. Update this document and [docs/vulkan_integration_playbook.md](./vulkan_integration_playbook.md).
4. Verify commented-out extensions are not described as active.
