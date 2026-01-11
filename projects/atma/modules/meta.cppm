
// yes, much of this file is inspired by kvasir.mpl

module;

#include <utility>
#include <type_traits>

#include <atma/unit_test.hpp>

export module atma.meta;


template <typename...>
struct no;


///
/// identity
/// 
export namespace atma::meta::lazy
{
	struct identity
	{
		template <typename T>
		using f = T;
	};
}

export namespace atma::meta
{
	template <typename T>
	using identity = T;
}


///
/// constant
/// 
export namespace atma::meta::lazy
{
	template <typename C>
	struct constant
	{
		template <typename...>
		using f = C;
	};
}

#if 0
// typeval
namespace atma::meta
{
	template <typename T>
	constexpr auto tv_ = T::type::value;
}
#endif


///
/// dependent call
/// ----------------
/// 
namespace atma::meta::lazy::detail
{
	// dependent-call
	template <typename, bool>
	struct dcc_impl
	{};

	template <typename F>
	struct dcc_impl<F, true> : F
	{};


	// dependent-call-function
	template <bool>
	struct dccf_impl
	{
		template <template <typename...> class F, typename... Args>
		using f = F<Args...>;
	};

	template <>
	struct dccf_impl<false>
	{
		template <template <typename...> class F, typename... Args>
		using f = F<>;
	};

}

export namespace atma::meta::lazy
{
	// dependent-call
	template <typename F, size_t Sz>
	using dcc = typename detail::dcc_impl<F, Sz < 1'000'000>;

	// dependent-call-function-template
	template <template <typename...> typename F, typename... Args>
	using dccf = typename detail::dccf_impl<(sizeof...(Args) > 0)>::template f<F, Args...>;
}


///
/// call-continuation
/// -------------------
/// calls a generic metafunction with variable arguments, even
/// if that metafunction doesn't take variable arguments (as long
/// as the number of supplied arguments is correct). even if the
/// number of arguments is correct, compilers throw errors about it
/// without some serious trickery.
///
export namespace atma::meta::lazy
{
	template <typename F, typename... Args>
	using cc = typename dcc<F, sizeof...(Args)>::template f<Args...>;

	template <typename F, typename... Args>
	using ccf = typename dcc<F, sizeof...(Args)>::template cf<Args...>;
}


///
/// continuation-from-eager
/// -------------------------
///
/// Since all continuations/metafunctions in our metaprogram have
/// their main operands sent through member-alias 'f', then any
/// (usually type-alias) metafunction that processes all its
/// template arguments as class-arguments must be converted.
/// 
///    template <typename A, typename B>
///    using less_than = bool_<A::value < B::value>;
/// 
///    using lazy_less_than = cfe<less_than, Continuation>;
/// 
///    using lazy_make_signed = cfe<std::make_signed_t, Continuation>;
///
export namespace atma::meta::lazy
{
	template <template <typename...> class F, typename C = identity>
	struct cfe
	{
		template <typename... Args>
		using f = typename C::template f<dccf<F, Args...>>;

		template <typename CC, typename... Args>
		using cf = typename CC::template f<dccf<F, Args...>>;
	};

	template <template <typename...> class F>
	struct cfe<F, identity>
	{
		template <typename... Args>
		using f = dccf<F, Args...>;

		template <typename CC, typename... Args>
		using cf = typename CC::template f<dccf<F, Args...>>;
	};
}


///
/// continuation-from-eager-type
/// ------------------------------
///
/// Many standard library and third-party libraries define traits
/// or metafunctions via a member-type named 'type'. This adapts
/// this pattern into a mpl metafunction.
/// 
///   template <typename A, typename B>
///   struct less_than
///   {
///      using type = bool_<A() < B()>;
///   };
/// 
///   using lazy_less_than = cfet<less_than>;
/// 
///   # equates to bool_<true>
///   invoke<lazy_less_than, int_<3>, int_<4>>
///
export namespace atma::meta::lazy
{
	template <template <typename...> typename F, typename C = identity>
	struct cfet
	{
		template <typename... args>
		using f = typename C::template f<typename dccf<F, args...>::type>;

		template <typename CC, typename... args>
		using cf = typename CC::template f<typename dccf<F, args...>::type>;
	};

	template <template <typename...> typename F>
	struct cfet<F, identity>
	{
		template <typename... args>
		using f = typename dccf<F, args...>::type;

		template <typename CC, typename... args>
		using cf = typename CC::template f<typename dccf<F, args...>::type>;
	};
}


///
/// invoke
/// 
export namespace atma::meta
{
	template <typename F, typename... Args>
	using invoke = typename F::template f<Args...>;
}



///
/// basic types
/// 
export namespace atma::meta
{
	struct nil {};

	template <typename...>
	using void_ = void;

	template <auto x>
	using integral_constant_of = std::integral_constant<decltype(x), x>;

