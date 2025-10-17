#!/bin/bash
# Launcher script for the Vulkan Engine
# This script sets environment variables to work around SDL3/Wayland issues

# Force X11 backend if Wayland is causing issues
export SDL_VIDEODRIVER=x11

# Run the engine
exec "$(dirname "$0")/build/linux-clang-debug/engine" "$@"
