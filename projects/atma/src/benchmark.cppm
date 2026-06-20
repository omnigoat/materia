module;

#include <atma/benchmark.hpp>

#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#  include <xmmintrin.h>
#  undef min
#  undef max

#include <Psapi.h>
#include <profileapi.h>

#include <atma/arena_allocator.hpp>
#include <atma/aligned_allocator.hpp>
#include <atma/ranges/zip.hpp>
#include <atma/assert.hpp>
#include <atma/types.hpp>

#include <iostream>

export module atma:benchmark;

import :meta;


namespace atma::bench
{
	struct param_identity
	{
		template <typename payload, typename... args>
		using f = payload;
	};



	template <typename payload, typename... args>
	using param_templated_eager = meta::if_<(sizeof...(args) == 0),
		payload, meta::invoke<payload, args...>>;

	struct param_templated
	{
		template <typename payload, typename... args>
		using f = typename meta::lazy::if_<meta::lazy::constant<meta::bool_<(sizeof...(args) == 0)>>,
			meta::lazy::constant<payload>,
			meta::lazy::dcc<payload, sizeof...(args)>>
				::template f<args...>;
	};


	//
	// param
	//
	template <string_literal name, typename payload, typename constructor = param_identity>
	struct param_t
	{
		constexpr static string_literal name = name.data;

		// constructor takes the payload-type and any gathered
		// arguments from our axis gatherer
		using constructor_fn = constructor;

		// the type given to the benchmark
		using payload_type = payload;
	};

	template <string_literal name, typename type, typename constructor = param_identity>
	using param = param_t<name, type, constructor>;

	// templated-param
	template <string_literal name, template <typename...> typename type, typename constructor = param_templated>
	using templated_param = param_t<name, meta::lazy::cfe<type>, constructor>;

	// constructed-param
	template <string_literal name, template <typename...> typename constructor_ft = param_templated_eager>
	using constructed_param = param_t<name, meta::nil, meta::lazy::skip<meta::usize_<1>, meta::lazy::cfe<constructor_ft>>>;

	template <typename f, typename... args>
	using construct_templated_type = meta::invoke<f, args...>;

	

	// key-value-param
	template <typename Key, typename Value>
	struct key_value_payload
	{
		using key_type = Key;
		using value_type = Value;

		static constexpr auto splat()
		{
			return meta::list<key_type, value_type>{};
		}

		static inline const auto default_key = Key{};
		static inline const auto default_value = Value{};
	};

	template <string_literal name, typename Key, typename Value>
	using key_value_param = param<name, key_value_payload<Key, Value>>;
}


///
/// baked_param
/// --------------
/// 
namespace atma::bench
{
	template <string_literal name, typename payload>
	struct baked_param
	{
		static constexpr string_literal name = name;
		using payload_type = payload;
	};
}

///
/// axis
/// -------
/// 
///
namespace atma::bench
{
	template <string_literal Name, typename Constructor, typename... Params>
	struct basic_axis
	{
		using argument_gather_fn = Constructor;
		using params_type = meta::list<Params...>;

		static constexpr string_literal name = Name.data;
		static constexpr auto params = meta::list<Params...>{};
	};

	template <typename axes, typename params, typename axis, typename param>
	using evaluate_parameter = meta::invoke<
		typename axis::argument_gather_fn,
		axes, params, axis, param>;


	struct gather_nothing
	{
		template <typename axes, typename params, typename axis, typename param>
		using f = baked_param<param::name,
			meta::invoke<typename param::constructor_fn, typename param::payload_type>>;
	};

#if 0
	struct constructor_template
	{
		template <typename axes, typename params, typename axis, typename param>
		using f = baked_param<param::name,
			meta::invoke<typename param::constructor_fn, typename param::payload_type>>;
	};
#endif

	template <string_literal Name, typename... Params>
	using axis = basic_axis<Name, gather_nothing, Params...>;

	template <string_literal Name, typename Constructor, typename... Params>
	using templated_axis = basic_axis<Name, Constructor, Params...>;


	template <size_t idx_vv>
	struct axis_type
		: meta::integral_constant_of<idx_vv>
	{};
}

namespace atma::bench
{
	using axis1 = axis_type<0>;
	using axis2 = axis_type<1>;
	using axis3 = axis_type<2>;
	using axis4 = axis_type<3>;
}


///
/// recursive_axes_definitions_found
/// -----------------------------------
/// 
/// if you've found your way here, it means you have two axis
/// definitions that reference each other - they are trying
/// to construct the other recursively. stop that.
/// 
namespace atma::bench
{
	struct recursive_axes_definitions_found
	{
		static constexpr string_literal name = "<error>";
	};
}





///
/// splatting
/// -----------
/// 
/// 
namespace atma::bench
{
	template <typename T>
	struct splat : T
	{};
}

namespace atma::bench::detail
{
	template <typename param, typename = std::void_t<>>
	struct splattify_impl
	{
		using type = param;
	};

	template <typename param>
	struct splattify_impl<param, std::void_t<decltype(param::payload_type::splat())>>
	{
		template <typename payload>
		using synthesize_param = baked_param<param::name.push_back(" (splat)"), payload>;

		using type = meta::map<synthesize_param, decltype(param::payload_type::splat())>;
	};

	template <typename axis>
	struct splattify
	{
		template <typename constructed_type>
		using f = meta::list<typename splattify_impl<constructed_type>::type>;
	};
	
	template <size_t idx>
	struct splattify<splat<axis_type<idx>>>
	{
		template <typename constructed_type>
		using f = typename splattify_impl<constructed_type>::type;
	};
}

namespace atma::bench::detail
{
	template <typename axis>
	struct replace_axis_with_nonesuch
	{
		template <typename candidate>
		using f = meta::if_<axis::name == candidate::name,
			recursive_axes_definitions_found,
			candidate>;
	};