	template <bool         x> using bool_    = integral_constant_of<x>;
	template <char         x> using char_    = integral_constant_of<x>;
	template <int          x> using int_     = integral_constant_of<x>;
	template <unsigned int x> using uint_    = integral_constant_of<x>;
	template <uint32_t     x> using uint32_  = integral_constant_of<x>;
	template <uint64_t     x> using uint64_  = integral_constant_of<x>;
	template <size_t       x> using usize_   = integral_constant_of<x>;

	using false_ = bool_<false>;
	using true_ = bool_<true>;
}





///
/// if
/// -------
/// 
namespace atma::meta::lazy::detail
{
	template <bool>
	struct _if_
	{
		template <typename tb, typename fb>
		using f = fb;
	};

	template <>
	struct _if_<true>
	{
		template <typename tb, typename fb>
		using f = tb;
	};
}

export namespace atma::meta::lazy
{
	template <typename predicate, typename tb, typename fb>
	struct if_
	{
		template <typename... args>
		using impl = typename detail::_if_<cc<predicate, args...>::value>::template f<tb, fb>;

		template <typename... args>
		using f = cc<impl<args...>, args...>;

		template <typename CC, typename... args>
		using cf = ccf<impl<args...>, CC, args...>;
	};
}

export namespace atma::meta
{
	template <bool predicate, typename tb, typename fb>
	using if_ = typename lazy::detail::_if_<predicate>::template f<tb, fb>;
}


///
/// if
/// 
namespace atma::meta::lazy::detail
{
	template <bool>
	struct _ift_
	{
		template <template <typename...> typename tb, template <typename...> typename fb>
		template <typename... args>
		using f = fb<args...>;
	};

	template <>
	struct _ift_<true>
	{
		template <template <typename...> typename tb, template <typename...> typename fb>
		template <typename... args>
		using f = tb<args...>;
	};
}

export namespace atma::meta::lazy
{
	template <typename predicate, template <typename...> typename tb, template <typename...> typename fb>
	struct ift_
	{
		template <typename... args>
		using f = dccf<typename detail::_ift_<predicate::value>
			::template f<tb, fb>,
			args...>;
	};
}








///
/// nullptr_v
///
#if 0
namespace atma::meta
{
	export template <typename T>
	inline constexpr T* nullptr_v = nullptr;
}
#endif


///
/// integral operations
/// 
export namespace atma::meta
{
	template <typename x> using inc = integral_constant_of<x::value + 1>;
	template <typename x> using dec = integral_constant_of<x::value - 1>;

	template <typename x, typename y> using mul = integral_constant_of<x() * y()>;
	template <typename x, typename y> using div = integral_constant_of<x() / y()>;
	template <typename x, typename y> using add = integral_constant_of<x() + y()>;
	template <typename x, typename y> using sub = integral_constant_of<x() - y()>;
}

export namespace atma::meta::lazy
{
	using mul = cfe<meta::mul>;
	using div = cfe<meta::div>;
	using add = cfe<meta::add>;
	using sub = cfe<meta::sub>;
}


///
/// any_t
/// 
export namespace atma::meta
{
	template <typename T = void>
	struct any_t
	{
		constexpr any_t() = default;

		constexpr any_t(T&&)
		{}
	};

	template <>
	struct any_t<void>
	{
		constexpr any_t() = default;

		template <typename T>
		constexpr any_t(T&&)
		{}
	};
}




///
/// list
///
export namespace atma::meta
{
	template <typename... es>
	struct list
	{
		using type = list;
		static inline constexpr size_t size = sizeof...(es);

		template <typename x>
		using push_front = list<x, es...>;

		template <typename x>
		using push_back = list<es..., x>;
	};
}


///
/// listify
///
export namespace atma::meta::lazy
{
	using listify = cfe<list>;
}

export namespace atma::meta
{
	template <typename... es>
	using listify = list<es...>;
}


///
/// unpack
/// ---------
/// 
/// takes a list as a singular argument, and calls the
/// continuation with every list element as an argument
/// 
///   template <typename... numbers>
///   using sum = integral_constant_of<(numbers() + ...)>;
/// 
///   using list_of_numbers = list<int_<1>, int_<2>, int_<3>, int_<4>>;
/// 
///   # equates to int_<10>
///   invoke<unpack<cfe<sum>, list_of_numbers>
///
namespace atma::meta::lazy::detail
{
	template <bool front, typename, typename, typename...>
	struct unpack_impl;

	template <typename C, typename... Es, typename... Args>
	struct unpack_impl<true, C, list<Es...>, Args...>
	{
		using type = cc<C, Es..., Args...>;
	};

	template <typename C, typename... Es, typename... Args>
	struct unpack_impl<false, C, list<Es...>, Args...>
	{
		using type = cc<C, Args..., Es...>;
	};
}

export namespace atma::meta::lazy
{
	template <typename C = listify>
	struct unpack_front
	{
		template <typename List, typename... Args>
		using f = typename detail::unpack_impl<true, C, List, Args...>::type;
	};

