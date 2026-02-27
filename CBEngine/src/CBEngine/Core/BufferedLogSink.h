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
			switch (msg.level) {
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

			std::string text = fmt::to_string(formatted);
			LogCategory category = LogCategory::General;

			// Detect category tag from the payload (not the formatted output)
			std::string payload(msg.payload.data(), msg.payload.size());
			struct TagMapping { const char* Tag; size_t Len; LogCategory Cat; };
			static const TagMapping tags[] = {
				{ "[Physics] ",   10, LogCategory::Physics },
				{ "[Voxels] ",    9,  LogCategory::Voxels },
				{ "[Scripting] ", 12, LogCategory::Scripting },
				{ "[Rendering] ", 12, LogCategory::Rendering },
			};
			for (const auto& t : tags) {
				if (payload.size() >= t.Len && payload.compare(0, t.Len, t.Tag) == 0) {
					category = t.Cat;
					// Strip the tag from the formatted text
					auto pos = text.find(t.Tag);
					if (pos != std::string::npos)
						text.erase(pos, t.Len);
					break;
				}
			}

			LogBuffer::Get().AddMessage(text, level,
				std::string(msg.logger_name.data(), msg.logger_name.size()),
				category);
		}

		void flush_() override
		{
		}
	};

	using BufferedLogSink_mt = BufferedLogSink<std::mutex>;
	using BufferedLogSink_st = BufferedLogSink<spdlog::details::null_mutex>;
}