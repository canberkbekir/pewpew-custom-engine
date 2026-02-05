#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>
#include <vector>

namespace CB
{
	using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

	struct ProfileResult
	{
		std::string Name;
		FloatingPointMicroseconds Start;
		std::chrono::microseconds ElapsedTime;
		std::thread::id ThreadID;
	};

	struct InstrumentationSession
	{
		std::string Name;
	};

	static constexpr size_t FRAME_HISTORY_SIZE = 120;

	class Instrumentor
	{
	public:
		Instrumentor(const Instrumentor&) = delete;
		Instrumentor(Instrumentor&&) = delete;

		void BeginSession(const std::string& name, const std::string& filepath = "results.json")
		{
			std::lock_guard lock(m_Mutex);
			if (m_CurrentSession)
			{
				// If there is already a current session, then close it before beginning new one.
				// Subsequent profiling output meant for the original session will end up in the
				// newly opened session instead. That's better than having badly formatted
				// profiling output.
				InternalEndSession();
			}
			m_OutputStream.open(filepath);

			if (m_OutputStream.is_open())
			{
				m_CurrentSession = new InstrumentationSession({ name });
				WriteHeader();
			}
		}

		void EndSession()
		{
			std::lock_guard lock(m_Mutex);
			InternalEndSession();
		}

		void WriteProfile(const ProfileResult& result)
		{
			std::lock_guard lock(m_Mutex);

			// Store in current frame scopes for UI
			m_CurrentFrameScopes.push_back(result);

			std::stringstream json;

			json << std::setprecision(3) << std::fixed;
			json << ",{";
			json << "\"cat\":\"function\",";
			json << "\"dur\":" << (result.ElapsedTime.count()) << ',';
			json << "\"name\":\"" << result.Name << "\",";
			json << "\"ph\":\"X\",";
			json << "\"pid\":0,";
			json << "\"tid\":" << result.ThreadID << ",";
			json << "\"ts\":" << result.Start.count();
			json << "}";

			if (m_CurrentSession)
			{
				m_OutputStream << json.str();
				m_OutputStream.flush();
			}
		}

		void BeginFrame()
		{
			std::lock_guard lock(m_Mutex);
			m_FrameStartTime = std::chrono::steady_clock::now();
			m_CurrentFrameScopes.clear();
		}

		void EndFrame()
		{
			std::lock_guard lock(m_Mutex);
			auto endTime = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration<float, std::milli>(endTime - m_FrameStartTime).count();

			m_LastFrameTime = elapsed;
			m_FrameTimeHistory[m_FrameTimeIndex] = elapsed;
			m_FrameTimeIndex = (m_FrameTimeIndex + 1) % FRAME_HISTORY_SIZE;

			// Copy current frame scopes for UI access
			m_LastFrameScopes = m_CurrentFrameScopes;
		}

		float GetFPS() const
		{
			return m_LastFrameTime > 0.0f ? 1000.0f / m_LastFrameTime : 0.0f;
		}

		float GetFrameTimeMs() const
		{
			return m_LastFrameTime;
		}

		const std::vector<ProfileResult>& GetCurrentFrameScopes() const
		{
			return m_LastFrameScopes;
		}

		const std::array<float, FRAME_HISTORY_SIZE>& GetFrameTimeHistory() const
		{
			return m_FrameTimeHistory;
		}

		size_t GetFrameTimeIndex() const
		{
			return m_FrameTimeIndex;
		}

		static Instrumentor& Get()
		{
			static Instrumentor instance;
			return instance;
		}

	private:
		Instrumentor()
			: m_CurrentSession(nullptr)
		{
			m_FrameTimeHistory.fill(0.0f);
		}

		~Instrumentor()
		{
			EndSession();
		}

		void WriteHeader()
		{
			m_OutputStream << "{\"otherData\": {},\"traceEvents\":[{}";
			m_OutputStream.flush();
		}

		void WriteFooter()
		{
			m_OutputStream << "]}";
			m_OutputStream.flush();
		}

		void InternalEndSession()
		{
			if (m_CurrentSession)
			{
				WriteFooter();
				m_OutputStream.close();
				delete m_CurrentSession;
				m_CurrentSession = nullptr;
			}
		}