	template <typename axes, typename params>
	struct recursively_construct_axis
	{
		template <typename axcd>
		using axis_constructor_t =
			typename meta::at<axcd::value, axes>::argument_gather_fn;

		template <typename axcd>
		using constructed_axis_t = meta::invoke<axis_constructor_t<axcd>,
			axes, params,
			meta::at<axcd::value, axes>,
			meta::at<axcd::value, params>>;

		template <typename axcd>
		using f = meta::invoke<splattify<axcd>, constructed_axis_t<axcd>>;
	};

	template <typename Constructor, typename Param>
	struct lazy_construct_templated_type
	{
		static constexpr bool use_param_constructor = 
			std::is_same_v<Constructor, meta::nil> ||
			std::is_same_v<typename Param::payload_type, meta::nil>;

		using constructor_type = meta::if_<use_param_constructor,
			typename Param::constructor_fn,
			Constructor>;

		template <typename... paramvals>
		using impl = meta::lazy::cc<constructor_type,
			typename Param::payload_type,
			typename paramvals::payload_type...>;

		template <typename... paramvals>
		using f = impl<paramvals...>;
	};

	template <typename constructor, typename axes, typename params, typename axis, typename param>
	struct construct_with_axes_impl
	{
		// remove ourselves from the list of axes to recursively construct.
		// this will any future constructions referencing us, causing
		// an infinite recursion, to print a nicer compiler error
		using axes_ = meta::map<typename replace_axis_with_nonesuch<axis>::template f, axes>;

		template <typename... axcds>
		using f = baked_param<param::name,
			meta::invoke<
				meta::lazy::exec
				<
					// recursively construct templated axes
					meta::lazy::map<recursively_construct_axis<axes_, params>>,
					// flatten into one big list
					meta::lazy::list_join<>,
					// construct templated type from list of payloads
					lazy_construct_templated_type<constructor, param>
				>,
				axcds...>>;
	};
}

namespace atma::bench
{
	template <typename... axcds>
	struct construct_from_axes
	{
		template <typename axes, typename params, typename axis, typename param>
		using f = typename detail::construct_with_axes_impl<meta::nil, axes, params, axis, param>::template f<axcds...>;
	};

	template <typename constructor, typename... axcds>
	struct construct_from_axes_with
	{
		template <typename axes, typename params, typename axis, typename param>
		using f = typename detail::construct_with_axes_impl<constructor, axes, params, axis, param>::template f<axcds...>;
	};
}


















namespace atma::bench
{
	struct measurement_t
	{
		std::string name;
		std::chrono::nanoseconds time;
		uint64_t iterations;
		//uint64_t branches;
		//uint64_t branch_misses;
	};

	struct axes_measurement_t
	{
		//std::map<std::string, std::string> axes;
		std::vector<std::string> axes;
		measurement_t measurement;
	};

	struct scenario_result_t
	{
		std::string name;
		std::vector<std::string> axes_headers;
		std::vector<axes_measurement_t> measurements;
	};
}

namespace atma::bench
{
	struct result_recorder_t
	{
		virtual void set_axes_count(uint64_t) {}
		virtual void set_axis_header(uint64_t index, std::string_view) {}
		virtual void set_axis_value(uint64_t index, std::string_view) {}

		virtual void record(measurement_t const&) = 0;
	};
}

export namespace atma::bench
{
	struct result_outputter_t
	{
		virtual ~result_outputter_t() = default;
		virtual void output(measurement_t const&) = 0;
	};

	struct stdout_outputter_t : result_outputter_t
	{
		virtual void output(measurement_t const& r) final
		{
			//for (auto const& [k, v] : r.axes)
			//	std::cout << k << ", " << v << ", ";
			std::cout << (r.time / r.iterations) << " (" << r.time <<  " / " << r.iterations << ")" << std::endl;
		}
	};
}

namespace atma::bench
{
	constexpr int max_outputters = 4;

	using result_outputter_ptr = std::unique_ptr<result_outputter_t>;

	result_outputter_ptr outputters_[max_outputters];

	void scenario_output(measurement_t const& r)
	{
		for (auto& outputter : outputters_)
		{
			if (outputter)
				outputter->output(r);
		}
	}
}

namespace atma::bench
{
	template <typename... Outputs>
	void set_scenario_output_impl(int index)
	{
		for (int i = index; i != max_outputters; ++i)
			outputters_[i].reset();
	}

	template <typename First, typename... Outputs>
	void set_scenario_output_impl(int index, First&& o, Outputs... outputs)
	{
		ATMA_ASSERT(index < max_outputters);
		outputters_[index] = std::make_unique<std::remove_reference_t<First>>(std::move(o));

		set_scenario_output_impl(index + 1, outputs...);
	}
}

export namespace atma::bench
{
	template <typename... Outputs>
	void set_scenario_output(Outputs... outputs)
	{
		set_scenario_output_impl(0, std::forward<Outputs>(outputs)...);
	}
}

export namespace atma::bench
{
	struct abstract_scenario_t
		: result_recorder_t
	{
		abstract_scenario_t();

		virtual scenario_result_t measure_all() = 0;
	};
}

namespace atma::bench::detail
{
	extern std::vector<abstract_scenario_t*> scenarios_;
}

export namespace atma::bench
{
	using measure_all_result_t = std::vector<scenario_result_t>;

	inline auto measure_all() -> measure_all_result_t
	{
		measure_all_result_t r;

		for (auto* scenario : detail::scenarios_)
		{
			scenario_result_t sr = scenario->measure_all();
			r.push_back(sr);
		}

		return r;
	}


	template <typename... Axis>
	void measure_along() {}
}