	template <typename C = listify>
	struct unpack_back
	{
		template <typename List, typename... Args>
		using f = typename detail::unpack_impl<false, C, List, Args...>::type;
	};

	// order doesn't matter, pick one
	template <typename C = listify>
	using unpack = unpack_front<C>;
}

namespace test_unpack
{
	using namespace atma::meta;

	template <typename... numbers> using sum = integral_constant_of<(... + numbers())>;
	template <typename... numbers> using sub = integral_constant_of<(... - numbers())>;

	using list_of_numbers = list<int_<1>, int_<2>, int_<3>, int_<4>>;
	
	// unpack (unpack_front)
	static_assert(std::is_same_v<
		int_<10>,
		invoke<lazy::unpack<lazy::cfe<sum>>, list_of_numbers>>);

	// unpack_front
	static_assert(std::is_same_v<
		int_<-18>,
		invoke<lazy::unpack_front<lazy::cfe<sub>>, list_of_numbers, int_<10>>>);

	// unpack_back
	static_assert(std::is_same_v<
		int_<0>,
		invoke<lazy::unpack_back<lazy::cfe<sub>>, list_of_numbers, int_<10>>>);
}


//
// at
// ----
// 
// gets the argument at index N
//
namespace atma::meta::lazy::detail
{
	template <size_t N, typename C>
	struct _at_
	{
		template <typename Head, typename... Tail>
		using f = cc<_at_<N - 1, C>, Tail...>;
	};

	template <typename C>
	struct _at_<0, C>
	{
		template <typename E0, typename...>
		using f = cc<C, E0>;
	};

	template <typename C>
	struct _at_<1, C>
	{
		template <typename E0, typename E1, typename... Elements>
		using f = cc<C, E1>;
	};

	template <typename C>
	struct _at_<2, C>
	{
		template <typename E0, typename E1, typename E2, typename...>
		using f = cc<C, E2>;
	};

	template <typename C>
	struct _at_<3, C>
	{
		template <typename E0, typename E1, typename E2, typename E3, typename...>
		using f = cc<C, E3>;
	};
}

export namespace atma::meta::lazy
{
	template <typename N, typename C = identity>
	struct at
	{
		template <typename... Elements>
		using f = cc<detail::_at_<N::value, C>, Elements...>;
	};
}

export namespace atma::meta
{
	template <size_t N, typename List>
	using at = lazy::cc<lazy::unpack<lazy::at<usize_<N>>>, List>;
}

///
/// at2
/// ----
/// 
/// gets the argument at index N, which is provided during invocation
///
export namespace atma::meta::lazy
{
	template <typename C = identity>
	struct at2
	{
		template <typename N, typename... Elements>
		using f = cc<detail::_at_<N::value, C>, Elements...>;
	};
}

///
/// list_push_back
/// 
export namespace atma::meta::lazy
{
	template <typename C = listify>
	struct list_push_back
		: unpack_back<C>
	{};
}

export namespace atma::meta
{
	template <typename list, typename x>
	using list_push_back = invoke<lazy::unpack_front<>, list, x>;
}




///
/// skip
/// -------
/// given elements, returns elements without the first N
namespace atma::meta::lazy::detail
{
	constexpr size_t _skip_step_(size_t n, size_t)
	{
		return // n > 256 ? 256 : n > 64 ? 64 : 
			n > 16 ? 16 : n > 8 ? 8 : n > 4 ? 4 : n;
	}

	template <size_t N, typename C>
	struct _skip_;

	template <typename C>
	struct _skip_<0, C>
	{
		template <size_t, typename... es>
		using f = cc<C, es...>;
	};

	template <typename C>
	struct _skip_<1, C>
	{
		template <size_t, typename e0, typename... es>
		using f = cc<C, es...>;
	};

	template <typename C>
	struct _skip_<2, C>
	{
		template <size_t, typename e0, typename e1, typename... es>
		using f = cc<C, es...>;
	};

	template <typename C>
	struct _skip_<3, C>
	{
		template <size_t, typename e0, typename e1, typename e2, typename... es>
		using f = cc<C, es...>;
	};

	template <typename C>
	struct _skip_<4, C>
	{
		template <size_t n, typename e0, typename e1, typename e2, typename e3, typename... es>
		using f = typename _skip_<n - 4, C>::template f<(n - 4), es...>;
	};

	template <typename C>
	struct _skip_<8, C>
	{
		template <size_t n,
			typename e0, typename e1, typename e2, typename e3,
			typename e4, typename e5, typename e6, typename e7,
			typename... es>
		using f = typename _skip_<_skip_step_(n - 8), C>::template f<(n - 8), es...>;
	};

	template <typename C>
	struct _skip_<16, C>
	{
		template <size_t n,
			typename e0, typename e1, typename e2, typename e3,
			typename e4, typename e5, typename e6, typename e7,
			typename e8, typename e9, typename e10, typename e11,
			typename e12, typename e13, typename e14, typename e15,
			typename... es>
		using f = typename _skip_<_skip_step_(n - 16), C>::template f<(n - 16), es...>;
	};
}

