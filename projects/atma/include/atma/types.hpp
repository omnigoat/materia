#pragma once

#include <atma/preprocessor.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <string>

#include <type_traits>

//
// fundamental integral types
// ----------------------------
//   we put oft-used types into the global namespace. so sue me.
//
using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using intptr = intptr_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using uintptr = uintptr_t;

using byte = std::byte;

using ptrdiff_t = std::ptrdiff_t;
using size_t = std::size_t;




#define ATMA_SPLAT_FN_II(counter, pattern) int ATMA_PP_CAT(splat, counter)[] = {0, (pattern, void(), 0)...};
#define ATMA_SPLAT_FN(pattern) ATMA_SPLAT_FN_II(__COUNTER__, pattern)

#define ATMA_SFINAE_TEST_METHOD(name, oper) \
	template <typename T> \
	struct name##_t \
	{ \
	private: \
		template <typename C> constexpr static bool test(decltype(&C::oper)) { return true; } \
		template <typename C> constexpr static bool test(...) { return false; } \
		\
	public: \
		static constexpr bool value = test<T>(0); \
	}; \
	template <typename T> constexpr bool name##_v = name##_t<T>::value





//
//  idxs_t
//  --------
//
namespace atma
{
	template <size_t... Idxs>
	using idxs_t = std::index_sequence<Idxs...>;
}

///
/// string_literal
/// ----------------
/// 
namespace atma
{
	template <size_t N>
	struct string_literal
	{
		constexpr string_literal(string_literal const& rhs)
		{
			std::copy_n(rhs.data, N, data);
		}

		constexpr string_literal(char const (&str)[N])
		{
			std::copy_n(str, N, data);
		}

		template <size_t L, size_t R>
			requires (L + R <= N)
		constexpr string_literal(char const (&lhs)[L], char const (&rhs)[R])
		{
			std::copy_n(lhs, L, data);
			std::copy_n(rhs, R, data + L);
		}

		template <size_t M>
		constexpr string_literal<N + M> push_back(char const (&str)[M]) const
		{
			return string_literal<N + M>{data, str};
		}

		constexpr operator char const* () const { return data; }
		constexpr operator std::string() const { return { data }; }

		char data[N];
	};
}


//
//  idxs_list_t
//  idxs_range_t
//  --------------
//    creates an idxs_t<> of [begin, end), interpolated by step
//
//      idxs_list_t<4> === idxs_t<0, 1, 2, 3>
//      idxs_range_t<5, 9> === idxs_t<5, 6, 7, 8>
//      idxs_range_t<7, 3, -1> === idxs_t<7, 6, 5, 4>
//      idxs_range_t<7, 3, -2> === idxs_t<7, 5>
//      idxs_range_t<7, 3, -3> === such compilation fail
//
namespace atma::detail
{
	template <size_t Begin, size_t End, int Step, size_t... Idxs>
	struct idxs_range_t_
	{
		static_assert(Step != 0, "bad arguments to idxs_range_tx");
		static_assert(Step > 0 || Begin > End, "bad arguments to idxs_range_tx");
		static_assert(Step < 0 || Begin < End, "bad arguments to idxs_range_tx");

		using type = typename idxs_range_t_<size_t(Begin + Step), End, Step, Idxs..., Begin>::type;
	};

	template <size_t Terminal, int Step, size_t... Idxs>
	struct idxs_range_t_<Terminal, Terminal, Step, Idxs...>
	{
		using type = idxs_t<Idxs...>;
	};
}

namespace atma
{
	// forward to std::make_index_sequence for compilation performance
	template <size_t Count>
	using idxs_list_t = std::make_index_sequence<Count>;

	template <size_t Begin, size_t End, int Step = 1>
	using idxs_range_t = typename detail::idxs_range_t_<Begin, End, Step>::type;
}


namespace atma
{
	//
	//  transfer_const_t
	//  ------------------
	//    takes T, and if const, transforms M to be const, otherwise just M
	//
	template <typename T, typename M>
	using transfer_const_t = std::conditional_t<std::is_const_v<T>, M const, M>;

	//
	//  has_functor_operator
	//  ----------------------
	//    boolean. true if type T has an `operator ()`
	//
	template <typename T>
	struct has_functor_operator
		: std::false_type
	{};

	template <typename T>
	requires requires { { &T::operator () }; }
	struct has_functor_operator<T>
		: std::true_type
	{};

	template <typename T>
	constexpr bool has_functor_operator_v = has_functor_operator<T>::value;

	//
	//  is_function_pointer
	//  ---------------------
	//    srsly, c++ std.
	//
	template <typename T>
	struct is_function_pointer_t
		: std::conditional_t<
		std::is_pointer_v<T>,
		std::is_function<std::remove_pointer_t<T>>,
		std::false_type>
	{
	};

	template <typename T>
	constexpr inline bool is_function_pointer_v = is_function_pointer_t<T>::value;

