#include "app/iggy3d/world/ProductLaunchState.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"

namespace iggy3d {

void clearProductGameplayLaunchState(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window) {
  window.gameplay.gameplayActive = false;
  window.gameplay.runtimeSessionCreated = false;
  activeRoom(window) = {};
  activeRoomCollision(window) = {};
  bumpActiveRoomRevision(window);
  activeSession.reset();
}

}  // namespace iggy3d