export namespace atma::meta::lazy
{
	template <typename n, typename C = identity>
	struct skip
	{
		template <typename... es>
		using f = typename detail::_skip_<detail::_skip_step_(n::value, sizeof...(es)), C>::template f<n::value, es...>;
	};
}

export namespace atma::meta
{
	template <size_t n, typename list>
	using skip = invoke<lazy::unpack<lazy::skip<usize_<n>>>, list>;
}


export namespace atma::meta
{
	template <typename A, typename B>
	struct is_same : false_
	{};

	template <typename T>
	struct is_same<T, T> : true_
	{};
}


// list_size
export namespace atma::meta::lazy
{
	template <typename C = identity>
	struct list_size
	{
		template <typename... es>
		using f = usize_<sizeof...(es)>;
	};
}

export namespace atma::meta
{
	template <typename List>
	inline constexpr size_t list_size_v = lazy::cc<lazy::unpack<lazy::list_size<>>, List>::value;
}

//
// list_element
//
export namespace atma::meta::lazy
{
	template <size_t Idx, typename C = identity>
	struct list_element
	{
		template <typename Head, typename... Rest>
		using f = cc<list_element<Idx - 1, C>, Rest...>;
	};

	template <typename C>
	struct list_element<0, C>
	{
		template <typename Head, typename... Rest>
		using f = cc<C, Head>;
	};
}

export namespace atma::meta
{
	template <size_t Idx, typename List>
	using list_element_t = lazy::cc<lazy::unpack<lazy::list_element<Idx>>, List>;
}

//
// fold-left
// -----------
// 
//
namespace atma::meta::lazy::detail
{
	template <bool>
	struct _foldl_
	{
		template <typename F, typename Acc, typename Arg0, typename... Args>
		using f = typename _foldl_<(sizeof...(Args) > 0)>::template f<F, typename F::template f<Acc, Arg0>, Args...>;
	};

	template <>
	struct _foldl_<false>
	{
		template <typename, typename Acc>
		using f = Acc;
	};
}

export namespace atma::meta::lazy
{
	template <typename F, typename C = identity>
	struct foldl
	{
		template <typename Init, typename... Args>
		using f = typename C::template f<typename detail::_foldl_<(sizeof...(Args) > 0)>::template f<F, Init, Args...>>;
	};
}

namespace atma::meta::detail
{
	template <size_t>
	struct _foldl_selector_;

	template <>
	struct _foldl_selector_<1>
	{ // no initial value
		template <typename F, typename List>
		using f = typename lazy::unpack<lazy::foldl<F>>::template f<List>;
	};

	template <>
	struct _foldl_selector_<2>
	{ // initial value (list expanded at back)
		template <typename F, typename Init, typename List>
		using f = typename lazy::unpack_back<lazy::foldl<F>>::template f<List, Init>;
	};
}

export namespace atma::meta
{
	template <template <typename...> typename F, typename... InitAndOrList>
	using foldl = typename detail::_foldl_selector_<sizeof...(InitAndOrList)>::template f<lazy::cfe<F>, InitAndOrList...>;

	template <template <typename...> typename F, typename Init, typename... Es>
	using foldl_pack = lazy::cc<lazy::foldl<lazy::cfe<F>>, Init, Es...>;
}


//
// fold-right
// -----------
// 
//
namespace atma::meta::lazy::detail
{
	template <size_t Argc>
	struct _foldr_
	{
		template <typename F, typename Arg0, typename... Args>
		using f = typename F::template f<Arg0, typename _foldr_<sizeof...(Args)>::template f<F, Args...>>;
	};

	template <>
	struct _foldr_<2>
	{
		template <typename F, typename Arg0, typename Arg1>
		using f = typename F::template f<Arg0, Arg1>;
	};

	template <>
	struct _foldr_<1>
	{
		template <typename F, typename Arg0>
		using f = Arg0;
	};
}

export namespace atma::meta::lazy
{
	template <typename F, typename C = identity>
	struct foldr
	{
		template <typename... Args>
		using f = typename C::template f<cc<detail::_foldr_<sizeof...(Args)>, F, Args...>>;

		template <typename CC, typename... Args>
		using cf = typename CC::template f<cc<detail::_foldr_<sizeof...(Args)>, F, Args...>>;
	};
}

namespace atma::meta::detail
{
	template <size_t>
	struct _foldr_selector_;

	template <>
	struct _foldr_selector_<1>
	{ // no initial value
		template <typename F, typename List>
		using f = typename lazy::unpack<lazy::foldr<F>>::template f<List>;
	};

	template <>
	struct _foldr_selector_<2>
	{ // initial value (list expanded at front)
		template <typename F, typename Init, typename List>
		using f = lazy::unpack_front<lazy::foldr<F>>::template f<List, Init>;
	};
}

