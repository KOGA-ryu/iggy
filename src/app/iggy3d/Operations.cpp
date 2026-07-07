#include "app/iggy3d/Operations.hpp"

#include <chrono>
#include <filesystem>
#include <string>
#include <utility>

#include "app/PackageRuntimeLookup.hpp"
#include "app/frontend/SaveBrowser.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/CreativeReasoningActivation.hpp"
#include "app/iggy3d/ascii_room/Authoring.hpp"
#include "app/iggy3d/ascii_room/Package.hpp"
#include "app/iggy3d/ascii_room/Preview.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/world/PackageSessionSeed.hpp"
#include "app/iggy3d/save/RoomMarkerBinding.hpp"
#include "app/iggy3d/view/CreativeFlyAnchorStore.hpp"
#include "app/iggy3d/world/Creation.hpp"
#include "content/PackageLoader.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "render/RenderDiagnostics.hpp"

namespace iggy3d {

namespace {

constexpr const char* kCreativeRoomBakeNoRenderableObjects =
    "creative_room_bake_no_renderable_objects";
constexpr const char* kProductCreativeBakedRoomClearedNoRenderableObjects =
    "product_creative_baked_room_cleared_no_renderable_objects";

std::string packageLoadStatusName(PackageLoadStatus status) {
  switch (status) {
    case PackageLoadStatus::Ok:
      return "ok";
    case PackageLoadStatus::MissingPackageFile:
      return "missing_package_file";
    case PackageLoadStatus::PackageReadFailed:
      return "package_read_failed";
    case PackageLoadStatus::ScenarioReadFailed:
      return "scenario_read_failed";
    case PackageLoadStatus::ParseError:
      return "parse_error";
    case PackageLoadStatus::UnsupportedKey:
      return "unsupported_key";
    case PackageLoadStatus::MissingRequiredKey:
      return "missing_required_key";
    case PackageLoadStatus::MissingScenarioId:
      return "missing_scenario_id";
    case PackageLoadStatus::InvalidNumber:
      return "invalid_number";
    case PackageLoadStatus::InvalidEnum:
      return "invalid_enum";
    case PackageLoadStatus::InvalidPath:
      return "invalid_path";
  }
  return "unknown";
}

std::uint64_t elapsedMicroseconds(
    std::chrono::steady_clock::time_point started) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - started)
          .count());
}

std::filesystem::path defaultProductPackagePath(const ProductAppOptions& options) {
  if (!options.devPackageOverride.empty()) {
    return options.devPackageOverride;
  }

  PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = PackageMode::BuildTreeProduct;
  lookupConfig.requireGraphicsRuntime = false;
  lookupConfig.requireShaderRoot = false;
  const PackageLookupResult lookup = resolvePackageRuntimeLookup(lookupConfig);
  if (lookup.outcome == RenderOutcome::Ok && !lookup.lookup.resourceRoot.empty()) {
    return lookup.lookup.resourceRoot / "demos" / "first_room" /
           "package.iggy3d.toml";
  }

  return std::filesystem::path{"fixtures"} / "demos" / "first_room" /
         "package.iggy3d.toml";
}

ProductAsciiRoomAuthoringRequest productAsciiRoomAuthoringRequestFromWorldSetup(
    const WorldSetupDraft& draft) {
  return productWorldSetupAuthoringRequest(draft);
}

bool createProductSessionFromPackage(const PackageLoadResult& package,
                                     std::optional<Session>& activeSession,
                                     ProductAppWindowState& window) {
  window.packageLoadStatus = packageLoadStatusName(package.status);
  if (package.status != PackageLoadStatus::Ok) {
    window.startup.runtimeSessionCreateMeasured = false;
    window.startup.runtimeSessionCreateMicroseconds = 0;
    window.startup.runtimeSessionCreateStatus = "package_load_failed";
    window.launchStatus = "package_load_failed";
    return false;
  }

  const auto sessionStarted = std::chrono::steady_clock::now();
  const ProductPackageSessionSeedResult seed =
      buildProductPackageSessionSeed(package);
  if (!seed.ok) {
    window.startup.runtimeSessionCreateMeasured = true;
    window.startup.runtimeSessionCreateMicroseconds =
        elapsedMicroseconds(sessionStarted);
    window.startup.runtimeSessionCreateStatus = seed.reasonCode;
    window.launchStatus = seed.reasonCode;
    return false;
  }

  SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = seed.seed;
  create.config = seed.seed.config;

  Result<Session> session = Session::create(create);
  if (session.status != ResultStatus::Ok) {
    window.launchStatus =
        session.error.code.empty() ? "session_create_failed" : session.error.code;
    window.startup.runtimeSessionCreateMeasured = true;
    window.startup.runtimeSessionCreateMicroseconds =
        elapsedMicroseconds(sessionStarted);
    window.startup.runtimeSessionCreateStatus = window.launchStatus;
    return false;
  }
  window.startup.runtimeSessionCreateMeasured = true;
  window.startup.runtimeSessionCreateMicroseconds =
      elapsedMicroseconds(sessionStarted);
  window.startup.runtimeSessionCreateStatus =
      "startup_runtime_session_created";

  activeRoom(window) = {};
  activeRoomCollision(window) = {};
  activeSession = std::move(session.value);
  if (!package.rooms.empty()) {
    activeRoom(window) = buildProductActiveRoomFromPackageRoom(
        package.rooms.front(), package.manifest.packageId, package.scenario.scenarioId);
  }
  bumpActiveRoomRevision(window);
  (void)ensureActiveRoomCollisionFresh(window, &*activeSession);
  window.runtimeSessionCreated = true;
  window.gameplayActive = true;
  window.runtimeStateHash = activeSession->stateHash();
  window.launchStatus = "runtime_session_created";
  return true;
}

bool createProductSession(const ProductAppOptions& options,
                          std::optional<Session>& activeSession,
                          ProductAppWindowState& window) {
  const auto lookupStarted = std::chrono::steady_clock::now();
  const std::filesystem::path packagePath = defaultProductPackagePath(options);
  window.startup.packagePath =
      packagePath.empty() ? "none" : packagePath.generic_string();
  window.startup.packageLookupMeasured = true;
  window.startup.packageLookupMicroseconds =
      elapsedMicroseconds(lookupStarted);
  window.startup.packageLookupStatus = "startup_package_lookup_resolved";

  const auto loadStarted = std::chrono::steady_clock::now();
  const PackageLoadResult package = loadPackage({packagePath.generic_string()});
  window.startup.packageLoadMeasured = true;
  window.startup.packageLoadMicroseconds = elapsedMicroseconds(loadStarted);
  window.startup.packageLoadStatus = packageLoadStatusName(package.status);
  return createProductSessionFromPackage(package, activeSession, window);
}

// F0 (blank stage): a CREATIVE world must stand on its own empty canvas, not
// the first_room demo ("Loop Keep") that createProductSession installs. Build a
// minimal session that seeds ONLY a local player at the world origin and leaves
// active-room state empty, so no demo room geometry projects. The map_maker grid
// (surfaced for the creative-document surface in ProjectionRefresh) draws the
// visible ground grid around the origin. Product / LegacyMapMaker launches
// keep calling createProductSession untouched.
bool createCreativeBlankSession(std::optional<Session>& activeSession,
                                ProductAppWindowState& window) {
  const auto lookupStarted = std::chrono::steady_clock::now();
  window.startup.packagePath = "creative_blank_stage";
  window.startup.packageLookupMeasured = true;
  window.startup.packageLookupMicroseconds = elapsedMicroseconds(lookupStarted);
  window.startup.packageLookupStatus = "startup_package_lookup_resolved";

  const auto loadStarted = std::chrono::steady_clock::now();
  window.packageLoadStatus = "ok";
  window.startup.packageLoadMeasured = true;
  window.startup.packageLoadMicroseconds = elapsedMicroseconds(loadStarted);
  window.startup.packageLoadStatus = "ok";

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
    window.launchStatus =
        session.error.code.empty() ? "session_create_failed" : session.error.code;
    window.startup.runtimeSessionCreateMeasured = true;
    window.startup.runtimeSessionCreateMicroseconds =
        elapsedMicroseconds(sessionStarted);
    window.startup.runtimeSessionCreateStatus = window.launchStatus;
    return false;
  }
  window.startup.runtimeSessionCreateMeasured = true;
  window.startup.runtimeSessionCreateMicroseconds =
      elapsedMicroseconds(sessionStarted);
  window.startup.runtimeSessionCreateStatus = "startup_runtime_session_created";

  activeRoom(window) = {};
  activeRoomCollision(window) = {};
  bumpActiveRoomRevision(window);
  activeSession = std::move(session.value);
  window.runtimeSessionCreated = true;
  window.gameplayActive = true;
  window.runtimeStateHash = activeSession->stateHash();
  window.launchStatus = "runtime_session_created";
  return true;
}

