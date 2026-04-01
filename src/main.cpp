#include "api/routes.hpp"
#include "config/config.hpp"
#include "http/server.hpp"
#include "pg/pool.hpp"

#include <atomic>
#include <csignal>
#include <print>

// Signal handler uses atomic flag — no raw pointer to templated type needed
static std::atomic<bool> g_shutdown{false};

static void signal_handler(int) {
    g_shutdown.store(true, std::memory_order_relaxed);
}

int main(int argc, char* argv[]) {
    using namespace getgresql;

    // Parse configuration
    auto cfg = config::parse_args(argc, argv);
    if (!cfg) {
        std::println(stderr, "Error: {}", error_message(cfg.error()));
        return 1;
    }

    std::println("getgreSQL v0.1.0");
    std::println("Connecting to: {}", cfg->pg_connstr);

    // Initialize connection pool
    pg::Pool pool(cfg->pg_connstr, cfg->pool_size);
    auto warm = pool.warm(2);
    if (!warm) {
        std::println(stderr, "Failed to connect to PostgreSQL: {}", error_message(warm.error()));
        return 1;
    }
    std::println("Connection pool ready ({} idle)", pool.idle_count());

    // Compile-time dispatch: lambda wraps the RouteTable — no std::function overhead
    auto dispatch = [](http::Request& req, http::AppContext& ctx) -> http::Response {
        return api::AppRoutes::dispatch(req, ctx);
    };

    // Server type is deduced from the lambda — full type info preserved
    http::AppContext app_ctx{pool, *cfg};
    http::Server server(app_ctx, std::move(dispatch), cfg->threads);

    if (!server.listen()) {
        return 1;
    }

    std::signal(SIGINT, [](int) { g_shutdown.store(true); });
    std::signal(SIGTERM, [](int) { g_shutdown.store(true); });

    // Monitor shutdown flag in a background thread
    std::jthread shutdown_monitor([&](std::stop_token) {
        while (!g_shutdown.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        server.stop();
    });

    server.run();

    std::println("\nShutdown complete.");
    return 0;
}
