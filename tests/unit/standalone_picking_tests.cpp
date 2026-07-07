#include "StandalonePicking.hpp"
#include "StandalonePreviewProxies.hpp"

#include "app/iggy3d/creative/document/Object.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/OrientedBox.hpp"
#include "core/math/Transform3.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

using iggy3d::Mat4;
using iggy3d::Transform3;
using iggy3d::Vec3;
using iggy3d_creative_app::ObjectVisualPickBounds;
using iggy3d_creative_app::ObjectVisualPickResult;
using iggy3d_creative_app::VisualBounds;
using iggy3d_creative_app::WorldRay;
using iggy3d_creative_app::buildObjectVisualPickBounds;
using iggy3d_creative_app::orientedVisualBoxForObject;
using iggy3d_creative_app::pickNearestVisualBoundsObject;
using iggy3d_creative_app::pickNearestVisualBoundsObjectBruteForce;
using iggy3d_creative_app::projectPointToScreen;
using iggy3d_creative_app::rayEntryDistanceForAabb;
using iggy3d_creative_app::visualBoundsForObject;

constexpr float kPi = 3.14159265358979323846F;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float a, float b, float tol = 1e-3F) {
  return std::fabs(a - b) <= tol;
}

cr::CreativeObject makeCrateObject(cr::CreativeObjectId id) {
  cr::CreativeObject object;
  object.id = id;
  object.kind = cr::CreativeObjectKind::Crate;
  object.name = "rotated crate";
  object.transform.position = {10.0, 0.0, 0.0};
  object.transform.scale = {1.0, 1.0, 1.0};
  object.bounds = {{9.0, -0.5, -0.5}, {11.0, 0.5, 0.5}};
  return object;
}

ObjectVisualPickBounds makeAabbCandidate(cr::CreativeObjectId id,
                                         Vec3 min,
                                         Vec3 max) {
  ObjectVisualPickBounds candidate;
  candidate.id = id;
  candidate.bounds = {min, max};
  return candidate;
}

ObjectVisualPickBounds makeOrientedCandidate(cr::CreativeObjectId id,
                                             Transform3 transform,
                                             iggy3d::Aabb3 localBounds) {
  ObjectVisualPickBounds candidate;
  candidate.id = id;
  candidate.orientedBounds = iggy3d::makeOrientedBox(transform, localBounds);
  candidate.bounds = {iggy3d::orientedBoxWorldAabb(*candidate.orientedBounds).min,
                      iggy3d::orientedBoxWorldAabb(*candidate.orientedBounds).max};
  return candidate;
}

Mat4 identityClip() {
  Mat4 out{};
  out.m[0] = 1.0F;
  out.m[5] = 1.0F;
  out.m[10] = 1.0F;
  out.m[15] = 1.0F;
  return out;
}

Mat4 projectionClipWithConstantW(float w) {
  Mat4 out{};
  out.m[0] = 0.5F;
  out.m[5] = -0.5F;
  out.m[15] = w;
  return out;
}

bool standaloneProjectionRoutesThroughCoreProjectedPoint() {
  const Vec3 point{1.0F, 1.0F, 1.0F};
  const iggy3d_creative_app::ScreenPoint divided =
      projectPointToScreen(projectionClipWithConstantW(2.0F), point, 200, 200);
  const iggy3d_creative_app::ScreenPoint raw =
      projectPointToScreen(projectionClipWithConstantW(1.0F), point, 200, 200);

  return expect(divided.valid, "divided projection remains valid") &&
         expect(near(divided.x, 125.0F) && near(divided.y, 125.0F),
                "divided projection uses core ndc result") &&
         expect(raw.valid, "unit-w projection remains valid") &&
         expect(near(raw.x, 150.0F) && near(raw.y, 150.0F),
                "unit-w projection preserves raw compatibility coordinates");
}

bool standaloneProjectionOwnsFrontFacingPolicy() {
  const Vec3 point{1.0F, 1.0F, 1.0F};
  const iggy3d_creative_app::ScreenPoint zeroW =
      projectPointToScreen(projectionClipWithConstantW(0.0F), point, 200, 200);
  const iggy3d_creative_app::ScreenPoint negativeW =
      projectPointToScreen(projectionClipWithConstantW(-2.0F), point, 200, 200);
  const iggy3d_creative_app::ScreenPoint infiniteW =
      projectPointToScreen(projectionClipWithConstantW(
                               std::numeric_limits<float>::infinity()),
                           point,
                           200,
                           200);

  return expect(!zeroW.valid, "standalone projection rejects zero clip w") &&
         expect(!negativeW.valid,
                "standalone projection rejects behind-camera negative clip w") &&
         expect(!infiniteW.valid,
                "standalone projection rejects non-finite clip w");
}