// F0: place the creative fly camera on the world origin and pitch it down so
// the origin ground grid (where objects will be created) is framed on entry.
void frameCreativeStageCameraOnOrigin(ProductAppWindowState& window) {
  bumpCreativeWorldEpoch(window);
  seedCreativeFlyAnchorFromOrigin(window);
  window.viewport.cameraYawDegrees = 0.0F;
  window.viewport.cameraPitchDegrees = -30.0F;
}

ProductWorldCreationResult prepareProductWorldCreationFromDraft(
    const ProductAppOptions& options,
    const ProductWorldTemplate& world,
    const WorldSetupDraft& draft,
    ProductAppWindowState& window) {
  window.worldSetup.title = draft.worldName;
  window.worldSetup.dungeonTitle = draft.worldName;
  window.worldSetup.dungeonCount = productBuiltinDungeonCatalog().size();
  const std::size_t dungeonIndex =
      productBuiltinDungeonIndexForRoomId(draft.asciiRoomId);
  window.worldSetup.dungeonIndex = 0;
  // branch-gate: BG-1137
  if (dungeonIndex < productBuiltinDungeonCatalog().size()) {
    window.worldSetup.dungeonIndex =
        static_cast<std::uint64_t>(dungeonIndex + 1U);
  }
  window.worldSetup.asciiRoomEnabled = draft.asciiRoomEnabled;
  window.worldSetup.asciiRoomTextPresent = !draft.asciiRoomText.empty();
  window.worldSetup.asciiRoomId =
      draft.asciiRoomId.empty() ? "none" : draft.asciiRoomId;
  window.worldSetup.asciiRoomSourceName =
      draft.asciiRoomSourceName.empty() ? "none" : draft.asciiRoomSourceName;
  const WorldSetupRouteResult setup =
      routeWorldSetupAction(draft, FrontendAction::CreateAndEnter);
  if (!setup.accepted || !setup.createRequested) {
    window.worldSetup.status = std::string(setup.reasonCode);
    window.worldCreation.status = std::string(setup.status);
    window.worldCreation.reasonCode = std::string(setup.reasonCode);
    return {};
  }
  window.worldSetup.title = setup.createRequest.worldName;
  window.worldSetup.status = std::string(setup.status);

  // createdAtUtc/savedAtUtc seed for the initial save: a real UTC timestamp so
  // the catalog's newest-first ordering and Continue policy reflect real
  // creation/save recency across worlds. The world id is minted unique per
  // world (the first world in an empty save root is world_0001) and preserved
  // across re-saves by carry-forward in the durable writer.
  ProductWorldCreationResult creation = prepareProductWorldCreation(
      makeProductWorldCreationInput(setup.createRequest,
                                    world,
                                    options.saveRoot,
                                    productSaveTimestampNowUtc(),
                                    nextProductWorldId(options.saveRoot)));
  window.worldCreation.status = std::string(creation.status);
  window.worldCreation.reasonCode = std::string(creation.reasonCode);
  window.worldCreation.worldId =
      creation.request.worldId.empty() ? "none" : creation.request.worldId;
  window.worldCreation.worldTitle =
      creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : creation.initialSavePlan.worldTitle;
  window.worldCreation.asciiRoomRequested = creation.request.asciiRoomRequested;
  window.worldCreation.asciiRoomId =
      creation.request.asciiRoomId.empty() ? "none" : creation.request.asciiRoomId;
  window.worldCreation.asciiRoomSourceName =
      creation.request.asciiRoomSourceName.empty()
          ? "none"
          : creation.request.asciiRoomSourceName;
  window.worldCreation.initialSaveRequested = creation.initialSavePlan.requested;
  window.worldCreation.initialSaveWritten = creation.initialSaveWritten;
  window.worldCreation.initialSaveId =
      creation.initialSavePlan.saveId.empty() ? "none"
                                              : creation.initialSavePlan.saveId;
  window.worldCreation.initialSaveTitle =
      creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : creation.initialSavePlan.worldTitle;
  window.worldCreation.routeAfterCreate = std::string(creation.routeAfterCreate);
  return creation;
}

void recordProductWorldInitialSaveResult(
    const ProductWorldInitialSaveResult& initialSave,
    ProductAppWindowState& window) {
  window.worldCreation.status = initialSave.status;
  window.worldCreation.reasonCode = initialSave.reasonCode;
  window.worldCreation.worldId = initialSave.creation.request.worldId.empty()
                                    ? "none"
                                    : initialSave.creation.request.worldId;
  window.worldCreation.worldTitle =
      initialSave.creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : initialSave.creation.initialSavePlan.worldTitle;
  window.worldCreation.asciiRoomRequested =
      initialSave.creation.request.asciiRoomRequested;
  window.worldCreation.asciiRoomId =
      initialSave.creation.request.asciiRoomId.empty()
          ? "none"
          : initialSave.creation.request.asciiRoomId;
  window.worldCreation.asciiRoomSourceName =
      initialSave.creation.request.asciiRoomSourceName.empty()
          ? "none"
          : initialSave.creation.request.asciiRoomSourceName;
  window.worldCreation.initialSaveRequested =
      initialSave.creation.initialSavePlan.requested;
  window.worldCreation.initialSaveWritten =
      initialSave.creation.initialSaveWritten;
  window.worldCreation.initialSaveId =
      initialSave.creation.initialSavePlan.saveId.empty()
          ? "none"
          : initialSave.creation.initialSavePlan.saveId;
  window.worldCreation.initialSaveTitle =
      initialSave.creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : initialSave.creation.initialSavePlan.worldTitle;
  window.worldCreation.routeAfterCreate =
      std::string(initialSave.creation.routeAfterCreate);
  window.saveSession.productSaveStatus = initialSave.saveWrite.status;
  window.saveSession.productSaveReasonCode = initialSave.saveWrite.reasonCode;
  window.saveSession.productSaveDurableReason = initialSave.saveWrite.durableReason;
  window.saveSession.productSaveSource = "initial_world";
  window.saveSession.productSaveSaveId = initialSave.saveWrite.record.id.empty()
                                 ? "none"
                                 : initialSave.saveWrite.record.id;
  window.saveSession.productSaveSessionSaved = initialSave.saveWrite.ok;
  if (initialSave.saveWrite.ok && !initialSave.saveWrite.record.id.empty()) {
    window.saveSession.activeProductSaveId = initialSave.saveWrite.record.id;
  }
}

void recordProductSaveLoadResult(const ProductSaveLoadResult& loaded,
                                 ProductAppWindowState& window) {
  window.saveSession.productSaveLoadResult = loaded;
  // Normalize an empty save id to "none" at the producer so ReceiptBuilder
  // stays a straight read of the typed result (preserves the prior receipt).
  if (window.saveSession.productSaveLoadResult.record.id.empty()) {
    window.saveSession.productSaveLoadResult.record.id = "none";
  }
  if (loaded.ok && !loaded.record.id.empty()) {
    window.saveSession.activeProductSaveId = loaded.record.id;
  }
}

void recordSavedRoomMarkerBindingResult(
    const ProductSavedRoomMarkerBindingResult& bound,
    ProductAppWindowState& window) {
  window.saveSession.savedMarkerBind = bound;
  // Normalize an empty room id to "none" at the producer so ReceiptBuilder
  // stays a straight read of the typed result (preserves the prior receipt).
  if (window.saveSession.savedMarkerBind.roomId.empty()) {
    window.saveSession.savedMarkerBind.roomId = "none";
  }
}

void clearProductGameplayLaunchState(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window) {
  window.gameplayActive = false;
  window.runtimeSessionCreated = false;
  window.runtimeStateHash = 0;
  activeRoom(window) = {};
  activeRoomCollision(window) = {};
  bumpActiveRoomRevision(window);
  activeSession.reset();
}

void recordProductSaveWriteResult(std::string_view source,
                                  const ProductSaveWriteResult& written,
                                  ProductAppWindowState& window) {
  window.saveSession.productSaveStatus = written.status;
  window.saveSession.productSaveReasonCode = written.reasonCode;
  window.saveSession.productSaveDurableReason = written.durableReason;
  window.saveSession.productSaveSource = std::string(source);
  window.saveSession.productSaveSaveId = written.record.id.empty() ? "none" : written.record.id;
  window.saveSession.productSaveSessionSaved = written.ok;
  if (written.ok && !written.record.id.empty()) {
    window.saveSession.activeProductSaveId = written.record.id;
  }
}

