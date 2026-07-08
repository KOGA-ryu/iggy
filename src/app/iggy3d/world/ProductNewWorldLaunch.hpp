#pragma once

#include <optional>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/Options.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductAppWindowState;

void launchProductNewWorld(const ProductAppOptions& options,
                           const WorldSetupDraft& worldSetupDraft,
                           FrontendState& frontend,
                           std::optional<Session>& activeSession,
                           ProductAppWindowState& window);

}  // namespace iggy3d
