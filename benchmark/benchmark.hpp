#pragma once

#include <functional>
#include <map>

namespace benchmark {

void set_num_runs(uint32_t runs);
std::map<std::string, std::function<void()>> get_modules();
void printt_header();

void alt_bn_128_benchmarking();
void modexp_benchmarking();

void benchmarking(std::string name, const std::function<void()>& func);

} // benchmark
