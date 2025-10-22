#define VMA_IMPLEMENTATION
#include "../ThirdParty/vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#if defined(__linux__)
#include <stb_image.h>
#else
#include <stb_image.h>
#endif
#define TINYOBJLOADER_IMPLEMENTATION
#include "../ThirdParty/tiny_obj_loader.h"