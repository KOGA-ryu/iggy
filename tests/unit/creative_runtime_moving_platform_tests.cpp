#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs, float epsilon = 1.0e-4F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

cr::CreativeDocumentCreateReceipt createBox(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    std::string name,
    cr::CreativeVec3 position,
    cr::CreativeBounds bounds) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.transform.position = position;
  request.hasTransformOverride = true;
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt createPoint(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    std::string name,
    cr::CreativeVec3 position) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.transform.position = position;
  request.hasTransformOverride = true;
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt createMovingPlatform(
    cr::CreativeDocument& document,
    bool startsActive = true) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::MovingPlatform;
  request.name = "Lift";
  request.transform.position = {0.0, 0.375, 0.0};
  request.hasTransformOverride = true;
  request.bounds = {{-1.0, 0.25, -1.0}, {1.0, 0.5, 1.0}};
  request.hasBoundsOverride = true;
  request.pathPoints = {{{0.0, 0.375, 0.0}}, {{0.0, 1.375, 0.0}}};
  request.hasPathOverride = true;
  request.movingPlatform.speedMetersPerSecond = 1.0;
  request.movingPlatform.startsActive = startsActive;
  request.hasMovingPlatformSettingsOverride = true;
  return document.createObject(request);
}

cr::CreativeDocument baseDocument(bool playerOnPlatform) {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Moving Platform Test");
  static_cast<void>(document.assignId(901U));
  static_cast<void>(createBox(
      document, cr::CreativeObjectKind::Floor, "Floor", {0.0, 0.0, 0.0},
      {{-5.0, 0.0, -5.0}, {5.0, 0.25, 5.0}}));
  static_cast<void>(createPoint(
      document, cr::CreativeObjectKind::SpawnPoint, "Spawn",
      playerOnPlatform ? cr::CreativeVec3{0.0, 0.5, 0.0}
                       : cr::CreativeVec3{4.0, 0.25, 0.0}));
  return document;
}

std::optional<cr::CreativeRuntimeSandbox> activate(
    cr::CreativeDocument& document) {
  static iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativePlayPreparationRequest preparation;
  preparation.document = &document;
  preparation.staticMeshAssetCatalog = &catalog;
  preparation.roomId = "moving_platform_test";
  cr::CreativePlayPreparationResult prepared =
      cr::prepareCreativePlay(preparation);
  if (!prepared.accepted || !prepared.payload.has_value()) {
    return std::nullopt;
  }
  cr::CreativeRuntimeSandboxActivationRequest request;
  request.sourceDocument = &document;
  request.payload = std::move(*prepared.payload);
  cr::CreativeRuntimeSandboxActivationResult activated =
      cr::activateCreativeRuntimeSandbox(std::move(request));
  return std::move(activated.sandbox);
}

cr::CreativeRuntimeInteractableState* findPlatform(
    cr::CreativeRuntimeSandbox& sandbox,
    cr::CreativeObjectId objectId) {
  const auto found = std::find_if(
      sandbox.interactables.begin(), sandbox.interactables.end(),
      [objectId](const cr::CreativeRuntimeInteractableState& state) {
        return state.definition.objectId == objectId;
      });
  return found == sandbox.interactables.end() ? nullptr : &*found;
}

bool routeKernelIsDeterministicAndRelative() {
  const std::vector<cr::CreativePathPoint> path{
      {{0.0, 0.0, 0.0}}, {{1.0, 0.0, 0.0}}};
  cr::CreativeMovingPlatformSettings settings;
  settings.speedMetersPerSecond = 1.0;
  const cr::CreativeRuntimeMovingPlatformBuildResult built =
      cr::buildCreativeRuntimeMovingPlatformDefinition(path, settings,
                                                       {5.0F, 2.0F, 0.0F});
  if (!built.ok) {
    return expect(false, "route definition builds");
  }
  cr::CreativeRuntimeMovingPlatformState state;
  state.positionMeters = built.definition.originPositionMeters;
  const auto step1 = cr::planCreativeRuntimeMovingPlatformStep(
      {&built.definition, &state, 2U, true});
  const auto step2 = cr::planCreativeRuntimeMovingPlatformStep(
      {&built.definition, &step1.nextState, 2U, true});
  const auto step3 = cr::planCreativeRuntimeMovingPlatformStep(
      {&built.definition, &step2.nextState, 2U, true});
  cr::CreativeRuntimeMovingPlatformState reversed = step1.nextState;
  reversed.travelSign = -1;
  const auto reverseStep = cr::planCreativeRuntimeMovingPlatformStep(
      {&built.definition, &reversed, 2U, true});
  const auto inactive = cr::planCreativeRuntimeMovingPlatformStep(
      {&built.definition, &step1.nextState, 2U, false});

  return expect(step1.ok && step2.ok && step3.ok,
                "ping-pong steps are accepted") &&
         expect(near(step1.nextState.positionMeters.x, 5.5F) &&
                    near(step2.nextState.positionMeters.x, 6.0F) &&
                    near(step3.nextState.positionMeters.x, 5.5F),
                "ping-pong route advances and reflects") &&
         expect(reverseStep.ok &&
                    near(reverseStep.nextState.positionMeters.x, 5.0F),
                "negative travel sign reverses route phase") &&
         expect(inactive.ok && !inactive.moved &&
                    inactive.nextState.phaseMeters == step1.nextState.phaseMeters,
                "inactive motor preserves phase");
}

