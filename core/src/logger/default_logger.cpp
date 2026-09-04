module;

#include <ctime>
#include <expected>
#include <format>
#include <iostream>
#include <memory>
#include <mutex>
#include <source_location>
#include <string_view>
#include "spdlog/async.h"
#include "spdlog/async_logger.h"
#include "spdlog/sinks/hourly_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

module silicon.logger;

import :default_logger;
import silicon.util;
import silicon.logger.error;

namespace silicon::logger {

struct default_logger::impl {
    std::shared_ptr<spdlog::logger> spdlog_logger{nullptr};
    const std::string_view pattern{"%^[%Y-%m-%d %H:%M:%S.%e][%t][%l]%v%$"};
};

default_logger::default_logger(): impl_(std::make_unique<impl>()) {}

default_logger::~default_logger() noexcept = default;

std::expected<void, std::error_code> default_logger::init(const std::string_view &log_path, const log_level log_level, const int32_t queue_size, const int32_t thread_num, const int32_t backtrace_num) {
    try {
        if(!is_initialized_) {
            std::scoped_lock<std::mutex> const lock(mutex_);

            spdlog::init_thread_pool(queue_size, thread_num);
            create_logger(log_level, util::str_cat(log_path, "/main.log"), backtrace_num);

            is_initialized_ = true;
        }
        return {};
    } catch(const spdlog::spdlog_ex &) {

        stop();
        return std::unexpected(make_error_code(logger_error::kInitFailed));
    }
}

void default_logger::create_logger(const log_level log_level, const std::string_view &log_file, const int32_t backtrace_num) {
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto hourly_sink = std::make_shared<spdlog::sinks::hourly_file_sink_mt>(log_file.data(), 0, 0);
    spdlog::sinks_init_list sinks{stdout_sink, hourly_sink};
    impl_->spdlog_logger = std::make_shared<spdlog::async_logger>("mainLogger", sinks, spdlog::thread_pool());
    impl_->spdlog_logger->set_pattern(impl_->pattern.data());
    impl_->spdlog_logger->enable_backtrace(backtrace_num);
    set_log_level(log_level);

    spdlog::register_logger(impl_->spdlog_logger);
}

void default_logger::set_log_level(const log_level log_level) const {
    switch(log_level) {
        case log_level::kTrace:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::trace);
        case log_level::kDebug:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::debug);
        case log_level::kInfo:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::info);
        case log_level::kWarn:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::warn);
        case log_level::kError:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::err);
        case log_level::kCritical:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::critical);
        case log_level::kOff:
            return impl_->spdlog_logger->set_level(spdlog::level::level_enum::off);
        default:
            return;
    }
}

void default_logger::stop() {
    try {
        std::scoped_lock<std::mutex> const lock(mutex_);

        if(!is_initialized_) {
            std::cout << "logger stop failed: not initialized\n";
            return;
        }

        if(impl_->spdlog_logger != nullptr) {
            impl_->spdlog_logger->flush();
            impl_->spdlog_logger.reset();

            spdlog::shutdown();
        }

        is_initialized_ = false;
    } catch(const spdlog::spdlog_ex &ex) {
        std::cout << "logger stop failed: " << ex.what() << '\n';
    }
}

void default_logger::trace(const std::string_view &msg, std::source_location &&location) const {
    impl_->spdlog_logger->trace(std::format("[{}:{}] {}", location.file_name(), location.line(), msg.data()));
}

void default_logger::debug(const std::string_view &msg, std::source_location &&location) const {
    impl_->spdlog_logger->debug(std::format("[{}:{}] {}", location.file_name(), location.line(), msg.data()));
}

void default_logger::info(const std::string_view &msg, std::source_location &&location) const {
    impl_->spdlog_logger->info(std::format("[{}:{}] {}", location.file_name(), location.line(), msg.data()));
}

void default_logger::warning(const std::string_view &msg, std::source_location &&location) const {
    impl_->spdlog_logger->warn(std::format("[{}:{}] {}", location.file_name(), location.line(), msg.data()));
}

void default_logger::error(const std::string_view &msg, std::source_location &&location) const {
    impl_->spdlog_logger->error(std::format("[{}:{}] {}", location.file_name(), location.line(), msg.data()));
}

void default_logger::critical(const std::string_view &msg, std::source_location &&location) const {
    impl_->spdlog_logger->critical(std::format("[{}:{}] {}", location.file_name(), location.line(), msg.data()));
}

}
