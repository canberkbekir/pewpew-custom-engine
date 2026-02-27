#pragma once

#include <string>
#include <vector>
#include <mutex>

#include "String.h"

namespace CB
{
	enum class LogLevel
	{
		Trace,
		Info,
		Debug,
		Warn,
		Error,
		Critical
	};

	enum class LogCategory : uint8_t
	{
		General,
		Physics,
		Voxels,
		Scripting,
		Rendering,
		Count
	};

	struct LogMessage
	{
		String Message;
		LogLevel Level;
		String LoggerName;
		LogCategory Category = LogCategory::General;
	};

	class LogBuffer
	{
	public:
		static LogBuffer& Get()
		{
			static LogBuffer instance;
			return instance;
		}

		void AddMessage(const String& message, LogLevel level, const String& loggerName,
			LogCategory category = LogCategory::General)
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			m_Messages.push_back({message, level, loggerName, category});

			// Keep max 5000 messages
			if (m_Messages.size() > 5000)
				m_Messages.erase(m_Messages.begin());
		}

		void Clear()
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			m_Messages.clear();
		}

		const std::vector<LogMessage>& GetMessages() const { return m_Messages; }
		size_t GetMessageCount() const { return m_Messages.size(); }
	private:
		LogBuffer() = default;
		std::vector<LogMessage> m_Messages;
		std::mutex m_Mutex;
	};
}