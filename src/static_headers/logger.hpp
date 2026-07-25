#pragma once

#include <string_view>

// Lightweight logging facade built on spdlog.
void log_info(std::string_view message, std::string_view subsystem = "core");
void log_error(std::string_view message, std::string_view subsystem = "core");
