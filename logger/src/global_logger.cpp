#include <ctime>
#include <iostream>
#include <memory>
#include <mutex>
#include <format>
#include <source_location>
#include <string_view>

import silicon.util;
#include "silicon/logger/global_logger.h"
#include "spdlog/async.h"
#include "spdlog/async_logger.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/hourly_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace silicon::logger {

// Pimpl implementation — spdlog types live here, invisible to module consumers
struct GlobalLogger::Impl {
    std::shared_ptr<spdlog::logger> spdlog_logger{nullptr};
    const std::string_view pattern{"%^[%Y-%m-%d %H:%M:%S.%e][%t][%l]%v%$"};
};

GlobalLogger::GlobalLogger() : impl_(std::make_unique<Impl>()) {}

GlobalLogger::~GlobalLogger() noexcept = default;

void GlobalLogger::Init(const std::string_view &log_path, const LogLevel log_level, const int32_t queue_size, const int32_t thread_num, const int32_t backtrace_num) {
    try {
        if (!is_initialized_) {
            std::scoped_lock<std::mutex> const lock(mutex_);

            spdlog::init_thread_pool(queue_size, thread_num);
            CreateLogger(log_level, util::StrCat(log_path, "/main.log"), backtrace_num);

            is_initialized_ = true;
        }
    } catch (const spdlog::spdlog_ex &ex) {
        std::cout << "log init failed:" << ex.what() << std::endl;

        Stop();
    }
}

void GlobalLogger::CreateLogger(const LogLevel log_level, const std::string_view &log_file, const int32_t backtrace_num) {
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto hourly_sink = std::make_shared<spdlog::sinks::hourly_file_sink_mt>(log_file.data(), 0, 0);
    spdlog::sinks_init_list sinks{stdout_sink, hourly_sink};
    impl_->spdlog_logger = std::make_shared<spdlog::async_logger>("mainLogger", sinks, spdlog::thread_pool());
    impl_->spdlog_logger->set_pattern(impl_->pattern.data());
    impl_->spdlog_logger->enable_backtrace(backtrace_num);
    SetLogLevel(log_level);

    spdlog::register_logger(impl_->spdlog_logger);
}

void GlobalLogger::SetLogLevel(const LogLevel log_level) const {
    switch (log_level) {
        case LogLevel::kTrace:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::trace);
        case LogLevel::kDebug:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::debug);
        case LogLevel::kInfo:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::info);
        case LogLevel::kWarn:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::warn);
        case LogLevel::kError:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::err);
        case LogLevel::kCritical:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::critical);
        case LogLevel::kOff:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::off);
        default:
            return;
    }
}

void GlobalLogger::Stop() {
    try {
        std::scoped_lock<std::mutex> const lock(mutex_);

        if (!is_initialized_) {
            std::cout << "logger stop failed: not initialized\n";
            return;
        }

        if (impl_->spdlog_logger != nullptr) {
            impl_->spdlog_logger->flush();
            impl_->spdlog_logger.reset();

            spdlog::shutdown();
        }

        is_initialized_ = false;
    } catch (const spdlog::spdlog_ex &ex) {
        std::cout << "logger stop failed: " << ex.what() << '\n';
    }
}

void GlobalLogger::Trace(const std::string_view &msg, std::source_location &&location) const {
    impl_->spdlog_logger->trace(std::format("[{}:{}] {}", location.file_name(), location.line(), msg.data()));
}

void GlobalLogger::Debug(const std::string_view &msg, std::source_location &&location) const {
    impl_->spdlog_logger->debug(std::format("[{}:{}] {}", location.file_name(), location.line(), msg.data()));
}

void GlobalLogger::Info(const std::string_view &msg, std::source_location &&location) const {
    impl_->spdlog_logger->info(std::format("[{}:{}] {}", location.file_name(), location.line(), msg.data()));
}

void GlobalLogger::Warning(const std::string_view &msg, std::source_location &&location) const {
    impl_->spdlog_logger->warn(std::format("[{}:{}] {}", location.file_name(), location.line(), msg.data()));
}

void GlobalLogger::Error(const std::string_view &msg, std::source_location &&location) const {
    impl_->spdlog_logger->error(std::format("[{}:{}] {}", location.file_name(), location.line(), msg.data()));
}

void GlobalLogger::Critical(const std::string_view &msg, std::source_location &&location) const {
    impl_->spdlog_logger->critical(std::format("[{}:{}] {}", location.file_name(), location.line(), msg.data()));
}

} // namespace silicon::logger