void recordProductSaveLoadSelection(std::string_view source,
                                    const SaveSlotPreview* slot,
                                    ProductAppWindowState& window) {
  window.saveSession.productSaveLoadSource = std::string(source);
  window.saveSession.productSaveLoadSelectedId =
      slot == nullptr || slot->id.empty() ? "none" : slot->id;
  window.saveSession.productSaveLoadSelectedEnabled = slot != nullptr && slot->enabled;
}

const SaveSlotPreview* saveSlotById(const SaveSlotList& slots,
                                    std::string_view selectedId) {
  for (const SaveSlotPreview& slot : slots.slots) {
    if (slot.id == selectedId) {
      return &slot;
    }
  }
  return nullptr;
}

const SaveSlotPreview* firstSelectableSaveSlot(const SaveSlotList& slots) {
  for (const SaveSlotPreview& slot : slots.slots) {
    if (slot.enabled) {
      return &slot;
    }
  }
  return slots.slots.empty() ? nullptr : &slots.slots.front();
}

void recordSelectedProductSaveSlot(const SaveSlotList& slots,
                                   const SaveSlotPreview* slot,
                                   ProductAppWindowState& window) {
  const SaveSlotRingModel ring =
      // branch-gate: BG-1020
      buildSaveSlotRingModel(slots, slot == nullptr ? "none" : slot->id);
  window.saveSession.saveSlotRingCount = static_cast<std::uint64_t>(ring.items.size());
  window.saveSession.saveSlotRingSelectedIndex = ring.selectedIndex;
  window.saveSession.saveSlotRingSelectedId = ring.selectedSlotId;
  window.saveSession.saveSlotRingSelectedStatus = ring.selectedStatus;

  if (slots.slots.empty()) {
    window.saveSession.selectedProductSave.id = "none";
    window.saveSession.selectedProductSave.enabled = false;
    window.saveSession.selectedProductSave.status = "empty";
    return;
  }
  if (slot == nullptr) {
    window.saveSession.selectedProductSave.id = "none";
    window.saveSession.selectedProductSave.enabled = false;
    window.saveSession.selectedProductSave.status = "missing";
    return;
  }
  window.saveSession.selectedProductSave.id = slot->id.empty() ? "none" : slot->id;
  window.saveSession.selectedProductSave.enabled = slot->enabled;
  window.saveSession.selectedProductSave.status = slot->enabled ? "selected" : "disabled";
}

void recordProductSaveSlotAction(ProductAppWindowState& window,
                                 const SaveSlotActionSpec& action,
                                 std::string_view status) {
  window.saveSession.saveSlotActionCommand = std::string(saveSlotCommandName(action.command));
  window.saveSession.saveSlotActionEnabled = action.enabled;
  window.saveSession.saveSlotActionConfirmationRequired = action.confirmationRequired;
  window.saveSession.saveSlotActionStatus = std::string(status);
}

void recordProductSaveFlowRequest(const ProductSaveFlowRequest& request,
                                  ProductAppWindowState& window) {
  window.saveSession.saveFlow.operation =
      std::string(productSaveFlowOperationName(request.operation));
  // branch-gate: BG-1020
  window.saveSession.saveFlow.sourceSurface = request.sourceSurface.empty()
                                     ? "none"
                                     : request.sourceSurface;
  window.saveSession.saveFlow.affectedSlotId =
      // branch-gate: BG-1020
      request.slotId.empty() ? "none" : request.slotId;
}

void recordProductSaveFlowResult(ProductSaveFlowOperation operation,
                                 std::string_view sourceSurface,
                                 const ProductSaveFlowResult& result,
                                 ProductAppWindowState& window) {
  window.saveSession.saveFlow.operation =
      std::string(productSaveFlowOperationName(operation));
  // branch-gate: BG-1020
  window.saveSession.saveFlow.sourceSurface =
      sourceSurface.empty() ? "none" : std::string(sourceSurface);
  window.saveSession.saveFlow.status = result.status;
  window.saveSession.saveFlow.reasonCode = result.reason;
  window.saveSession.saveFlow.affectedSlotId =
      // branch-gate: BG-1020
      result.affectedSlotId.empty() ? "none" : result.affectedSlotId;
  window.saveSession.saveFlow.activeCountBefore = result.activeCountBefore;
  window.saveSession.saveFlow.activeCountAfter = result.activeCountAfter;
  window.saveSession.saveFlow.deletedCountAfter = result.deletedCountAfter;
  window.saveSession.saveFlow.selectedSlotAfter =
      // branch-gate: BG-1020
      result.selectedSlotAfter.empty() ? "none" : result.selectedSlotAfter;
}

void setCreativeNewWorldLaunchStatus(
    ProductCreativeNewWorldLaunchResult& result,
    std::string reason) {
  result.status = std::move(reason);
  result.reasonCode = result.status;
}

void setCreativeOpenWorldLaunchStatus(
    ProductCreativeOpenWorldLaunchResult& result,
    std::string reason) {
  result.status = std::move(reason);
  result.reasonCode = result.status;
}

void setCurrentCreativeSaveStatus(
    ProductCreativeCurrentWorldSaveResult& result,
    std::string reason) {
  result.status = std::move(reason);
  result.reasonCode = result.status;
}

void setCreativeBakedActiveRoomRefreshStatus(
    ProductCreativeBakedActiveRoomRefreshResult& result,
    std::string reason) {
  result.status = std::move(reason);
  result.reasonCode = result.status;
}

std::string fallbackString(std::string_view value, std::string_view fallback) {
  return value.empty() ? std::string(fallback) : std::string(value);
}

ProductActiveRoomState clearedCreativeBakedActiveRoom(
    const ProductCreativeBakedActiveRoomRefreshRequest& request) {
  ProductActiveRoomState activeRoom;
  activeRoom.status = kProductCreativeBakedRoomClearedNoRenderableObjects;
  activeRoom.reasonCode = kProductCreativeBakedRoomClearedNoRenderableObjects;
  activeRoom.source = "creative_room_bake";
  activeRoom.roomId = fallbackString(request.roomId, "creative_room");
  activeRoom.sourceName = fallbackString(request.sourceName, "iggy3d.creative");
  activeRoom.sourceSubset =
      fallbackString(request.sourceSubset, "creative_document_bake");
  activeRoom.room.id = activeRoom.roomId;
  activeRoom.room.version = 1;
  activeRoom.room.units = "m";
  activeRoom.room.source = "iggy3d.creative_document";
  activeRoom.room.sourceFile = activeRoom.sourceName;
  activeRoom.room.sourceSubset = activeRoom.sourceSubset;
  return activeRoom;
}

class ProductCreativeBakedRoomRefreshService {
 public:
  ProductCreativeBakedRoomRefreshService(
      const ProductCreativeBakedActiveRoomRefreshRequest& request,
      std::optional<Session>& activeSession,
      ProductAppWindowState& window,
      const creative::CreativeAppState& creativeApp)
      : request_(request),
        activeSession_(activeSession),
        window_(window),
        creativeApp_(creativeApp) {}

  ProductCreativeBakedActiveRoomRefreshResult execute() {
    const creative::CreativeDocument& document = creativeApp_.facade.document();
    result_.documentId = document.id();
    result_.objectCount = document.objectCount();

    if (!passesPreconditions(document)) {
      return result_;
    }

    const creative::CreativeRoomBakeResult bake = bakeDocument(document);
    mirrorBakeResult(bake);
    if (!bake.receipt.accepted) {
      return handleRejectedBake(document, bake);
    }

    return installBakedRoom(document, bake.room);
  }

 private:
  bool passesPreconditions(const creative::CreativeDocument& document) {
    if (window_.interactionMode != ProductInteractionMode::Creative) {
      setCreativeBakedActiveRoomRefreshStatus(
          result_,
          "product_creative_baked_room_inactive");
      return false;
    }
    if (!activeSession_.has_value()) {
      setCreativeBakedActiveRoomRefreshStatus(
          result_,
          "product_creative_baked_room_session_missing");
      return false;
    }
    if (document.id() == creative::kInvalidDocumentId || !document.isValid()) {
      setCreativeBakedActiveRoomRefreshStatus(
          result_,
          "product_creative_baked_room_document_invalid");
      return false;
    }
    return true;
  }

  creative::CreativeRoomBakeRequest bakeRequestFor(
      const creative::CreativeDocument& document) const {
    creative::CreativeRoomBakeRequest bakeRequest;
    bakeRequest.document = &document;
    bakeRequest.roomId = request_.roomId;
    bakeRequest.sourceName = request_.sourceName;
    bakeRequest.sourceSubset = request_.sourceSubset;
    bakeRequest.includeHidden = request_.includeHidden;
    return bakeRequest;
  }

