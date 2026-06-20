#pragma once

#include <atma/config/platform.hpp>
#include <atma/preprocessor.hpp>

#include <boost/preprocessor.hpp>

#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>

#include <intrin.h>

//import atma.meta;



#define ATMA_BENCH_SCENARIO_ta_op(r, data, elem) typename elem##_type
#define ATMA_BENCH_SCENARIO_ta(...) \
	BOOST_PP_SEQ_ENUM( \
		BOOST_PP_SEQ_TRANSFORM(ATMA_BENCH_SCENARIO_ta_op, ~, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)))

#define ATMA_BENCH_SCENARIO_a_op(r, data, elem) elem##_type
#define ATMA_BENCH_SCENARIO_a(...) \
	BOOST_PP_SEQ_ENUM( \
		BOOST_PP_SEQ_TRANSFORM(ATMA_BENCH_SCENARIO_a_op, ~, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)))

#define ATMA_BENCH_SCENARIO_ii(scenario, scenario_name, ...) \
	struct scenario : ::atma::bench::base_scenario<scenario __VA_OPT__(, __VA_ARGS__)> \
	{ \
		static constexpr char const* name = scenario_name; \
		template <typename __VA_OPT__(, ATMA_BENCH_SCENARIO_ta(__VA_ARGS__))> \
		void execute(); \
		__VA_OPT__(template <ATMA_BENCH_SCENARIO_ta(__VA_ARGS__)>) \
		void execute_simple(); \
	}; \
	static scenario ATMA_PP_CAT(scenario, _instance); \
	template <typename __VA_OPT__(, ATMA_BENCH_SCENARIO_ta(__VA_ARGS__))> \
	void scenario::execute()

#define ATMA_BENCH_SCENARIO(name, ...) \
	ATMA_BENCH_SCENARIO_ii(ATMA_PP_CAT(_atma_bench_scenario_, __LINE__), name __VA_OPT__(, __VA_ARGS__))

#define ATMA_BENCHMARK_ii(name, file, line, benchmark) \
	if (auto benchmark = this->register_benchmark(name, file, line, (uintptr_t)_ReturnAddress())) \
		for (auto _atma_bench_execbm_ = benchmark.execute(); _atma_bench_execbm_.epochs_remaining(); _atma_bench_execbm_.update()) \
			for (auto _ : _atma_bench_execbm_.execute_epoch())

#define ATMA_BENCHMARK(name) \
	ATMA_BENCHMARK_ii(name, __FILE__, __LINE__, ATMA_PP_CAT(abmi, __LINE__))

#define ATMA_BENCH_SUBMEASURE() \
	for (int i = (_atma_bench_execbm_.reset(), 1); i--; _atma_bench_execbm_.record_submeasurement())

#define ATMA_BENCHMARK_SIMPLE(name, ...) \
	ATMA_BENCH_SCENARIO(name, __VA_ARGS__) \
	{ \
		ATMA_BENCHMARK_ii(name, __FILE__, __LINE__, ATMA_PP_CAT(abmi, __LINE__)) \
		{ \
			this->execute_simple __VA_OPT__(<ATMA_BENCH_SCENARIO_a(__VA_ARGS__)>)(); \
		} \
	} \
	__VA_OPT__(template <ATMA_BENCH_SCENARIO_ta(__VA_ARGS__)>) \
	void ATMA_PP_CAT(_atma_bench_scenario_, __LINE__)::execute_simple()