bool routeKernelRejectsDegeneratePathAndLoops() {
  cr::CreativeMovingPlatformSettings settings;
  const std::vector<cr::CreativePathPoint> degenerate{
      {{0.0, 0.0, 0.0}}, {{0.0, 0.0, 0.0}}};
  const auto rejected = cr::buildCreativeRuntimeMovingPlatformDefinition(
      degenerate, settings, {});

  settings.traversalMode = cr::CreativeMovingPlatformTraversalMode::Loop;
  settings.speedMetersPerSecond = 1.0;
  const std::vector<cr::CreativePathPoint> loop{
      {{0.0, 0.0, 0.0}}, {{1.0, 0.0, 0.0}}, {{1.0, 0.0, 1.0}}};
  const auto built = cr::buildCreativeRuntimeMovingPlatformDefinition(
      loop, settings, {});
  cr::CreativeRuntimeMovingPlatformState state;
  const auto step = cr::planCreativeRuntimeMovingPlatformStep(
      {&built.definition, &state, 1U, true});
  return expect(!rejected.ok, "zero-length route is rejected") &&
         expect(built.ok && step.ok &&
                    near(step.nextState.positionMeters.x, 1.0F) &&
                    near(step.nextState.positionMeters.z, 0.0F),
                "loop route follows authored open segments before closure");
}

bool routeSamplerMatchesTheStepKernel() {
  const std::vector<cr::CreativePathPoint> path{
      {{0.0, 0.0, 0.0}}, {{1.0, 0.0, 0.0}}};
  cr::CreativeMovingPlatformSettings settings;
  settings.speedMetersPerSecond = 1.0;
  const cr::CreativeRuntimeMovingPlatformBuildResult built =
      cr::buildCreativeRuntimeMovingPlatformDefinition(path, settings,
                                                       {5.0F, 2.0F, 0.0F});
  if (!built.ok) {
    return expect(false, "sample route definition builds");
  }
  const auto start = cr::sampleCreativeRuntimeMovingPlatformProgress(
      built.definition, 0.0);
  const auto quarter = cr::sampleCreativeRuntimeMovingPlatformProgress(
      built.definition, 0.25);
  const auto midpoint = cr::sampleCreativeRuntimeMovingPlatformProgress(
      built.definition, 0.5);
  const auto threeQuarter = cr::sampleCreativeRuntimeMovingPlatformProgress(
      built.definition, 0.75);
  const auto endpoint = cr::sampleCreativeRuntimeMovingPlatformProgress(
      built.definition, 1.0);

  cr::CreativeRuntimeMovingPlatformState stepped;
  stepped.positionMeters = built.definition.originPositionMeters;
  for (int tick = 0; tick < 3; ++tick) {
    const auto result = cr::planCreativeRuntimeMovingPlatformStep(
        {&built.definition, &stepped, 4U, true});
    if (!result.ok) {
      return expect(false, "sample parity step is accepted");
    }
    stepped = result.nextState;
  }
  const auto parity = cr::sampleCreativeRuntimeMovingPlatformProgress(
      built.definition, stepped.positionMeters.x - 5.0F);
  const auto nonFinite = cr::sampleCreativeRuntimeMovingPlatformProgress(
      built.definition, std::numeric_limits<double>::quiet_NaN());
  const auto outside = cr::sampleCreativeRuntimeMovingPlatformProgress(
      built.definition, 1.01);

  return expect(start.ok && quarter.ok && midpoint.ok && threeQuarter.ok &&
                    endpoint.ok,
                "complete normalized route is sampleable") &&
         expect(near(start.state.positionMeters.x, 5.0F) &&
                    near(quarter.state.positionMeters.x, 5.25F) &&
                    near(midpoint.state.positionMeters.x, 5.5F) &&
                    near(threeQuarter.state.positionMeters.x, 5.75F) &&
                    near(endpoint.state.positionMeters.x, 6.0F),
                "ping-pong normalized progress spans authored endpoints") &&
         expect(parity.ok &&
                    near(parity.state.positionMeters.x,
                         stepped.positionMeters.x),
                "route sampler and fixed-tick planner share geometry") &&
         expect(!nonFinite.ok && !outside.ok,
                "invalid normalized progress is rejected");
}

