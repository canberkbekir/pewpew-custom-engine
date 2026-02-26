#include "cbpch.h"
#include "CBEngine/FileWatcher/FileWatcher.h"
#include "CBEngine/Core/Log.h"

#ifdef CB_PLATFORM_WINDOWS
#include <Windows.h>

namespace CB
{
	FileWatcher::FileWatcher(const std::filesystem::path& directory,FileWatchCallback callback)
		: m_Directory(directory), m_Callback(callback)
	{
	}

	FileWatcher::~FileWatcher() { Stop(); }

	void FileWatcher::Start()
	{
		if (m_Running)
			return;

		// Open directory handle
		m_DirectoryHandle = CreateFileW(
			m_Directory.wstring().c_str(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			nullptr
		);

		if (m_DirectoryHandle == INVALID_HANDLE_VALUE) {
			CB_CORE_ERROR("FileWatcher: Failed to open directory: {0}", m_Directory.string());
			return;
		}

		// Create stop event
		m_StopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

		m_Running = true;
		m_WatchThread = std::thread(&FileWatcher::WatchThread, this);

		CB_CORE_INFO("FileWatcher started for: {0}", m_Directory.string());
	}

	void FileWatcher::Stop()
	{
		if (!m_Running)
			return;

		m_Running = false;

		// Signal stop event
		if (m_StopEvent) { SetEvent(m_StopEvent); }

		// Wait for thread to finish
		if (m_WatchThread.joinable()) { m_WatchThread.join(); }

		// Close handles
		if (m_DirectoryHandle && m_DirectoryHandle != INVALID_HANDLE_VALUE) {
			CloseHandle(m_DirectoryHandle);
			m_DirectoryHandle = nullptr;
		}

		if (m_StopEvent) {
			CloseHandle(m_StopEvent);
			m_StopEvent = nullptr;
		}

		CB_CORE_INFO("FileWatcher stopped");
	}

	void FileWatcher::WatchThread()
	{
		constexpr DWORD bufferSize = 32768;
		std::vector<BYTE> buffer(bufferSize);
		OVERLAPPED overlapped = {};
		overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

		HANDLE handles[2] = {overlapped.hEvent, m_StopEvent};

		while (m_Running) {
			DWORD bytesReturned = 0;

			ResetEvent(overlapped.hEvent);

			BOOL success = ReadDirectoryChangesW(
				m_DirectoryHandle,
				buffer.data(),
				bufferSize,
				TRUE, // Watch subtree
				FILE_NOTIFY_CHANGE_FILE_NAME |
				FILE_NOTIFY_CHANGE_DIR_NAME |
				FILE_NOTIFY_CHANGE_LAST_WRITE,
				&bytesReturned,
				&overlapped,
				nullptr
			);

			if (!success) {
				DWORD error = GetLastError();
				if (error != ERROR_IO_PENDING) {
					CB_CORE_ERROR("FileWatcher: ReadDirectoryChangesW failed: {0}", error);
					break;
				}
			}

			// Wait for either change notification or stop event
			DWORD waitResult = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

			if (waitResult == WAIT_OBJECT_0) // Change notification
			{
				if (!GetOverlappedResult(m_DirectoryHandle, &overlapped, &bytesReturned, FALSE)) { continue; }

				if (bytesReturned == 0)
					continue;

				auto notify = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data());

				do {
					// Convert filename to path
					std::wstring filename(notify->FileName, notify->FileNameLength / sizeof(WCHAR));
					std::filesystem::path filePath = m_Directory / filename;

					FileWatcherEvent event;
					event.FilePath = filePath;

					switch (notify->Action) {
					case FILE_ACTION_ADDED:
						event.Action = FileAction::Added;
						break;
					case FILE_ACTION_REMOVED:
						event.Action = FileAction::Removed;
						break;
					case FILE_ACTION_MODIFIED:
						event.Action = FileAction::Modified;
						break;
					case FILE_ACTION_RENAMED_OLD_NAME:
						event.Action = FileAction::Renamed;
						event.OldFilePath = filePath;
						break;
					case FILE_ACTION_RENAMED_NEW_NAME:
						event.Action = FileAction::Renamed;
						// Note: Old name was in previous notification
						break;
					}

					// Skip .meta files for notifications (we handle them internally)
					if (filePath.extension() != ".meta") { m_Callback(event); }

					// Move to next entry
					if (notify->NextEntryOffset == 0)
						break;

					notify = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
						reinterpret_cast<BYTE*>(notify) + notify->NextEntryOffset
					);
				}
				while (true);
			}
			else if (waitResult == WAIT_OBJECT_0 + 1) // Stop event
			{
				break;
			}
		}

		CloseHandle(overlapped.hEvent);
	}
}

#endif