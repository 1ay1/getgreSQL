#pragma once

#include "config/config.hpp"
#include "pg/pool.hpp"

namespace getgresql::http {

// ─── AppContext: shared state available to all handlers ──────────────
// Defined in its own header to break the circular dependency between
// router.hpp (needs AppContext forward decl) and handlers (need full def).

struct AppContext {
    pg::Pool& pool;
    const config::Config& config;
};

} // namespace getgresql::http