bool runtimePublishesGeometryAndCarriesRider() {
  cr::CreativeDocument document = baseDocument(true);
  const cr::CreativeDocumentCreateReceipt created =
      createMovingPlatform(document);
  std::optional<cr::CreativeRuntimeSandbox> sandbox = activate(document);
  if (!created.accepted || !sandbox.has_value()) {
    return expect(false, "moving platform sandbox activates");
  }
  cr::CreativeRuntimeInteractableState* platform =
      findPlatform(*sandbox, created.objectId);
  const iggy3d::EntityState* playerBefore =
      sandbox->session.state().world.findById({1U});
  if (platform == nullptr || playerBefore == nullptr ||
      platform->targetSurfaces.empty()) {
    return expect(false, "moving platform runtime state is complete");
  }
  const float playerYBefore = playerBefore->transform.position.y;
  const float surfaceYBefore =
      platform->targetSurfaces.front().surface.pointsMeters.front().y;
  const std::uint64_t hashBefore = sandbox->session.stateHash();
  const cr::CreativeRuntimeMovingPlatformUpdateReceipt update =
      cr::updateCreativeRuntimeMovingPlatforms(*sandbox, 10U);
  const iggy3d::EntityState* playerAfter =
      sandbox->session.state().world.findById({1U});
  const auto surface = std::find_if(
      sandbox->room.spatialSurfaces.begin(),
      sandbox->room.spatialSurfaces.end(),
      [platform](const iggy3d::RoomSpatialSurface& candidate) {
        return candidate.id ==
               platform->targetSurfaces.front().surface.id;
      });

  if (!(update.accepted && update.changed &&
        update.movedPlatformCount == 1U &&
        update.carriedActorCount == 1U)) {
    std::cerr << "moving receipt status="
              << static_cast<int>(update.status) << " reason="
              << update.reasonCode << " moved=" << update.movedPlatformCount
              << " blocked=" << update.blockedPlatformCount
              << " carried=" << update.carriedActorCount << " platformY="
              << platform->movingPlatform.positionMeters.y << " playerY="
              << (playerAfter != nullptr ? playerAfter->transform.position.y
                                         : -999.0F)
              << '\n';
  }

  return expect(update.accepted && update.changed &&
                    update.movedPlatformCount == 1U &&
                    update.carriedActorCount == 1U,
                "runtime advances platform and carries standing player") &&
         expect(platform->definition.kind ==
                        cr::CreativeRuntimeInteractableKind::MovingPlatform &&
                    near(platform->movingPlatform.positionMeters.y, 0.475F),
                "runtime motor publishes deterministic platform position") &&
         expect(playerAfter != nullptr &&
                    near(playerAfter->transform.position.y,
                         playerYBefore + 0.1F),
                "rider transform moves with platform") &&
         expect(surface != sandbox->room.spatialSurfaces.end() &&
                    near(surface->pointsMeters.front().y,
                         surfaceYBefore + 0.1F),
                "room collision surface moves with platform") &&
         expect(sandbox->geometryRevision == 1U &&
                    sandbox->session.stateHash() != hashBefore &&
                    sandbox->collisionSurfaces.size() ==
                        sandbox->room.spatialSurfaces.size(),
                "geometry, collision, and state hash publish together");
}