export namespace atma::meta
{
	template <template <typename...> typename F, typename... InitAndOrList>
	using foldr = typename detail::_foldr_selector_<sizeof...(InitAndOrList)>::template f<lazy::cfe<F>, InitAndOrList...>;

	template <template <typename...> typename F, typename... Es>
	using foldr_pack = lazy::cc<lazy::foldr<lazy::cfe<F>>, Es...>;
}



// logical operators
export namespace atma::meta::lazy
{
	template <typename C = identity>
	struct not_
	{
		template <typename x>
		using f = typename C::template f<bool_<!x::value>>;
	};

	template <typename C = identity>
	struct and_
	{
		template <typename A, typename B>
		using f = typename C::template f<bool_<A::value && B::value>>;
	};

	template <typename C = identity>
	struct or_
	{
		template <typename A, typename B>
		using f = typename C::template f<bool_<A::value || B::value>>;
	};
}

export namespace atma::meta
{
	template <typename A, typename B>
	using and_ = bool_<A::value && B::value>;

	template <typename A, typename B>
	using or_ = bool_<A::value || B::value>;

	template <typename x>
	using not_ = bool_<!x::value>;
}




///
/// list-join
/// ------------
/// 
namespace atma::meta::lazy::detail
{
	template <typename lhs, typename rhs>
	struct _list_join_impl2_;

	//template <typename... lhs, typename rhs>
	//struct _list_join_impl2_<list<lhs...>, rhs>
	//{
	//	using type = list<lhs..., rhs>;
	//};

	template <typename... lhs, typename... rhs>
	struct _list_join_impl2_<list<lhs...>, list<rhs...>>
	{
		using type = list<lhs..., rhs...>;
	};

	struct _list_join_impl_
	{
		template <typename lhs, typename rhs>
		using f = typename _list_join_impl2_<lhs, rhs>::type;
	};
}

export namespace atma::meta::lazy
{
	template <typename C = listify>
	struct list_join
	{
		template <typename... lists>
		using f = invoke<foldl<detail::_list_join_impl_, unpack<C>>,
			list<>,
			lists...>;

		template <typename CC, typename... lists>
		using cf = invoke<foldl<detail::_list_join_impl_, unpack<CC>>,
			list<>,
			lists...>;
	};
}






// all/any
export namespace atma::meta
{
	template <typename List>
	using all = foldl<and_, bool_<true>, List>;

	template <typename List>
	using any = foldl<or_, bool_<false>, List>;
}



// map
export namespace atma::meta::lazy
{
	template <typename F, typename C = listify>
	struct map
	{
		template <typename... Ts>
		using f = typename C::template f<typename F::template f<Ts>...>;

		template <typename CC, typename... Ts>
		using cf = typename CC::template f<typename F::template f<Ts>...>;
	};
}

export namespace atma::meta
{
	template <template <typename> typename F, typename List>
	using map = typename lazy::unpack<lazy::map<lazy::cfe<F>>>::template f<List>;
}

///
/// zip
/// ------
///
export namespace atma::meta::lazy
{
	namespace detail
	{
		template <typename... lhs_args, typename... rhs_args>
		auto zip_result(list<lhs_args...>, list<rhs_args...>) -> list<list<lhs_args, rhs_args>...>;
	}

	template <typename C = identity>
	struct zip
	{
		template <typename listA, typename listB>
		using f = typename C::template f<decltype(detail::zip_result(listA{}, listB{}))>;
	};

	template <>
	struct zip<identity>
	{
		template <typename listA, typename listB>
		using f = decltype(detail::zip_result(listA{}, listB{}));
	};
}

export namespace atma::meta
{
	template <typename ListA, typename ListB>
	using zip = invoke<lazy::zip<>, ListA, ListB>;
}









//
// permutations lazy-stylez
//
namespace atma::meta::lazy
{
	template <typename acc_tt, typename list_of_lists_tt, size_t idx>
	struct thing;

	template <typename acc_tt, size_t idx>
	struct thing<acc_tt, list<>, idx>
	{
		using type = acc_tt;
	};

	template <typename... aes, typename lhead, typename... ltail, size_t idx>
	struct thing<list<aes...>, list<lhead, ltail...>, idx>
	{
		using type = typename thing<
			list<aes..., list_element<(idx / (list_size_v<ltail> * ... * 1)) % list_size_v<lhead>, lhead>>,
			list<ltail...>,
			idx>::type;
	};
}

namespace atma::meta::lazy::detail
{
	// permutation accumulator
	template <size_t idx_vv, typename list_tt = list<>>
	struct perm_acc : integral_constant_of<idx_vv>
	{
		using list_type = list_tt;
	};

	// "permutations_f"
	template <typename C = identity>
	struct perm_f
	{
		template <typename List, typename Perm>
		using f = typename C::template f<perm_acc<
			Perm::value / list_size_v<List>,
			typename Perm::list_type::template push_front<list_element_t<(Perm::value % list_size_v<List>), List>>
		>>;
	};

