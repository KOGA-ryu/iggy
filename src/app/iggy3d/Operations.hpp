#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/input/InputAction.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

bool createCreativeBlankSession(std::optional<Session>& activeSession,
                                ProductAppWindowState& window);
void frameCreativeStageCameraOnOrigin(ProductAppWindowState& window);
void clearProductGameplayLaunchState(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window);

}  // namespace iggy3d