bool runtimeBlocksMotionWithoutAdvancingPhase() {
  cr::CreativeDocument document = baseDocument(false);
  const cr::CreativeDocumentCreateReceipt created =
      createMovingPlatform(document);
  const cr::CreativeDocumentCreateReceipt blocker = createBox(
      document, cr::CreativeObjectKind::Floor, "Low Ceiling",
      {0.0, 0.60, 0.0}, {{-1.0, 0.55, -1.0}, {1.0, 0.65, 1.0}});
  std::optional<cr::CreativeRuntimeSandbox> sandbox = activate(document);
  if (!created.accepted || !blocker.accepted || !sandbox.has_value()) {
    return expect(false, "blocked platform sandbox activates");
  }
  cr::CreativeRuntimeInteractableState* platform =
      findPlatform(*sandbox, created.objectId);
  const double phaseBefore =
      platform != nullptr ? platform->movingPlatform.phaseMeters : -1.0;
  const cr::CreativeRuntimeMovingPlatformUpdateReceipt update =
      cr::updateCreativeRuntimeMovingPlatforms(*sandbox, 10U);
  return expect(platform != nullptr && update.accepted && !update.changed &&
                    update.blockedPlatformCount == 1U &&
                    update.status ==
                        cr::CreativeRuntimeMovingPlatformUpdateStatus::Blocked,
                "obstruction blocks platform motion") &&
         expect(platform->movingPlatform.blocked &&
                    platform->movingPlatform.phaseMeters == phaseBefore &&
                    sandbox->geometryRevision == 0U,
                "blocked motion preserves route phase and geometry revision");
}

bool runtimeHonorsStartsActiveAndReverseLogic() {
  cr::CreativeDocument pausedDocument = baseDocument(false);
  const cr::CreativeDocumentCreateReceipt pausedPlatform =
      createMovingPlatform(pausedDocument, false);
  std::optional<cr::CreativeRuntimeSandbox> paused =
      activate(pausedDocument);
  if (!pausedPlatform.accepted || !paused.has_value()) {
    return expect(false, "paused platform sandbox activates");
  }
  const auto pausedUpdate =
      cr::updateCreativeRuntimeMovingPlatforms(*paused, 10U);
  cr::CreativeRuntimeInteractableState* pausedState =
      findPlatform(*paused, pausedPlatform.objectId);

  cr::CreativeDocument linkedDocument = baseDocument(false);
  const cr::CreativeDocumentCreateReceipt moving =
      createMovingPlatform(linkedDocument, true);
  const cr::CreativeDocumentCreateReceipt button = createPoint(
      linkedDocument, cr::CreativeObjectKind::Button, "Reverse", {3.0, 0.5, 0.0});
  const cr::CreativeLogicLinkMutationReceipt link = linkedDocument.setLogicLink(
      {button.objectId, moving.objectId, cr::CreativeLogicLinkAction::Reverse});
  std::optional<cr::CreativeRuntimeSandbox> linked = activate(linkedDocument);
  if (!moving.accepted || !button.accepted || !link.accepted ||
      !linked.has_value()) {
    return expect(false, "reverse-linked platform sandbox activates");
  }
  cr::CreativeRuntimeInteractableState* movingState =
      findPlatform(*linked, moving.objectId);
  cr::CreativeRuntimeInteractableState* buttonState =
      findPlatform(*linked, button.objectId);
  if (movingState == nullptr || buttonState == nullptr) {
    return expect(false, "reverse link runtime endpoints exist");
  }
  const std::uint64_t geometryBefore = linked->geometryRevision;
  const auto reversed = cr::applyCreativeRuntimeInteractionEffect(
      *linked, buttonState->entity);

  return expect(pausedUpdate.accepted && !pausedUpdate.changed &&
                    pausedState != nullptr && !pausedState->targetActive,
                "starts-active false keeps platform paused") &&
         expect(reversed.accepted && reversed.changed &&
                    movingState->movingPlatform.travelSign == -1 &&
                    movingState->targetActive,
                "reverse action flips travel sign without pausing") &&
         expect(linked->geometryRevision == geometryBefore,
                "state-only reverse does not rebuild geometry");
}

}  // namespace

int main() {
  const bool ok = routeKernelIsDeterministicAndRelative() &&
                  routeKernelRejectsDegeneratePathAndLoops() &&
                  routeSamplerMatchesTheStepKernel() &&
                  runtimePublishesGeometryAndCarriesRider() &&
                  runtimeBlocksMotionWithoutAdvancingPhase() &&
                  runtimeHonorsStartsActiveAndReverseLogic();
  return ok ? 0 : 1;
}