	// get subtype "list_type"
	template <typename C = identity>
	struct subtype_list_type
	{
		template <typename T>
		using f = typename C::template f<typename T::list_type>;
	};

	template <typename Idxs, typename C>
	struct _select_combinations_;

	template <size_t... idxs, typename C>
	struct _select_combinations_<std::index_sequence<idxs...>, C>
	{
		template <typename... lists_tt>
		using f = typename C::template f<
			cc<lazy::foldr<perm_f<>, subtype_list_type<>>, lists_tt..., perm_acc<idxs>>...
		>;
	};
}

export namespace atma::meta::lazy
{
	template <typename C = listify>
	struct select_combinations
	{
		template <typename... lists_tt>
		using f = typename detail::_select_combinations_<std::make_index_sequence<(list_size_v<lists_tt> * ... * 1)>, C>::template f<lists_tt...>;
	};
}

export namespace atma::meta
{
	template <typename... lists_tt>
	using select_combinations_t = lazy::cc<lazy::select_combinations<>, lists_tt...>;
}

export namespace atma::meta::lazy
{
#if 0
	namespace detail
	{
		
		// permutation accumulator
		template <typename idx_tt, typename list_tt = list<>>
		struct perm_acc
		{
			using idx = idx_tt;
			using list_type = list_tt;
		};

		// "permutations_f"
		template <typename C = identity>
		struct perm_f
		{
			template <typename List, typename Perm>
			using f = typename C::template f<perm_acc<
				div<typename Perm::idx, usize_<list_size_v<List>>>,
				typename Perm::list_type::template push_front<list_element_t<(typename Perm::idx() % list_size_v<List>), List>>
			>>;
		};

		// get subtype "list_type"
		template <typename C = identity>
		struct subtype_list_type
		{
			template <typename T>
			using f = typename C::template f<typename T::list_type>;

			template <typename CC, typename T>
			using cf = typename CC::template f<typename T::list_type>;
		};


		template <typename Idxs, typename C>
		struct _select_combinations_;

		template <size_t... idxs, typename C>
		struct _select_combinations_<std::index_sequence<idxs...>, C>
		{
			template <typename... lists_tt>
			using f = typename C::template f<
				cc<lazy::foldr<perm_f<>, subtype_list_type<>>, lists_tt..., perm_acc<usize_<idxs>>>...
			>;
		};
	}

	template <typename C = identity>
	struct product
	{
		template <typename... numbers>
		using f = typename C::template f<uint_<(numbers::value * ... * 1)>>;

		template <typename CC, typename... numbers>
		using cf = cc<CC, uint_<(numbers::value * ... * 1)>>;
	};

	template <template <typename...> typename F, typename... trailing_args_tt>
	struct bind1_front
	{
		template <typename arg0, typename... args>
		using f = cc<F<arg0, trailing_args_tt...>, args...>;
	};
#endif

	template <typename C, typename step, typename... rest>
	struct _exec_
	{
		template <typename... args>
		using f = ccf<step, _exec_<C, rest...>, args...>;
	};

	template <typename C, typename step>
	struct _exec_<C, step>
	{
		template <typename... args>
		using f = ccf<step, C, args...>;
	};

	template <typename step, typename... rest>
	struct exec
	{
		template <typename... args>
		using f = //ccf<step, exec<rest...>, args...>;
			typename dcc<step, sizeof...(args)>::template cf<exec<rest...>, args...>; //dcc<step, sizeof...(Args)>::template cf<Args...>;

		template <typename CC, typename... args>
		using cf = ccf<step, _exec_<CC, rest...>, args...>;
	};

	template <typename last_step>
	struct exec<last_step>
	{
		template <typename... args>
		using f = cc<last_step, args...>;

		//template <typename CC, typename... args>
		//using cf = ccf<last_step, CC, args...>;
	};

#if 0
#if 1
	template <typename C, typename idxs>
	struct _make_index_sequence_;

	template <typename C, size_t... idxs>
	struct _make_index_sequence_<C, std::index_sequence<idxs...>>
	{
		using type = typename C::template f<usize_<idxs>...>;
	};

	template <typename C = listify>
	struct make_index_sequence
	{
		template <typename size>
		using f = typename _make_index_sequence_<C, std::make_index_sequence<size::value>>::type;

		template <typename CC, typename size>
		using cf = typename _make_index_sequence_<CC, std::make_index_sequence<size::value>>::type;
	};

	template <typename C, typename... ps>
	struct hold_args
	{
		template <typename... args>
		using f = cc<C, args..., ps...>;

		template <typename CC, typename... args>
		using cf = cc<CC, args..., ps...>;
	};

	template <typename lhs, typename rhs>
	struct exec_and_push_back
	{
		template <typename... args>
		using f = ccf<lhs, typename hold_args<rhs, args>::template f<args...>...>;
	};

