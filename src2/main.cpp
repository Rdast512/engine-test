//
// Created by rdast on 21.08.2025.
//

#include "./core/vk_engine.hpp"
#include <iostream>


void CheckSTL() {
    std::cout << "--------------------------------------------------\n";
    std::cout << "Build Configuration Check:\n";

    // 1. Check Compiler
    #if defined(__clang__)
        std::cout << "  Compiler: Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__ << "\n";
    #elif defined(__GNUC__)
        std::cout << "  Compiler: GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "\n";
    #elif defined(_MSC_VER)
        std::cout << "  Compiler: MSVC " << _MSC_VER << "\n";
    #endif

    // 2. Check Standard Library (STL)
    #if defined(_LIBCPP_VERSION)
        std::cout << "  STL:      LLVM libc++ (Version: " << _LIBCPP_VERSION << ")\n";
    #elif defined(__GLIBCXX__)
        std::cout << "  STL:      GNU libstdc++ (Date: " << __GLIBCXX__ << ")\n";
    #elif defined(_MSVC_STL_VERSION)
        std::cout << "  STL:      Microsoft STL (Version: " << _MSVC_STL_VERSION << ")\n";
    #else
        std::cout << "  STL:      Unknown\n";
    #endif

    std::cout << "--------------------------------------------------\n";
}

int main() {
    CheckSTL();
    try {
        Engine engine;
        engine.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
