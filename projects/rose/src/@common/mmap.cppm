module;

#include <atma/config/platform.hpp>

#include <rose/rose_fwd.hpp>
#include <atma/assert.hpp>

#include <filesystem>

export module rose:mmap;

import atma;

namespace stdfs = std::filesystem;

export namespace rose
{
	struct mmap_t
		: atma::ref_counted
	{
		mmap_t() = delete;
		mmap_t(stdfs::path const&, file_access_mask_t = file_access_t::read);
		mmap_t(mmap_t const&) = delete;
		mmap_t(mmap_t&&);
		virtual ~mmap_t();

		mmap_t& operator = (mmap_t const&) = delete;
		mmap_t& operator = (mmap_t&&);

		virtual auto valid() const -> bool;
		auto size() const -> size_t;
		auto access_mask() const -> file_access_mask_t;

	protected:
		std::filesystem::path path_;
		file_access_mask_t access_mask_;
		size_t size_ {};

		friend struct mmap_bytestream_t;
	};

	using mmap_ptr = atma::intrusive_ptr<mmap_t>;
}


///
/// make_mmap
/// -----------
/// implementation defined
/// 
export namespace rose
{
	mmap_ptr make_mmap(stdfs::path const&, file_access_mask_t = file_access_t::read);
}

export namespace rose
{
	auto mmap_t::access_mask() const -> file_access_mask_t
	{
		return access_mask_;
	}

	auto mmap_t::size() const -> size_t
	{
		return size_;
	}
}

