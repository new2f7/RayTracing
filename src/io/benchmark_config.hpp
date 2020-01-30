// Copyright (c) 2016 Matthias Noack (ma.noack.pr@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef benchmark_config_hpp
#define benchmark_config_hpp

#include <string>

#include <boost/program_options.hpp>

class benchmark_config {
public:
	benchmark_config(const std::string& config_file_name);

	void parse(const std::string& config_file_name);
	void help(std::ostream& out) const;

	// config value getters
	const size_t& benchmark_width() const { return benchmark_width_; }
	const size_t& benchmark_height() const { return benchmark_height_; }
	const size_t& benchmark_kernel_runs() const { return benchmark_kernel_runs_; }

private:
	boost::program_options::options_description desc_;

	// config values
	size_t benchmark_width_ = 1280;
	size_t benchmark_height_ = 720;
	size_t benchmark_kernel_runs_ = 1;

};

#endif // benchmark_config_hpp