	//
	//  is_function_reference
	//
	template <typename T>
	struct is_function_reference_t
		: std::conditional_t<
		std::is_reference_v<T>,
		std::is_function<std::remove_reference_t<T>>,
		std::false_type>
	{
	};

	template <typename T>
	constexpr inline bool is_function_reference_v = is_function_reference_t<T>::value;

	//
	//  is_callable_v
	//
	template <typename T>
	constexpr bool is_callable_v =
		is_function_pointer_v<T> ||
		is_function_reference_v<T> ||
		std::is_member_function_pointer_v<T> ||
		has_functor_operator_v<T>;

	//
	//  storage_type_t
	//
	template <typename T>
	using storage_type_t = std::conditional_t<std::is_rvalue_reference_v<T>, std::remove_reference_t<T>, T>;

	//
	//  rm_ref_t
	//
	template <typename T>
	using rm_ref_t = std::remove_reference_t<T>;

	//
	//  rm_cvref_t
	//
	template <typename T>
		using rm_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

	//
	//  iter_reference_t
	//
	template <typename T>
	using iter_reference_t = decltype(*std::declval<T&>());
}


//
//  is_implicitly_default_constructible
//
namespace atma::detail
{
	template <class T, class = void>
	struct is_implicitly_default_constructible_impl
		: std::false_type {
	}; // determine whether T can be copy-initialized with {}

	template <class T>
	void is_implicitly_default_constructible_impl_fn(const T&);

	template <class T>
	struct is_implicitly_default_constructible_impl < T, std::void_t<decltype(is_implicitly_default_constructible_impl_fn<T>({})) >>
		: std::true_type {};
}

namespace atma
{
	template <typename T>
	constexpr bool is_implicitly_default_constructible_v = detail::is_implicitly_default_constructible_impl<T>::value;
}

///
/// actually_false_v
/// ------------------
///
namespace atma
{
	template <typename T>
	inline constexpr bool actually_false_v = false;
}

//
//  visit_with
//  -----------------
//    it's std::overload but with a better name
//
namespace atma
{
	template <typename... Fs>
	struct visit_with : Fs...
	{
		using Fs::operator()...;
	};

	template <typename... Ts>
	visit_with(Ts&&...) -> visit_with<Ts...>;
}


//
//  value_type_of
//  ---------------
//    deducing value-type from R
//    
//     - tries remove-ref(*begin(R))
//     - tries R::value_type
//     - fails
//
namespace atma
{
	namespace detail
	{
		// value_type_of_ii
		template <typename R, typename = std::void_t<>>
		struct value_type_of_ii
		{
			static_assert(actually_false_v<R>, "couldn't identify value_type for a supposed range concept");
		};

		// anything with a ::value_type subtype is that
		template <typename R>
		struct value_type_of_ii<R, std::void_t<typename std::remove_reference_t<R>::value_type>>
		{
			using type = typename std::remove_reference_t<R>::value_type;
		};

		// static arrays are obvious
		template <typename R, size_t N>
		struct value_type_of_ii<R[N], std::void_t<>>
		{
			using type = R;
		};

		template <typename R, size_t N>
		struct value_type_of_ii<R(&)[N], std::void_t<>>
		{
			using type = R;
		};

		// pointers are their pointed-to-type
		template <typename R>
		struct value_type_of_ii<R*, std::void_t<>>
		{
			using type = R;
		};

		template <typename R>
		struct value_type_of_ii<R* const, std::void_t<>>
		{
			using type = R;
		};
	}

	template <typename R, typename = std::void_t<>>
		struct value_type_of
	{
		using type = typename detail::value_type_of_ii<R>::type;
	};

	template <typename R>
	struct value_type_of<R, std::void_t<decltype(*begin(std::declval<R&>()))>>
	{
		using type = std::remove_reference_t<decltype(*begin(std::declval<R&>()))>;
	};

	template <typename R>
		using value_type_of_t = typename value_type_of<R>::type;
}



//
//  allocator_type_of
//  -------------------
//    returns a "Range"'s allocator, or std::allocator<value-type> if possible
//
namespace atma
{
	template <typename R, typename = std::void_t<>>
	struct allocator_type_of
	{
		using type = std::allocator<rm_cvref_t<value_type_of_t<R>>>;
	};

	template <typename R>
	struct allocator_type_of<R, std::void_t<typename R::allocator_type>>
	{
		using type = typename R::allocator_type;
	};

	template <typename R>
	using allocator_type_of_t = typename allocator_type_of<std::remove_reference_t<R>>::type;
}


namespace atma::detail
{
	// deduce functor
	template <typename T>
	struct _function_traits_
		: _function_traits_<decltype(&T::operator())>
	{
	};

	// deduce function-pointer
	template <typename R, typename... Args>
	struct _function_traits_<R(*)(Args...)>
		: _function_traits_<R(Args...)>
	{
	};

