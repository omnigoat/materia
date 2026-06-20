module;

#include <type_traits>
#include <concepts>
#include <memory>

#include <atma/types.hpp>

export module atma:bridge;

import :intrusive_ptr;



namespace atma::detail
{
	template <typename T>
	struct is_some_sort_of_ptr
		: std::false_type
	{};

	template <typename T>
	struct is_some_sort_of_ptr<atma::intrusive_ptr<T>>
		: std::true_type
	{};

	template <typename T>
	struct is_some_sort_of_ptr<std::unique_ptr<T>>
		: std::true_type
	{};

	template <typename T>
	constexpr bool is_some_sort_of_ptr_v = false;

	template <typename T>
	constexpr bool is_some_sort_of_ptr_v<std::unique_ptr<T>> = true;


	template <typename HostPtr, typename Dynast>
	using dynast_type_of_host_ptr = atma::transfer_const_t<
		std::remove_reference_t<
			decltype(*std::declval<HostPtr>())>, Dynast>;

	template <typename T>
	requires requires { { T::static_host_size() }; }
	constexpr size_t get_static_size(T const&)
	{
		return T::static_host_size();
	}

	template <typename T>
	requires requires { { static_host_size(std::declval<T>()) }; }
	constexpr size_t get_static_size(T const& x)
	{
		return static_host_size(x);
	}

	template <typename T>
	concept has_static_size = requires(T const& x)
	{
		{ detail::get_static_size(x) };
	};

	template <typename T>
	concept pointer_ish = (std::is_pointer_v<T> || requires(T x)
	{
		{ *x };
		{ x.operator ->() };
		{ x.get() };
	});
}

export namespace atma
{
	template <typename Host, typename Dynast>
	struct bridge_t
		: Host
	{
		bridge_t(Host&& i, Dynast&& c)
			: Host{std::move(i)}
			, dynast_{std::move(c)}
		{}

		template <typename... Args>
		requires std::constructible_from<Host, Args...>
			&& std::constructible_from<Dynast, Args...>
		bridge_t(Args&&... args)
			: Host{std::forward<Args>(args)...}
			, dynast_{std::forward<Args>(args)...}
		{}

	protected:
		Dynast dynast_;
	};

	template <typename pointer_type, typename Host, typename Dynast>
	auto make_bridge(Host&& host, Dynast&& dynast) -> pointer_type
	{
		return pointer_type{new bridge_t{
			std::forward<Host>(host),
			std::forward<Dynast>(dynast)}};
	}

	template <typename pointer_type, typename Host, typename Dynast, typename... Args>
	auto make_bridge_ptr(Args&&... args) -> pointer_type
	{
		return pointer_type{new bridge_t<Host, Dynast>{std::forward<Args>(args)...}};
	}
}

export namespace atma
{
	template <typename T>
	size_t bridge_sizeof_host(T const& x)
	{
		return x.host_size();
	}

	template <typename T>
	requires detail::has_static_size<T>
	size_t bridge_sizeof_host(T const& x)
	{
		return detail::get_static_size(x);
	}

	template <typename T>
	requires std::is_pointer_v<T>
	size_t bridge_sizeof_host(T const& x)
	{
		return bridge_sizeof_host(*x);
	}
}

export namespace atma
{
	template <typename HostPtr, typename Dynast>
	struct bridge_dynast_ptr
	{
		using dynast_type = detail::dynast_type_of_host_ptr<HostPtr, Dynast>;

		bridge_dynast_ptr() = default;
		bridge_dynast_ptr(bridge_dynast_ptr const&) = default;
		bridge_dynast_ptr(bridge_dynast_ptr&&) = default;

		explicit bridge_dynast_ptr(HostPtr const& host)
			: host_{host}
		{}

		bridge_dynast_ptr& operator = (bridge_dynast_ptr const&) = default;
		bridge_dynast_ptr& operator = (bridge_dynast_ptr&&) = default;

		operator bool () const { return host_; }
		auto operator ! () const -> bool { return !host_; }

		auto operator -> () const -> dynast_type*
		{
			return host_
				? std::start_lifetime_as<dynast_type>(reinterpret_cast<std::byte const*>(host_.get()) + bridge_sizeof_host(host_))
				: nullptr;
		}

		auto operator * () const -> dynast_type&
		{
			return *operator->();
		}

	private:
		HostPtr host_;
	};
}

export namespace atma
{
	template <typename Dynast, typename HostPtr>
	auto bridge_cross(HostPtr const& host) -> bridge_dynast_ptr<HostPtr, Dynast>
	{
		return bridge_dynast_ptr<HostPtr, Dynast>{host};
	}

	template <typename Dynast, typename HostPtr>
	auto bridge_cross_unsafe(HostPtr const& host) -> detail::dynast_type_of_host_ptr<HostPtr, Dynast>*
	{
		using dynast_type = detail::dynast_type_of_host_ptr<HostPtr, Dynast>;

		return host
			? std::start_lifetime_as<dynast_type>(reinterpret_cast<std::byte const*>(host.get()) + bridge_sizeof_host(host))
			: nullptr;
	}

	template <typename Dynast, typename Host>
	auto bridge_cross_unsafe(Host* host) -> detail::dynast_type_of_host_ptr<Host*, Dynast>*
	{
		using dynast_type = detail::dynast_type_of_host_ptr<Host*, Dynast>;
		using byte_type = atma::transfer_const_t<Host, std::byte>;

		return host
			? std::start_lifetime_as<dynast_type>(reinterpret_cast<byte_type*>(host) + bridge_sizeof_host(host))
			: nullptr;
	}
}
