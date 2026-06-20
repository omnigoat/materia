module;

#include <atma/config/platform.hpp>

#include <rose/rose_fwd.hpp>

#include <atma/assert.hpp>

#include <functional>
#include <map>
#include <set>
#include <atomic>
#include <chrono>
#include <tuple>
#include <memory>

export module rose:runtime;

import atma;

import :console;
import :path;

#if ATMA_PLATFORM_WINDOWS
import :windows_runtime;
namespace rose { using _platform_runtime_ = windows_runtime_t; }
#endif

export namespace rose
{
	struct runtime_t
	{
		using file_change_callback_t = std::function<void(path_t const&, file_change_t)>;

		static constexpr size_t host_size() { return sizeof(runtime_t); }

		runtime_t();
		runtime_t(atma::thread_work_provider_t*);
		~runtime_t();

		// console
		auto console() -> console_t& { return console_; }
		auto console_logging_handler() -> atma::logging_handler_t* { return &default_console_log_handler_; }

		// file-watch
		auto register_directory_watch(
			path_t const&,
			bool recursive,
			file_change_mask_t,
			file_change_callback_t const&) -> void;

	private:
		console_t console_;

		// we must provide an easy way to log stuff
		default_console_log_handler_t default_console_log_handler_;
	};

	constexpr size_t static_host_size(runtime_t const&)
	{
		return sizeof(runtime_t);
	}

	//static constexpr size_t static_host_size = sizeof(runtime_t);

	using runtime_ptr = std::unique_ptr<runtime_t>;
}

export namespace rose
{
	auto make_runtime() -> runtime_ptr
	{
		return atma::make_bridge_ptr<runtime_ptr, runtime_t, _platform_runtime_>();
	}

	auto make_runtime(atma::thread_work_provider_t* wp) -> runtime_ptr
	{
		return atma::make_bridge_ptr<runtime_ptr, runtime_t, _platform_runtime_>(wp);
	}
}

export namespace rose
{
	inline auto setup_default_logging_to_console(runtime_t& RR) -> void
	{
		atma::default_logging_runtime()->attach_handler(RR.console_logging_handler());
	}
}

namespace rose
{
	auto runtime_t::register_directory_watch(
		path_t const& path, bool recursive, file_change_mask_t changes,
		file_change_callback_t const& callback) -> void
	{
		atma::bridge_cross_unsafe<_platform_runtime_>(this)
			->register_directory_watch(path, recursive, changes, callback);
	}
}



