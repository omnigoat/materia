module;

#include <atma/config/platform.hpp>
#include <atma/assert.hpp>

#include <rose/rose_fwd.hpp>

#include <winnt.h>

#include <functional>
#include <map>
#include <set>
#include <atomic>
#include <chrono>
#include <tuple>
#include <memory>

export module rose:windows_runtime;

import atma;

import :path;
import :console;

namespace rose
{
	using file_change_callback_t = std::function<void(path_t const&, file_change_t)>;

	struct dir_watch_t
	{
		using callbacks_t = std::vector<file_change_callback_t>;

		static const int bufsize = 512;

		OVERLAPPED overlapped;
		path_t path;
		alignas(4) char bufs[2][bufsize];
		uint32 bufidx = 0;
		uint32 notify;
		HANDLE handle;
		callbacks_t callbacks;

		// changes
		bool pending_change = false;
		std::chrono::time_point<std::chrono::high_resolution_clock> trigger;
		std::set<std::tuple<atma::string, file_change_t>> files;
	};
}

export namespace rose
{
	struct windows_runtime_t
	{
		windows_runtime_t();
		windows_runtime_t(atma::thread_work_provider_t*);
		//~windows_runtime_t();

		using dir_watch_handles_t = atma::vector<HANDLE>;
		using dir_watch_infos_t = atma::vector<dir_watch_t>;
		using dir_watchers_t = std::map<path_t, size_t>;

		// directory watching
		dir_watch_handles_t dir_handles_;
		dir_watch_infos_t dir_infos_;
		dir_watchers_t dir_watchers_;

		void initialize_watching();

		// file-watch
		auto register_directory_watch(
			path_t const&,
			bool recursive,
			file_change_mask_t,
			file_change_callback_t const&) -> void;
		
		// placed last for good reason (other thread still using members!)
		atma::inplace_engine_t filewatch_engine_;
		atma::thread_work_provider_t* work_provider_;
		atma::work_token_t token_;
		std::atomic_bool running_ = false;
	};
}

namespace rose
{
	VOID CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
}

namespace rose
{
	windows_runtime_t::windows_runtime_t()
		: filewatch_engine_{atma::inplace_engine_t::defer_start_t{}, 512}
		, work_provider_{&filewatch_engine_}
	{}

	windows_runtime_t::windows_runtime_t(atma::thread_work_provider_t* wp)
		: work_provider_{wp}
	{}

	//windows_runtime_t::~windows_runtime_t()
	//{
	//	running_ = false;
	//}

	void windows_runtime_t::initialize_watching()
	{
		static_assert(std::is_implicit_lifetime_v<int>);

		work_provider_->ensure_running();

		work_provider_->enqueue_repeat_against(token_, [&]
		{
			auto status = WaitForMultipleObjectsEx((DWORD)dir_handles_.size(), dir_handles_.data(), FALSE, 100, TRUE);
		});

		work_provider_->enqueue_repeat_against(token_, [&]()
		{
			auto now = std::chrono::high_resolution_clock::now();

			for (auto& info : dir_infos_)
			{
				auto d = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.trigger);
				if (info.pending_change && d > std::chrono::milliseconds{100})
				{
					for (auto const [path, action] : info.files)
						for (auto const& c : info.callbacks)
							c(path, action);

					info.pending_change = false;
					info.files.clear();
				}
			}
		});
	}

	auto windows_runtime_t::register_directory_watch(
		path_t const& path, bool recursive, file_change_mask_t changes,
		file_change_callback_t const& callback) -> void
	{
		if (running_.exchange(true) == false)
			initialize_watching();

		// WIN32 file-notify flags
		int notify = 0;
		if ((changes & file_change_t::created) || (changes & file_change_t::renamed) || (changes & file_change_t::deleted))
			notify |= FILE_NOTIFY_CHANGE_FILE_NAME;
		if (changes & file_change_t::attributed)
			notify |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
		if (changes & file_change_t::changed)
			notify |= FILE_NOTIFY_CHANGE_LAST_WRITE;
		if (changes & file_change_t::securitied)
			notify |= FILE_NOTIFY_CHANGE_SECURITY;


		work_provider_->enqueue_against(token_, [&, path, recursive, notify, callback]
		{
			auto candidate = dir_watchers_.find(path);
			if (candidate != dir_watchers_.end())
			{
				ATMA_ASSERT(dir_infos_.size() > candidate->second);
				dir_infos_[(int)candidate->second].callbacks.push_back(callback);
				return;
			}

			// WIN32 strings :(
			int char16s = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(), -1, nullptr, 0);
			wchar_t* wpath = (wchar_t*)alloca(sizeof(wchar_t) * char16s);
			int r = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(), -1, wpath, char16s);
			if (r == 0)
				return;

			auto& info = dir_infos_.emplace_back();
			info.path = path;
			info.notify = notify;
			HANDLE dir = CreateFileW(wpath,
				FILE_LIST_DIRECTORY, FILE_SHARE_READ,
				NULL, OPEN_EXISTING,
				FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
				NULL);

			if (dir == INVALID_HANDLE_VALUE)
				return;

			info.handle = dir;
			info.callbacks.push_back(callback);

			DWORD bytes = 0;
			BOOL success = ReadDirectoryChangesW(dir,
				info.bufs[info.bufidx], info.bufsize,
				FALSE, notify,
				&bytes, &info.overlapped,
				&rose::FileIOCompletionRoutine);

			if (success)
			{
				dir_handles_.push_back(dir);
			}

			dir_watchers_[path] = dir_handles_.size() - 1;
		});
	}
}

namespace rose
{
	VOID CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		auto& info = *reinterpret_cast<dir_watch_t*>(lpOverlapped);

		if (FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)info.bufs[info.bufidx])
		{
			for (;;)
			{
				fni = (FILE_NOTIFY_INFORMATION*)((byte*)fni + fni->NextEntryOffset);

				char buf[256];
				memset(buf, 0, 256);
				int L = WideCharToMultiByte(CP_ACP, 0, fni->FileName, -1, buf, 256, NULL, NULL);
				auto filename = atma::string{buf, buf + L - 1}; // minus one to exclude null-terminator

#if 0 
				FILE* f = nullptr;
				auto yay = (info.path / filename).string();
				while ((f = fopen(yay.c_str(), "r")) == nullptr)
					std::cout << "waiting..." << std::endl;
				fclose(f);
#endif

				info.trigger = std::chrono::high_resolution_clock::now();
				info.pending_change = true;
				info.files.insert(std::make_tuple(filename, file_change_t::changed));

				if (fni->NextEntryOffset == 0)
					break;

				fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<char*>(info.bufs[info.bufidx]) + fni->NextEntryOffset);
			}

		}

		memset(info.bufs[info.bufidx], 0, sizeof(info.bufs[0]));
		info.bufidx = (info.bufidx + 1) % 2;


		ReadDirectoryChangesW(
			info.handle,
			info.bufs[info.bufidx], info.bufsize,
			FALSE, info.notify,
			nullptr, &info.overlapped,
			&rose::FileIOCompletionRoutine);
	}
}

