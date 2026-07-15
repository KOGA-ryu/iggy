#include "EditorDesktopCommands.hpp"
#include "EditorPlayMode.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/render/CreativeSceneFrame.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "render/FrameInput.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool addObject(cr::CreativeDocument& document,
               cr::CreativeObjectKind kind,
               std::string name,
               cr::CreativeVec3 position,
               std::optional<cr::CreativeBounds> bounds = std::nullopt) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.transform.position = position;
  request.hasTransformOverride = true;
  if (bounds.has_value()) {
    request.bounds = *bounds;
    request.hasBoundsOverride = true;
  }
  return document.createObject(request).accepted;
}

cr::CreativeDocument playableDocument(bool includeActors = true,
                                      cr::CreativeDocumentId id = 901U) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Editor Play");
  static_cast<void>(document.assignId(id));
  static_cast<void>(addObject(
      document, cr::CreativeObjectKind::Floor, "Play Floor", {0.0, 0.0, 0.0},
      cr::CreativeBounds{{-8.0, 0.0, -8.0}, {8.0, 0.25, 8.0}}));
  static_cast<void>(addObject(document, cr::CreativeObjectKind::SpawnPoint,
                              "Player Spawn", {0.0, 0.25, 0.0}));
  if (includeActors) {
    static_cast<void>(addObject(document, cr::CreativeObjectKind::NpcSpawn,
                                "Friendly NPC", {3.0, 0.25, 0.0}));
    static_cast<void>(addObject(document, cr::CreativeObjectKind::EnemySpawn,
                                "Hostile Monster", {-3.0, 0.25, 0.0}));
  }
  return document;
}

app::CreativeEditorPlayStartReceipt start(
    app::CreativeEditorPlayMode& mode,
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog& catalog) {
  app::CreativeEditorPlayStartRequest request;
  request.document = &document;
  request.staticMeshAssetCatalog = &catalog;
  return app::startCreativeEditorPlayMode(mode, std::move(request));
}

bool startStopAndProjectionPreserveAuthoredDocument() {
  cr::CreativeDocument document = playableDocument();
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativeEditorPlayMode mode;
  const std::uint64_t revision = document.revision();
  const std::size_t objectCount = document.objectCount();

  const app::CreativeEditorPlayStartReceipt started =
      start(mode, document, catalog);
  const app::CreativeEditorPlayScene projected =
      app::buildCreativeEditorPlayScene(mode);
  const std::size_t npcCount = static_cast<std::size_t>(std::count_if(
      projected.scene.items.begin(), projected.scene.items.end(),
      [](const iggy3d::SceneItem& item) {
        return item.kind == iggy3d::SceneItemKind::Npc;
      }));
  const iggy3d::DebugProjectionResult debug;
  const iggy3d::FrameInput frame = iggy3d::makeCreativeVulkanFrame(
      projected.scene, debug, 1U, 1280U, 720U, mode.cameraYawDegrees,
      mode.cameraPitchDegrees, true, projected.cameraAnchorMeters);
  const cr::CreativeRuntimeSandboxStopReceipt stopped =
      app::stopCreativeEditorPlayMode(mode);

  return expect(started.accepted &&
                    started.status == app::CreativeEditorPlayStartStatus::Started &&
                    app::toString(started.status) == "started",
                "valid authored map starts play") &&
         expect(projected.available && projected.scene.room.loaded &&
                    projected.scene.playerCount == 1U && npcCount == 2U,
                "play scene projects room, player, npc, and monster") &&
         expect(iggy3d::validateFrameInput(frame) ==
                        iggy3d::FrameInputStatus::Valid &&
                    iggy3d::nearlyEqual(
                        frame.camera.worldEye,
                        projected.cameraAnchorMeters +
                            iggy3d::Vec3{0.0F, 1.7F, 0.0F}),
                "runtime projection produces a valid player-follow frame") &&
         expect(document.revision() == revision &&
                    document.objectCount() == objectCount,
                "start and projection do not mutate authored content") &&
         expect(stopped.stopped && !app::creativeEditorPlayModeActive(mode),
                "stop destroys only the runtime sandbox");
}

