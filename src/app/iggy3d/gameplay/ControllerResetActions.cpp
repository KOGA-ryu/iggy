#include "app/iggy3d/gameplay/ControllerResetActions.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ControllerTargetOutcomeProof.hpp"
#include "runtime/session/Session.hpp"

#include <string>

namespace iggy3d {

void submitProductReset(Session& session,
                        ProductAppWindowState& window,
                        std::string_view source) {
  const SessionResetResult reset = session.resetToBaseline();
  clearProductTargetProof(window);
  clearProductOutcomeProof(window);
  window.gameplay.gameplayInputUsed = true;
  window.gameplay.gameplayInputSource = std::string(source);
  window.gameplay.gameplayCommand.kind = "reset";
  window.gameplay.gameplayCommand.submitted = true;
  window.gameplay.gameplayCommand.accepted = reset.reset;
  window.gameplay.gameplayCommand.status = reset.reset ? "accepted" : "rejected";  // branch-gate: BG-1155
}

}  // namespace iggy3d