	private:
		std::mutex m_Mutex;
		InstrumentationSession* m_CurrentSession;
		std::ofstream m_OutputStream;

		// Per-frame tracking for UI
		std::chrono::time_point<std::chrono::steady_clock> m_FrameStartTime;
		std::vector<ProfileResult> m_CurrentFrameScopes;
		std::vector<ProfileResult> m_LastFrameScopes;
		std::array<float, FRAME_HISTORY_SIZE> m_FrameTimeHistory;
		size_t m_FrameTimeIndex = 0;
		float m_LastFrameTime = 0.0f;
	};

	class InstrumentationTimer
	{
	public:
		InstrumentationTimer(const char* name)
			: m_Name(name), m_Stopped(false)
		{
			m_StartTimepoint = std::chrono::steady_clock::now();
		}

		~InstrumentationTimer()
		{
			if (!m_Stopped)
				Stop();
		}

		void Stop()
		{
			auto endTimepoint = std::chrono::steady_clock::now();
			auto highResStart = FloatingPointMicroseconds{ m_StartTimepoint.time_since_epoch() };
			auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch()
				- std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

			Instrumentor::Get().WriteProfile({ m_Name, highResStart, elapsedTime, std::this_thread::get_id() });

			m_Stopped = true;
		}

	private:
		const char* m_Name;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
		bool m_Stopped;
	};

	namespace InstrumentorUtils
	{
		template <size_t N>
		struct ChangeResult
		{
			char Data[N];
		};

		template <size_t N, size_t K>
		constexpr auto CleanupOutputString(const char(&expr)[N], const char(&remove)[K])
		{
			ChangeResult<N> result = {};

			size_t srcIndex = 0;
			size_t dstIndex = 0;
			while (srcIndex < N)
			{
				size_t matchIndex = 0;
				while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
					matchIndex++;
				if (matchIndex == K - 1)
					srcIndex += matchIndex;
				result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
				srcIndex++;
			}
			return result;
		}
	}
}

// Profiling macros
#define CB_PROFILE 1
#if CB_PROFILE
	// Resolve which function signature macro will be used
	#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
		#define CB_FUNC_SIG __PRETTY_FUNCTION__
	#elif defined(__DMC__) && (__DMC__ >= 0x810)
		#define CB_FUNC_SIG __PRETTY_FUNCTION__
	#elif (defined(__FUNCSIG__) || (_MSC_VER))
		#define CB_FUNC_SIG __FUNCSIG__
	#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
		#define CB_FUNC_SIG __FUNCTION__
	#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
		#define CB_FUNC_SIG __FUNC__
	#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
		#define CB_FUNC_SIG __func__
	#elif defined(__cplusplus) && (__cplusplus >= 201103)
		#define CB_FUNC_SIG __func__
	#else
		#define CB_FUNC_SIG "CB_FUNC_SIG unknown!"
	#endif

	#define CB_PROFILE_BEGIN_SESSION(name, filepath) ::CB::Instrumentor::Get().BeginSession(name, filepath)
	#define CB_PROFILE_END_SESSION() ::CB::Instrumentor::Get().EndSession()
	#define CB_PROFILE_SCOPE_LINE2(name, line) constexpr auto fixedName##line = ::CB::InstrumentorUtils::CleanupOutputString(name, "__cdecl ");\
											   ::CB::InstrumentationTimer timer##line(fixedName##line.Data)
	#define CB_PROFILE_SCOPE_LINE(name, line) CB_PROFILE_SCOPE_LINE2(name, line)
	#define CB_PROFILE_SCOPE(name) CB_PROFILE_SCOPE_LINE(name, __LINE__)
	#define CB_PROFILE_FUNCTION() CB_PROFILE_SCOPE(CB_FUNC_SIG)
	#define CB_PROFILE_BEGIN_FRAME() ::CB::Instrumentor::Get().BeginFrame()
	#define CB_PROFILE_END_FRAME() ::CB::Instrumentor::Get().EndFrame()
#else
	#define CB_PROFILE_BEGIN_SESSION(name, filepath)
	#define CB_PROFILE_END_SESSION()
	#define CB_PROFILE_SCOPE(name)
	#define CB_PROFILE_FUNCTION()
	#define CB_PROFILE_BEGIN_FRAME()
	#define CB_PROFILE_END_FRAME()
#endif