bool fixedTickMovementAndCatchUpAreBounded() {
  cr::CreativeDocument document = playableDocument(false, 902U);
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativeEditorPlayMode mode;
  const app::CreativeEditorPlayStartReceipt started =
      start(mode, document, catalog);
  if (!started.accepted || !mode.sandbox.has_value()) {
    return expect(false, "movement setup starts play");
  }
  const std::uint64_t revision = document.revision();
  const iggy3d::EntityState* before =
      mode.sandbox->session.state().world.findById({1U});
  if (before == nullptr) {
    return expect(false, "movement setup has local player");
  }
  const iggy3d::Vec3 startPosition = before->transform.position;

  app::CreativeEditorPlayTickRequest tick;
  tick.sourceDocument = &document;
  tick.input.moveForward = 1.0F;
  tick.monotonicTimeNanoseconds = 1U;
  const app::CreativeEditorPlayTickReceipt primed =
      app::tickCreativeEditorPlayMode(mode, tick);
  tick.monotonicTimeNanoseconds = 50'000'000U;
  const app::CreativeEditorPlayTickReceipt early =
      app::tickCreativeEditorPlayMode(mode, tick);
  tick.monotonicTimeNanoseconds = 50'000'001U;
  const app::CreativeEditorPlayTickReceipt first =
      app::tickCreativeEditorPlayMode(mode, tick);
  const iggy3d::EntityState* afterFirst =
      mode.sandbox->session.state().world.findById({1U});
  const bool movedForward =
      afterFirst != nullptr && afterFirst->transform.position.z < startPosition.z;

  tick.monotonicTimeNanoseconds = 2'050'000'001U;
  const app::CreativeEditorPlayTickReceipt catchUp =
      app::tickCreativeEditorPlayMode(mode, tick);

  return expect(primed.status == app::CreativeEditorPlayTickStatus::ClockPrimed &&
                    primed.ticksAdvanced == 0U,
                "first play frame only primes the fixed-step clock") &&
         expect(early.status == app::CreativeEditorPlayTickStatus::NoTickDue &&
                    early.ticksAdvanced == 0U,
                "49,999,999 ns does not advance a 20 Hz tick") &&
         expect(first.status == app::CreativeEditorPlayTickStatus::Advanced &&
                    first.ticksAdvanced == 1U &&
                    first.movementCommandsSubmitted == 1U && movedForward,
                "50 ms submits one collision-backed move") &&
         expect(catchUp.status == app::CreativeEditorPlayTickStatus::Advanced &&
                    catchUp.ticksAdvanced == mode.tuning.maximumCatchUpTicks,
                "long frame is capped by the catch-up bound") &&
         expect(document.revision() == revision,
                "runtime movement leaves authored revision unchanged");
}

bool idleTicksAdvanceAndStaleDocumentsStop() {
  cr::CreativeDocument document = playableDocument(false, 903U);
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativeEditorPlayMode mode;
  if (!start(mode, document, catalog).accepted) {
    return expect(false, "idle setup starts play");
  }

  app::CreativeEditorPlayTickRequest tick;
  tick.sourceDocument = &document;
  tick.monotonicTimeNanoseconds = 10U;
  static_cast<void>(app::tickCreativeEditorPlayMode(mode, tick));
  tick.monotonicTimeNanoseconds = 50'000'010U;
  const app::CreativeEditorPlayTickReceipt idle =
      app::tickCreativeEditorPlayMode(mode, tick);

  static_cast<void>(addObject(document, cr::CreativeObjectKind::Prop,
                              "Authored During Play", {1.0, 0.25, 1.0}));
  tick.monotonicTimeNanoseconds = 100'000'010U;
  const app::CreativeEditorPlayTickReceipt stale =
      app::tickCreativeEditorPlayMode(mode, tick);

  return expect(idle.status == app::CreativeEditorPlayTickStatus::Advanced &&
                    idle.commandsSubmitted == 1U &&
                    idle.movementCommandsSubmitted == 0U &&
                    idle.sourceTick > 0U,
                "idle frame submits Wait so runtime systems advance") &&
         expect(stale.status ==
                        app::CreativeEditorPlayTickStatus::SourceDocumentChanged &&
                    !stale.active && !app::creativeEditorPlayModeActive(mode),
                "changed source document invalidates and stops sandbox");
}

