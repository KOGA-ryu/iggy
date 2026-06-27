#pragma once

#include <optional>

#include "app/iggy3d/ReceiptBuilder.hpp"

namespace iggy3d {

class Session;

void runScriptedProductGameplaySmoke(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window);

}  // namespace iggy3d
