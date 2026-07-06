#pragma once

#include <optional>

#include "app/iggy3d/ProductAppWindowState.hpp"

namespace iggy3d {

class Session;

void runScriptedProductGameplaySmoke(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window);

}  // namespace iggy3d