bool desktopPlayTogglesAndBlocksEditing() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = playableDocument(false, 904U);
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const std::uint64_t revision = appState.facade.document().revision();
  app::CreativeEditorState editor;
  app::CreativeEditorPlayMode mode;
  iggy3d::StaticMeshAssetCatalog catalog;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, &saveId, &mode, &catalog};

  app::CreativeDesktopCommandFrame frame;
  frame.push(app::CreativeDesktopCommandId::Play);
  const app::CreativeDesktopCommandResult started =
      app::dispatchCreativeDesktopCommands(frame, context);
  const bool activeAfterStart = app::creativeEditorPlayModeActive(mode);
  frame.clear();
  frame.push(app::CreativeDesktopCommandId::NewDocument);
  const app::CreativeDesktopCommandResult blocked =
      app::dispatchCreativeDesktopCommands(frame, context);
  frame.clear();
  frame.push(app::CreativeDesktopCommandId::Play);
  const app::CreativeDesktopCommandResult stopped =
      app::dispatchCreativeDesktopCommands(frame, context);
  const bool inactiveAfterStop = !app::creativeEditorPlayModeActive(mode);

  return expect(started.accepted && activeAfterStart,
                "desktop Play starts the runtime owner") &&
         expect(!blocked.accepted && !blocked.documentReplaced &&
                    appState.facade.document().revision() == revision,
                "desktop mutations are rejected while play owns the frame") &&
         expect(stopped.accepted && inactiveAfterStop,
                "desktop Play toggles to Stop");
}

bool invalidMapAndTuningFailClosed() {
  cr::CreativeDocument invalid = cr::CreativeDocument::create("Invalid Play");
  static_cast<void>(invalid.assignId(905U));
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativeEditorPlayMode mode;
  const app::CreativeEditorPlayStartReceipt missingSpawn =
      start(mode, invalid, catalog);

  cr::CreativeDocument valid = playableDocument(false, 906U);
  app::CreativeEditorPlayStartRequest badTuning;
  badTuning.document = &valid;
  badTuning.staticMeshAssetCatalog = &catalog;
  badTuning.tuning.maximumCatchUpTicks = 0U;
  const app::CreativeEditorPlayStartReceipt invalidTuning =
      app::startCreativeEditorPlayMode(mode, std::move(badTuning));

  return expect(!missingSpawn.accepted &&
                    missingSpawn.status ==
                        app::CreativeEditorPlayStartStatus::PreparationRejected &&
                    !app::creativeEditorPlayModeActive(mode),
                "invalid map never creates a runtime sandbox") &&
         expect(!invalidTuning.accepted &&
                    invalidTuning.status ==
                        app::CreativeEditorPlayStartStatus::InvalidTuning &&
                    !app::creativeEditorPlayModeActive(mode),
                "invalid fixed-step tuning fails before preparation");
}

}  // namespace

int main() {
  const bool ok = startStopAndProjectionPreserveAuthoredDocument() &&
                  fixedTickMovementAndCatchUpAreBounded() &&
                  idleTicksAdvanceAndStaleDocumentsStop() &&
                  desktopPlayTogglesAndBlocksEditing() &&
                  invalidMapAndTuningFailClosed();
  if (!ok) {
    return 1;
  }
  std::cout << "creative_editor_play_mode_tests: PASS\n";
  return 0;
}
