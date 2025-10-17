# vcpkg Integration Guide

This project uses **vcpkg** for managing third-party C++ dependencies.

## Overview

vcpkg is automatically bootstrapped and configured when you run CMake. The `vcpkg.json` manifest file declares all required dependencies, and vcpkg installs them automatically during configuration.

## Automatic Setup

When you first configure the project, the following happens automatically:

1. **vcpkg Bootstrap**: If vcpkg doesn't exist, it will be cloned and bootstrapped in `ThirdParty/vcpkg/`
2. **Dependency Installation**: All packages listed in `vcpkg.json` are installed automatically
3. **CMake Integration**: vcpkg's toolchain is chained with your platform-specific Clang toolchain

## Dependencies Managed by vcpkg

The following libraries are automatically installed via vcpkg:

- **glfw3** - Window and input management
- **glm** - OpenGL Mathematics library
- **boost-filesystem** - Boost filesystem operations
- **boost-log** - Boost logging framework
- **boost-system** - Boost system utilities
- **sdl3** - Simple DirectMedia Layer
- **vk-bootstrap** - Vulkan bootstrapping helper

## Manual vcpkg Operations

### Adding a New Dependency

Edit `vcpkg.json` and add the package to the `dependencies` array:

```json
{
  "dependencies": [
    "your-new-package",
    ...
  ]
}
```

Then reconfigure CMake:

```bash
cmake --preset linux-clang-debug  # or your platform preset
```

### Updating vcpkg

To update vcpkg itself:

```bash
cd ThirdParty/vcpkg
git pull
./bootstrap-vcpkg.sh  # Linux/macOS
# or bootstrap-vcpkg.bat on Windows
```

### Updating the Baseline

The `builtin-baseline` in `vcpkg.json` pins the package versions. To update to newer versions:

1. Update vcpkg: `cd ThirdParty/vcpkg && git pull`
2. Get the latest commit: `git rev-parse HEAD`
3. Update the `builtin-baseline` field in `vcpkg.json` with the new commit hash
4. Reconfigure CMake

### Clean vcpkg Installation

To force a clean reinstall of all packages:

```bash
rm -rf build/*/vcpkg_installed/
cmake --preset linux-clang-debug
```

## Project Structure

```
ThirdParty/
├── vcpkg/              # vcpkg installation (gitignored)
└── CMakeLists.txt      # Third-party dependencies

vcpkg.json              # vcpkg manifest (package list)
CMakePresets.json       # CMake presets with vcpkg integration
cmake/
└── toolchains/         # Platform-specific toolchains
```

## CMake Integration Details

### Toolchain Chainloading

The project uses vcpkg's **chainload** feature to combine vcpkg with platform-specific Clang toolchains:

1. `CMakePresets.json` sets `CMAKE_TOOLCHAIN_FILE` to vcpkg's toolchain
2. Each preset sets `VCPKG_CHAINLOAD_TOOLCHAIN_FILE` to the platform-specific toolchain
3. vcpkg loads its packages, then chains to your Clang toolchain

### Environment Variables

- `VCPKG_ROOT`: Set automatically by CMake presets to `${sourceDir}/ThirdParty/vcpkg`
- `CMAKE_TOOLCHAIN_FILE`: Points to vcpkg's CMake integration script

## Troubleshooting

### "No index file found" Error

This error appears when CMake hasn't been configured yet. Simply run:

```bash
cmake --preset linux-clang-debug
```

### Package Not Found

If CMake can't find a package:

1. Check that it's listed in `vcpkg.json`
2. Verify the package name is correct: `./ThirdParty/vcpkg/vcpkg search <package>`
3. Clean and reconfigure: `rm -rf build && cmake --preset <your-preset>`

### vcpkg Lock File Issue

If you see "waiting for lock" messages:

```bash
pkill -9 vcpkg
rm -rf build
cmake --preset linux-clang-debug
```

## Benefits

✅ **Reproducible Builds**: `vcpkg.json` ensures everyone uses the same package versions  
✅ **Cross-Platform**: Works on Linux, macOS, and Windows  
✅ **Automatic**: Dependencies install during CMake configuration  
✅ **Version Pinning**: The `builtin-baseline` locks package versions  
✅ **No System Pollution**: Everything installs to the project directory  

## More Information

- [vcpkg Documentation](https://github.com/microsoft/vcpkg)
- [vcpkg Manifest Mode](https://learn.microsoft.com/en-us/vcpkg/users/manifests)
- [vcpkg Package List](https://vcpkg.io/en/packages.html)
