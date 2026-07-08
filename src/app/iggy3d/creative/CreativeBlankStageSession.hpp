#pragma once

#include <optional>

#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductAppWindowState;

bool createCreativeBlankSession(std::optional<Session>& activeSession,
                                ProductAppWindowState& window);
void frameCreativeStageCameraOnOrigin(ProductAppWindowState& window);

}  // namespace iggy3d