	template <typename R, typename... Args>
	struct _function_traits_<R(*)(Args...) noexcept>
		: _function_traits_<R(Args...)>
	{
	};

	// deduce member-function-pointer
	template <typename C, typename R, typename... Args>
	struct _function_traits_<R(C::*)(Args...)>
		: _function_traits_<R(Args...)>
	{
	};

	template <typename C, typename R, typename... Args>
	struct _function_traits_<R(C::*)(Args...) noexcept>
		: _function_traits_<R(Args...)>
	{
	};

	template <typename C, typename R, typename... Args>
	struct _function_traits_<R(C::*)(Args...) const>
		: _function_traits_<R(Args...)>
	{
	};

	template <typename C, typename R, typename... Args>
	struct _function_traits_<R(C::*)(Args...) const noexcept>
		: _function_traits_<R(Args...)>
	{
	};

	// remove reference
	template <typename T>
	struct _function_traits_<T&>
		: _function_traits_<T>
	{
	};

	template <typename T>
	struct _function_traits_<T&&>
		: _function_traits_<T>
	{
	};

	// remove const/volatile
	template <typename T>
	struct _function_traits_<T const>
		: _function_traits_<T>
	{
	};

	template <typename T>
	struct _function_traits_<T volatile>
		: _function_traits_<T>
	{
	};

	// function-type
	template <typename R, typename... Args>
	struct _function_traits_<R(Args...)>
	{
		using result_type = R;
		using tupled_args_type = std::tuple<Args...>;

		template <size_t i>
		using arg_type = std::tuple_element_t<i, std::tuple<Args...>>;

		constexpr static size_t const arity = sizeof...(Args);
	};
}

namespace atma
{
	//
	//  function_traits_override
	//  ---------------------------
	//
	template <typename T>
	struct function_traits_override
		: detail::_function_traits_<T>
	{
	};

	//
	//  function_traits
	//  -----------------
	//
	template <typename F>
	using function_traits = function_traits_override<F>;
}


//
// is_instance_of
// ----------------
//   evaluates to true if the type T is an instantiation of
//   the templated-type Y
//
namespace atma
{
	template <typename T, template <typename...> typename Y>
	struct is_instance_of
		: std::false_type
	{
	};

	template <template <typename...> typename Y, typename... Args>
	struct is_instance_of<Y<Args...>, Y>
		: std::true_type
	{
	};

	template <typename F, template <typename... Args> typename Y>
	inline constexpr bool is_instance_of_v = is_instance_of<F, Y>::value;
}

//
// detection metaprogramming
// ---------------------------
// can be used to query if a type possesses a _thing_
//
// I should really write more words here. you can see 
// std::experimental::is_detected for more
//
namespace atma
{
	struct nonesuch
	{
		nonesuch() = delete;
		~nonesuch() = delete;
		nonesuch(nonesuch const&) = delete;
		void operator=(nonesuch const&) = delete;
	};
}

namespace atma::detail
{
	template <typename Default, typename, template <typename...> typename Op, typename... Args>
	struct detector
	{
		using value_t = std::false_type;
		using type = Default;
	};

	template <typename Default, template <typename...> typename Op, typename... Args>
	struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
	{
		using value_t = std::true_type;
		using type = Op<Args...>;
	};
}

namespace atma
{
	// is_detected / detected_or
	template <template <typename...> typename Op, typename... Args>
	using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

	template <typename Default, template <typename...> typename Op, typename... Args>
	using detected_or = detail::detector<Default, void, Op, Args...>;

	// detected_t / detected_or_t
	template <template <typename...> typename Op, typename... Args>
	using detected_t = typename detected_or<nonesuch, Op, Args...>::type;

	template <typename Default, template <typename...> typename Op, typename... Args>
	using detected_or_t = typename detected_or<Default, Op, Args...>::type;

	// is_detected_exact / is_detected_convertible
	template <typename Expected, template <typename...> typename Op, typename... Args>
	using is_detected_exact = std::is_same<detected_t<Op, Args...>, Expected>;

	template <typename To, template <typename...> typename Op, typename... Args>
	using is_detected_convertible = std::is_convertible<detected_t<Op, Args...>, To>;

	// is_detected_v / is_detected_exact_v / is_detected_convertible_v
	template <template <typename...> typename Op, typename... Args>
	constexpr inline bool is_detected_v = is_detected<Op, Args...>::value;

	template <typename Expected, template <typename...> typename Op, typename... Args>
	constexpr inline bool is_detected_exact_v = is_detected_exact<Expected, Op, Args...>::value;

	template <typename To, template <typename...> typename Op, typename... Args>
	constexpr inline bool is_detected_convertible_v = is_detected_convertible<To, Op, Args...>::value;
}


///
/// what_is
/// ----------
/// prints out the type in the debugger
/// 
/// use:
/// 
///    what_is<some_type>; // don't actually make a variable
/// 
namespace atma
{
	template <typename...>
	struct what_is;
}
