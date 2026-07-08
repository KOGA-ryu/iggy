#pragma once

#include <optional>

namespace iggy3d {

struct ProductAppWindowState;
class Session;

void runScriptedProductGameplaySmoke(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window);

}  // namespace iggy3d
