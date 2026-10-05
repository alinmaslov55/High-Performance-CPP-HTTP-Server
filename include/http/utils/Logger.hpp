#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <memory>
#include <spdlog/spdlog.h>

namespace http {
namespace utils {

class Logger {
public:
    static void init();
};

} // namespace utils
} // namespace http

#define LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define LOG_WARN(...) spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#define LOG_CRIT(...) spdlog::critical(__VA_ARGS__)

#endif // LOGGER_HPP