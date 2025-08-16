#pragma once

#include <boost/program_options.hpp>

#include <string>
#include <optional>

namespace programm_options {

using namespace std::literals;

struct Args {
    int64_t tick_period{0};
    std::string config_file;
    std::string www_root;
    bool randomize_spawn_points{false};
    std::string state_file;
    int64_t save_state_period{0};
};


[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]);

} // namespace programm_options