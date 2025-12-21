#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>

// Note: Heavy includes (Vulkan, SDL, GLM) are now handled by src/pch.h
// and implicitly included in all .cpp files.

static std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    return buffer;
}