	struct becomes_int
	{
		template <typename... args>
		using f = int;

		template <typename CC, typename... args>
		using cf = int;
	};

	template <typename C = listify>
	struct select_combinations
	{
		using g = exec_and_push_back<
			exec<
				map<unpack<list_size<>>>,      // map from lists to list size
				product<>,                     // get product (number)
				make_index_sequence<>
				//,         // make an index-list
				//map<cfe<detail::perm_acc>>   // turn each index list into a perm_acc
			>,
			//map<foldr<detail::perm_f<>>>
			listify
		>;

			//,
			//	product<
			//		make_index_sequence<
			//			bind1_front<detail::_select_combinations_, C>
			//		>
			//	>
			//>;

		//template <typename... lists_tt>
		//using f = typename g::template f<lists_tt...>;

		template <typename... lists_tt>
		using f = typename detail::_select_combinations_<std::make_index_sequence<(list_size_v<lists_tt> * ... * 1)>, C>::template f<lists_tt...>;

		
	};
#endif
#if 0
	call<
		exec
		<
			map<unpack<list_size<>>>,
			product<>,
			make_index_sequence<>,
			foldr<perm_f<>>,
			subtype_list_type<>
		>,
		lists_tt..., perm_acc<idxs>
	>

#endif
#endif
}


//
// permutations
//
export namespace atma::meta::old
{
	template <typename L, size_t PermSize>
	struct list_perm
	{
		using list_type = L;
		static inline constexpr size_t perm_size = PermSize;

		template <size_t Idx>
		using list_element_type = list_element_t<(Idx / perm_size) % list_type::size, list_type>;
	};

	template <typename Acc, typename... Lists>
	struct generate_list_perms;

	template <typename Acc>
	struct generate_list_perms<Acc>
	{
		using type = Acc;
	};

	template <typename... Acc, typename List, typename... Rest>
	struct generate_list_perms<list<Acc...>, List, Rest...>
	{
		using element = list_perm<List, (Rest::size * ... * 1)>;
		using type = typename generate_list_perms<list<Acc..., element>, Rest...>::type;
	};

	template <typename ListPerms, typename IndexSequence>
	struct splat_perms {};

	template <size_t Idx, typename... Perms>
	struct perm_get
	{
		using type = list<typename Perms::template list_element_type<Idx>...>;
	};

	template <typename... Perms, size_t... Idxs>
	struct splat_perms<list<Perms...>, std::index_sequence<Idxs...>>
	{
		using type = list<typename perm_get<Idxs, Perms...>::type...>;
	};

	template <typename... Lists>
	struct permutations
	{
		static inline constexpr size_t total_size = (Lists::size * ...);

		using list_perms_type = typename generate_list_perms<list<>, Lists...>::type;

		using type = typename splat_perms<list_perms_type, std::make_index_sequence<total_size>>::type;
	};
}
















//template <typename T>
//struct is_signed : std::bool_constant<std::is_signed_v<T>>
//{};

struct is_signed
{
	template <typename T>
	using f = std::bool_constant<std::is_signed_v<T>>;
};

struct make_unsigned
{
	template <typename T>
	using f = std::make_unsigned_t<T>;
};

//template <typename T>
//using mu = std::make_unsigned_t

template <typename... Ts>
struct blahz
{};


struct blahzify
{
	template <typename... Ts>
	using f = blahz<Ts...>;
};

template <typename C = atma::meta::lazy::listify>
struct size_is_32bit_or_less
{
	template <typename T>
	using f = atma::meta::integral_constant_of<sizeof(T) <= 4>;
};

enum class MathsOperation
{
	Mul, Div, Add, Sub
};

template <template <typename...> typename Op, typename Value>
struct oper : Value
{};

template <typename A, typename B>
struct oper_action;

template <typename A, template <typename...> typename Op, typename Value>
struct oper_action<A, oper<Op, Value>>
{
	using type = Op<A, Value>;
};

template <typename A, template <typename...> typename Op, typename Value>
struct oper_action<oper<Op, Value>, A>
{
	using type = Op<Value, A>;
};

template <typename A, typename B>
using oper_action_t = typename oper_action<A, B>::type;

