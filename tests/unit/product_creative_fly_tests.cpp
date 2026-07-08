#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/view/CreativeFlyAnchorStore.hpp"
#include "app/iggy3d/view/ViewportState.hpp"

#include <cmath>
#include <iostream>

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 0.0001F;
}

bool vecNear(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) {
  return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

iggy3d::ProductCreativeFlyConfig testConfig() {
  iggy3d::ProductCreativeFlyConfig config;
  config.enabled = true;
  config.speedMetersPerSecond = 6.0F;
  config.sprintMultiplier = 2.0F;
  config.inputStepSeconds = 0.5F;
  return config;
}

bool disabledAndInvalidDoNotMove() {
  iggy3d::ProductCreativeFlyConfig disabled = testConfig();
  disabled.enabled = false;
  iggy3d::ProductCreativeFlyInput input;
  input.moveY = 1.0F;
  const iggy3d::ProductCreativeFlyResult disabledResult =
      iggy3d::applyProductCreativeFlyInput(disabled, input, {1.0F, 2.0F, 3.0F});

  iggy3d::ProductCreativeFlyConfig invalid = testConfig();
  invalid.speedMetersPerSecond = 0.0F;
  const iggy3d::ProductCreativeFlyResult invalidResult =
      iggy3d::applyProductCreativeFlyInput(invalid, input, {});

  return expect(!disabledResult.applied, "disabled not applied") &&
         expect(disabledResult.reasonCode == "creative_fly_disabled",
                "disabled reason") &&
         expect(vecNear(disabledResult.finalPositionMeters, {1.0F, 2.0F, 3.0F}),
                "disabled unchanged") &&
         expect(!invalidResult.applied, "invalid not applied") &&
         expect(invalidResult.reasonCode == "creative_fly_invalid_config",
                "invalid reason");
}

bool appliesYawRelativeMovement() {
  iggy3d::ProductCreativeFlyInput forward;
  forward.moveY = 1.0F;
  forward.cameraYawDegrees = 0.0F;
  const iggy3d::ProductCreativeFlyResult yaw0 =
      iggy3d::applyProductCreativeFlyInput(testConfig(), forward, {});

  forward.cameraYawDegrees = 90.0F;
  const iggy3d::ProductCreativeFlyResult yaw90 =
      iggy3d::applyProductCreativeFlyInput(testConfig(), forward, {});

  return expect(yaw0.applied, "yaw 0 applied") &&
         expect(vecNear(yaw0.finalPositionMeters, {0.0F, 0.0F, -3.0F}),
                "yaw 0 forward negative z") &&
         expect(yaw90.applied, "yaw 90 applied") &&
         expect(vecNear(yaw90.finalPositionMeters, {3.0F, 0.0F, 0.0F}),
                "yaw 90 forward positive x");
}

bool normalizesVerticalAndSprints() {
  iggy3d::ProductCreativeFlyInput input;
  input.moveX = 1.0F;
  input.moveZ = 1.0F;
  input.sprinting = true;
  const iggy3d::ProductCreativeFlyResult result =
      iggy3d::applyProductCreativeFlyInput(testConfig(), input, {});
  const float expected = 6.0F * 2.0F * 0.5F / std::sqrt(2.0F);

  iggy3d::ProductCreativeFlyInput noInput;
  const iggy3d::ProductCreativeFlyResult idle =
      iggy3d::applyProductCreativeFlyInput(testConfig(), noInput, {});

  return expect(result.applied, "diagonal applied") &&
         expect(near(result.speedMetersPerSecond, 12.0F), "sprint speed") &&
         expect(vecNear(result.finalPositionMeters, {expected, expected, 0.0F}),
                "diagonal normalized") &&
         expect(!idle.applied, "idle not applied") &&
         expect(idle.reasonCode == "creative_fly_no_input", "idle reason");
}

bool anchorStoreDefaultsAreUnavailable() {
  iggy3d::ProductCreativeFlyAnchorStore store;
  iggy3d::ProductViewportState viewport;

  return expect(store.provenance ==
                    iggy3d::ProductCreativeFlyAnchorProvenance::Unseeded,
                "default store unseeded") &&
         expect(!iggy3d::productCreativeFlyAnchorAvailable(store),
                "default store unavailable") &&
         expect(!iggy3d::productCreativeFlyAnchorFreshForEpoch(store, 0),
                "default store not fresh") &&
         expect(!iggy3d::productCreativeFlyAnchorAvailable(
                    viewport.creativeFlyAnchor),
                "viewport store default unavailable");
}

bool anchorProvenanceNamesAreStable() {
  using iggy3d::ProductCreativeFlyAnchorProvenance;

  return expect(iggy3d::productCreativeFlyAnchorProvenanceName(
                    ProductCreativeFlyAnchorProvenance::Unseeded) ==
                    "Unseeded",
                "unseeded name") &&
         expect(iggy3d::productCreativeFlyAnchorProvenanceName(
                    ProductCreativeFlyAnchorProvenance::OriginFramed) ==
                    "OriginFramed",
                "origin framed name") &&
         expect(iggy3d::productCreativeFlyAnchorProvenanceName(
                    ProductCreativeFlyAnchorProvenance::PlayerSeeded) ==
                    "PlayerSeeded",
                "player seeded name") &&
         expect(iggy3d::productCreativeFlyAnchorProvenanceName(
                    ProductCreativeFlyAnchorProvenance::SceneSeeded) ==
                    "SceneSeeded",
                "scene seeded name") &&
         expect(iggy3d::productCreativeFlyAnchorProvenanceName(
                    ProductCreativeFlyAnchorProvenance::FlyIntegrated) ==
                    "FlyIntegrated",
                "fly integrated name");
}

bool anchorFreshnessRequiresMatchingEpochAndSeededProvenance() {
  using iggy3d::ProductCreativeFlyAnchorProvenance;

  iggy3d::ProductCreativeFlyAnchorStore store;
  store.positionMeters = {1.0F, 2.0F, 3.0F};
  store.provenance = ProductCreativeFlyAnchorProvenance::PlayerSeeded;
  store.seededFromWorldEpoch = 7;

  iggy3d::ProductCreativeFlyAnchorStore unseededForEpoch;
  unseededForEpoch.seededFromWorldEpoch = 7;

  return expect(iggy3d::productCreativeFlyAnchorAvailable(store),
                "seeded store available") &&
         expect(iggy3d::productCreativeFlyAnchorFreshForEpoch(store, 7),
                "matching epoch fresh") &&
         expect(!iggy3d::productCreativeFlyAnchorFreshForEpoch(store, 6),
                "stale epoch not fresh") &&
         expect(!iggy3d::productCreativeFlyAnchorFreshForEpoch(
                    unseededForEpoch, 7),
                "unseeded matching epoch not fresh");
}

bool originSeedStampsCurrentEpochAndLegacyFields() {
  using iggy3d::ProductCreativeFlyAnchorProvenance;

  iggy3d::ProductAppWindowState window;
  window.viewport.creativeWorldEpoch = 3;

  const iggy3d::ProductCreativeFlyAnchorStore& store =
      iggy3d::seedCreativeFlyAnchorFromOrigin(window);

  return expect(store.provenance ==
                    ProductCreativeFlyAnchorProvenance::OriginFramed,
                "origin provenance") &&
         expect(store.seededFromWorldEpoch == 3, "origin epoch") &&
         expect(vecNear(store.positionMeters, {0.0F, 6.0F, 10.0F}),
                "origin position") &&
         expect(vecNear(window.viewport.creativeFlyAnchor.positionMeters,
                        {0.0F, 6.0F, 10.0F}),
                "origin stores position");
}

bool ensureFreshReseedsWhenEpochChanges() {
  using iggy3d::ProductCreativeFlyAnchorProvenance;

  iggy3d::ProductAppWindowState window;
  window.runtimeStateHash = 42;
  window.viewport.creativeWorldEpoch = 1;
  iggy3d::recordCreativeFlyAnchorIntegrated(window, {9.0F, 8.0F, 7.0F});

  const std::uint64_t sameRuntimeHash = window.runtimeStateHash;
  iggy3d::bumpCreativeWorldEpoch(window);
  const iggy3d::ProductCreativeFlyAnchorStore& store =
      iggy3d::ensureFreshCreativeFlyAnchor(window, nullptr);

  return expect(window.runtimeStateHash == sameRuntimeHash,
                "runtime hash unchanged") &&
         expect(store.seededFromWorldEpoch == 2, "reseeded epoch") &&
         expect(store.provenance ==
                    ProductCreativeFlyAnchorProvenance::PlayerSeeded,
                "reseeded via session ensure path") &&
         expect(vecNear(store.positionMeters, {}), "null session fallback") &&
         expect(vecNear(window.viewport.creativeFlyAnchor.positionMeters, {}),
                "ensure stores position");
}

bool ensureFreshDoesNotRewriteFreshAnchor() {
  using iggy3d::ProductCreativeFlyAnchorProvenance;

  iggy3d::ProductAppWindowState window;
  window.viewport.creativeWorldEpoch = 5;
  iggy3d::seedCreativeFlyAnchorFromScene(window, {4.0F, 5.0F, 6.0F});

  const iggy3d::ProductCreativeFlyAnchorStore& store =
      iggy3d::ensureFreshCreativeFlyAnchor(window, nullptr);

  return expect(store.seededFromWorldEpoch == 5, "fresh epoch unchanged") &&
         expect(store.provenance ==
                    ProductCreativeFlyAnchorProvenance::SceneSeeded,
                "fresh provenance unchanged") &&
         expect(vecNear(store.positionMeters, {4.0F, 5.0F, 6.0F}),
                "fresh position unchanged");
}

bool sceneAndIntegratedSeedsStampProvenanceAndEpoch() {
  using iggy3d::ProductCreativeFlyAnchorProvenance;

  iggy3d::ProductAppWindowState window;
  window.viewport.creativeWorldEpoch = 8;

  const iggy3d::ProductCreativeFlyAnchorStore& scene =
      iggy3d::seedCreativeFlyAnchorFromScene(window, {1.0F, 2.0F, 3.0F});
  const bool sceneOk =
      scene.provenance == ProductCreativeFlyAnchorProvenance::SceneSeeded &&
      scene.seededFromWorldEpoch == 8 &&
      vecNear(scene.positionMeters, {1.0F, 2.0F, 3.0F});

  const iggy3d::ProductCreativeFlyAnchorStore& integrated =
      iggy3d::recordCreativeFlyAnchorIntegrated(window, {7.0F, 6.0F, 5.0F});

  return expect(sceneOk, "scene seed stamps state") &&
         expect(integrated.provenance ==
                    ProductCreativeFlyAnchorProvenance::FlyIntegrated,
                "integrated provenance") &&
         expect(integrated.seededFromWorldEpoch == 8, "integrated epoch") &&
         expect(vecNear(integrated.positionMeters, {7.0F, 6.0F, 5.0F}),
                "integrated position") &&
         expect(vecNear(window.viewport.creativeFlyAnchor.positionMeters,
                        {7.0F, 6.0F, 5.0F}),
                "integrated stores position");
}

bool originSeedWinsBeforeLazyEnsureInSameEpoch() {
  using iggy3d::ProductCreativeFlyAnchorProvenance;

  iggy3d::ProductAppWindowState window;
  iggy3d::bumpCreativeWorldEpoch(window);
  iggy3d::seedCreativeFlyAnchorFromOrigin(window);
  const iggy3d::ProductCreativeFlyAnchorStore& ensured =
      iggy3d::ensureFreshCreativeFlyAnchor(window, nullptr);

  return expect(ensured.seededFromWorldEpoch == window.viewport.creativeWorldEpoch,
                "origin ensure epoch") &&
         expect(ensured.provenance ==
                    ProductCreativeFlyAnchorProvenance::OriginFramed,
                "origin ensure keeps provenance") &&
         expect(vecNear(ensured.positionMeters, {0.0F, 6.0F, 10.0F}),
                "origin ensure keeps position");
}

bool integrationThenNextOriginFrameReseedsForNewEpoch() {
  using iggy3d::ProductCreativeFlyAnchorProvenance;

  iggy3d::ProductAppWindowState window;
  iggy3d::bumpCreativeWorldEpoch(window);
  iggy3d::seedCreativeFlyAnchorFromOrigin(window);
  iggy3d::recordCreativeFlyAnchorIntegrated(window, {3.0F, 4.0F, 5.0F});
  const bool integrated =
      window.viewport.creativeFlyAnchor.provenance ==
          ProductCreativeFlyAnchorProvenance::FlyIntegrated &&
      iggy3d::productCreativeFlyAnchorFreshForEpoch(
          window.viewport.creativeFlyAnchor, window.viewport.creativeWorldEpoch);

  iggy3d::bumpCreativeWorldEpoch(window);
  iggy3d::seedCreativeFlyAnchorFromOrigin(window);

  return expect(integrated, "integrated anchor fresh before next epoch") &&
         expect(window.viewport.creativeFlyAnchor.provenance ==
                    ProductCreativeFlyAnchorProvenance::OriginFramed,
                "next origin frame reseeds provenance") &&
         expect(window.viewport.creativeFlyAnchor.seededFromWorldEpoch ==
                    window.viewport.creativeWorldEpoch,
                "next origin frame reseeds epoch") &&
         expect(vecNear(window.viewport.creativeFlyAnchor.positionMeters,
                        {0.0F, 6.0F, 10.0F}),
                "next origin frame resets position");
}

bool repeatedEnsureIsIdempotentWithinEpoch() {
  using iggy3d::ProductCreativeFlyAnchorProvenance;

  iggy3d::ProductAppWindowState window;
  window.viewport.creativeWorldEpoch = 11;
  const iggy3d::ProductCreativeFlyAnchorStore first =
      iggy3d::ensureFreshCreativeFlyAnchor(window, nullptr);
  const iggy3d::ProductCreativeFlyAnchorStore second =
      iggy3d::ensureFreshCreativeFlyAnchor(window, nullptr);

  return expect(first.seededFromWorldEpoch == 11,
                "first ensure stamps epoch") &&
         expect(second.seededFromWorldEpoch == first.seededFromWorldEpoch,
                "second ensure keeps epoch") &&
         expect(second.provenance == first.provenance,
                "second ensure keeps provenance") &&
         expect(second.provenance ==
                    ProductCreativeFlyAnchorProvenance::PlayerSeeded,
                "ensure provenance player seeded") &&
         expect(vecNear(second.positionMeters, first.positionMeters),
                "second ensure keeps position");
}

}  // namespace

int main() {
  const bool ok = disabledAndInvalidDoNotMove() &&
                  appliesYawRelativeMovement() &&
                  normalizesVerticalAndSprints() &&
                  anchorStoreDefaultsAreUnavailable() &&
                  anchorProvenanceNamesAreStable() &&
                  anchorFreshnessRequiresMatchingEpochAndSeededProvenance() &&
                  originSeedStampsCurrentEpochAndLegacyFields() &&
                  ensureFreshReseedsWhenEpochChanges() &&
                  ensureFreshDoesNotRewriteFreshAnchor() &&
                  sceneAndIntegratedSeedsStampProvenanceAndEpoch() &&
                  originSeedWinsBeforeLazyEnsureInSameEpoch() &&
                  integrationThenNextOriginFrameReseedsForNewEpoch() &&
                  repeatedEnsureIsIdempotentWithinEpoch();
  if (!ok) {
    return 1;
  }
  std::cout << "product_creative_fly_tests=pass\n";
  return 0;
}
