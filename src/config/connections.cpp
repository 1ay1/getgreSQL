#include "config/connections.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace getgresql::config {

namespace fs = std::filesystem;

auto connections_path() -> std::string {
    std::string dir;
    if (auto* home = std::getenv("HOME")) {
        dir = std::string(home) + "/.getgresql";
    } else {
        dir = "/tmp/.getgresql";
    }
    return dir + "/connections.tsv";
}

auto load_connections() -> std::vector<SavedConnection> {
    std::vector<SavedConnection> result;
    auto path = connections_path();

    std::ifstream file(path);
    if (!file.is_open()) return result;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        // Tab-separated: name \t url \t color
        std::istringstream ss(line);
        SavedConnection conn;
        if (!std::getline(ss, conn.name, '\t')) continue;
        if (!std::getline(ss, conn.url, '\t')) continue;
        std::getline(ss, conn.color, '\t');  // optional

        if (!conn.name.empty() && !conn.url.empty()) {
            result.push_back(std::move(conn));
        }
    }
    return result;
}

auto save_connections(const std::vector<SavedConnection>& conns) -> Result<void> {
    auto path = connections_path();

    // Ensure directory exists
    auto dir = fs::path(path).parent_path();
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        return make_error(ConfigError{"Failed to create directory: " + dir.string()});
    }

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        return make_error(ConfigError{"Failed to write: " + path});
    }

    file << "# getgreSQL saved connections (name\\turl\\tcolor)\n";
    for (auto& c : conns) {
        file << c.name << '\t' << c.url << '\t' << c.color << '\n';
    }
    return {};
}

auto add_connection(const std::string& name, const std::string& url,
                    const std::string& color) -> Result<void> {
    auto conns = load_connections();

    // Remove existing with same name (update)
    std::erase_if(conns, [&](auto& c) { return c.name == name; });

    conns.push_back({name, url, color});
    return save_connections(conns);
}

auto remove_connection(const std::string& name) -> Result<void> {
    auto conns = load_connections();
    std::erase_if(conns, [&](auto& c) { return c.name == name; });
    return save_connections(conns);
}

} // namespace getgresql::config
