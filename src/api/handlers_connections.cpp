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

    // Get current DB name
    auto conn = ctx.pool.checkout();
    std::string current_db = "unknown";
    if (conn) current_db = std::string(conn->get().dbname());

    auto render_content = [&](Html& out) {
        out.raw("<div style=\"max-width:720px;margin:0 auto;padding:var(--sp-5)\">");

        // Current connection info
        out.raw("<div class=\"conn-current\">"
              "<h3>Current Connection</h3>"
              "<div class=\"conn-info-card\">"
              "<span class=\"conn-dot-lg\"></span>"
              "<div><strong>").text(current_db).raw("</strong>"
              "<div class=\"conn-url\">").text(current_connstr).raw("</div>"
              "</div></div></div>");

        // Add new connection form
        out.raw("<div class=\"conn-add-section\">"
              "<h3>Add Connection</h3>"
              "<form hx-post=\"/connections/save\" hx-target=\"#conn-list\" hx-swap=\"innerHTML\">"
              "<div class=\"conn-form-grid\">"
              "<div class=\"form-field\">"
              "<label>Name</label>"
              "<input type=\"text\" name=\"name\" placeholder=\"Production DB\" required class=\"form-input\">"
              "</div>"
              "<div class=\"form-field\">"
              "<label>Connection URL</label>"
              "<input type=\"text\" name=\"url\" placeholder=\"postgresql://user:pass@host/db\" required class=\"form-input\">"
              "</div>"
              "<div class=\"form-field\">"
              "<label>Color</label>"
              "<select name=\"color\" class=\"form-input\">"
              "<option value=\"\">Default</option>"
              "<option value=\"#3b82f6\">Blue</option>"
              "<option value=\"#10b981\">Green</option>"
              "<option value=\"#f59e0b\">Amber</option>"
              "<option value=\"#ef4444\">Red</option>"
              "<option value=\"#8b5cf6\">Purple</option>"
              "<option value=\"#ec4899\">Pink</option>"
              "</select>"
              "</div>"
              "</div>"
              "<div class=\"conn-form-actions\">"
              "<button type=\"button\" class=\"btn btn-sm\" hx-post=\"/connections/test\" "
              "hx-include=\"closest form\" hx-target=\"#test-result\" hx-swap=\"innerHTML\">Test</button>"
              "<button type=\"submit\" class=\"btn btn-sm btn-primary\">Save Connection</button>"
              "</div>"
              "<div id=\"test-result\"></div>"
              "</form></div>");

        // Saved connections list
        out.raw("<div class=\"conn-saved-section\">"
              "<h3>Saved Connections</h3>"
              "<div id=\"conn-list\">");
        render_connection_list(out, conns, current_connstr);
        out.raw("</div></div>");

        out.raw("</div>");
    };

    if (req.is_htmx()) {
        auto out = Html::with_capacity(8192);
        render_content(out);
        return Response::html(std::move(out).finish());
    }
    return Response::html(render_page("Connections", "Connections", render_content));
}

auto ConnectionsPageHandler::render_connection_list(Html& h,
    const std::vector<config::SavedConnection>& conns,
    std::string_view current_connstr) -> void {

    if (conns.empty()) {
        h.raw("<div class=\"empty-state\"><div class=\"empty-icon\">&#128268;</div>"
              "<p>No saved connections. Add one above.</p></div>");
        return;
    }

    for (auto& c : conns) {
        bool is_active = (c.url == current_connstr);
        h.raw("<div class=\"conn-item");
        if (is_active) h.raw(" conn-active");
        h.raw("\"");
        if (!c.color.empty()) {
            h.raw(" style=\"border-left: 3px solid ").raw(c.color).raw("\"");
        }
        h.raw(">");
        h.raw("<div class=\"conn-item-info\">"
              "<strong>").text(c.name).raw("</strong>");
        if (is_active) h.raw(" <span class=\"badge badge-success\">active</span>");
        h.raw("<div class=\"conn-url\">").text(c.url).raw("</div>"
              "</div>"
              "<div class=\"conn-item-actions\">");
        if (!is_active) {
            h.raw("<button class=\"btn btn-sm btn-primary\" "
                  "hx-post=\"/connections/switch\" "
                  "hx-vals='{\"name\":\"").text(c.name).raw("\"}' "
                  "hx-confirm=\"Switch to ").text(c.name).raw("?\" "
                  "hx-target=\"body\">Switch</button>");
        }
        h.raw("<button class=\"btn btn-sm btn-danger\" "
              "hx-post=\"/connections/delete\" "
              "hx-vals='{\"name\":\"").text(c.name).raw("\"}' "
              "hx-target=\"#conn-list\" hx-swap=\"innerHTML\" "
              "hx-confirm=\"Delete ").text(c.name).raw("?\">Delete</button>");
        h.raw("</div></div>\n");
    }
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

    // Re-render the connection list
    auto conns = config::load_connections();
    auto h = Html::with_capacity(4096);
    ConnectionsPageHandler::render_connection_list(h, conns, ctx.pool.connstr());
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
    auto h = Html::with_capacity(4096);
    ConnectionsPageHandler::render_connection_list(h, conns, ctx.pool.connstr());
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
