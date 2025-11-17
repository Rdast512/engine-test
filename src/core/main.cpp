#include <iostream>

int main() {
#ifdef _LIBCPP_VERSION
    std::cout << "Using libc++ (LLVM/Clang standard library)\n";
    std::cout << "Version: " << _LIBCPP_VERSION << "\n";
#elif defined(__GLIBCXX__)
    std::cout << "Using libstdc++ (GNU standard library)\n";
    std::cout << "Version: " << __GLIBCXX__ << "\n";
#elif defined(_MSVC_STL_VERSION)
    std::cout << "Using MSVC STL\n";
    std::cout << "Version: " << _MSVC_STL_VERSION << "\n";
#else
    std::cout << "Unknown standard library\n";
#endif
    return 0;
}