  creative::CreativeRoomBakeResult bakeDocument(
      const creative::CreativeDocument& document) {
    const creative::CreativeRoomBakeRequest bakeRequest =
        bakeRequestFor(document);
    result_.bakeMeasured = true;
    result_.bakedDocumentRevision = document.revision();
    const auto bakeStarted = std::chrono::steady_clock::now();
    const creative::CreativeRoomBakeResult bake =
        creative::buildRoomAssetFromCreativeDocument(bakeRequest);
    result_.bakeElapsedMicroseconds = elapsedMicroseconds(bakeStarted);
    return bake;
  }

  void mirrorBakeResult(const creative::CreativeRoomBakeResult& bake) {
    result_.bakeReceipt = bake.receipt;
    result_.staticMeshCount =
        static_cast<std::uint64_t>(bake.room.staticMeshes.size());
    result_.anchorCount = static_cast<std::uint64_t>(bake.room.anchors.size());
    result_.spatialSurfaceCount =
        static_cast<std::uint64_t>(bake.room.spatialSurfaces.size());
    result_.staticMeshSourceCount =
        static_cast<std::uint64_t>(bake.staticMeshSources.size());
    result_.anchorSourceCount =
        static_cast<std::uint64_t>(bake.anchorSources.size());
    result_.spatialSurfaceSourceCount =
        static_cast<std::uint64_t>(bake.spatialSurfaceSources.size());
  }

  ProductCreativeBakedActiveRoomRefreshResult handleRejectedBake(
      const creative::CreativeDocument& document,
      const creative::CreativeRoomBakeResult& bake) {
    if (request_.clearOnNoRenderable &&
        bake.receipt.reasonCode == kCreativeRoomBakeNoRenderableObjects) {
      activeRoom(window_) = clearedCreativeBakedActiveRoom(request_);
      bumpActiveRoomRevision(window_);
      (void)ensureActiveRoomCollisionFresh(window_, &*activeSession_);

      result_.accepted = true;
      result_.clearedActiveRoom = true;
      mirrorActiveRoomState();
      setCreativeBakedActiveRoomRefreshStatus(
          result_,
          kProductCreativeBakedRoomClearedNoRenderableObjects);
      recordProductCreativeBakedRoomFresh(window_,
                                          document.id(),
                                          document.revision());
      return result_;
    }

    setCreativeBakedActiveRoomRefreshStatus(result_, bake.receipt.reasonCode);
    return result_;
  }

  ProductCreativeBakedActiveRoomRefreshResult installBakedRoom(
      const creative::CreativeDocument& document,
      const RoomAsset& room) {
    ProductActiveRoomState bakedActiveRoom = buildProductActiveRoomFromPackageRoom(
        room, "iggy3d.creative", "creative.document");

    activeRoom(window_) = std::move(bakedActiveRoom);
    bumpActiveRoomRevision(window_);
    (void)ensureActiveRoomCollisionFresh(window_, &*activeSession_);

    if (request_.activationHook) {
      request_.activationHook(*activeSession_,
                              activeRoom(window_).room,
                              document);
    }

    mirrorActiveRoomState();
    result_.accepted = true;
    setCreativeBakedActiveRoomRefreshStatus(
        result_,
        "product_creative_baked_room_refreshed");
    recordProductCreativeBakedRoomFresh(window_,
                                        document.id(),
                                        document.revision());
    return result_;
  }

  void mirrorActiveRoomState() {
    result_.activeRoomLoaded = activeRoom(window_).loaded;
    result_.activeRoomStatus = activeRoom(window_).status;
    result_.collisionReady = activeRoomCollision(window_).ready;
    result_.collisionQuerySurfaceCount =
        activeRoomCollision(window_).querySurfaceCount;
  }

  const ProductCreativeBakedActiveRoomRefreshRequest& request_;
  std::optional<Session>& activeSession_;
  ProductAppWindowState& window_;
  const creative::CreativeAppState& creativeApp_;
  ProductCreativeBakedActiveRoomRefreshResult result_;
};

std::string idOrNone(std::string_view value) {
  return value.empty() ? "none" : std::string(value);
}

std::string pathOrNone(const std::filesystem::path& path) {
  return path.empty() ? "none" : path.generic_string();
}

bool missingWindowIdentity(std::string_view value) {
  return value.empty() || value == "none";
}

void recordActiveCreativeSaveIdentity(
    creative::CreativeActiveIdentity& identity,
    std::string_view saveId,
    const std::filesystem::path& path,
    std::string_view worldId,
    creative::CreativeDocumentId documentId,
    std::uint64_t objectCount,
    creative::CreativeObjectId nextObjectId) {
  identity.saveId = idOrNone(saveId);
  identity.savePath = pathOrNone(path);
  identity.worldId = idOrNone(worldId);
  identity.documentId = documentId;
  identity.objectCount = objectCount;
  identity.nextObjectId = nextObjectId;
  identity.saveStatus = "creative_world_save_not_requested";
  identity.saveReasonCode = "creative_world_save_not_requested";
  identity.saveDirtyFlagsBefore = 0;
  identity.saveDirtyFlagsDrained = 0;
  identity.saveDirtyFlagsAfter = 0;
  identity.saveSavedAtUtc = "none";
}

void recordActiveCreativeSaveResult(
    creative::CreativeActiveIdentity& identity,
    const ProductCreativeCurrentWorldSaveResult& result) {
  identity.saveStatus = result.status;
  identity.saveReasonCode = result.reasonCode;
  identity.saveDirtyFlagsBefore = result.dirtyFlagsBefore;
  identity.saveDirtyFlagsDrained = result.dirtyFlagsDrained;
  identity.saveDirtyFlagsAfter = result.dirtyFlagsAfter;
  identity.saveSavedAtUtc =
      result.saveResult.savedAtUtc.empty() ? "none" : result.saveResult.savedAtUtc;
  if (result.accepted && result.saved) {
    identity.saveId = idOrNone(result.saveId);
    identity.savePath = pathOrNone(result.path);
    identity.worldId = idOrNone(result.worldId);
    identity.documentId = result.documentId;
    identity.objectCount = result.objectCount;
    identity.nextObjectId = result.nextObjectId;
    identity.saveStatus = result.status;
    identity.saveReasonCode = result.reasonCode;
    identity.saveDirtyFlagsBefore = result.dirtyFlagsBefore;
    identity.saveDirtyFlagsDrained = result.dirtyFlagsDrained;
    identity.saveDirtyFlagsAfter = result.dirtyFlagsAfter;
    identity.saveSavedAtUtc =
        result.saveResult.savedAtUtc.empty() ? "none" : result.saveResult.savedAtUtc;
  }
}

void mirrorCreativeWorldCreateResult(
    ProductCreativeNewWorldLaunchResult& result,
    const CreativeWorldCreateResult& create) {
  result.createResult = create;
  result.saveId = create.saveId.empty() ? "none" : create.saveId;
  result.path = create.path;
  result.worldId = create.worldId;
  result.documentId = create.documentId;
  result.objectCount = create.document.objectCount();
  result.nextObjectId = create.document.nextObjectId();
}

void mirrorCreativeWorldOpenResult(
    ProductCreativeOpenWorldLaunchResult& result,
    const CreativeWorldOpenResult& open) {
  result.openResult = open;
  result.saveId = open.saveId.empty() ? "none" : open.saveId;
  result.path = open.path;
  result.worldId = open.worldId;
  result.documentId = open.documentId;
  result.objectCount = open.objectCount;
  result.nextObjectId = open.nextObjectId;
}

void mirrorCreativeDocumentInstallResult(
    ProductCreativeNewWorldLaunchResult& result,
    const creative::CreativeFacadeDocumentInstallReceipt& install) {
  result.installReceipt = install;
  result.documentInstalled = install.accepted;
  if (install.accepted) {
    result.documentId = install.nextDocumentId;
    result.objectCount = install.nextObjectCount;
    result.nextObjectId = result.createResult.document.nextObjectId();
  }
}

void mirrorCreativeDocumentInstallResult(
    ProductCreativeOpenWorldLaunchResult& result,
    const creative::CreativeFacadeDocumentInstallReceipt& install) {
  result.installReceipt = install;
  result.documentInstalled = install.accepted;
  if (install.accepted) {
    result.documentId = install.nextDocumentId;
    result.objectCount = install.nextObjectCount;
    result.nextObjectId = result.openResult.document.nextObjectId();
  }
}

void mirrorCreativeBakedActiveRoomRefreshResult(
    ProductCreativeNewWorldLaunchResult& result,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh) {
  result.bakedActiveRoomRefreshRequested = true;
  result.bakedActiveRoomRefreshAccepted = refresh.accepted;
  result.bakedActiveRoomRefresh = refresh;
}

