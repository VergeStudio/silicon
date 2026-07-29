#include "silicon/logger/default_logger.h"

#include <ctime>
#include <format>
#include <iostream>
#include <memory>
#include <mutex>
#include <source_location>
#include <string_view>

#include "silicon/core/string.hpp"
#include "spdlog/async.h"
#include "spdlog/async_logger.h"
#include "spdlog/sinks/hourly_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace silicon::logger {

// Pimpl implementation — spdlog types live here, invisible to module consumers
struct DefaultLogger::Impl {
    std::shared_ptr<spdlog::logger> pLogger{nullptr};
    const std::string_view pattern{"%^[%Y-%m-%d %H:%M:%S.%e][%t][%l]%v%$"};
};

DefaultLogger::DefaultLogger(): m_impl(std::make_unique<Impl>()) {}

DefaultLogger::~DefaultLogger() noexcept = default;

void DefaultLogger::Init(const std::string_view &rLogPath, const LogLevel logLevel, const int32_t qsize, const int32_t threadNum, const int32_t backtraceNum) {
    try {
        if(!m_isInitialized) {
            std::scoped_lock<std::mutex> const lock(m_mutex);

            spdlog::init_thread_pool(qsize, threadNum);
            CreateLogger(logLevel, util::StrCat(rLogPath, "/main.log"), backtraceNum);

            m_isInitialized = true;
        }
    } catch(const spdlog::spdlog_ex &ex) {
        std::cout << "log init failed:" << ex.what() << std::endl;

        Stop();
    }
}

void DefaultLogger::CreateLogger(const LogLevel logLevel, const std::string_view &rLogFile, const int32_t backtraceNum) {
    auto pStdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto pHourlySink = std::make_shared<spdlog::sinks::hourly_file_sink_mt>(rLogFile.data(), 0, 0);
    spdlog::sinks_init_list sinks{pStdoutSink, pHourlySink};
    m_impl->pLogger = std::make_shared<spdlog::async_logger>("mainLogger", sinks, spdlog::thread_pool());
    m_impl->pLogger->set_pattern(m_impl->pattern.data());
    m_impl->pLogger->enable_backtrace(backtraceNum);
    SetLogLevel(logLevel);

    spdlog::register_logger(m_impl->pLogger);
}

void DefaultLogger::SetLogLevel(const LogLevel logLevel) const {
    switch(logLevel) {
        case LogLevel::Trace:
            return m_impl->pLogger->set_level(spdlog::level::level_enum::trace);
        case LogLevel::Debug:
            return m_impl->pLogger->set_level(spdlog::level::level_enum::debug);
        case LogLevel::Info:
            return m_impl->pLogger->set_level(spdlog::level::level_enum::info);
        case LogLevel::Warn:
            return m_impl->pLogger->set_level(spdlog::level::level_enum::warn);
        case LogLevel::Error:
            return m_impl->pLogger->set_level(spdlog::level::level_enum::err);
        case LogLevel::Critical:
            return m_impl->pLogger->set_level(spdlog::level::level_enum::critical);
        case LogLevel::Off:
            return m_impl->pLogger->set_level(spdlog::level::level_enum::off);
        default:
            return;
    }
}

void DefaultLogger::Stop() {
    try {
        std::scoped_lock<std::mutex> const lock(m_mutex);

        if(!m_isInitialized) {
            std::cout << "logger stop failed: not initialized\n";
            return;
        }

        if(m_impl->pLogger != nullptr) {
            m_impl->pLogger->flush();
            m_impl->pLogger.reset();

            spdlog::shutdown();
        }

        m_isInitialized = false;
    } catch(const spdlog::spdlog_ex &ex) {
        std::cout << "logger stop failed: " << ex.what() << '\n';
    }
}

void DefaultLogger::Trace(const std::string_view &msg, std::source_location &&rLocation) const {
    m_impl->pLogger->trace(std::format("[{}:{}] {}", rLocation.file_name(), rLocation.line(), msg.data()));
}

void DefaultLogger::Debug(const std::string_view &msg, std::source_location &&rLocation) const {
    m_impl->pLogger->debug(std::format("[{}:{}] {}", rLocation.file_name(), rLocation.line(), msg.data()));
}

void DefaultLogger::Info(const std::string_view &msg, std::source_location &&rLocation) const {
    m_impl->pLogger->info(std::format("[{}:{}] {}", rLocation.file_name(), rLocation.line(), msg.data()));
}

void DefaultLogger::Warning(const std::string_view &msg, std::source_location &&rLocation) const {
    m_impl->pLogger->warn(std::format("[{}:{}] {}", rLocation.file_name(), rLocation.line(), msg.data()));
}

void DefaultLogger::Error(const std::string_view &msg, std::source_location &&rLocation) const {
    m_impl->pLogger->error(std::format("[{}:{}] {}", rLocation.file_name(), rLocation.line(), msg.data()));
}

void DefaultLogger::Critical(const std::string_view &msg, std::source_location &&rLocation) const {
    m_impl->pLogger->critical(std::format("[{}:{}] {}", rLocation.file_name(), rLocation.line(), msg.data()));
}

} // namespace silicon::logger
