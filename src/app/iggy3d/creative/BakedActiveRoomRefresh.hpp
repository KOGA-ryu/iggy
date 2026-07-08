#pragma once

#include <optional>

#include "app/iggy3d/ProductCreativeBakedRoomRefresh.hpp"

namespace iggy3d {

class Session;
struct ProductAppWindowState;

namespace creative {
struct CreativeAppState;
}  // namespace creative

ProductCreativeBakedActiveRoomRefreshResult refreshProductCreativeBakedActiveRoom(
    const ProductCreativeBakedActiveRoomRefreshRequest& request,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    const creative::CreativeAppState& creativeApp);

}  // namespace iggy3d