void mirrorCreativeBakedActiveRoomRefreshResult(
    ProductCreativeOpenWorldLaunchResult& result,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh) {
  result.bakedActiveRoomRefreshRequested = true;
  result.bakedActiveRoomRefreshAccepted = refresh.accepted;
  result.bakedActiveRoomRefresh = refresh;
}

void recordSelectedDeletedProductSaveSlot(const SaveSlotList& slots,
                                          const SaveSlotPreview* slot,
                                          ProductAppWindowState& window) {
  if (slots.slots.empty()) {
    window.saveSession.deletedSelectedSaveId = "none";
    window.saveSession.deletedSelectedSaveEnabled = false;
    window.saveSession.deletedSelectedSaveStatus = "empty";
    return;
  }
  if (slot == nullptr) {
    window.saveSession.deletedSelectedSaveId = "none";
    window.saveSession.deletedSelectedSaveEnabled = false;
    window.saveSession.deletedSelectedSaveStatus = "missing";
    return;
  }
  window.saveSession.deletedSelectedSaveId = slot->id.empty() ? "none" : slot->id;
  window.saveSession.deletedSelectedSaveEnabled = slot->enabled;
  window.saveSession.deletedSelectedSaveStatus = slot->enabled ? "selected" : "disabled";
}

const SaveSlotPreview* initializeSelectedDeletedProductSaveSlot(
    const SaveSlotList& slots,
    ProductAppWindowState& window) {
  const SaveSlotPreview* current =
      window.saveSession.deletedSelectedSaveId == "none"
          ? nullptr
          : saveSlotById(slots, window.saveSession.deletedSelectedSaveId);
  const SaveSlotPreview* selected =
      current == nullptr ? firstSelectableSaveSlot(slots) : current;
  recordSelectedDeletedProductSaveSlot(slots, selected, window);
  return selected;
}

}  // namespace

std::string_view productSaveFlowOperationName(ProductSaveFlowOperation operation) {
  // branch-gate: BG-1020
  switch (operation) {
    case ProductSaveFlowOperation::None:
      return "none";
    case ProductSaveFlowOperation::Delete:
      return "delete";
  }
  return "none";
}

ProductWorldTemplate productWorldTemplateFromOptions(
    const ProductAppOptions& options) {
  ProductWorldTemplate world =
      options.devPackageOverride.empty()
          ? defaultProductWorldTemplate()
          : devOverrideProductWorldTemplate(
                options.devPackageOverride.generic_string(), options.devScenario);
  const PackageLoadResult package =
      loadPackage({defaultProductPackagePath(options).generic_string()});
  if (package.status == PackageLoadStatus::Ok) {
    world.packageId = package.manifest.packageId;
    world.scenarioId = package.scenario.scenarioId;
  }
  return world;
}

ProductSaveWriteResult writeProductCurrentSessionSave(
    const ProductAppOptions& options,
    const std::optional<Session>& activeSession,
    std::string_view source,
    ProductAppWindowState& window) {
  if (!activeSession.has_value()) {
    ProductSaveWriteResult missing;
    missing.status = "product_save_session_missing";
    missing.reasonCode = "product_save_session_missing";
    missing.durableReason = "not_requested";
    recordProductSaveWriteResult(source, missing, window);
    return missing;
  }

  ProductSaveWriteRequest request;
  request.saveRoot = options.saveRoot;
  request.saveIdHint = window.saveSession.activeProductSaveId == "none"
                           ? std::string{}
                           : window.saveSession.activeProductSaveId;
  request.attemptToken = "attempt_002";
  request.state = &activeSession->state();
  if (activeRoom(window).hasAuthoredRoom) {
    request.authoredRoom = &activeRoom(window).authoredRoom;
  }
  // A pause/progress save is a manual save and advances the save time. The
  // remaining identity (worldId/worldTitle/saveTitle/createdAtUtc) is left
  // empty on purpose: writeProductSessionSaveDurably carries it forward from
  // the existing save on disk so re-saving never wipes the world identity.
  request.saveType = "manual";
  request.savedAtUtc = productSaveTimestampNowUtc();
  const ProductSaveWriteResult written = writeProductSessionSaveDurably(request);
  recordProductSaveWriteResult(source, written, window);
  return written;
}

const SaveSlotPreview* initializeSelectedProductSaveSlot(
    const SaveSlotList& slots,
    ProductAppWindowState& window) {
  const SaveSlotPreview* current =
      window.saveSession.selectedProductSave.id == "none"
          ? nullptr
          : saveSlotById(slots, window.saveSession.selectedProductSave.id);
  const SaveSlotPreview* selected =
      current == nullptr ? firstSelectableSaveSlot(slots) : current;
  recordSelectedProductSaveSlot(slots, selected, window);
  return selected;
}

const SaveSlotPreview* moveSelectedProductSaveSlot(const SaveSlotList& slots,
                                                   InputAction action,
                                                   ProductAppWindowState& window) {
  if (slots.slots.empty()) {
    recordSelectedProductSaveSlot(slots, nullptr, window);
    return nullptr;
  }

  const bool previous = action == InputAction::MenuUp;
  const std::string selectedId =
      nextSaveSlotRingSelection(slots, window.saveSession.selectedProductSave.id, previous);
  const SaveSlotPreview* selected = saveSlotById(slots, selectedId);
  recordSelectedProductSaveSlot(slots, selected, window);
  return selected;
}

bool selectProductSaveSlotById(const SaveSlotList& slots,
                               std::string_view selectedId,
                               ProductAppWindowState& window) {
  const SaveSlotPreview* slot = saveSlotById(slots, selectedId);
  recordSelectedProductSaveSlot(slots, slot, window);
  return slot != nullptr;
}

ProductSaveBridgeResult scanDeletedProductSavesForOptions(
    const ProductAppOptions& options) {
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  return scanDeletedProductSaves(options.saveRoot, world.packageId, world.scenarioId);
}

void recordDeletedProductSaveSlots(const ProductSaveBridgeResult& deletedSaves,
                                   ProductAppWindowState& window) {
  window.saveSession.deletedSaveCount =
      static_cast<std::uint64_t>(deletedSaves.slots.slots.size());
  window.saveSession.deletedCompatibleSaveCount = deletedSaves.slots.compatibleCount;
}

bool selectDeletedProductSaveSlotById(const SaveSlotList& slots,
                                      std::string_view selectedId,
                                      ProductAppWindowState& window) {
  const SaveSlotPreview* slot = saveSlotById(slots, selectedId);
  recordSelectedDeletedProductSaveSlot(slots, slot, window);
  return slot != nullptr;
}

void openDeletedProductSaveBrowser(const ProductAppOptions& options,
                                   ProductAppWindowState& window,
                                   FrontendState& frontend) {
  const ProductSaveBridgeResult deletedSaves =
      scanDeletedProductSavesForOptions(options);
  recordDeletedProductSaveSlots(deletedSaves, window);
  window.saveSession.deletedSaveBrowserOpen = true;
  initializeSelectedDeletedProductSaveSlot(deletedSaves.slots, window);
  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.saveBrowserMode = FrontendSaveBrowserMode::Load;
  window.saveSession.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
  frontend.status = "deleted_save_browser_open";
}

void executeProductSaveRecover(const ProductAppOptions& options,
                               ProductAppWindowState& window,
                               FrontendState& frontend) {
  const ProductSaveBridgeResult deletedBefore =
      scanDeletedProductSavesForOptions(options);
  recordDeletedProductSaveSlots(deletedBefore, window);
  const SaveSlotPreview* selected =
      window.saveSession.deletedSelectedSaveId == "none"
          ? initializeSelectedDeletedProductSaveSlot(deletedBefore.slots, window)
          : saveSlotById(deletedBefore.slots, window.saveSession.deletedSelectedSaveId);
  recordSelectedDeletedProductSaveSlot(deletedBefore.slots, selected, window);

  const std::string recoverId =
      selected == nullptr || selected->id.empty() ? "none" : selected->id;
  window.saveSession.saveRecover.saveId = recoverId;
  window.saveSession.saveRecover.snapshotRecovered = false;
  window.saveSession.saveRecover.snapshotMissing = false;
  if (recoverId == "none") {
    window.saveSession.saveRecover.status = "product_save_recover_id_missing";
    window.saveSession.saveRecover.reasonCode = "product_save_recover_id_missing";
    window.saveSession.saveRecover.executed = false;
    frontend.childScreen = FrontendScreen::LoadSave;
    frontend.status = "save_recover_failed";
    return;
  }

  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  const ProductSaveMutationResult mutation = recoverProductSaveAndRefresh({
      options.saveRoot,
      recoverId,
      world.packageId,
      world.scenarioId,
  });
  const ProductSaveRecoverResult& recovered = mutation.recover;
  window.saveSession.saveRecover.status = recovered.status;
  window.saveSession.saveRecover.reasonCode = recovered.reasonCode;
  window.saveSession.saveRecover.executed = recovered.ok;
  window.saveSession.saveRecover.saveId = recovered.saveId.empty() ? "none" : recovered.saveId;
  window.saveSession.saveRecover.snapshotRecovered = recovered.snapshotRecovered;
  window.saveSession.saveRecover.snapshotMissing = recovered.snapshotMissing;

  recordDeletedProductSaveSlots(mutation.deletedSaves, window);
  if (recovered.ok) {
    window.saveSession.deletedSaveBrowserOpen = false;
    recordSelectedDeletedProductSaveSlot(mutation.deletedSaves.slots, nullptr, window);
    selectProductSaveSlotById(mutation.activeSaves.slots, recovered.saveId, window);
  }

  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.selectedAction = FrontendAction::LoadSave;
  frontend.status = recovered.ok ? "save_recover_recovered" : "save_recover_failed";
}

