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

    // ── Free-fly movement (camera-local axes) ─────────────────
    void moveForward(float delta);  // along forward vector
    void moveRight(float delta);    // along right vector
    void moveUp(float delta);       // along world-up vector

    // ── Mouse look ────────────────────────────────────────────
    // yawDelta, pitchDelta in radians
    void rotate(float yawDelta, float pitchDelta);

    // ── GPU upload ────────────────────────────────────────────
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

private:
    void updateVectors();

    // Orientation (radians)
    float yaw   = -glm::half_pi<float>();   // facing -Z initially
    float pitch = 0.0f;

    // Camera-local basis (recomputed each frame)
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 right  {1.0f, 0.0f,  0.0f};
    glm::vec3 worldUp{0.0f, 1.0f,  0.0f};

    static constexpr float kPitchLimit = glm::radians(89.0f);
    static constexpr float kMouseSensitivity = 0.002f;  // rad/pixel
};