namespace atma::bench
{
#if defined(_MSC_VER)
#  pragma optimize("", off)
	void doNotOptimizeAwaySink(void const*) {}
#  pragma optimize("", on)
#endif

	export template <typename T>
	void doNotOptimizeAway(T const& val) {
		doNotOptimizeAwaySink(&val);
	}
}

export namespace atma::bench
{
	void measure_all_to_stdout()
	{
		auto r = measure_all();
	
		stdout_outputter_t stdout_writer;
		for (auto const& sr : r)
		{
			std::vector<size_t> padding;
		
			for (size_t i = 0u; i != sr.axes_headers.size(); ++i)
			{
				size_t pad = sr.axes_headers[i].size();
				for (auto const& mm : sr.measurements)
					pad = std::max(pad, mm.axes[i].size());
				padding.push_back(pad);
			}

			//std::cout << " #" << std::endl;
			//std::cout << " # " << sr.name << std::endl;
			//std::cout << " #" << std::endl;
			std::cout << sr.name << std::endl;
			std::cout << std::endl;
			//for (auto const& axis : sr.axes_headers)
			std::cout << " ";
			for (size_t i = 0; i != sr.axes_headers.size(); ++i)
				std::cout << std::setw(padding[i]) << sr.axes_headers[i] << " | ";
			std::cout << "measurement" << std::endl;
			std::cout << " ---------------------------------------------------" << std::endl;

			for (auto const& amr : sr.measurements)
			{
				std::cout << " ";
				int i = 0;
				for (auto const& axis : amr.axes)
					std::cout << std::setw(padding[i++]) << axis << " | ";

				std::cout << amr.measurement.name << " -> ";
				stdout_writer.output(amr.measurement);
			}

			std::cout << std::endl << std::endl;
		}
	}
}





namespace atma::bench::detail
{
	using clock_type = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
		std::chrono::high_resolution_clock,
		std::chrono::steady_clock>;

	std::chrono::nanoseconds get_resolution()
	{
		constexpr int iterations = 20;

		clock_type::duration shortest = clock_type::duration::max();
		for (int i = 0; i != iterations; ++i)
		{
			clock_type::time_point const e = clock_type::now();
			clock_type::time_point e2;
			do { e2 = clock_type::now(); } while (e2 == e);
			shortest = std::min(shortest, e2 - e);
		}

		return std::chrono::duration_cast<std::chrono::nanoseconds>(shortest);
	}
}


namespace atma::bench
{
	struct benchmark_signature
	{
		constexpr benchmark_signature() = default;

		constexpr benchmark_signature(char const* name, char const* file, int line, uintptr_t id)
			: name(name), file(file), line(line), id{id}
		{}

		char const* name{};
		char const* file{};
		int line{};
		uintptr_t id{};
	};

	template <typename R, typename C>
	struct base_sso_comparator
	{
		using member_ptr_type = R(C::*);

		base_sso_comparator(member_ptr_type member)
			: member{member}
		{}

	protected:
		member_ptr_type const member;
	};

	template <typename C>
	struct sso_strcmp : base_sso_comparator<char const*, C>
	{
		using base_sso_comparator<char const*, C>::base_sso_comparator;

		template <typename A, typename B>
		int operator () (A const& lhs, B const& rhs) const
		{
			return strcmp((lhs.*this->member), (rhs.*this->member));
		}
	};

	template <typename C>
	sso_strcmp(char const* (C::*)) -> sso_strcmp<C>;

	template <typename R, typename C>
	struct sso_sub : base_sso_comparator<R, C>
	{
		using base_sso_comparator<R, C>::base_sso_comparator;

		template <typename A, typename B>
		int operator () (A const& lhs, B const& rhs) const
		{
			return (int)(lhs.*this->member) - (int)(rhs.*this->member);
		}
	};

	template <typename A, typename B>
	auto spaceship_chain(A const&, B const&)
	{
		return 0;
	}

	template <typename A, typename B, typename C, typename... Comparators>
	auto spaceship_chain(A const& lhs, B const& rhs, C comparator, Comparators... comparators)
	{
		if (auto const r = comparator(lhs, rhs); r == 0)
			return spaceship_chain(lhs, rhs, comparators...);
		else
			return r;
	}

	inline int operator <=> (benchmark_signature const& lhs, benchmark_signature const& rhs)
	{
		return spaceship_chain(lhs, rhs,
			sso_strcmp{&benchmark_signature::name},
			sso_strcmp{&benchmark_signature::file},
			sso_sub{&benchmark_signature::line},
			sso_sub{&benchmark_signature::id});
	}
}


//
// executing_epoch_t
// -------------------
//
namespace atma::bench
{
	struct executing_epoch_t
	{
		struct iterator
		{
			uint64_t i;

			uint64_t operator*() const { return i; }
			void operator ++() {++i;}
			bool operator != (iterator rhs) { return i != rhs.i; }
		};

		uint64_t iters;

		auto begin() const -> iterator { return {0}; }
		auto end() const -> iterator { return {iters}; }
	};
}


//
// executing_benchmark_t
// -----------------------
//
namespace atma::bench
{
	struct benchmark_t;

	struct executing_benchmark_t
	{
		enum class state_t { spinning_up, measuring };

		executing_benchmark_t(benchmark_t*);
		~executing_benchmark_t();

		size_t epochs_remaining() const;
		auto execute_epoch() -> executing_epoch_t;

		void update(std::chrono::nanoseconds elapsed);

		void update()
		{
			auto now = detail::clock_type::now();
			if (submeasures_ == 0)
				elapsed_ += std::chrono::duration_cast<std::chrono::nanoseconds>(now - time_start_);

			update(elapsed_);

			submeasures_ = 0;
			elapsed_ = std::chrono::nanoseconds::zero();
		}

