# engine-test

Cross-platform Vulkan engine prototype configured for Clang/LLVM toolchains on Windows and Linux.

## Prerequisites

- CMake 3.29+
- Ninja build system
- Clang/LLVM toolchain (clang/clang++, llvm-ar, llvm-ranlib, llvm-nm) on your `PATH`
- Vulkan SDK installed and exported through `VULKAN_SDK`
- Optional: vcpkg-installed dependencies in `$HOME/.vcpkg-clion/vcpkg/installed/x64-linux-dynamic` (auto-detected on Linux)

> **Note:** MSVC builds are intentionally unsupported. Configure CMake with a Clang or GCC toolchain.

## Configure & build

Windows debug build:

```powershell
cmake --preset windows-clang-debug
cmake --build --preset windows-clang-debug
```

Windows release build:

```powershell
cmake --preset windows-clang-release
cmake --build --preset windows-clang-release
```

Linux debug build:

```bash
cmake --preset linux-clang-debug
cmake --build --preset linux-clang-debug
```

Linux release build:

```bash
cmake --preset linux-clang-release
cmake --build --preset linux-clang-release
```

To install artifacts use the matching preset, for example:

```powershell
cmake --install --preset windows-clang-release
```

## `clangd` integration

`compile_commands.json` is generated inside each preset's build directory. Keep the file out of version control (see `.gitignore`). Point your editor to the correct directory or create a symlink:

- **Windows (PowerShell, Developer Mode enabled for symlinks):**

  ```powershell
  New-Item -ItemType SymbolicLink -Path compile_commands.json -Target build/windows-clang-debug/compile_commands.json
  ```

- **Linux:**

  ```bash
  ln -sf build/linux-clang-debug/compile_commands.json compile_commands.json
  ```

Alternatively, copy the file when switching build trees:

```powershell
Copy-Item build/windows-clang-debug/compile_commands.json compile_commands.json
```

Update your `clangd` configuration (e.g., VS Code `clangd.arguments`) to include:

```
--compile-commands-dir=${workspaceFolder}/build/windows-clang-debug
```

Switch the path when using a different preset.
