#pragma once

#include <optional>
#include <string_view>

#include "app/iggy3d/ReceiptBuilder.hpp"

namespace iggy3d {

struct ActionState;
class Session;

void applyProductGameplayActions(Session& session,
                                 const ActionState& actions,
                                 ProductAppWindowState& window,
                                 std::string_view source);

void runScriptedProductGameplaySmoke(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window);

}  // namespace iggy3d
