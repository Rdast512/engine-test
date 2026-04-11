#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vulkan/vulkan.hpp>

struct alignas(8) SlangHandle {
    uint32_t resourceIndex;
    uint32_t samplerIndex;
};

// Must perfectly match the PushData struct in Slang.
struct PushData {
    vk::DeviceAddress uboAddress;
    SlangHandle texture;
    SlangHandle samplerHandle;
};

static_assert(sizeof(SlangHandle) == sizeof(uint32_t) * 2,
              "Descriptor handle push layout must be uint2");
static_assert(sizeof(SlangHandle) == 8);
static_assert(alignof(PushData) % 4 == 0, "PushData alignment must be a multiple of 4");
static_assert(sizeof(PushData) % 4 == 0, "PushData size must be a multiple of 4");
static_assert(std::is_trivially_copyable_v<PushData>);
static_assert(offsetof(PushData, uboAddress) == 0);
static_assert(offsetof(PushData, texture) == 8);
static_assert(offsetof(PushData, samplerHandle) == 16);
static_assert(sizeof(PushData) == 24);
