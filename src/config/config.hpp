#pragma once

#include "core/expected.hpp"

#include <cstdint>
#include <string>

namespace getgresql::config {

struct Config {
    std::string pg_connstr = "postgresql://localhost/postgres";
    std::string bind_address = "127.0.0.1";
    std::uint16_t port = 5433;
    unsigned threads = 4;
    std::size_t pool_size = 5;
};

// Parse from command-line arguments
auto parse_args(int argc, char* argv[]) -> Result<Config>;

} // namespace getgresql::config
