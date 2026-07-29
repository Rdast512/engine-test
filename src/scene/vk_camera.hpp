#pragma once
#include <array>
#include <core/types.hpp>
#include "core/vk_swapchain.hpp"
#include "vk_camera.hpp"
#include "tracy/Tracy.hpp"
class Camera
{
public:
    Camera(SwapChain& swapChain);
    ~Camera();

    void moveTo(int32_t x, int32_t y, int32_t z);
    void moveToWithTime();
    void addToX(glm::int32_t delta);
    void addToY(glm::int32_t delta);
    void addToZ(glm::int32_t delta);
    void updateCameraData(uint8_t currentImage);
    [[nodiscard]] const std::array<vk::DeviceAddress, 2>& getCameraBufferAddresses() const noexcept { return cameraBufferAddresses; }

    CameraData getCameraData(size_t currentImageIndex) const { return cameraData; }

    VmaAllocator allocator = nullptr;
    SwapChain& swapChain;
    CameraData cameraData = {};
    glm::mat4 prevViewProj = {};
    std::array<void*, 2> cameraBuffersMapped = {nullptr, nullptr};
    std::array<vk::raii::Buffer, 2> cameraBuffers = {nullptr, nullptr};
    std::array<VmaAllocation, 2> cameraBuffersMemory = {nullptr, nullptr};
    std::array<vk::DeviceAddress, 2> cameraBufferAddresses = {0, 0};
};