void openProductSaveDeleteConfirmation(const SaveSlotList& slots,
                                       ProductAppWindowState& window,
                                       FrontendState& frontend) {
  frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
  window.saveSession.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
  const SaveSlotPreview* slot =
      window.saveSession.selectedProductSave.id == "none"
          ? nullptr
          : saveSlotById(slots, window.saveSession.selectedProductSave.id);
  if (slot == nullptr) {
    recordSelectedProductSaveSlot(slots, nullptr, window);
    recordProductSaveSlotAction(
        window,
        SaveSlotActionSpec{FrontendAction::Delete,
                           SaveSlotCommand::Delete,
                           "DELETE SELECTED",
                           false,
                           true,
                           "save_delete_unavailable"},
        "save_slot_action_disabled");
    window.saveSession.saveDelete.confirmationOpen = false;
    window.saveSession.saveDelete.candidateId = "none";
    window.saveSession.saveDelete.candidateEnabled = false;
    window.saveSession.saveDelete.status =
        slots.slots.empty() ? "save_delete_unavailable" : "save_delete_missing";
    window.saveSession.saveDelete.reasonCode = window.saveSession.saveDelete.status;
    window.saveSession.saveDelete.type = "soft";
    window.saveSession.saveDelete.recoverable = false;
    window.saveSession.saveDelete.executed = false;
    frontend.status = window.saveSession.saveDelete.status;
    return;
  }

  recordSelectedProductSaveSlot(slots, slot, window);
  recordProductSaveSlotAction(
      window,
      SaveSlotActionSpec{FrontendAction::Delete,
                         SaveSlotCommand::Delete,
                         "DELETE SELECTED",
                         true,
                         true,
                         "none"},
      "save_slot_action_confirm_requested");
  window.saveSession.saveDelete.confirmationOpen = true;
  window.saveSession.saveDelete.candidateId = slot->id.empty() ? "none" : slot->id;
  window.saveSession.saveDelete.candidateEnabled = slot->enabled;
  window.saveSession.saveDelete.status = "confirm_open";
  window.saveSession.saveDelete.reasonCode = "confirm_open";
  window.saveSession.saveDelete.type = "soft";
  window.saveSession.saveDelete.recoverable = false;
  window.saveSession.saveDelete.executed = false;
  frontend.childScreen = FrontendScreen::DeleteConfirm;
  frontend.selectedAction = FrontendAction::Delete;
  frontend.status = "save_delete_confirm_open";
}

void cancelProductSaveDeleteConfirmation(ProductAppWindowState& window,
                                         FrontendState& frontend) {
  window.saveSession.saveDelete.confirmationOpen = false;
  window.saveSession.saveDelete.status = "cancelled";
  window.saveSession.saveDelete.reasonCode = "cancelled";
  window.saveSession.saveDelete.type = "soft";
  window.saveSession.saveDelete.recoverable = false;
  window.saveSession.saveDelete.executed = false;
  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
  window.saveSession.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
  frontend.status = "save_delete_cancelled";
}

ProductSaveFlowResult executeProductSaveSoftDelete(
    const ProductAppOptions& options,
    ProductSaveBridgeResult& saves,
    ProductAppWindowState& window,
    FrontendState& frontend) {
  ProductSaveFlowResult flow;
  flow.activeCountBefore = static_cast<std::uint64_t>(saves.slots.slots.size());
  ProductSaveFlowRequest request;
  request.operation = ProductSaveFlowOperation::Delete;
  request.slotId = window.saveSession.saveDelete.candidateId;
  request.sourceSurface = "delete_world_browser";
  request.confirmationToken = window.saveSession.saveDelete.confirmationOpen
                                  ? "delete_confirm_open"
                                  : "delete_confirm_missing";
  recordProductSaveFlowRequest(request, window);
  window.saveSession.saveDelete.confirmationOpen = false;
  window.saveSession.saveDelete.type = "soft";
  window.saveSession.saveDelete.recoverable = false;
  if (window.saveSession.saveDelete.candidateId == "none" ||
      window.saveSession.saveDelete.candidateId.empty()) {
    window.saveSession.saveDelete.status = "product_save_delete_id_missing";
    window.saveSession.saveDelete.reasonCode = "product_save_delete_id_missing";
    window.saveSession.saveDelete.executed = false;
    frontend.childScreen = FrontendScreen::LoadSave;
    frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
    window.saveSession.saveSlotBrowserMode =
        std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
    frontend.status = "save_delete_failed";
    flow.status = window.saveSession.saveDelete.status;
    flow.reason = window.saveSession.saveDelete.reasonCode;
    flow.affectedSlotId = "none";
    flow.activeCountAfter = flow.activeCountBefore;
    flow.deletedCountAfter = window.saveSession.deletedSaveCount;
    flow.selectedSlotAfter = window.saveSession.selectedProductSave.id;
    recordProductSaveFlowResult(ProductSaveFlowOperation::Delete,
                                "delete_world_browser",
                                flow,
                                window);
    return flow;
  }

  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  const ProductSaveMutationResult mutation = softDeleteProductSaveAndRefresh({
      options.saveRoot,
      window.saveSession.saveDelete.candidateId,
      world.packageId,
      world.scenarioId,
  });
  const ProductSaveSoftDeleteResult& deleted = mutation.softDelete;
  window.saveSession.saveDelete.status = deleted.status;
  window.saveSession.saveDelete.reasonCode = deleted.reasonCode;
  window.saveSession.saveDelete.executed = deleted.ok;
  window.saveSession.saveDelete.recoverable = deleted.ok;
  flow.ok = deleted.ok;
  flow.status = deleted.status;
  flow.reason = deleted.reasonCode;
  // branch-gate: BG-1020
  flow.affectedSlotId = deleted.saveId.empty() ? "none" : deleted.saveId;
  if (deleted.ok) {
    window.saveSession.selectedProductSave.enabled = false;
    window.saveSession.selectedProductSave.status = "missing";
    saves = mutation.activeSaves;
    initializeSelectedProductSaveSlot(saves.slots, window);
  }
  recordDeletedProductSaveSlots(mutation.deletedSaves, window);
  flow.activeCountAfter = static_cast<std::uint64_t>(saves.slots.slots.size());
  flow.deletedCountAfter =
      static_cast<std::uint64_t>(mutation.deletedSaves.slots.slots.size());
  flow.selectedSlotAfter = window.saveSession.selectedProductSave.id;
  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
  window.saveSession.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
  frontend.status = deleted.ok ? "save_delete_soft_deleted" : "save_delete_failed";
  recordProductSaveFlowResult(ProductSaveFlowOperation::Delete,
                              "delete_world_browser",
                              flow,
                              window);
  return flow;
}