		template <typename F>
		executing_benchmark_t& measure(F&& f)
		{
			time_start_ = detail::clock_type::now();
			f();
			elapsed_ += std::chrono::duration_cast<std::chrono::nanoseconds>(detail::clock_type::now() - time_start_);
			++submeasures_;
			return *this;
		}

		void reset()
		{
			time_start_ = detail::clock_type::now();
		}

		void record_submeasurement(detail::clock_type::time_point now = detail::clock_type::now())
		{
			elapsed_ += std::chrono::duration_cast<std::chrono::nanoseconds>(now - time_start_);
			++submeasures_;
		}

		template <typename F>
		executing_benchmark_t& perform(F&& f)
		{
			f();
			return *this;
		}

	private:
		size_t estimate_best_iter_count(std::chrono::nanoseconds elapsed, uint64_t iterations) const;

	private:
		benchmark_t* benchmark_{};

		std::chrono::nanoseconds clock_resolution_;
		std::chrono::nanoseconds target_epoch_duration_;

		// iteration logic
		state_t state_{};
		size_t epoch_iters_{};
		detail::clock_type::time_point time_start_;
		std::chrono::nanoseconds elapsed_{};
		uint64_t submeasures_{};

		// accumulation
		size_t total_iterations_{};
		std::chrono::nanoseconds total_elapsed_{};
		size_t total_epochs_{};
	};
}


//
// benchmark_t
// -------------
//
namespace atma::bench
{
	struct benchmark_t
	{
		constexpr benchmark_t(result_recorder_t* rr, char const* name)
			: result_recorder_{rr}
			, name{name}
		{}

		bool begin()
		{
			return !executed && (executed = true);
		}
		
		void end()
		{
			result_recorder_->record(measurement_t{
				.name = name,
				.time = time,
				.iterations = iterations});
		}

		// results
		result_recorder_t* result_recorder_{};
		std::string name;

		// config
		size_t epochs{11};
		size_t clock_multiplier{1000};
		std::chrono::nanoseconds min_epoch_duration{std::chrono::milliseconds(1)};
		std::chrono::nanoseconds max_epoch_duration{std::chrono::milliseconds(100)};
		size_t min_epoch_iterations{1};
		size_t epoch_iterations{};

		// accumulation
		std::chrono::nanoseconds time{};
		size_t iterations{};
		size_t cycles{};
		size_t branch_instructions{};
		size_t branch_misses{};
		size_t cache_misses{};

		bool executed{};
	};
}


//
// benchmark_handle
// ------------------
//
namespace atma::bench
{
	struct benchmark_handle
	{
		constexpr benchmark_handle() = default;
		constexpr benchmark_handle(benchmark_t* bm)
			: benchmark_{bm}
		{}

		~benchmark_handle()
		{
			if (benchmark_)
			{
				benchmark_->end();
			}
		}

		operator bool() const
		{
			return benchmark_ != nullptr;
		}

		executing_benchmark_t execute() const
		{
			return executing_benchmark_t{benchmark_};
		}

		benchmark_t* get() const { return benchmark_; }

	private:
		benchmark_t* benchmark_{};
	};
}


//
// executing_benchmark_t implementation
//
namespace atma::bench
{
	inline executing_benchmark_t::executing_benchmark_t(benchmark_t* bm)
		: benchmark_{bm}
		, clock_resolution_{detail::get_resolution()}
		, target_epoch_duration_{benchmark_->clock_multiplier * clock_resolution_}
		, epoch_iters_{benchmark_->min_epoch_iterations}
	{
		if (target_epoch_duration_ < benchmark_->min_epoch_duration)
			target_epoch_duration_ = benchmark_->min_epoch_duration;
		if (target_epoch_duration_ > benchmark_->max_epoch_duration)
			target_epoch_duration_ = benchmark_->max_epoch_duration;
	}

	inline executing_benchmark_t::~executing_benchmark_t()
	{
		benchmark_->time += total_elapsed_;
		benchmark_->iterations += total_iterations_;
	}

	inline size_t executing_benchmark_t::epochs_remaining() const
	{
		ATMA_ASSERT(total_epochs_ <= benchmark_->epochs);
		return benchmark_->epochs - total_epochs_;
	}

	inline auto executing_benchmark_t::execute_epoch() -> executing_epoch_t
	{
		time_start_ = detail::clock_type::now();
		return executing_epoch_t{epoch_iters_};
	}

	inline size_t executing_benchmark_t::estimate_best_iter_count(std::chrono::nanoseconds elapsed, uint64_t iterations) const
	{
		auto const delapsed = (double)elapsed.count();
		auto const dtarget_duration = (double)target_epoch_duration_.count();
		auto const dremaining_iters = std::max(dtarget_duration, 0.0) / delapsed * (double)epoch_iters_;

		return static_cast<size_t>(dremaining_iters * 1.2 + 0.5);
	}

	inline void executing_benchmark_t::update(std::chrono::nanoseconds elapsed)
	{
		if (elapsed == std::chrono::nanoseconds::zero())
			elapsed = clock_resolution_;

		if (state_ == state_t::spinning_up)
		{
			// two-thirds or more of the way to the target epoch duration is
			// goood enough - consider this a valid epoch and add it to our
			// measurements, but still recalculate the number of iterations
			if (elapsed * 3 >= target_epoch_duration_ * 2)
			{
				state_ = state_t::measuring;
				total_iterations_ += epoch_iters_;
				total_elapsed_ += elapsed;
				epoch_iters_ = estimate_best_iter_count(total_elapsed_, total_iterations_);
				++total_epochs_;
			}
			// we're very far away from the target duration, x10 the iterations
			else if (elapsed * 10 < target_epoch_duration_)
			{
				// watch out for overflow
				epoch_iters_ = (epoch_iters_ * 10 > epoch_iters_)
					? epoch_iters_ *= 10
					: 0;
			}
			// neither very far away nor close enough - just recalculate
			else
			{
				epoch_iters_ = estimate_best_iter_count(elapsed, epoch_iters_);
			}
		}
		else
		{
			total_iterations_ += epoch_iters_;
			total_elapsed_ += elapsed;
			epoch_iters_ = estimate_best_iter_count(total_elapsed_, total_iterations_);
			++total_epochs_;
		}

		if (total_epochs_ == benchmark_->epochs)
		{
			epoch_iters_ = 0;
		}
	}
}

