#include "runtime/collision/EntityHitQuery.hpp"

#include <cmath>
#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool approx(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

iggy3d::EntityState entity(iggy3d::EntityId id,
                           std::string_view name,
                           iggy3d::Vec3 position,
                           bool active = true,
                           bool attackable = true) {
  iggy3d::EntityState result;
  result.id = id;
  result.stableName = std::string(name);
  result.kind = iggy3d::EntityKind::Npc;
  result.transform.position = position;
  result.localBounds = {{-0.5F, 0.0F, -0.5F}, {0.5F, 1.0F, 0.5F}};
  result.active = active;
  result.targeting.targetable = attackable;
  if (attackable) {
    result.targeting.actions.push_back(iggy3d::TargetAction::Attack);
  }
  return result;
}

iggy3d::WorldState worldWith(std::initializer_list<iggy3d::EntityState> entities) {
  iggy3d::WorldState world;
  for (iggy3d::EntityState entityState : entities) {
    const iggy3d::WorldEntityResult seeded = world.seedEntity(std::move(entityState));
    if (seeded.status != iggy3d::WorldStatus::Ok) {
      std::cerr << "failed to seed entity\n";
    }
  }
  return world;
}

iggy3d::EntityHitQueryRequest baseRequest(const iggy3d::WorldState& world) {
  iggy3d::EntityHitQueryRequest request;
  request.world = &world;
  request.startMeters = {0.0F, 0.5F, 0.0F};
  request.endMeters = {0.0F, 0.5F, 10.0F};
  request.radiusMeters = 0.0F;
  return request;
}

bool closestAttackableEntityWins() {
  const iggy3d::WorldState world = worldWith({
      entity({2}, "far_dummy", {0.0F, 0.0F, 8.0F}),
      entity({1}, "near_dummy", {0.0F, 0.0F, 4.0F}),
  });
  const iggy3d::EntityHitQueryResult hit =
      iggy3d::queryFirstEntityHit(baseRequest(world));

  return expect(hit.status == iggy3d::EntityHitStatus::Hit, "hit expected") &&
         expect(hit.entity == iggy3d::EntityId{1}, "near entity wins") &&
         expect(hit.stableName == "near_dummy", "stable name kept") &&
         expect(approx(hit.pointMeters.z, 3.5F), "entry point") &&
         expect(hit.checkedEntityCount == 2U, "checked count") &&
         expect(hit.candidateEntityCount == 2U, "candidate count") &&
         expect(hit.reasonCode == "entity_hit", "hit reason") &&
         expect(iggy3d::entityHitStatusName(hit.status) == "hit", "status name");
}

bool ignoredInactiveAndNonAttackableEntitiesAreSkipped() {
  const iggy3d::WorldState world = worldWith({
      entity({1}, "caster", {0.0F, 0.0F, 2.0F}),
      entity({2}, "inactive", {0.0F, 0.0F, 3.0F}, false, true),
      entity({3}, "marker", {0.0F, 0.0F, 4.0F}, true, false),
      entity({4}, "target", {0.0F, 0.0F, 5.0F}),
  });
  iggy3d::EntityHitQueryRequest request = baseRequest(world);
  request.ignoredEntity = {1};
  const iggy3d::EntityHitQueryResult hit = iggy3d::queryFirstEntityHit(request);

  return expect(hit.status == iggy3d::EntityHitStatus::Hit, "filtered hit expected") &&
         expect(hit.entity == iggy3d::EntityId{4}, "filtered target wins") &&
         expect(hit.candidateEntityCount == 1U, "only target candidate");
}

bool radiusExpandsHurtVolume() {
  const iggy3d::WorldState world = worldWith({
      entity({1}, "offset_target", {0.75F, 0.0F, 4.0F}),
  });
  iggy3d::EntityHitQueryRequest request = baseRequest(world);
  const iggy3d::EntityHitQueryResult noHit = iggy3d::queryFirstEntityHit(request);
  request.radiusMeters = 0.30F;
  const iggy3d::EntityHitQueryResult hit = iggy3d::queryFirstEntityHit(request);

  return expect(noHit.status == iggy3d::EntityHitStatus::NoHit, "radius absent no hit") &&
         expect(hit.status == iggy3d::EntityHitStatus::Hit, "radius hit") &&
         expect(hit.entity == iggy3d::EntityId{1}, "radius target");
}

bool invalidInputsAreDiagnosed() {
  const iggy3d::WorldState world = worldWith({
      entity({1}, "target", {0.0F, 0.0F, 4.0F}),
  });
  iggy3d::EntityHitQueryRequest missingWorld = baseRequest(world);
  missingWorld.world = nullptr;
  const iggy3d::EntityHitQueryResult missing =
      iggy3d::queryFirstEntityHit(missingWorld);

  iggy3d::EntityHitQueryRequest zeroLength = baseRequest(world);
  zeroLength.endMeters = zeroLength.startMeters;
  const iggy3d::EntityHitQueryResult invalid =
      iggy3d::queryFirstEntityHit(zeroLength);

  return expect(missing.status == iggy3d::EntityHitStatus::MissingWorld,
                "missing world") &&
         expect(missing.reasonCode == "entity_hit_missing_world", "missing reason") &&
         expect(invalid.status == iggy3d::EntityHitStatus::InvalidInput,
                "invalid input") &&
         expect(invalid.reasonCode == "entity_hit_invalid_input", "invalid reason");
}

}  // namespace

int main() {
  const bool ok = closestAttackableEntityWins() &&
                  ignoredInactiveAndNonAttackableEntitiesAreSkipped() &&
                  radiusExpandsHurtVolume() &&
                  invalidInputsAreDiagnosed();
  return ok ? 0 : 1;
}