void launchProductNewWorld(const ProductAppOptions& options,
                           const WorldSetupDraft& worldSetupDraft,
                           FrontendState& frontend,
                           std::optional<Session>& activeSession,
                           ProductAppWindowState& window) {
  window.launchAction = "create_and_enter";
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  ProductWorldCreationResult creation =
      prepareProductWorldCreationFromDraft(options, world, worldSetupDraft, window);
  if (!creation.accepted) {
    window.launchStatus = std::string(creation.reasonCode);
    frontend.status = "opening_menu_new_world_failed";
    return;
  }

  ProductAsciiRoomAuthoringRequest asciiRequest;
  ProductAsciiRoomAuthoringResult asciiRoom;
  const SaveAuthoredRoomSection* initialSaveAuthoredRoom = nullptr;
  if (worldSetupDraft.asciiRoomEnabled) {
    asciiRequest = productAsciiRoomAuthoringRequestFromWorldSetup(worldSetupDraft);
    asciiRoom = buildProductAsciiRoomAuthoring(asciiRequest);
    recordProductAsciiRoomPreview(asciiRequest.sourceName,
                                  asciiRequest.roomId,
                                  asciiRoom,
                                  window);
    if (!asciiRoom.ok) {
      window.launchStatus = asciiRoom.reasonCode;
      frontend.status = "opening_menu_new_world_failed";
      return;
    }

    const PackageLoadResult asciiPackage =
        makeProductAsciiRoomPackage(asciiRoom.roomAsset.room,
                                    world.packageId,
                                    world.scenarioId);
    if (!createProductSessionFromPackage(asciiPackage, activeSession, window)) {
      frontend.status = "opening_menu_new_world_failed";
      return;
    }
    activeRoom(window) =
        buildProductActiveRoomFromAsciiAuthoring(asciiRequest, asciiRoom);
    bumpActiveRoomRevision(window);
    (void)ensureActiveRoomCollisionFresh(window, &*activeSession);
    initialSaveAuthoredRoom = &asciiRoom.authoredRoom.authoredRoom;
  } else if (!createProductSession(options, activeSession, window)) {
    frontend.status = "opening_menu_new_world_failed";
    return;
  }

  ProductWorldInitialSaveRequest initialSaveRequest;
  initialSaveRequest.creation = creation;
  initialSaveRequest.state = &activeSession->state();
  initialSaveRequest.authoredRoom = initialSaveAuthoredRoom;
  initialSaveRequest.attemptToken = "attempt_001";
  const ProductWorldInitialSaveResult initialSave =
      writeProductWorldInitialSaveDurably(initialSaveRequest);
  recordProductWorldInitialSaveResult(initialSave, window);
  if (!initialSave.ok) {
    window.launchStatus = initialSave.reasonCode;
    clearProductGameplayLaunchState(activeSession, window);
    frontend.status = "opening_menu_new_world_failed";
    return;
  }

  window.launchStatus = initialSave.status;
  window.interactionMode = ProductInteractionMode::Player;
  enterProductGameplayTransition(frontend, window, FrontendAction::CreateAndEnter);
}

ProductCreativeNewWorldLaunchResult launchProductCreativeNewWorld(
    const ProductAppOptions& options,
    const ProductCreativeNewWorldLaunchRequest& request,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    creative::CreativeAppState& creativeApp) {
  creative::Facade& facade = creativeApp.facade;
  ProductCreativeNewWorldLaunchResult result;
  window.launchAction = "creative_create_and_enter";

  CreativeWorldCreateRequest createRequest;
  createRequest.saveRoot = options.saveRoot;
  createRequest.title = request.title;
  createRequest.templateId = request.templateId;
  createRequest.requestedAtUtc = request.requestedAtUtc;
  createRequest.attemptToken = request.attemptToken;
  createRequest.packageId = request.packageId;
  createRequest.scenarioId = request.scenarioId;

  const CreativeWorldCreateResult create = createCreativeWorld(createRequest);
  mirrorCreativeWorldCreateResult(result, create);
  window.startup.creativeWorldIdScanMeasured = create.worldIdScanMeasured;
  window.startup.creativeWorldIdScanMicroseconds =
      create.worldIdScanMicroseconds;
  window.startup.creativeWorldIdScanEntryCount =
      create.worldIdScanEntryCount;
  window.startup.creativeWorldIdScanStatus = create.worldIdScanStatus;
  window.startup.creativeDocumentIdScanMeasured =
      create.documentIdScanMeasured;
  window.startup.creativeDocumentIdScanMicroseconds =
      create.documentIdScanMicroseconds;
  window.startup.creativeDocumentIdScanEntryCount =
      create.documentIdScanEntryCount;
  window.startup.creativeDocumentIdScanStatus = create.documentIdScanStatus;
  if (!create.accepted) {
    setCreativeNewWorldLaunchStatus(result, create.reasonCode);
    window.launchStatus = result.reasonCode;
    return result;
  }

  // F0: enter a blank creative stage (empty ground grid at origin), NOT the
  // first_room demo. Product New World keeps createProductSession.
  if (!createCreativeBlankSession(activeSession, window)) {
    setCreativeNewWorldLaunchStatus(result, window.launchStatus);
    return result;
  }
  frameCreativeStageCameraOnOrigin(window);
  result.sessionCreated = activeSession.has_value();

  creative::CreativeDocument documentToInstall = create.document;
  const creative::CreativeFacadeDocumentInstallReceipt install =
      facade.installDocument(std::move(documentToInstall));
  mirrorCreativeDocumentInstallResult(result, install);
  if (!install.accepted) {
    setCreativeNewWorldLaunchStatus(result, std::string{install.reasonCode});
    window.launchStatus = result.reasonCode;
    clearProductGameplayLaunchState(activeSession, window);
    return result;
  }
  creative::clearCreativeUndoStack(creativeApp.undoStack);
  window.creativeUndo.available = false;
  window.creativeUndo.depth = 0;

  enterProductGameplayTransition(frontend, window, FrontendAction::CreateAndEnter);
  window.interactionMode = ProductInteractionMode::Creative;
  result.enteredGameplay = true;
  result.accepted = true;
  setCreativeNewWorldLaunchStatus(result, "product_creative_world_launched");
  window.launchStatus = result.reasonCode;
  recordActiveCreativeSaveIdentity(creativeApp.identity,
                                   result.saveId,
                                   result.path,
                                   result.worldId,
                                   result.documentId,
                                   result.objectCount,
                                   result.nextObjectId);
  const ProductCreativeBakedActiveRoomRefreshResult bakedActiveRoom =
      refreshProductCreativeBakedActiveRoom(
          ProductCreativeBakedActiveRoomRefreshRequest{},
          activeSession,
          window,
          creativeApp);
  mirrorCreativeBakedActiveRoomRefreshResult(result, bakedActiveRoom);
  return result;
}

ProductCreativeOpenWorldLaunchResult launchProductCreativeOpenWorld(
    const ProductAppOptions& options,
    const ProductCreativeOpenWorldLaunchRequest& request,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    creative::CreativeAppState& creativeApp) {
  creative::Facade& facade = creativeApp.facade;
  ProductCreativeOpenWorldLaunchResult result;
  window.launchAction = "creative_open_and_enter";

  CreativeWorldOpenRequest openRequest;
  openRequest.saveRoot = options.saveRoot;
  openRequest.saveId = request.saveId;

  const CreativeWorldOpenResult open = openCreativeWorld(openRequest);
  mirrorCreativeWorldOpenResult(result, open);
  if (!open.accepted) {
    setCreativeOpenWorldLaunchStatus(result, open.reasonCode);
    window.launchStatus = result.reasonCode;
    return result;
  }

  // F0: opening a creative world also stands on the blank stage, not first_room.
  if (!createCreativeBlankSession(activeSession, window)) {
    setCreativeOpenWorldLaunchStatus(result, window.launchStatus);
    return result;
  }
  frameCreativeStageCameraOnOrigin(window);
  result.sessionCreated = activeSession.has_value();

  creative::CreativeDocument documentToInstall = open.document;
  const creative::CreativeFacadeDocumentInstallReceipt install =
      facade.installDocument(std::move(documentToInstall));
  mirrorCreativeDocumentInstallResult(result, install);
  if (!install.accepted) {
    setCreativeOpenWorldLaunchStatus(result, std::string{install.reasonCode});
    window.launchStatus = result.reasonCode;
    clearProductGameplayLaunchState(activeSession, window);
    return result;
  }
  creative::clearCreativeUndoStack(creativeApp.undoStack);
  window.creativeUndo.available = false;
  window.creativeUndo.depth = 0;

  enterProductGameplayTransition(frontend, window, FrontendAction::Load);
  window.interactionMode = ProductInteractionMode::Creative;
  result.enteredGameplay = true;
  result.accepted = true;
  setCreativeOpenWorldLaunchStatus(result, "product_creative_world_opened");
  window.launchStatus = result.reasonCode;
  recordActiveCreativeSaveIdentity(creativeApp.identity,
                                   result.saveId,
                                   result.path,
                                   result.worldId,
                                   result.documentId,
                                   result.objectCount,
                                   result.nextObjectId);
  const ProductCreativeBakedActiveRoomRefreshResult bakedActiveRoom =
      refreshProductCreativeBakedActiveRoom(
          ProductCreativeBakedActiveRoomRefreshRequest{},
          activeSession,
          window,
          creativeApp);
  mirrorCreativeBakedActiveRoomRefreshResult(result, bakedActiveRoom);
  return result;
}