#if 0
namespace std
{
	template <>
	struct hash<atma::bench::benchmark_signature>
	{
		size_t operator()(atma::bench::benchmark_signature const& x) const
		{
			return x.line;
		}
	};
}
#endif

namespace atma::bench
{
	struct scenario_recorder_t
		: result_recorder_t
	{
		virtual void set_axes_count(uint64_t count)
		{
			axes.clear();
			axes.resize(count);
		}

		virtual void set_axis_header(uint64_t index, std::string_view header)
		{
			axes[index].first = header;
		}

		virtual void set_axis_value(uint64_t index, std::string_view value)
		{
			axes[index].second = value;
		}

		virtual void record(measurement_t const& r) override
		{
			measurement_t rr = r;
			scenario_output(rr);
		}

		std::vector<std::pair<std::string, std::string>> axes;
	};
}

namespace atma::bench
{
	//template <typename T>
	//concept no_concept = false;

	

	template <typename... Args>
	void invoke_expand(auto&& f)
	{
		(std::invoke(f, Args{}), ...);
	}

	template <typename... Args>
	void invoke_expand_i(auto&& f)
	{
		uint64_t index = 0;
		(std::invoke(f, index++, Args{}), ...);
	}

	template <typename S, typename... Axis>
	struct base_scenario
		: abstract_scenario_t
	{
		using axes_type = meta::list<Axis...>;
		using combinations = meta::select_combinations_t<typename Axis::params_type...>;
		//using zipped_combinations = meta::zip<meta::list<Axis...>, combinations>;

		base_scenario()
			: result_{static_cast<S*>(this)->name}
		{
			invoke_expand_i<Axis...>(
				[&](uint64_t index, auto a) {
					result_.axes_headers.push_back(a.name);
				});
		}

		

		auto measure_all() -> scenario_result_t override
		{
			[&]<typename... Combos>(meta::list<Combos...>)
			{
				([&]<typename... Axes>(meta::list<Axes...>)
				{
					this->template execute_wrapper<
						evaluate_parameter<axes_type, meta::list<meta::at<1, Axes>...>,
						meta::at<0, Axes>, meta::at<1, Axes>>...
					>();
				}(meta::zip<axes_type, Combos>{}), ...);
			}(combinations{});

			return result_;
		}

		template <typename... params>
		void execute_wrapper()
		{
			current_axes_values.clear();

			//no<params...> {};

			invoke_expand<params...>(
				[&](auto p) {
					current_axes_values.push_back(p.name);
				});

			do
			{
				executed_benchmark_this_run_ = false;
				static_cast<S*>(this)->template execute<void, typename params::payload_type...>();
			} while (executed_benchmark_this_run_);
		}

		benchmark_handle register_benchmark(char const* name, char const* file, int line, uintptr_t id)
		{
			if (executed_benchmark_this_run_)
			{
				return benchmark_handle{};
			}
			else if (auto R = benchmarks_.emplace(benchmark_signature(name, file, line, id), benchmark_t{this, name}); R.second)
			{
				executed_benchmark_this_run_ = true;
				current_ = &R.first->second;
				return benchmark_handle{&R.first->second};
			}
			else
			{
				return benchmark_handle{};
			}
		}

		auto current_benchmark() -> benchmark_t*
		{
			return current_;
		}

		void record(measurement_t const& m) override
		{
			//result_.results.push_back(r);
			axes_measurement_t a;
			a.axes = current_axes_values;
			a.measurement = m;

			result_.measurements.emplace_back(std::move(a));
		}

	private:
		scenario_recorder_t recorder_;

		std::vector<std::string> current_axes_values;
		benchmark_t* current_;

	protected:
		using benchmarks_t = std::map<benchmark_signature, benchmark_t>;
		
		benchmarks_t benchmarks_;
		scenario_result_t result_;
		bool executed_benchmark_this_run_{};
	};
}

export namespace atma::bench::detail
{
	std::vector<abstract_scenario_t*> scenarios_;
}

namespace atma::bench
{
	abstract_scenario_t::abstract_scenario_t()
	{
		detail::scenarios_.push_back(this);
	}
}

























#if 0
ATMA_BENCH_SCENARIO("numbers")
{
	double d = 1.0;

	ATMA_BENCHMARK("double thing")
	{
		d += 1.0 / d;
		if (d > 5.0) {
			d -= 5.0;
		}

		atma::bench::doNotOptimizeAway(d);
	}
}
#endif

#if 1
namespace test_simple_axis
{
	using silliness_axis = atma::bench::axis<"silliness",
		atma::bench::param<"shenanigans", int>,
		atma::bench::param<"tomfoolery", float>>;

	ATMA_BENCHMARK_SIMPLE("addition", silliness_axis)
	{
		silliness_axis_type i{};
		i += 4;
	}
}
#endif

namespace
{
	struct less_comparator
	{
		template <typename T>
		bool operator ()(T&& lhs, T&& rhs) const
		{
			if constexpr (std::is_class_v<std::remove_cvref_t<T>>)
			{
				return (std::operator <=> (lhs, rhs)) < 0;
			}
			else
			{
				return lhs < rhs;
			}
		}
	};
}

