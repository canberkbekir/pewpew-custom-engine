#pragma once

#include <filesystem>
#include <functional>
#include <thread>
#include <atomic>

namespace PewPew
{
	enum class FileAction
	{
		Added,
		Removed,
		Modified,
		Renamed
	};

	struct FileWatcherEvent
	{
		FileAction Action;
		std::filesystem::path FilePath;
		std::filesystem::path OldFilePath;  // Only for Renamed
	};

	using FileWatchCallback = std::function<void(const FileWatcherEvent&)>;

	class FileWatcher
	{
	public:
		FileWatcher(const std::filesystem::path& directory, FileWatchCallback callback);
		~FileWatcher();

		void Start();
		void Stop();

		bool IsRunning() const { return m_Running; }

	private:
		void WatchThread();

	private:
		std::filesystem::path m_Directory;
		FileWatchCallback m_Callback;
		std::thread m_WatchThread;
		std::atomic<bool> m_Running{false};

		void* m_DirectoryHandle = nullptr;  // HANDLE on Windows
		void* m_StopEvent = nullptr;        // HANDLE for stop signaling
	};
}