ProductCreativeCurrentWorldSaveResult saveProductCurrentCreativeWorld(
    const ProductAppOptions& options,
    creative::CreativeAppState& creativeApp,
    std::string_view source,
    ProductAppWindowState& window) {
  (void)source;
  creative::Facade& facade = creativeApp.facade;
  const creative::CreativeActiveIdentity& identity = creativeApp.identity;

  ProductCreativeCurrentWorldSaveResult result;
  result.saveId = identity.saveId;
  result.path =
      missingWindowIdentity(identity.savePath)
          ? std::filesystem::path{}
          : std::filesystem::path{identity.savePath};
  result.worldId = identity.worldId;
  result.documentId = facade.document().id();
  result.objectCount = facade.document().objectCount();
  result.nextObjectId = facade.document().nextObjectId();
  result.dirtyFlagsBefore = facade.document().dirtyFlags();
  result.dirtyFlagsAfter = result.dirtyFlagsBefore;

  if (window.interactionMode != ProductInteractionMode::Creative) {
    setCurrentCreativeSaveStatus(result, "product_creative_save_inactive");
    recordActiveCreativeSaveResult(creativeApp.identity, result);
    return result;
  }
  if (missingWindowIdentity(identity.saveId)) {
    setCurrentCreativeSaveStatus(result, "product_creative_save_id_missing");
    recordActiveCreativeSaveResult(creativeApp.identity, result);
    return result;
  }
  if (facade.document().id() == creative::kInvalidDocumentId) {
    setCurrentCreativeSaveStatus(result,
                                 "product_creative_save_document_id_missing");
    recordActiveCreativeSaveResult(creativeApp.identity, result);
    return result;
  }

  CreativeWorldSaveRequest request;
  request.saveRoot = options.saveRoot;
  request.saveId = identity.saveId;
  request.attemptToken = "attempt_002";
  request.document = &facade.documentForPersistence();
  if (!missingWindowIdentity(identity.worldId)) {
    request.worldId = identity.worldId;
  }
  request.savedAtUtc = productSaveTimestampNowUtc();

  const CreativeWorldSaveResult saved = saveCreativeWorld(request);
  result.saveResult = saved;
  result.saveId = saved.saveId.empty() ? result.saveId : saved.saveId;
  result.path = saved.path.empty() ? result.path : saved.path;
  result.worldId = saved.worldId.empty() ? result.worldId : saved.worldId;
  result.documentId = saved.documentId;
  result.objectCount = saved.objectCount;
  result.nextObjectId = saved.nextObjectId;
  result.dirtyFlagsBefore = saved.dirtyFlagsBefore;
  result.dirtyFlagsDrained = saved.dirtyFlagsDrained;
  result.dirtyFlagsAfter = saved.dirtyFlagsAfter;
  result.saved = saved.saved;
  if (!saved.accepted || !saved.saved) {
    setCurrentCreativeSaveStatus(result, saved.reasonCode);
    recordActiveCreativeSaveResult(creativeApp.identity, result);
    return result;
  }

  result.accepted = true;
  setCurrentCreativeSaveStatus(result, "product_creative_world_saved");
  creative::clearCreativeUndoStack(creativeApp.undoStack);
  window.creativeUndo.available = false;
  window.creativeUndo.depth = 0;
  recordActiveCreativeSaveResult(creativeApp.identity, result);
  return result;
}

ProductCreativeBakedActiveRoomRefreshResult refreshProductCreativeBakedActiveRoom(
    const ProductCreativeBakedActiveRoomRefreshRequest& request,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    const creative::CreativeAppState& creativeApp) {
  // Default the activation hook to the reasoning-graph fill: build the L4 graph from the baked room
  // and install it, so the shipped stealth guard reasons over the authored room instead of the
  // empty-graph fallback. Callers may still supply their own hook (tests do).
  ProductCreativeBakedActiveRoomRefreshRequest resolved = request;
  if (!resolved.activationHook) {
    resolved.activationHook = &activateCreativeReasoningGraph;
  }
  return ProductCreativeBakedRoomRefreshService{
      resolved,
      activeSession,
      window,
      creativeApp,
  }.execute();
}

void launchProductSaveSlot(const ProductAppOptions& options,
                           const ProductWorldTemplate& world,
                           const SaveSlotPreview* slot,
                           FrontendAction launchAction,
                           std::string_view source,
                           FrontendState& frontend,
                           std::optional<Session>& activeSession,
                           ProductAppWindowState& window) {
  window.launchAction = std::string(frontendActionName(launchAction));
  recordProductSaveLoadSelection(source, slot, window);
  if (slot == nullptr) {
    window.launchStatus = "no_compatible_save";
    window.saveSession.productSaveLoadResult.status = "no_compatible_save";
    window.saveSession.productSaveLoadResult.reasonCode = "no_compatible_save";
    frontend.status = source == "load_save_selector"
                          ? "load_save_action_disabled"
                          : "opening_menu_action_disabled";
    return;
  }
  if (!slot->enabled || slot->compatibility != SaveSlotCompatibility::Compatible) {
    const std::string reason =
        slot->reason.empty() ? "save_slot_disabled" : slot->reason;
    window.launchStatus = reason;
    window.saveSession.productSaveLoadResult.status = reason;
    window.saveSession.productSaveLoadResult.reasonCode = reason;
    frontend.status = "load_save_action_disabled";
    return;
  }

  if (!createProductSession(options, activeSession, window)) {
    frontend.status = source == "load_save_selector"
                          ? "load_save_launch_failed"
                          : "opening_menu_continue_failed";
    return;
  }

  ProductSaveLoadRequest loadRequest;
  loadRequest.path = slot->path;
  loadRequest.session = &*activeSession;
  loadRequest.expectedPackageId = world.packageId;
  loadRequest.expectedScenarioId = world.scenarioId;
  const ProductSaveLoadResult loaded = loadProductSessionSave(loadRequest);
  recordProductSaveLoadResult(loaded, window);
  if (!loaded.ok) {
    window.launchStatus = loaded.reasonCode;
    clearProductGameplayLaunchState(activeSession, window);
    frontend.status = source == "load_save_selector"
                          ? "load_save_launch_failed"
                          : "opening_menu_continue_failed";
    return;
  }

  if (loaded.authoredRoomPresent) {
    activeRoom(window) =
        buildProductActiveRoomFromSavedAuthoredRoom(loaded.authoredRoom);
    bumpActiveRoomRevision(window);
    const ProductSavedRoomMarkerBindingResult bound =
        bindSavedRoomMarkersToSession(activeRoom(window), *activeSession);
    recordSavedRoomMarkerBindingResult(bound, window);
    if (!bound.ok) {
      window.launchStatus = bound.reasonCode;
      clearProductGameplayLaunchState(activeSession, window);
      frontend.status = source == "load_save_selector"
                            ? "load_save_launch_failed"
                            : "opening_menu_continue_failed";
      return;
    }
  }
  (void)ensureActiveRoomCollisionFresh(window, &*activeSession);
  window.launchStatus = loaded.status;
  window.runtimeStateHash = activeSession->stateHash();
  window.interactionMode = ProductInteractionMode::Player;
  enterProductGameplayTransition(frontend, window, launchAction);
}

void launchProductContinueSave(const ProductAppOptions& options,
                               const ProductWorldTemplate& world,
                               const ProductSaveBridgeResult& saves,
                               FrontendState& frontend,
                               std::optional<Session>& activeSession,
                               ProductAppWindowState& window) {
  // Continue selects the newest compatible active save via the catalog policy
  // (selectProductContinueSave), then resolves the matching projected slot to
  // load. The catalog is sorted newest-first and the policy is unit-tested in
  // product_save_catalog_tests; this replaces an ad-hoc reverse scan that
  // returned the OLDEST compatible save.
  const ProductContinueSelectionResult continueSelection =
      selectProductContinueSave(saves.catalog.catalog);
  const SaveSlotPreview* slot =
      continueSelection.selected
          ? saveSlotById(saves.slots, continueSelection.selectedSaveId)
          : nullptr;
  launchProductSaveSlot(options,
                        world,
                        slot,
                        FrontendAction::Continue,
                        "continue",
                        frontend,
                        activeSession,
                        window);
}

void launchProductLoadSaveSelection(const ProductAppOptions& options,
                                    const ProductWorldTemplate& world,
                                    const ProductSaveBridgeResult& saves,
                                    FrontendState& frontend,
                                    std::optional<Session>& activeSession,
                                    ProductAppWindowState& window) {
  frontend.selectedAction = FrontendAction::Load;
  const SaveSlotPreview* selected =
      initializeSelectedProductSaveSlot(saves.slots, window);
  launchProductSaveSlot(options,
                        world,
                        selected,
                        FrontendAction::Load,
                        "load_save_selector",
                        frontend,
                        activeSession,
                        window);
}

}  // namespace iggy3d