//
#if 1
namespace test_templated_axis
{
	using hash_map_axis = atma::bench::axis<"hash_map",
		atma::bench::templated_param<"std::map", std::map>,
		atma::bench::templated_param<"std::unordered_map", std::unordered_map>>;

	using key_types_axis = atma::bench::axis<"key-types",
		atma::bench::param<"u64", uint64_t>,
		atma::bench::param<"f32", float>>;

	using value_types_axis = atma::bench::axis<"value-types",
		atma::bench::param<"u64", uint64_t>,
		atma::bench::param<"f32", float>>;

	ATMA_BENCH_SCENARIO("hash-maps", hash_map_axis, key_types_axis, value_types_axis)
	{
		using hash_map_type = atma::bench::construct_templated_type<
			hash_map_axis_type,
			key_types_axis_type,
			value_types_axis_type>;

		auto const default_key = key_types_axis_type{};
		auto const default_value = value_types_axis_type{};

		ATMA_BENCHMARK("clear")
		{
			hash_map_type hash_map;

			hash_map[default_key] = default_value;
			ATMA_BENCH_SUBMEASURE()
			{
				hash_map.clear();
			}
		}
	}
}
#endif


#if 1
namespace test_templated_axis_constructed_dynamically
{
	namespace abl = atma::bench;

	template <typename key, typename value>
	using map_constructor = std::map<key, value, less_comparator>;

	using hash_map_axis = abl::templated_axis<"hash_map",
		abl::construct_from_axes<abl::axis2, abl::axis3>,
		abl::constructed_param<"std::map", map_constructor>
		//abl::templated_param<"std::unordered_map", std::unordered_map>
		>;

	using key_types_axis = abl::axis<"key-types",
		abl::param<"u64", uint64_t>,
		abl::param<"f32", float>,
		abl::param<"string", std::string>>;

	using value_types_axis = abl::axis<"value-types",
		abl::param<"u64", uint64_t>,
		abl::param<"f32", float>,
		abl::param<"string", std::string>>;

	ATMA_BENCH_SCENARIO("hash-maps", hash_map_axis, key_types_axis, value_types_axis)
	{
		using hash_map_type = hash_map_axis_type;

		auto const default_key = key_types_axis_type{};
		auto const default_value = value_types_axis_type{};

		ATMA_BENCHMARK("insert")
		{
			hash_map_type hash_map;

			ATMA_BENCH_SUBMEASURE()
			{
				hash_map[default_key] = default_value;
			}

			hash_map.clear();
		}
	}
}
#endif



#if 0
namespace test_templated_axis_constructed_dynamically_with_splat
{
	namespace ab = atma::bench;

	using hash_map_axis = atma::bench::templated_axis<"hash_map",
		ab::construct_from_axes<ab::splat<ab::axis2>>,
		//ab::templated_param<"std::map", std::map>,
		//ab::templated_param<"std::unordered_map", std::unordered_map>
		>;

	using types_axis = atma::bench::axis<"types",
		atma::bench::key_value_param<"u64|u64", uint64_t, uint64_t>,
		atma::bench::key_value_param<"u64|string", uint64_t, std::string>>;

	ATMA_BENCH_SCENARIO("hash-maps", hash_map_axis, types_axis)
	{
		using hash_map_type = hash_map_axis_type;
		
		auto default_key = types_axis_type::default_key;
		auto const default_value = types_axis_type::default_value;

		ATMA_BENCHMARK("erase")
		{
			hash_map_type hash_map;

			hash_map[default_key] = default_value;

			ATMA_BENCH_SUBMEASURE()
			{
				hash_map.erase(default_key);
			}

			ATMA_ASSERT(hash_map.empty());
		}
	}
}
#endif



#if 1
namespace test_templated_axis_constructed_dynamically_with_splat2
{
	namespace ab = atma::bench;

	
	template <typename key, typename value, typename allocator>
	using map_constructor = std::map<key, value, less_comparator, allocator>;

	template <typename key, typename value, typename allocator>
	using unordered_map_constructor = std::unordered_map<key, value, std::hash<key>, std::equal_to<key>, allocator>;

	using hash_map_axis = atma::bench::templated_axis<"hash_map",
		ab::construct_from_axes<ab::splat<ab::axis2>, ab::axis3>,
		ab::constructed_param<"std::map", map_constructor>//,
		//ab::constructed_param<"std::unordered_map", unordered_map_constructor>
		>;
	//using hash_map_axis = atma::bench::axis<"yeah",
	//	ab::param<"int", std::map<int, std::string>>>;


	using types_axis = atma::bench::axis<"types",
		atma::bench::key_value_param<"u64|u64", uint64_t, uint64_t>,
		atma::bench::key_value_param<"u64|string", uint64_t, std::string>>;


	struct allocator_constructor
	{
		template <typename payload, typename key, typename value>
		using f = atma::meta::invoke<payload, std::pair<key const, value>>;
	};

	template <typename key, typename value>
	using make_aligned_allocator = atma::aligned_allocator_t<std::pair<key const, value>>;

	using allocators_axis = ab::templated_axis<"allocators",
		ab::construct_from_axes_with<allocator_constructor, ab::splat<ab::axis2>>,
		ab::templated_param<"std::allocator", std::allocator>,
		ab::templated_param<"arena_allocator", atma::arena_allocator_t>,
		ab::constructed_param<"aligned_allocator", make_aligned_allocator>>;

	ATMA_BENCH_SCENARIO("hash-maps", hash_map_axis, types_axis, allocators_axis)
	{
		using hash_map_type = hash_map_axis_type;

		auto default_key = types_axis_type::default_key;
		auto const default_value = types_axis_type::default_value;

		ATMA_BENCHMARK("erase")
		{
			hash_map_type hash_map;

			hash_map[default_key] = default_value;

			//ATMA_BENCH_SUBMEASURE()
			{
				hash_map.erase(default_key);
			}

			ATMA_ASSERT(hash_map.empty());
		}
	}
}
#endif










































































