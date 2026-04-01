#include "api/handlers_connections.hpp"
#include "config/connections.hpp"
#include "ssr/components.hpp"

#include <format>

namespace getgresql::api {

using namespace ssr;

// ─── URL decoding (shared with handlers_query.cpp) ──────────────────

static auto url_decode(std::string_view input) -> std::string {
    std::string decoded;
    decoded.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '+') decoded += ' ';
        else if (input[i] == '%' && i + 2 < input.size()) {
            decoded += static_cast<char>(std::stoi(std::string(input.substr(i + 1, 2)), nullptr, 16));
            i += 2;
        } else decoded += input[i];
    }
    return decoded;
}

static auto form_value(std::string_view body, std::string_view key) -> std::string {
    auto needle = std::string(key) + "=";
    auto pos = body.find(needle);
    if (pos == std::string_view::npos) return {};
    // Ensure it's at start or after &
    if (pos > 0 && body[pos - 1] != '&') {
        // Search for &key=
        needle = "&" + needle;
        pos = body.find(needle);
        if (pos == std::string_view::npos) return {};
        pos++; // skip the &
    }
    auto start = pos + std::string_view(key).size() + 1;
    auto end = body.find('&', start);
    return url_decode((end == std::string_view::npos) ? body.substr(start) : body.substr(start, end - start));
}

// ─── ConnectionsPageHandler ─────────────────────────────────────────

auto ConnectionsPageHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto conns = config::load_connections();
    auto current_connstr = ctx.pool.connstr();

    auto conn = ctx.pool.checkout();
    std::string current_db = "unknown";
    if (conn) current_db = std::string(conn->get().dbname());

    // Build component data
    std::vector<ConnectionInfo> conn_infos;
    for (auto& c : conns) {
        conn_infos.push_back({.name = c.name, .url = c.url, .color = c.color,
                              .is_active = (c.url == current_connstr)});
    }

    auto render_content = [&](Html& h) {
        ConnectionsPage::render({
            .current_db = current_db,
            .current_url = current_connstr,
            .connections = conn_infos,
        }, h);
    };

    if (req.is_htmx()) return Response::html(render_partial(render_content));
    return Response::html(render_page("Connections", "Connections", render_content));
}

// ─── ConnectionSaveHandler ──────────────────────────────────────────

auto ConnectionSaveHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto body = std::string(req.body());
    auto name = form_value(body, "name");
    auto url = form_value(body, "url");
    auto color = form_value(body, "color");

    if (name.empty() || url.empty()) {
        return Response::html(render_to_string<Alert>({"Name and URL are required", "warning"}));
    }

    auto result = config::add_connection(name, url, color);
    if (!result) {
        return Response::html(render_to_string<Alert>({error_message(result.error()), "error"}));
    }

    // Re-render the connection list via SSR component
    auto conns = config::load_connections();
    std::vector<ConnectionInfo> infos;
    for (auto& c : conns) infos.push_back({c.name, c.url, c.color, c.url == ctx.pool.connstr()});
    auto h = Html::with_capacity(4096);
    ConnectionsPage::render_list(infos, ctx.pool.connstr(), h);
    return Response::html(std::move(h).finish());
}

// ─── ConnectionDeleteHandler ────────────────────────────────────────

auto ConnectionDeleteHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto body = std::string(req.body());
    auto name = form_value(body, "name");

    if (name.empty()) {
        return Response::html(render_to_string<Alert>({"Name is required", "warning"}));
    }

    config::remove_connection(name);

    auto conns = config::load_connections();
    std::vector<ConnectionInfo> infos;
    for (auto& c : conns) infos.push_back({c.name, c.url, c.color, c.url == ctx.pool.connstr()});
    auto h = Html::with_capacity(4096);
    ConnectionsPage::render_list(infos, ctx.pool.connstr(), h);
    return Response::html(std::move(h).finish());
}

// ─── ConnectionSwitchHandler ────────────────────────────────────────

auto ConnectionSwitchHandler::handle(Request& req, AppContext& ctx) -> Response {
    auto body = std::string(req.body());
    auto name = form_value(body, "name");

    auto conns = config::load_connections();
    auto it = std::find_if(conns.begin(), conns.end(),
        [&](auto& c) { return c.name == name; });

    if (it == conns.end()) {
        return Response::html(render_to_string<Alert>({"Connection not found: " + name, "error"}));
    }

    auto result = ctx.pool.reconnect(it->url);
    if (!result) {
        return Response::html(render_to_string<Alert>(
            {"Failed to connect: " + error_message(result.error()), "error"}));
    }

    // Full page redirect to dashboard
    return Response::redirect("/");
}

// ─── ConnectionTestHandler ──────────────────────────────────────────

auto ConnectionTestHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    auto body = std::string(req.body());
    auto url = form_value(body, "url");

    if (url.empty()) {
        return Response::html(render_to_string<Alert>({"URL is required", "warning"}));
    }

    auto conn = pg::PgConnection<pg::Disconnected>::connect(url);
    if (!conn) {
        return Response::html(render_to_string<Alert>(
            {"Connection failed: " + error_message(conn.error()), "error"}));
    }

    auto db = conn->dbname();
    return Response::html(render_to_string<Alert>(
        {"Connected to " + std::string(db), "success"}));
}

// ─── ConnectionInfoHandler ──────────────────────────────────────────

auto ConnectionInfoHandler::handle(Request& /*req*/, AppContext& ctx) -> Response {
    auto conn = ctx.pool.checkout();
    if (!conn) return Response::json(R"({"db":"unknown"})");

    auto db = std::string(conn->get().dbname());
    return Response::json("{\"db\":\"" + db + "\"}");
}

} // namespace getgresql::api
