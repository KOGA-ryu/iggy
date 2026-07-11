#include "runtime/world/WorldState.hpp"

#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"

#include <iostream>
#include <limits>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::Transform3 transformAt(float x, float y, float z) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = {x, y, z};
  return transform;
}

iggy3d::Aabb3 unitBounds() {
  return iggy3d::makeAabb3({-0.5F, 0.0F, -0.5F}, {0.5F, 1.0F, 0.5F});
}

iggy3d::EntityState makePlayer() {
  iggy3d::EntityState entity;
  entity.stableName = "player";
  entity.kind = iggy3d::EntityKind::Player;
  entity.transform = transformAt(0.0F, 0.0F, 0.0F);
  entity.localBounds = unitBounds();
  entity.targeting.targetable = false;
  return entity;
}

iggy3d::EntityState makeGoldKey() {
  iggy3d::EntityState entity;
  entity.stableName = "gold_key";
  entity.kind = iggy3d::EntityKind::Pickup;
  entity.transform = transformAt(3.0F, 0.0F, 0.0F);
  entity.localBounds = iggy3d::makeAabb3({-0.1F, 0.0F, -0.1F}, {0.1F, 0.1F, 0.1F});
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::TargetAction::Interact, iggy3d::TargetAction::Inspect};
  entity.interaction.kind = iggy3d::InteractionKind::Pickup;
  entity.interaction.primaryEffect = iggy3d::InteractionEffectKind::AddItemToInventory;
  entity.interaction.itemId = "gold_key";
  entity.interaction.itemCount = 1;
  entity.interaction.objectiveId = "collect_gold_key";
  entity.interaction.deactivateTargetOnSuccess = true;
  return entity;
}

iggy3d::EntityState makeMarker() {
  iggy3d::EntityState entity;
  entity.stableName = "tactical_marker_alpha";
  entity.kind = iggy3d::EntityKind::Marker;
  entity.transform = transformAt(2.0F, 0.0F, 1.0F);
  entity.localBounds = iggy3d::makeAabb3({-0.1F, 0.0F, -0.1F}, {0.1F, 0.1F, 0.1F});
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::TargetAction::Move, iggy3d::TargetAction::Inspect};
  return entity;
}

iggy3d::WorldState seededWorld() {
  iggy3d::WorldState world;
  iggy3d::EntityState player = makePlayer();
  player.id = {1};
  iggy3d::EntityState gold = makeGoldKey();
  gold.id = {2};
  iggy3d::EntityState marker = makeMarker();
  marker.id = {3};
  (void)world.seedEntity(player);
  (void)world.seedEntity(gold);
  (void)world.seedEntity(marker);
  return world;
}

bool defaultWorldStartsEmptyWithNextIdOne() {
  const iggy3d::WorldState world;
  return expect(world.empty(), "default empty") && expect(world.size() == 0U, "default size") &&
         expect(world.nextEntityId() == iggy3d::EntityId{1}, "default next id");
}

bool addEntityAssignsIdsInOrder() {
  iggy3d::WorldState world;
  const auto player = world.addEntity(makePlayer());
  const auto gold = world.addEntity(makeGoldKey());
  const auto marker = world.addEntity(makeMarker());
  return expect(player.status == iggy3d::WorldStatus::Ok && player.id == iggy3d::EntityId{1},
                "add player id") &&
         expect(gold.status == iggy3d::WorldStatus::Ok && gold.id == iggy3d::EntityId{2},
                "add gold id") &&
         expect(marker.status == iggy3d::WorldStatus::Ok && marker.id == iggy3d::EntityId{3},
                "add marker id") &&
         expect(world.entities()[0].stableName == "player" &&
                    world.entities()[1].stableName == "gold_key" &&
                    world.entities()[2].stableName == "tactical_marker_alpha",
                "add order") &&
         expect(world.nextEntityId() == iggy3d::EntityId{4}, "add next id");
}

bool seedEntityPreservesExplicitFixtureIds() {
  const iggy3d::WorldState world = seededWorld();
  return expect(world.entities()[0].id == iggy3d::EntityId{1}, "seed player id") &&
         expect(world.entities()[1].id == iggy3d::EntityId{2}, "seed gold id") &&
         expect(world.entities()[2].id == iggy3d::EntityId{3}, "seed marker id") &&
         expect(world.nextEntityId() == iggy3d::EntityId{4}, "seed next id");
}

bool findByIdAndStableNameReturnExpectedRecords() {
  const iggy3d::WorldState world = seededWorld();
  const iggy3d::EntityState* goldById = world.findById({2});
  const iggy3d::EntityState* goldByName = world.findByStableName("gold_key");
  return expect(goldById != nullptr && goldById->stableName == "gold_key", "find id") &&
         expect(goldByName != nullptr && goldByName->id == iggy3d::EntityId{2}, "find name") &&
         expect(world.findById({99}) == nullptr, "missing id") &&
         expect(world.findByStableName("missing") == nullptr, "missing name");
}

bool upsertReplacesSameIdWithoutReordering() {
  iggy3d::WorldState world = seededWorld();
  iggy3d::EntityState gold = *world.findById({2});
  gold.active = false;
  const auto result = world.upsertEntity(gold);
  return expect(result.status == iggy3d::WorldStatus::Ok && result.index == 1U, "upsert result") &&
         expect(!world.entities()[1].active, "upsert active") &&
         expect(world.entities()[0].stableName == "player" &&
                    world.entities()[2].stableName == "tactical_marker_alpha",
                "upsert order");
}

