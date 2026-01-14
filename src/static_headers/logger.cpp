#include "logger.hpp"

#include <memory>
#include <mutex>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace {
// Lazily create a colored console logger once.
auto get_logger() -> spdlog::logger & {
	static std::once_flag flag;
	static std::shared_ptr<spdlog::logger> logger;
	std::call_once(flag, [] {
		auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		logger = std::make_shared<spdlog::logger>("engine", sink);
		logger->set_level(spdlog::level::info);
		logger->set_pattern("%^[%T] [%l] %v%$");
		spdlog::register_logger(logger);
	});
	return *logger;
}
} // namespace

void log_info(std::string_view message) {
	get_logger().info("{}", message);
}
