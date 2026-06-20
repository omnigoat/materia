module;

#include <atma/aligned_allocator.hpp>
#include <atma/assert.hpp>
#include <atma/types.hpp>

export module atma:string;

import :memory;

#if _MSC_VER
#  define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#elif __clang__
#  define NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#  error bad compiler
#endif

namespace atma
{
	struct DefaultStringTraits
	{
		using char_type = char;
		using allocator_type = atma::aligned_allocator_t<char_type, 1>;
	};

	template <typename Traits = DefaultStringTraits>
	struct string
	{
		using char_type = typename Traits::char_type;
		using allocator_type = typename Traits::allocator_type;

		string() = default;

	private:
		void ensure_additional_space(size_t);

	private:
		NO_UNIQUE_ADDRESS allocator_type allocator_;
		char_type* text_ {};
		size_t size_ {};
		size_t capacity_ {};
	};
}

#if 0
namespace atma
{
	template <typename T>
	void string<T>::ensure_additional_space(size_t sz)
	{
		ATMA_ASSERT(sz, "you should not ensure non-growth");

		if (size_t new_size = (size_ + sz + 1); new_size >= capacity_)
		{
			// if we were to naturally grow this buffer
			size_t const grown_cap_bytes = (capacity_ / 2 * 3) * sizeof(char_type);
			// smallest amount of space we need to accomodate
			size_t const min_cap_bytes = (sz * sizeof(char_type));

			size_t newcap_bytes = sizeof(char_type) * newcap;

			auto newmem = (std::byte*)allocator_.alloc(newcap_bytes);
			//memory_copy(
			//	xfer_dest(newmem, newcap_bytes),
			//	xfer_src(storage_.data(), )
		}
	}
}
#endif


