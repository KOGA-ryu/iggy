#pragma once

#include <optional>

#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductAppWindowState;

void clearProductGameplayLaunchState(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window);

}  // namespace iggy3d