bool updateTransformValidatesBeforeMutation() {
  iggy3d::WorldState world = seededWorld();
  bool ok = expect(world.updateTransform({1}, transformAt(2.0F, 0.0F, 0.0F)).status ==
                       iggy3d::WorldStatus::Ok,
                   "update ok");
  ok = ok && expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                        iggy3d::Vec3{2.0F, 0.0F, 0.0F}),
                    "update position");
  const iggy3d::Transform3 before = world.findById({1})->transform;
  ok = ok && expect(world.updateTransform({99}, transformAt(5.0F, 0.0F, 0.0F)).status ==
                       iggy3d::WorldStatus::MissingEntity,
                   "update missing");
  iggy3d::Transform3 invalid = transformAt(3.0F, 0.0F, 0.0F);
  invalid.position.x = std::numeric_limits<float>::quiet_NaN();
  ok = ok && expect(world.updateTransform({1}, invalid).status ==
                       iggy3d::WorldStatus::InvalidTransform,
                   "update invalid");
  return ok && expect(iggy3d::nearlyEqual(world.findById({1})->transform.position, before.position),
                      "failed update unchanged");
}

bool setActiveRejectsMissingIdWithoutMutation() {
  iggy3d::WorldState world = seededWorld();
  bool ok = expect(world.setActive({2}, false).status == iggy3d::WorldStatus::Ok, "active ok") &&
            expect(!world.findById({2})->active, "active changed");
  const std::size_t size = world.size();
  const iggy3d::EntityId nextId = world.nextEntityId();
  ok = ok && expect(world.setActive({99}, true).status == iggy3d::WorldStatus::MissingEntity,
                    "active missing");
  return ok && expect(world.size() == size && world.nextEntityId() == nextId, "active failed unchanged");
}

bool duplicateStableNameIsRejected() {
  iggy3d::WorldState world;
  (void)world.addEntity(makePlayer());
  iggy3d::EntityState duplicate = makeGoldKey();
  duplicate.stableName = "player";
  const iggy3d::EntityId beforeNext = world.nextEntityId();
  const auto result = world.addEntity(duplicate);
  return expect(result.status == iggy3d::WorldStatus::DuplicateStableName, "duplicate name") &&
         expect(world.size() == 1U && world.nextEntityId() == beforeNext, "duplicate unchanged");
}

bool invalidEntityDataIsRejectedBeforeInsert() {
  bool ok = true;
  auto assertRejected = [&ok](iggy3d::EntityState entity, iggy3d::WorldStatus status) {
    iggy3d::WorldState world;
    const iggy3d::EntityId beforeNext = world.nextEntityId();
    const auto result = world.addEntity(entity);
    ok = ok && expect(result.status == status, "invalid entity status") &&
         expect(world.empty() && world.nextEntityId() == beforeNext, "invalid entity unchanged");
  };
  iggy3d::EntityState entity = makePlayer();
  entity.stableName.clear();
  assertRejected(entity, iggy3d::WorldStatus::InvalidName);
  entity = makePlayer();
  entity.kind = iggy3d::EntityKind::Unknown;
  assertRejected(entity, iggy3d::WorldStatus::InvalidKind);
  entity = makePlayer();
  entity.transform.position.y = std::numeric_limits<float>::infinity();
  assertRejected(entity, iggy3d::WorldStatus::InvalidTransform);
  entity = makePlayer();
  entity.localBounds.min.x = 2.0F;
  entity.localBounds.max.x = 1.0F;
  assertRejected(entity, iggy3d::WorldStatus::InvalidBounds);
  return ok;
}

bool resetFromBaselineRestoresExactCopy() {
  const iggy3d::WorldState baseline = seededWorld();
  iggy3d::WorldState mutated = baseline;
  (void)mutated.updateTransform({1}, transformAt(5.0F, 0.0F, 0.0F));
  (void)mutated.setActive({2}, false);
  mutated.resetFromBaseline(baseline);
  return expect(iggy3d::nearlyEqual(mutated.findById({1})->transform.position,
                                    iggy3d::Vec3{0.0F, 0.0F, 0.0F}),
                "reset position") &&
         expect(mutated.findById({2})->active, "reset active") &&
         expect(mutated.entities()[1].stableName == baseline.entities()[1].stableName,
                "reset metadata") &&
         expect(mutated.nextEntityId() == baseline.nextEntityId(), "reset next id");
}

}  // namespace

int main() {
  const bool ok = defaultWorldStartsEmptyWithNextIdOne() && addEntityAssignsIdsInOrder() &&
                  seedEntityPreservesExplicitFixtureIds() &&
                  findByIdAndStableNameReturnExpectedRecords() &&
                  upsertReplacesSameIdWithoutReordering() &&
                  updateTransformValidatesBeforeMutation() &&
                  setActiveRejectsMissingIdWithoutMutation() && duplicateStableNameIsRejected() &&
                  invalidEntityDataIsRejectedBeforeInsert() && resetFromBaselineRestoresExactCopy();
  return ok ? 0 : 1;
}
