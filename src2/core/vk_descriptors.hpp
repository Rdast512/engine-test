#pragma once
#include <vector>


class DescriptorManager {
public:

    DescriptorManager(const vk::raii::Device& device,
                      const std::vector<vk::raii::Buffer>& uniformBuffers,
                      const vk::raii::Sampler& textureSampler,
                      const vk::raii::ImageView& textureImageView);

    ~DescriptorManager() = default;

    void init();

    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();

    const vk::raii::DescriptorSetLayout& getDescriptorSetLayout() { return descriptorSetLayout; }


    const vk::raii::Device& device;
    const std::vector<vk::raii::Buffer>& uniformBuffers;
    const vk::raii::Sampler& textureSampler;
    const vk::raii::ImageView& textureImageView;


    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet> descriptorSets;

};

