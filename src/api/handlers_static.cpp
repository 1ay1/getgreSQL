#include "api/routes.hpp"

#include <string>

// Try to include embedded assets (generated at build time)
#if __has_include("embedded_assets.hpp")
#include "embedded_assets.hpp"
#define HAS_EMBEDDED_ASSETS 1
#else
#define HAS_EMBEDDED_ASSETS 0
#endif

namespace getgresql::api {

auto StaticHandler::handle(Request& req, AppContext& /*ctx*/) -> Response {
    auto path = std::string("/") + std::string(req.param("path"));

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
