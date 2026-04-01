#pragma once

#include "core/expected.hpp"

#include <string>
#include <vector>

namespace getgresql::config {

// ─── Saved connection entry ──────────────────────────────────────────

struct SavedConnection {
    std::string name;
    std::string url;
    std::string color;  // optional CSS color for visual identification
};

// ─── Connections file manager ────────────────────────────────────────
// Reads/writes saved connections to ~/.getgresql/connections.json.
// Format: one connection per line as "name\turl\tcolor".
// Simple tab-separated format — no JSON parser dependency needed.

auto connections_path() -> std::string;

auto load_connections() -> std::vector<SavedConnection>;
auto save_connections(const std::vector<SavedConnection>& conns) -> Result<void>;

auto add_connection(const std::string& name, const std::string& url,
                    const std::string& color = "") -> Result<void>;
auto remove_connection(const std::string& name) -> Result<void>;

} // namespace getgresql::config
