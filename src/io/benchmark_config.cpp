// Copyright (c) 2016 Matthias Noack (ma.noack.pr@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "benchmark_config.hpp"

#include <fstream>
#include <iostream>

namespace bpo = ::boost::program_options;

benchmark_config::benchmark_config(const std::string& config_file_name)
	: desc_("Benchmark options")
{
	// initialise program options description
	desc_.add_options()
		("benchmark.width", bpo::value(&benchmark_width_)->default_value(benchmark_width_), "Number of width used.")
		("benchmark.height", bpo::value(&benchmark_height_)->default_value(benchmark_height_), "Non-counted warm-up kernel runs.")
		("benchmark.kernel-runs", bpo::value(&benchmark_kernel_runs_)->default_value(benchmark_kernel_runs_), "Kernel runs (including warmups).")
	;

	parse(config_file_name);
}

// TODO: Replace with config::parse from noma::ocl? (better error handling, using config_error from noma::ocl)
void benchmark_config::parse(const std::string& config_file_name)
{
	try {
		std::ifstream config_file(config_file_name);
		bpo::variables_map vm;
		// true argument enables allow_unregistered options
		bpo::store(bpo::parse_config_file(config_file, desc_, true), vm);
		notify(vm);
	} catch (...) {
		std::cerr << desc_ << std::endl;
		std::terminate(); // unrecoverable error, TODO: ideally, the throw chain should go back up for a complete stack unwinding an dtor calling... 
	}
}

void benchmark_config::help(std::ostream& out) const
{
	out << desc_ << std::endl;
}