// win32 implementation


//export module atma.bench;

using NTSTATUS = long;





export namespace atma::bench
{
	template <int Line, typename Expr>
	inline __forceinline void no_optimize(Expr&& expr)
	{
		static char const volatile* volatile _ = &reinterpret_cast<char const volatile&>(expr);
	}
}



export typedef enum _KPROFILE_SOURCE
{
    ProfileTime                     = 0x00,
    ProfileTotalIssues              = 0x02,
    ProfileBranchInstructions       = 0x06,
    ProfileCacheMisses              = 0x0A,
    ProfileBranchMispredictions     = 0x0B,
    ProfileTotalCycles              = 0x13,
    ProfileUnhaltedCoreCycles       = 0x19,
    ProfileInstructionRetired       = 0x1A,
    ProfileUnhaltedReferenceCycles  = 0x1B,
    ProfileLLCReference             = 0x1C,
    ProfileLLCMisses                = 0x1D,
    ProfileBranchInstructionRetired = 0x1E,
    ProfileBranchMispredictsRetired = 0x1F,
} KPROFILE_SOURCE, *PKPROFILE_SOURCE;


typedef NTSYSAPI NTSTATUS NTAPI F_NtCreateProfile(
    OUT PHANDLE        ProfileHandle,
    IN HANDLE          Process OPTIONAL,
    IN PVOID           ImageBase,
    IN ULONG           ImageSize,
    IN ULONG           BucketSize,
    IN PVOID           Buffer,
    IN ULONG           BufferSize,
    IN KPROFILE_SOURCE ProfileSource,
    IN KAFFINITY       Affinity
);

typedef NTSYSAPI NTSTATUS NTAPI F_NtStartProfile(
    IN HANDLE           ProfileHandle
);

typedef NTSYSAPI NTSTATUS NTAPI F_NtStopProfile(
    IN HANDLE           ProfileHandle
);

using ZwCreateProfileEx_fnptr_type = NTSYSAPI NTSTATUS (NTAPI*)(
    OUT PHANDLE        ProfileHandle,
    IN HANDLE          Process OPTIONAL,
    IN PVOID           ImageBase,
    IN ULONG           ImageSize,
    IN ULONG           BucketSize,
    IN PVOID           Buffer,
    IN ULONG           BufferSize,
    IN KPROFILE_SOURCE ProfileSource,
    IN USHORT          GroupCount,
	IN PGROUP_AFFINITY GroupAffinity);

using ZwStartProfile_fnptr_type = NTSYSAPI NTSTATUS (NTAPI*)(
	IN HANDLE ProfileHandle);

using ZwStopProfile_fnptr_type = NTSYSAPI NTSTATUS (NTAPI*)(
	IN HANDLE ProfileHandle);


HMODULE GetCurrentModule()
{
    HMODULE module = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR) GetCurrentModule, &module);
    return module;
}

#define DO_NOT_OPTIMIZE(var) 
#define SUM_COUNT (100 * 1000 * 1000)

// expected count of cycles for loop when testing IPC
#define CYCLE_COUNT (1000 * 1000 * 1000)

static uint8_t arr[SUM_COUNT];

static void SumArray(void* Arg)
{
	uint64_t result = 0;
	DO_NOT_OPTIMIZE(result);

	for (size_t i = 0; i < SUM_COUNT; i++)
	{
		if (arr[i] < 128)
		{
			DO_NOT_OPTIMIZE(result); // force compiler to generate actual branch
			result += arr[i];
		}
	}

	DO_NOT_OPTIMIZE(result);
}

// sorts array with counting sort
static void SortArray(void)
{
	size_t offset = 0;
	uint32_t counts[256] = { 0 };
	for (size_t i = 0; i < SUM_COUNT; i++)
	{
		counts[arr[i]]++;
	}
	for (size_t i = 0; i < 256; i++)
	{
		for (size_t c = 0; c < counts[i]; c++)
		{
			arr[offset++] = (uint8_t)i;
		}
	}
}

// fill array with random values
static void RandomizeArray(void)
{
	uint32_t random = 1;
	for (size_t i = 0; i < SUM_COUNT; i++)
	{
		arr[i] = (uint8_t)(random >> 24);
		random = 0x01000193 * random + 0x811c9dc5;
	}
}

export struct WinNtModuleContext
{
	WinNtModuleContext()
		: process_{GetCurrentProcess()}
		, module_{GetCurrentModule()}
	{
		HMODULE nt = LoadLibraryA("ntdll.dll");

		create_profile = (ZwCreateProfileEx_fnptr_type)GetProcAddress(nt, "ZwCreateProfileEx");
		start_profile = (ZwStartProfile_fnptr_type)GetProcAddress(nt, "ZwStartProfile");
		stop_profile = (ZwStopProfile_fnptr_type)GetProcAddress(nt, "ZwStopProfile");

		MODULEINFO module_info;
		if (BOOL module_info_success = GetModuleInformation(process_, module_, &module_info, sizeof(module_info)))
		{
			image_base_ = module_info.lpBaseOfDll;
			image_size_ = module_info.SizeOfImage;
		}
		else
		{
			// assert properly here
			printf("GetModuleInformation() failed!\n");
		}
	}

	HANDLE process() const { return process_; }
	HMODULE module() const { return module_; }

	void* image_base() const { return image_base_; }
	DWORD image_size() const { return image_size_; }

	ZwCreateProfileEx_fnptr_type create_profile;
	ZwStartProfile_fnptr_type start_profile;
	ZwStopProfile_fnptr_type stop_profile;

private:
	HANDLE process_{};
	HMODULE module_{};

