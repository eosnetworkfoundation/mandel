#include <chrono>
#include <iostream>
#include <iomanip>
#include <locale>

#include <benchmark.hpp>

namespace benchmark {

// update this map when a new module is supported
// key is the name and value is the function doing benchmarking
std::map<std::string, std::function<void()>> modules {
   { "alt_bn_128", alt_bn_128_benchmarking },
   { "modexp", modexp_benchmarking }
};

// values to control cout format
constexpr auto name_width = 30;
constexpr auto runs_width = 5;
constexpr auto time_width = 14;
constexpr auto ns_width = 3;

uint32_t num_runs = 1;

std::map<std::string, std::function<void()>> get_modules() {
   return modules;
}

void set_num_runs(uint32_t runs) {
   num_runs = runs;
}

void printt_header() {
   std::cout << std::left << std::setw(name_width) << "function"
      << std::setw(runs_width) << "runs"
      << std::setw(time_width + ns_width) << std::right << "avg"
      << std::setw(time_width + ns_width) << "min"
      << std::setw(time_width + ns_width) << "max"
      << std::endl << std::endl;
}

void print_results(std::string name, uint32_t runs, uint64_t total, uint64_t min, uint64_t max) {
   std::cout.imbue(std::locale(""));
   std::cout
      << std::setw(name_width) << std::left << name
      << std::setw(runs_width)  << runs
      // std::fixed for not printing 1234 in 1.234e3.
      // setprecision(0) for not printing fractions
      << std::right << std::fixed << std::setprecision(0)
      << std::setw(time_width) << total/runs << std::setw(ns_width) << " ns"
      << std::setw(time_width) << min << std::setw(ns_width) << " ns"
      << std::setw(time_width) << max << std::setw(ns_width) << " ns"
      << std::endl;
}

void benchmarking(std::string name, const std::function<void()>& func) {
   uint64_t total {0}, min {std::numeric_limits<uint64_t>::max()}, max {0};

   for (auto i = 0U; i < num_runs; ++i) {
      auto start_time = std::chrono::steady_clock::now();
      func();
      auto end_time = std::chrono::steady_clock::now();

      uint64_t duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
      total += duration;
      min = std::min(min, duration);
      max = std::max(max, duration);
   }

   print_results(name, num_runs, total, min, max);
}

} // benchmark
