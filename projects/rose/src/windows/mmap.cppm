module;

#include <atma/config/platform.hpp>
#include <rose/rose_fwd.hpp>

#include <filesystem>

export module rose:mmap_windows;

export import :mmap;

namespace stdfs = std::filesystem;

export namespace rose
{
	struct mmap_windows_t
		: mmap_t
	{
		using handle_t = HANDLE;

		mmap_windows_t() = delete;
		mmap_windows_t(stdfs::path const&, file_access_mask_t = file_access_t::read);
		virtual ~mmap_windows_t();

		virtual auto valid() const -> bool override;

	private:
		handle_t handle_{ INVALID_HANDLE_VALUE };
	};

	using mmap_ptr = atma::intrusive_ptr<mmap_t>;
}

export namespace rose
{
	mmap_ptr make_mmap(stdfs::path const& path, file_access_mask_t fam)
	{
		return mmap_ptr{new mmap_windows_t{path, fam}};
	}
}

namespace rose
{
	mmap_windows_t::mmap_windows_t(stdfs::path const& path, file_access_mask_t fam)
		: mmap_t{path, fam}
	{
		if (!stdfs::exists(path_))
			return;

		DWORD file_access = (fam & file_access_t::read ? GENERIC_READ : 0) | (fam & file_access_t::write ? GENERIC_WRITE : 0);
		auto file_handle = CreateFileW(path_.c_str(), file_access, 0, nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);

		DWORD map_access = fam & file_access_t::write ? PAGE_READWRITE : PAGE_READONLY;
		handle_ = CreateFileMapping(file_handle, nullptr, map_access, 0, 0, 0);

		LARGE_INTEGER i;
		GetFileSizeEx(file_handle, &i);
		size_ = i.QuadPart;

		CloseHandle(file_handle);
	}

	mmap_windows_t::~mmap_windows_t()
	{
		if (handle_ != INVALID_HANDLE_VALUE)
			CloseHandle(handle_);
	}
}

namespace rose
{
	auto mmap_windows_t::valid() const -> bool
	{
		return handle_ != INVALID_HANDLE_VALUE;
	}
}
