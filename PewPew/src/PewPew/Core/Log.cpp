#include "pewpch.h"
#include "Log.h"
#include "BufferedLogSink.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace PewPew
{
    Ref<spdlog::logger> Log::s_CoreLogger;
    Ref<spdlog::logger> Log::s_ClientLogger;

    void Log::Init()
    {
        // Create sinks
        auto consoleSink = CreateRef<spdlog::sinks::stdout_color_sink_mt>();
        auto bufferedSink = CreateRef<BufferedLogSink_mt>();

        consoleSink->set_pattern("%^[%T] %n: %v%$");
        bufferedSink->set_pattern("[%T] %n: %v");

        std::vector<spdlog::sink_ptr> coreSinks = { consoleSink, bufferedSink };
        std::vector<spdlog::sink_ptr> clientSinks = { consoleSink, bufferedSink };

        // Create loggers with multiple sinks
        s_CoreLogger = CreateRef<spdlog::logger>("PewPew", coreSinks.begin(), coreSinks.end());
        s_CoreLogger->set_level(spdlog::level::trace);
        spdlog::register_logger(s_CoreLogger);

        s_ClientLogger = CreateRef<spdlog::logger>("Client", clientSinks.begin(), clientSinks.end());
        s_ClientLogger->set_level(spdlog::level::trace);
        spdlog::register_logger(s_ClientLogger);
    }
}
