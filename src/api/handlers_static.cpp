#include "api/routes.hpp"
#include "ssr/component_registry.hpp"

#include <string>

// Try to include embedded assets (generated at build time)
#if __has_include("embedded_assets.hpp")
#include "embedded_assets.hpp"
#define HAS_EMBEDDED_ASSETS 1
#else
#define HAS_EMBEDDED_ASSETS 0
#endif

namespace getgresql::api {

// Lazy-initialized component asset bundles (computed once, cached forever)
static auto component_css() -> const std::string& {
    static const auto s = ssr::collect_css();
    return s;
}
static auto component_js() -> const std::string& {
    static const auto s = ssr::collect_js();
    return s;
}

auto StaticHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    auto path = std::string("/") + std::string(req.param("path"));

    // Component asset bundles (collected from C++ constexpr strings)
    if (path == "/css/components.css") return Response::css(component_css());
    if (path == "/js/components.js") return Response::js(component_js());

#if HAS_EMBEDDED_ASSETS
    auto it = assets::all.find(path);
    if (it != assets::all.end()) {
        // Determine content type from extension
        if (path.ends_with(".css")) {
            return Response::css(std::string(it->second));
        } else if (path.ends_with(".js")) {
            return Response::js(std::string(it->second));
        } else {
            return Response::text(std::string(it->second));
        }
    }
#endif

    (void)path;
    return Response::not_found(req.path());
}

} // namespace getgresql::api