bool unrotatedObjectKeepsCheapAabbPath() {
  const cr::CreativeObject object = makeCrateObject(11);
  const ObjectVisualPickBounds candidate =
      buildObjectVisualPickBounds(object, identityClip(), 640, 480);

  return expect(!candidate.orientedBounds.has_value(),
                "unrotated candidate has no oriented sidecar") &&
         expect(near(candidate.bounds.min.x, 9.0F) &&
                    near(candidate.bounds.max.x, 11.0F),
                "unrotated visual x bounds unchanged") &&
         expect(near(candidate.bounds.min.z, -0.5F) &&
                    near(candidate.bounds.max.z, 0.5F),
                "unrotated visual z bounds unchanged");
}

bool rotatedObjectBuildsCenteredOrientedVisualBounds() {
  cr::CreativeObject object = makeCrateObject(12);
  object.transform.rotation.y = static_cast<double>(kPi * 0.25F);

  const VisualBounds bounds = visualBoundsForObject(object);
  const ObjectVisualPickBounds candidate =
      buildObjectVisualPickBounds(object, identityClip(), 640, 480);

  return expect(candidate.orientedBounds.has_value(),
                "rotated candidate has oriented sidecar") &&
         expect(orientedVisualBoxForObject(object).has_value(),
                "rotated visual box helper returns value") &&
         expect(near((bounds.min.x + bounds.max.x) * 0.5F, 10.0F),
                "rotated world aabb stays centered on transform anchor") &&
         expect(bounds.max.z > 1.0F && bounds.min.z < -1.0F,
                "rotated visual z bounds expand beyond stale aabb");
}

bool rotatedPickHitsRealRotatedFaceOutsideStaleAabb() {
  cr::CreativeObject object = makeCrateObject(13);
  object.transform.rotation.y = static_cast<double>(kPi * 0.25F);
  const ObjectVisualPickBounds candidate =
      buildObjectVisualPickBounds(object, identityClip(), 640, 480);
  const VisualBounds staleBounds{{9.0F, -0.5F, -0.5F},
                                 {11.0F, 0.5F, 0.5F}};

  const WorldRay ray{true, {11.02F, 2.0F, -0.35F}, {0.0F, -1.0F, 0.0F}};
  float staleT = std::numeric_limits<float>::max();
  const bool staleHit = rayEntryDistanceForAabb(ray, staleBounds, staleT);
  const ObjectVisualPickResult pick =
      pickNearestVisualBoundsObject(std::vector<ObjectVisualPickBounds>{candidate},
                                    ray);

  return expect(!staleHit, "stale unrotated aabb misses rotated face ray") &&
         expect(pick.objectId == object.id,
                "oriented narrow phase picks rotated object") &&
         expect(pick.hitCount == 1U, "oriented ray records one hit");
}

bool rotatedPickRejectsStaleOnlyAabbSpace() {
  cr::CreativeObject object = makeCrateObject(14);
  object.transform.rotation.y = static_cast<double>(kPi * 0.25F);
  const ObjectVisualPickBounds candidate =
      buildObjectVisualPickBounds(object, identityClip(), 640, 480);
  const VisualBounds staleBounds{{9.0F, -0.5F, -0.5F},
                                 {11.0F, 0.5F, 0.5F}};

  const WorldRay ray{true, {10.95F, -2.0F, 0.49F}, {0.0F, 1.0F, 0.0F}};
  float staleT = std::numeric_limits<float>::max();
  const bool staleHit = rayEntryDistanceForAabb(ray, staleBounds, staleT);
  const ObjectVisualPickResult pick =
      pickNearestVisualBoundsObject(std::vector<ObjectVisualPickBounds>{candidate},
                                    ray);

  return expect(staleHit, "stale unrotated aabb would have hit") &&
         expect(pick.objectId == cr::kInvalidObjectId,
                "oriented narrow phase rejects stale-only corner") &&
         expect(pick.hitCount == 0U, "oriented stale-only ray records no hit");
}

bool nonUnitRayComparesMixedCandidateDistancesInMeters() {
  ObjectVisualPickBounds fartherAabb;
  fartherAabb.id = 21;
  fartherAabb.bounds = {{-1.0F, -1.0F, -10.0F}, {1.0F, 1.0F, -9.0F}};

  ObjectVisualPickBounds nearerObb;
  nearerObb.id = 22;
  nearerObb.bounds = {{-1.0F, -1.0F, -7.0F}, {1.0F, 1.0F, -5.0F}};
  nearerObb.orientedBounds = iggy3d::makeOrientedBox(
      Transform3{{0.0F, 0.0F, -6.0F}, {0.0F, kPi * 0.25F, 0.0F}, {1.0F, 1.0F, 1.0F}},
      iggy3d::makeAabb3(Vec3{-1.0F, -1.0F, -1.0F}, Vec3{1.0F, 1.0F, 1.0F}));

  const WorldRay nonUnitRay{true, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -2.0F}};
  const ObjectVisualPickResult pick = pickNearestVisualBoundsObject(
      std::vector<ObjectVisualPickBounds>{fartherAabb, nearerObb},
      nonUnitRay);

  return expect(pick.hitCount == 2U, "non-unit mixed ray hits both candidates") &&
         expect(pick.objectId == nearerObb.id,
                "nearest candidate uses meter distance for AABB and OBB") &&
         expect(pick.entryDistance < 9.0F,
                "nearest distance is closer than farther aabb face");
}

