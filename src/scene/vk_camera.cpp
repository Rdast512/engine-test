#include "vk_camera.hpp"

#include <glm/gtc/constants.hpp>

Camera::Camera(SwapChain& swapChain) : swapChain(swapChain)
{
    cameraData.cameraPos = glm::vec3(2.0f, 2.0f, 6.0f);

    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                     static_cast<float>(swapChain.swapChainExtent.width) /
                                         static_cast<float>(swapChain.swapChainExtent.height),
                                     0.1f, 20.0f);
    proj[1][1] *= -1;
    cameraData.proj = proj;
}

Camera::~Camera()
{
    for (size_t i = 0; i < cameraBuffersMemory.size(); ++i)
    {
        if (cameraBuffersMemory[i] == nullptr) continue;

        if (cameraBuffersMapped[i] != nullptr)
        {
            vmaUnmapMemory(allocator, cameraBuffersMemory[i]);
            cameraBuffersMapped[i] = nullptr;
        }

        VkBuffer rawBuf = VK_NULL_HANDLE;
        if (*cameraBuffers[i] != VK_NULL_HANDLE)
        {
            rawBuf = cameraBuffers[i].release();
        }

        vmaDestroyBuffer(allocator, rawBuf, cameraBuffersMemory[i]);
        cameraBuffersMemory[i] = nullptr;
    }
}

// ── Free-fly movement ────────────────────────────────────────────

void Camera::moveForward(float delta) { cameraData.cameraPos += forward * delta; }
void Camera::moveRight(float delta)   { cameraData.cameraPos += right * delta; }
void Camera::moveUp(float delta)      { cameraData.cameraPos += worldUp * delta; }

void Camera::rotate(float yawDelta, float pitchDelta)
{
    yaw   += yawDelta * kMouseSensitivity;
    pitch += pitchDelta * kMouseSensitivity;

    // Clamp pitch to avoid gimbal lock / view flipping
    if (pitch > kPitchLimit)  pitch = kPitchLimit;
    if (pitch < -kPitchLimit) pitch = -kPitchLimit;
}

// ── Recompute basis vectors from yaw/pitch ───────────────────────

void Camera::updateVectors()
{
    // Standard FPS camera basis:
    //   forward = direction the camera looks
    //   right   = perpendicular to forward and world-up
    forward.x = cosf(pitch) * sinf(yaw);
    forward.y = -sinf(pitch);
    forward.z = cosf(pitch) * cosf(yaw);
    forward   = glm::normalize(forward);

    right = glm::normalize(glm::cross(forward, worldUp));
}

// ── GPU upload ───────────────────────────────────────────────────

void Camera::updateCameraData(uint8_t currentImage)
{
    ZoneScopedN("Camera::updateCameraData");

    updateVectors();

    cameraData.view     = glm::lookAt(cameraData.cameraPos, cameraData.cameraPos + forward, worldUp);
    cameraData.viewProj = cameraData.proj * cameraData.view;

    memcpy(this->cameraBuffersMapped[currentImage], &this->cameraData, sizeof(this->cameraData));
}
