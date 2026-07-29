#include "vk_camera.hpp"


Camera::Camera(SwapChain& swapChain) : swapChain(swapChain)
{
    cameraData.cameraPos = glm::vec3(2.0f, 2.0f, 6.0f);

    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                     static_cast<float>(swapChain.swapChainExtent.width) / static_cast<float>(swapChain.swapChainExtent.height),
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

void Camera::addToX(glm::int32_t delta)
{
    cameraData.cameraPos.x += delta;
}
void Camera::addToY(glm::int32_t delta)
{
    cameraData.cameraPos.y += delta;
}
void Camera::addToZ(glm::int32_t delta)
{
    cameraData.cameraPos.z += delta;
}

void Camera::moveTo(int32_t x, int32_t y, int32_t z)
{
    cameraData.cameraPos = glm::vec3(x, y, z);
}

void Camera::updateCameraData(uint8_t currentImage)
{
    ZoneScopedN("Camera::updateCameraData");

    const glm::vec3 target(0.0f, 0.0f, 0.0f);
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    cameraData.view     = glm::lookAt(cameraData.cameraPos, target, up);
    cameraData.viewProj = cameraData.proj * cameraData.view;

    memcpy(this->cameraBuffersMapped[currentImage], &this->cameraData, sizeof(this->cameraData));
}
