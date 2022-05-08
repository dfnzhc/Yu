//
// Created by 秋鱼 on 2022/5/6.
//

#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace ST {

const char* LOGGER_NAME = "Setaria";

class Log final
{
public:
    Log()
    {
        Init();
    }

    ~Log()
    {
        Shutdown();
    }

    static void Init()
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("%^[%T]: %v%$");
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs.txt", true);
        file_sink->set_pattern("[%T] [%l]: %v");

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>(ST::LOGGER_NAME, sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::trace);
        spdlog::register_logger(logger);
    }

    static void Shutdown()
    {
        spdlog::shutdown();
    }
};

} // namespace ST


// Mainly for IDEs
#ifndef ROOT_PATH_SIZE
#	define ROOT_PATH_SIZE 0
#endif

//#define __FILENAME__ (static_cast<const char *>(__FILE__) + ROOT_PATH_SIZE)

#define LOG_TRACE(...)    if (spdlog::get(ST::LOGGER_NAME) != nullptr) {spdlog::get(ST::LOGGER_NAME)->trace(__VA_ARGS__);}
#define LOG_DEBUG(...)    if (spdlog::get(ST::LOGGER_NAME) != nullptr) {spdlog::get(ST::LOGGER_NAME)->debug(__VA_ARGS__);}
#define LOG_INFO(...)    if (spdlog::get(ST::LOGGER_NAME) != nullptr) {spdlog::get(ST::LOGGER_NAME)->info(__VA_ARGS__);}
#define LOG_WARN(...)    if (spdlog::get(ST::LOGGER_NAME) != nullptr) {spdlog::get(ST::LOGGER_NAME)->warn(__VA_ARGS__);}
#define LOG_ERROR(...)    if (spdlog::get(ST::LOGGER_NAME) != nullptr) \
    {spdlog::get(ST::LOGGER_NAME)->error("[{}:{}] {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__));}