std::vector<ObjectVisualPickBounds> manyObjectPickFixture() {
  std::vector<ObjectVisualPickBounds> candidates;
  candidates.push_back(
      makeAabbCandidate(101, {-0.8F, -0.8F, -4.0F}, {0.8F, 0.8F, -3.0F}));
  candidates.push_back(
      makeAabbCandidate(102, {-0.8F, -0.8F, -10.0F}, {0.8F, 0.8F, -9.0F}));
  candidates.push_back(makeOrientedCandidate(
      103,
      Transform3{{4.0F, 0.0F, -6.0F},
                 {0.0F, kPi * 0.25F, 0.0F},
                 {1.0F, 1.0F, 1.0F}},
      iggy3d::makeAabb3(Vec3{-0.9F, -0.9F, -0.9F},
                        Vec3{0.9F, 0.9F, 0.9F})));

  cr::CreativeObjectId id = 200;
  for (int x = -6; x <= 6; ++x) {
    for (int z = 0; z < 8; ++z) {
      const float centerX = static_cast<float>(x) * 5.0F + 30.0F;
      const float centerZ = -20.0F - static_cast<float>(z) * 3.0F;
      candidates.push_back(makeAabbCandidate(
          id++, {centerX - 0.5F, -0.5F, centerZ - 0.5F},
          {centerX + 0.5F, 0.5F, centerZ + 0.5F}));
    }
  }
  return candidates;
}

bool indexedPickMatchesBruteForceAcrossManyObjects() {
  const std::vector<ObjectVisualPickBounds> candidates = manyObjectPickFixture();
  const std::vector<WorldRay> rays{
      {true, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}},
      {true, {0.0F, 0.0F, 0.0F}, {4.0F, 0.0F, -6.0F}},
      {true, {0.0F, 0.0F, 0.0F}, {-8.0F, 1.0F, -4.0F}},
      {true, {0.0F, 0.0F, 0.0F}, {0.0F, 4.0F, -1.0F}},
  };

  bool sawReducedCandidateSet = false;
  bool ok = true;
  for (const WorldRay& ray : rays) {
    const ObjectVisualPickResult indexed =
        pickNearestVisualBoundsObject(candidates, ray);
    const ObjectVisualPickResult brute =
        pickNearestVisualBoundsObjectBruteForce(candidates, ray);
    sawReducedCandidateSet =
        sawReducedCandidateSet || indexed.testedCount < brute.testedCount;
    ok = expect(indexed.objectId == brute.objectId,
                "indexed pick returns same object as brute force") &&
         expect(indexed.hitCount == brute.hitCount,
                "indexed pick sees same exact hit count as brute force") &&
         expect((brute.objectId == cr::kInvalidObjectId &&
                 indexed.entryDistance == std::numeric_limits<float>::max()) ||
                    near(indexed.entryDistance, brute.entryDistance),
                "indexed pick preserves nearest entry distance") &&
         expect(indexed.testedCount <= brute.testedCount,
                "indexed pick tests no more candidates than brute force") &&
         ok;
  }
  return expect(sawReducedCandidateSet,
                "broadphase reduces at least one many-object pick") &&
         ok;
}

bool broadphaseFallsBackWhenRayQueryExceedsGridRange() {
  const std::vector<ObjectVisualPickBounds> candidates{
      makeAabbCandidate(501,
                        {-0.5F, -0.5F, -0.5F},
                        {0.5F, 0.5F, 0.5F}),
  };
  const WorldRay ray{true, {20000000.0F, 0.0F, 0.0F}, {-1.0F, 0.0F, 0.0F}};

  const ObjectVisualPickResult indexed =
      pickNearestVisualBoundsObject(candidates, ray);
  const ObjectVisualPickResult brute =
      pickNearestVisualBoundsObjectBruteForce(candidates, ray);

  return expect(indexed.objectId == brute.objectId,
                "out-of-range broadphase query falls back without dropping hit") &&
         expect(indexed.hitCount == brute.hitCount,
                "out-of-range fallback preserves hit count") &&
         expect(indexed.testedCount == brute.testedCount,
                "out-of-range fallback tests the full candidate set");
}

}  // namespace

int main() {
  const bool ok =
      unrotatedObjectKeepsCheapAabbPath() &&
      standaloneProjectionRoutesThroughCoreProjectedPoint() &&
      standaloneProjectionOwnsFrontFacingPolicy() &&
      rotatedObjectBuildsCenteredOrientedVisualBounds() &&
      rotatedPickHitsRealRotatedFaceOutsideStaleAabb() &&
      rotatedPickRejectsStaleOnlyAabbSpace() &&
      nonUnitRayComparesMixedCandidateDistancesInMeters() &&
      indexedPickMatchesBruteForceAcrossManyObjects() &&
      broadphaseFallsBackWhenRayQueryExceedsGridRange();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