	void* image_base_{};
	DWORD image_size_{};
};

export struct WinNtProfileSession
{
	WinNtProfileSession(WinNtModuleContext& ctx)
		: ctx_{ctx}
	{
		if (!ctx_.image_base())
		{
			// no valid address to instrument
			return;
		}

		// find largest power of two that fits image_size
		for (auto i = ctx_.image_size(); i > 4; i >>= 1)
			++pow2base_;
			
		create_profile(total_cycles_handle_, total_cycles_, ProfileTotalCycles);
		create_profile(branches_handle_, branches_, ProfileBranchInstructions);
		create_profile(branches_mispredicted_handle_, branches_mispredicted_, ProfileBranchMispredictions);
		create_profile(cache_misses_handle_, cache_misses_, ProfileCacheMisses);
	}

private:
	void create_profile(HANDLE& handle, ULONG& bucket, KPROFILE_SOURCE source)
	{
		[[maybe_unused]] NTSTATUS create_status = ctx_.create_profile(&handle, 
			ctx_.process(), ctx_.image_base(), ctx_.image_size(), 
			pow2base_, &bucket, sizeof(ULONG), 
			source,
			0, nullptr);

		ATMA_ASSERT(create_status == 0);
	}

private:
	WinNtModuleContext& ctx_;
	ULONG pow2base_{2};

	// handles
	HANDLE total_cycles_handle_{};
	HANDLE branches_handle_{};
	HANDLE branches_mispredicted_handle_{};
	HANDLE cache_misses_handle_{};

	// buckets
	ULONG total_cycles_{};
	ULONG branches_{};
	ULONG branches_mispredicted_{};
	ULONG cache_misses_{};
};

#if 0
ATMA_BENCHMARK_SUITE("hash-maps")
	.add_axis()
{
}
#endif

void test()
{
    HMODULE nt = LoadLibraryA("ntdll.dll");

    F_NtCreateProfile* NtCreateProfile = (F_NtCreateProfile*) GetProcAddress(nt, "NtCreateProfile");
    F_NtStartProfile* NtStartProfile = (F_NtStartProfile*) GetProcAddress(nt, "NtStartProfile");
    F_NtStopProfile* NtStopProfile = (F_NtStopProfile*) GetProcAddress(nt, "NtStopProfile");

	//[[maybe_unused]] auto blah = GetProcAddress(nt, "ZwCreateProfileEx");;

	


    HANDLE process = GetCurrentProcess();
    HMODULE module = GetCurrentModule();
    printf("%p %p\n", process, module);

    MODULEINFO module_info;
    BOOL module_info_success = GetModuleInformation(process, module, &module_info, sizeof(module_info));
    if (!module_info_success)
    {
        printf("GetModuleInformation() failed!\n");
        return;
    }

    void* image_base = module_info.lpBaseOfDll;
    auto image_size = module_info.SizeOfImage;

    printf("%p\n", image_base);
    printf("%d\n", (int) image_size);

    HANDLE profile, profile2;

    ULONG buffer[1] = {};
	ULONG buffer2[1] = {};


	// let's see how many profiles we can create at once
	{
		size_t i = 0;
		for ( ; i != 512; ++i)
		{
			NTSTATUS create_status = NtCreateProfile(&profile, process, image_base, image_size, 30, buffer, 4, ProfileLLCMisses, ULONG_PTR(-1));
			if (create_status != 0)
				break;
		}
		printf("concurrent profiles available: %llu\n", i);
	}

    NTSTATUS create_status = NtCreateProfile(&profile, process, image_base, image_size, 30, buffer, 4, ProfileLLCMisses, ULONG_PTR(-1));
	NTSTATUS create_status2 = NtCreateProfile(&profile2, process, image_base, image_size, 30, buffer2, 4, ProfileBranchMispredictions, ULONG_PTR(-1));
    printf("NtCreateProfile: %x\n", create_status);
	printf("NtCreateProfile: %x\n", create_status2);

    NtStartProfile(profile);
	NtStartProfile(profile2);
    {
        int* matrix = (int*) malloc(sizeof(int) * 16 * 1024 * 16 * 1024);
        for (int i = 0; i < 10000; i++)
            for (int j = 0; j < 10000; j++)
                matrix[i * 16 * 1024 + j] *= i * j;
        free(matrix);
    }
    NtStopProfile(profile);
	NtStopProfile(profile2);
    printf("Cache misses 1: %d\n", buffer[0]);
	printf("Branch mispredictions 1: %d\n", buffer2[0]);
    buffer[0] = 0;
	buffer2[0] = 0;


    NtStartProfile(profile);
	NtStartProfile(profile2);
    {
        int* matrix = (int*) malloc(sizeof(int) * 16 * 1024 * 16 * 1024);
        for (int i = 0; i < 10000; i++)
            for (int j = 0; j < 10000; j++)
                matrix[j * 16 * 1024 + i] *= i * j;
        free(matrix);
    }
    NtStopProfile(profile);
	NtStopProfile(profile2);
    printf("Cache misses 2: %d\n", buffer[0]);
	printf("Branch mispredictions 2: %d\n", buffer2[0]);
    buffer[0] = 0;
	buffer2[0] = 0;


	RandomizeArray();
	buffer2[0] = 0;
	NtStartProfile(profile2);
	{
		SumArray(nullptr);
	}
	NtStopProfile(profile2);
	printf("array - Branch mispredictions 1: %d\n", buffer2[0]);

	SortArray();
	buffer2[0] = 0;
	NtStartProfile(profile2);
	{
		SumArray(nullptr);
	}
	NtStopProfile(profile2);
	printf("array - Branch mispredictions 2: %d\n", buffer2[0]);
}
