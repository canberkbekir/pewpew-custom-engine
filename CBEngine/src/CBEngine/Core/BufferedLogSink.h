#pragma once

#include "LogBuffer.h"
#include "spdlog/sinks/base_sink.h"

namespace CB
{
    template <typename Mutex>
    class BufferedLogSink : public spdlog::sinks::base_sink<Mutex>
    {
    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            spdlog::memory_buf_t formatted;
            spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

            auto level = LogLevel::Info;
            switch (msg.level)
            {
            case spdlog::level::trace: level = LogLevel::Trace;
                break;
            case spdlog::level::debug: level = LogLevel::Debug;
                break;
            case spdlog::level::info: level = LogLevel::Info;
                break;
            case spdlog::level::warn: level = LogLevel::Warn;
                break;
            case spdlog::level::err: level = LogLevel::Error;
                break;
            case spdlog::level::critical: level = LogLevel::Critical;
                break;
            default: break;
            }

            LogBuffer::Get().AddMessage(
                fmt::to_string(formatted),
                level,
                std::string(msg.logger_name.data(), msg.logger_name.size())
            );
        }

        void flush_() override
        {
        }
    };

    using BufferedLogSink_mt = BufferedLogSink<std::mutex>;
    using BufferedLogSink_st = BufferedLogSink<spdlog::details::null_mutex>;
}
