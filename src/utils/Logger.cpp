#include "http/utils/Logger.hpp"
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <vector>
#include <iostream>

namespace http {
namespace utils {

void Logger::init() {
    try {
        // Queue for async logging (8192 items, 1 backing thread)
        spdlog::init_thread_pool(8192, 1);

        // Console sink with colors
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        console_sink->set_pattern("[%^%l%$] %v"); // [info] Message

        // Rotating file sink (Max 5MB per file, max 3 files)
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/server.log", 1024 * 1024 * 5, 3);
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] %v");

        std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};

        // Create the async logger
        auto async_logger = std::make_shared<spdlog::async_logger>(
            "multi_sink", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
            
        // Register it as the global default logger
        spdlog::set_default_logger(async_logger);
        spdlog::set_level(spdlog::level::info);
        spdlog::flush_on(spdlog::level::warn);

    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
    }
}

} // namespace utils
} // namespace http