export void test_mythings()
{
	using namespace atma;
	using namespace atma::meta;

	//no<invoke<lazy::exec<
	//	lazy::map<lazy::cfl<std::is_integral>>
	//>, int, float, short, double>> {};

	static_assert(std::is_same_v<
		//list<bool_<true>, bool_<false>, bool_<true>, bool_<false>>,
		int_<14>,
		invoke<lazy::exec<
			lazy::map<lazy::cfe<inc>>,
			lazy::foldl<lazy::cfe<add>>
			//lazy::map<lazy::cfe<not_>>,
			//lazy::foldl<
		>, int_<1>, int_<2>, int_<3>, int_<4>>
	>);

	//static_assert(std::is_same_v<
	//	list<bool_<true>, bool_<true>, bool_<true>, bool_<false>>,
	//	typename invoke<lazy::map<make_unsigned, size_is_32bit_or_less<>>, int, short, long, long long>
	//>);
	using a_list  = list<int, char, float>;

	
	//
	//
	//
	using make_index1_unsigned_lvalue = 
		lazy::unpack<
		lazy::at<uint_<1>,
		lazy::cfe<std::make_unsigned_t,
		lazy::cfe<std::add_lvalue_reference_t>>>>;

	static_assert(std::is_same_v<
		list<unsigned short&, unsigned int&, unsigned long&>,
		lazy::cc<lazy::map<make_index1_unsigned_lvalue>,
			list<float, short>,
			list<double, int>,
			list<long double, long>
		>
	>);

	static_assert(std::is_same_v<
		list<unsigned int, unsigned short, unsigned long>,
		map<std::make_unsigned_t, list<int, short, long>>
	>);

	//
	//
	//
	static_assert(std::is_same_v<
		uint_<1>,
		foldl<sub, uint_<11>, list<uint_<1>, uint_<2>, uint_<3>, uint_<4>>>
	>);

	static_assert(std::is_same_v<
		uint_<1>,
		typename lazy::foldl<lazy::cfe<sub>>::template f<uint_<11>, uint_<1>, uint_<2>, uint_<3>, uint_<4>>
	>);


	static_assert(std::is_same_v<
		uint_<33>,
		atma::meta::foldr<oper_action_t, uint_<3>, list<oper<sub, uint_<40>>, oper<meta::div, uint_<21>>>>
	>);

	static_assert(std::is_same_v<
		uint_<33>,
		foldr_pack<oper_action_t, oper<sub, uint_<40>>, oper<meta::div, uint_<21>>, uint_<3>>
	>);

	static_assert(std::is_same_v<invoke<lazy::map<make_unsigned>, int, short, long>, list<unsigned int, unsigned short, unsigned long>>);

	//static_assert(lazy::cc<lazy::if_<size_is_32bit_or_less<>,
	//	lazy::identity,
	//	lazy::identity>, int>, int>);

	//if_<false, int, float>;

	static_assert(std::is_same_v<int, if_<true, int, float>>);

	using first_param_is_signed = lazy::at<usize_<0>, lazy::cfe<std::is_signed>>;
	static_assert(std::is_same_v<int, 
		lazy::cc<lazy::if_<first_param_is_signed, lazy::at<usize_<0>>, lazy::at<usize_<1>>>, int, float>>);
	//static_assert(std::is_same_v<if_<bool_<true>, int, float>, int>);


	struct one{}; struct two{}; struct three{};
	struct A{}; struct B{}; struct C{};
	struct cat{}; struct dog{}; struct rabbit{};

	//using g = typename old::permutations<
	//	list<int, float, short, int, float, short, A, B, A, B, A, B, A, B, int, float, short, int, float, short, A, B, A, B, A, B, A, B>,
	//	list<char, double, char, double, meta::list<>, int, int, int, int, int, int, float, short, int, float, short, A, B, A, B, A, B, A, B>,
	//	list<A, B, A, B, A, B, A, B, A, B, int, float, short, int, float, short, A, B, A, B, A, B, A, B>,
	//	list<int, float, short, int, float, short, A, B, A, B, A, B, A, B>
	//>::type;

	using selected_combinations = lazy::cc<lazy::select_combinations<>, list<one, two, three>, list<A, B, C>, list<cat, dog, rabbit>>;
	static_assert(std::is_same_v<selected_combinations,
		list<
			list<one, A, cat>,
			list<one, A, dog>,
			list<one, A, rabbit>,
			list<one, B, cat>,
			list<one, B, dog>,
			list<one, B, rabbit>,
			list<one, C, cat>,
			list<one, C, dog>,
			list<one, C, rabbit>,
			list<two, A, cat>,
			list<two, A, dog>,
			list<two, A, rabbit>,
			list<two, B, cat>,
			list<two, B, dog>,
			list<two, B, rabbit>,
			list<two, C, cat>,
			list<two, C, dog>,
			list<two, C, rabbit>,
			list<three, A, cat>,
			list<three, A, dog>,
			list<three, A, rabbit>,
			list<three, B, cat>,
			list<three, B, dog>,
			list<three, B, rabbit>,
			list<three, C, cat>,
			list<three, C, dog>,
			list<three, C, rabbit>
		>
	>);

	// permutations - works
#if 1
	static_assert(std::is_same_v<typename old::permutations<list<int, float, short>, list<char, double>, list<A, B>>::type,
		list<
			list<int, char, A>,
			list<int, char, B>,
			list<int, double, A>,
			list<int, double, B>,
			list<float, char, A>,
			list<float, char, B>,
			list<float, double, A>,
			list<float, double, B>,
			list<short, char, A>,
			list<short, char, B>,
			list<short, double, A>,
			list<short, double, B>
		>
	>);
#endif
}
