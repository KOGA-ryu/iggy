#include "app/iggy3d/Operations.hpp"

#include <chrono>
#include <filesystem>
#include <string>
#include <utility>

#include "app/frontend/SaveBrowser.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/creative/BakedActiveRoomRefresh.hpp"
#include "app/iggy3d/creative/CreativeWorldOperations.hpp"
#include "app/iggy3d/view/CreativeFlyAnchorStore.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"

namespace iggy3d {

namespace {

std::uint64_t elapsedMicroseconds(
    std::chrono::steady_clock::time_point started) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - started)
          .count());
}

// F0 (blank stage): a CREATIVE world must stand on its own empty canvas, not
// the first_room demo ("Loop Keep") that createProductSession installs. Build a
// minimal session that seeds ONLY a local player at the world origin and leaves
// active-room state empty, so no demo room geometry projects. The map_maker grid
// (surfaced for the creative-document surface in ProjectionRefresh) draws the
// visible ground grid around the origin. Product / LegacyMapMaker launches
// keep calling createProductSession untouched.
bool createCreativeBlankSessionImpl(std::optional<Session>& activeSession,
                                    ProductAppWindowState& window) {
  const auto lookupStarted = std::chrono::steady_clock::now();
  window.frontendShell.startup.packagePath = "creative_blank_stage";
  window.frontendShell.startup.packageLookupMeasured = true;
  window.frontendShell.startup.packageLookupMicroseconds = elapsedMicroseconds(lookupStarted);
  window.frontendShell.startup.packageLookupStatus = "startup_package_lookup_resolved";

  const auto loadStarted = std::chrono::steady_clock::now();
  window.frontendShell.packageLoadStatus = "ok";
  window.frontendShell.startup.packageLoadMeasured = true;
  window.frontendShell.startup.packageLoadMicroseconds = elapsedMicroseconds(loadStarted);
  window.frontendShell.startup.packageLoadStatus = "ok";

  const auto sessionStarted = std::chrono::steady_clock::now();
  FixtureScenarioSeed seed;
  seed.scenarioId = "creative_blank";
  ScenarioEntitySeed player;
  player.stableName = "player";
  player.kind = EntityKind::Player;
  player.transform = identityTransform3();
  player.localBounds = makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  player.active = true;
  player.persistent = true;
  seed.players.push_back({0, PlayerSlotKind::Local, "player"});
  seed.entities.push_back(std::move(player));
  // Session::create requires at least one objective. A creative stage has no
  // gameplay goal, so seed a single inert objective: condition "None" +
  // initialStatus Active never self-completes (only an ObjectiveTrigger would),
  // so the session stays Playing and never finalizes an outcome.
  ScenarioObjectiveSeed stageObjective;
  stageObjective.id = "creative_blank_stage";
  stageObjective.initialStatus = ObjectiveStatusSeed::Active;
  stageObjective.condition = "None";
  stageObjective.playerSlot = 0;
  seed.objectives.push_back(std::move(stageObjective));

  SessionCreateRequest create;
  create.packageId = "iggy3d.creative_blank";
  create.seed = seed;
  create.config = seed.config;

  Result<Session> session = Session::create(create);
  if (session.status != ResultStatus::Ok) {
    window.frontendShell.launchStatus =
        session.error.code.empty() ? "session_create_failed" : session.error.code;
    window.frontendShell.startup.runtimeSessionCreateMeasured = true;
    window.frontendShell.startup.runtimeSessionCreateMicroseconds =
        elapsedMicroseconds(sessionStarted);
    window.frontendShell.startup.runtimeSessionCreateStatus = window.frontendShell.launchStatus;
    return false;
  }
  window.frontendShell.startup.runtimeSessionCreateMeasured = true;
  window.frontendShell.startup.runtimeSessionCreateMicroseconds =
      elapsedMicroseconds(sessionStarted);
  window.frontendShell.startup.runtimeSessionCreateStatus = "startup_runtime_session_created";

  activeRoom(window) = {};
  activeRoomCollision(window) = {};
  bumpActiveRoomRevision(window);
  activeSession = std::move(session.value);
  window.gameplay.runtimeSessionCreated = true;
  window.gameplay.gameplayActive = true;
  window.frontendShell.launchStatus = "runtime_session_created";
  return true;
}

// F0: place the creative fly camera on the world origin and pitch it down so
// the origin ground grid (where objects will be created) is framed on entry.
void frameCreativeStageCameraOnOriginImpl(ProductAppWindowState& window) {
  bumpCreativeWorldEpoch(window);
  seedCreativeFlyAnchorFromOrigin(window);
  window.viewport.cameraYawDegrees = 0.0F;
  window.viewport.cameraPitchDegrees = -30.0F;
}

void clearProductGameplayLaunchStateImpl(std::optional<Session>& activeSession,
                                         ProductAppWindowState& window) {
  window.gameplay.gameplayActive = false;
  window.gameplay.runtimeSessionCreated = false;
  activeRoom(window) = {};
  activeRoomCollision(window) = {};
  bumpActiveRoomRevision(window);
  activeSession.reset();
}

}  // namespace

bool createCreativeBlankSession(std::optional<Session>& activeSession,
                                ProductAppWindowState& window) {
  return createCreativeBlankSessionImpl(activeSession, window);
}

void frameCreativeStageCameraOnOrigin(ProductAppWindowState& window) {
  frameCreativeStageCameraOnOriginImpl(window);
}

void clearProductGameplayLaunchState(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window) {
  clearProductGameplayLaunchStateImpl(activeSession, window);
}

}  // namespace iggy3d
