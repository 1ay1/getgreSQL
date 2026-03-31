#include "config/config.hpp"

#include <cstdlib>
#include <print>
#include <string_view>

namespace getgresql::config {

static void print_usage() {
    std::println("getgreSQL — PostgreSQL web manager");
    std::println("");
    std::println("Usage: getgresql [OPTIONS]");
    std::println("");
    std::println("Options:");
    std::println("  --pg-url URL       PostgreSQL connection string (default: postgresql://localhost/postgres)");
    std::println("  --bind ADDR        Bind address (default: 127.0.0.1)");
    std::println("  --port PORT        HTTP port (default: 5433)");
    std::println("  --threads N        Worker threads (default: 4)");
    std::println("  --pool-size N      Connection pool size (default: 5)");
    std::println("  --help             Show this help");
}

auto parse_args(int argc, char* argv[]) -> Result<Config> {
    Config cfg;

    // Check environment variables first
    if (auto* env = std::getenv("DATABASE_URL")) {
        cfg.pg_connstr = env;
    }
    if (auto* env = std::getenv("GETGRESQL_PORT")) {
        cfg.port = static_cast<std::uint16_t>(std::atoi(env));
    }

    for (int i = 1; i < argc; ++i) {
        auto arg = std::string_view(argv[i]);

        if (arg == "--help" || arg == "-h") {
            print_usage();
            std::exit(0);
        }

        auto next = [&]() -> std::string_view {
            if (i + 1 >= argc) return "";
            return argv[++i];
        };

        if (arg == "--pg-url" || arg == "--database-url") {
            cfg.pg_connstr = std::string(next());
        } else if (arg == "--bind") {
            cfg.bind_address = std::string(next());
        } else if (arg == "--port" || arg == "-p") {
            auto val = next();
            cfg.port = static_cast<std::uint16_t>(std::atoi(std::string(val).c_str()));
        } else if (arg == "--threads") {
            auto val = next();
            cfg.threads = static_cast<unsigned>(std::atoi(std::string(val).c_str()));
        } else if (arg == "--pool-size") {
            auto val = next();
            cfg.pool_size = static_cast<std::size_t>(std::atoi(std::string(val).c_str()));
        } else {
            return make_error(ConfigError{
                std::string("Unknown option: ") + std::string(arg)
            });
        }
    }

    if (cfg.port == 0) cfg.port = 5433;
    if (cfg.threads == 0) cfg.threads = 4;
    if (cfg.pool_size == 0) cfg.pool_size = 5;

    return cfg;
}

} // namespace getgresql::config
