#pragma once

// Standard Library
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <map>
#include <unordered_map>
#include <ranges>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <assert.h>

// Third Party
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE 1
#define VULKAN_HPP_NO_STD_MODULE
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vk_platform.h>

#define VMA_VULKAN_VERSION 1004000
#include <vma/vk_mem_alloc.h>

#include "../ThirdParty/termcolor.hpp"
#include "../ThirdParty/tiny_obj_loader.h"
#include "../ThirdParty/stb_image.h"