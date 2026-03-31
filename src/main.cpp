#include "api/routes.hpp"
#include "config/config.hpp"
#include "http/server.hpp"
#include "pg/pool.hpp"

#include <csignal>
#include <print>

static getgresql::http::Server* g_server = nullptr;

static void signal_handler(int) {
    if (g_server) g_server->stop();
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
    auto warm = pool.warm(2);  // pre-create 2 connections
    if (!warm) {
        std::println(stderr, "Failed to connect to PostgreSQL: {}", error_message(warm.error()));
        return 1;
    }
    std::println("Connection pool ready ({} idle)", pool.idle_count());

    // Build the dispatch function from the compile-time route table
    auto dispatch = [](http::Request& req, http::AppContext& ctx) -> http::Response {
        return api::AppRoutes::dispatch(req, ctx);
    };

    // Start HTTP server
    http::AppContext app_ctx{pool, *cfg};
    http::Server server(app_ctx, dispatch, cfg->threads);
    g_server = &server;

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    server.run();

    std::println("\nShutdown complete.");
    return 0;
}
