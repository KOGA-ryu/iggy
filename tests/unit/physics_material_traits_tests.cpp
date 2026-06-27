#include "runtime/physics/PhysicsMaterialTraits.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

iggy3d::PhysicsMaterialDescriptor material(
    std::string key = "test_material") {
  iggy3d::PhysicsMaterialDescriptor descriptor;
  descriptor.key = std::move(key);
  descriptor.staticFriction = 0.80F;
  descriptor.dynamicFriction = 0.50F;
  descriptor.restitution = 0.25F;
  descriptor.dampingMultiplier = 0.95F;
  descriptor.weightClass = iggy3d::PhysicsWeightClass::Medium;
  descriptor.flags = iggy3d::kPhysicsMaterialFlagWalkable |
                     iggy3d::kPhysicsMaterialFlagPushable;
  return descriptor;
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsWeightClassName(
                    iggy3d::PhysicsWeightClass::Static) == "static",
                "static weight name") &&
         expect(iggy3d::physicsWeightClassName(
                    iggy3d::PhysicsWeightClass::Light) == "light",
                "light weight name") &&
         expect(iggy3d::physicsWeightClassName(
                    iggy3d::PhysicsWeightClass::Medium) == "medium",
                "medium weight name") &&
         expect(iggy3d::physicsWeightClassName(
                    iggy3d::PhysicsWeightClass::Heavy) == "heavy",
                "heavy weight name") &&
         expect(iggy3d::physicsWeightClassName(
                    iggy3d::PhysicsWeightClass::Massive) == "massive",
                "massive weight name") &&
         expect(iggy3d::physicsMaterialStatusName(
                    iggy3d::PhysicsMaterialStatus::InvalidKey) ==
                    "physics_material_invalid_key",
                "invalid key status") &&
         expect(iggy3d::physicsMaterialStatusName(
                    iggy3d::PhysicsMaterialStatus::MaterialAdded) ==
                    "physics_material_added",
                "added status") &&
         expect(!iggy3d::isValidPhysicsMaterialId({0U}),
                "invalid material id") &&
         expect(iggy3d::isValidPhysicsMaterialId({1U}),
                "valid material id");
}

bool validationRejectsInvalidDescriptorFacts() {
  const float inf = std::numeric_limits<float>::infinity();
  iggy3d::PhysicsMaterialDescriptor emptyKey = material("");
  iggy3d::PhysicsMaterialDescriptor invalidKey = material("bad key");
  iggy3d::PhysicsMaterialDescriptor invalidStatic = material();
  invalidStatic.staticFriction = -0.1F;
  iggy3d::PhysicsMaterialDescriptor invalidDynamic = material();
  invalidDynamic.dynamicFriction = inf;
  iggy3d::PhysicsMaterialDescriptor invalidRestitution = material();
  invalidRestitution.restitution = 1.1F;
  iggy3d::PhysicsMaterialDescriptor invalidDamping = material();
  invalidDamping.dampingMultiplier = -0.1F;
  iggy3d::PhysicsMaterialDescriptor invalidWeight = material();
  invalidWeight.weightClass = static_cast<iggy3d::PhysicsWeightClass>(99U);

  const iggy3d::PhysicsMaterialValidationResult missing =
      iggy3d::validatePhysicsMaterialDescriptor(nullptr);
  const iggy3d::PhysicsMaterialValidationResult empty =
      iggy3d::validatePhysicsMaterialDescriptor(&emptyKey);
  const iggy3d::PhysicsMaterialValidationResult key =
      iggy3d::validatePhysicsMaterialDescriptor(&invalidKey);
  const iggy3d::PhysicsMaterialValidationResult staticFriction =
      iggy3d::validatePhysicsMaterialDescriptor(&invalidStatic);
  const iggy3d::PhysicsMaterialValidationResult dynamicFriction =
      iggy3d::validatePhysicsMaterialDescriptor(&invalidDynamic);
  const iggy3d::PhysicsMaterialValidationResult restitution =
      iggy3d::validatePhysicsMaterialDescriptor(&invalidRestitution);
  const iggy3d::PhysicsMaterialValidationResult damping =
      iggy3d::validatePhysicsMaterialDescriptor(&invalidDamping);
  const iggy3d::PhysicsMaterialValidationResult weight =
      iggy3d::validatePhysicsMaterialDescriptor(&invalidWeight);

  return expect(!missing.ok, "missing rejected") &&
         expect(missing.reasonCode == "physics_material_missing_descriptor",
                "missing reason") &&
         expect(!empty.ok, "empty key rejected") &&
         expect(empty.reasonCode == "physics_material_invalid_key",
                "empty key reason") &&
         expect(!key.ok, "invalid key rejected") &&
         expect(key.reasonCode == "physics_material_invalid_key",
                "key reason") &&
         expect(!staticFriction.ok, "static friction rejected") &&
         expect(staticFriction.reasonCode ==
                    "physics_material_invalid_static_friction",
                "static friction reason") &&
         expect(!dynamicFriction.ok, "dynamic friction rejected") &&
         expect(dynamicFriction.reasonCode ==
                    "physics_material_invalid_dynamic_friction",
                "dynamic friction reason") &&
         expect(!restitution.ok, "restitution rejected") &&
         expect(restitution.reasonCode ==
                    "physics_material_invalid_restitution",
                "restitution reason") &&
         expect(!damping.ok, "damping rejected") &&
         expect(damping.reasonCode ==
                    "physics_material_invalid_damping_multiplier",
                "damping reason") &&
         expect(!weight.ok, "weight rejected") &&
         expect(weight.reasonCode == "physics_material_invalid_weight_class",
                "weight reason");
}

bool addAndReadMaterialPreservesSoAFacts() {
  iggy3d::PhysicsMaterialTable table;
  const iggy3d::PhysicsMaterialTableResult added = table.add(material());
  const iggy3d::PhysicsMaterialTableResult read = table.read(added.id);

  return expect(added.ok, "add ok") &&
         expect(added.status == iggy3d::PhysicsMaterialStatus::MaterialAdded,
                "add status") &&
         expect(added.reasonCode == "physics_material_added", "add reason") &&
         expect(added.id.value == 1U, "first id") &&
         expect(added.index == 0U, "first index") &&
         expect(table.size() == 1U, "table count") &&
         expect(table.ids().size() == 1U, "ids soa count") &&
         expect(table.keys().size() == 1U, "keys soa count") &&
         expect(table.staticFrictions().size() == 1U,
                "static friction soa count") &&
         expect(table.dynamicFrictions().size() == 1U,
                "dynamic friction soa count") &&
         expect(table.restitutions().size() == 1U,
                "restitution soa count") &&
         expect(table.dampingMultipliers().size() == 1U,
                "damping soa count") &&
         expect(table.weightClasses().size() == 1U,
                "weight soa count") &&
         expect(table.flags().size() == 1U, "flags soa count") &&
         expect(read.ok, "read ok") &&
         expect(read.material.key == "test_material", "read key") &&
         expect(nearlyEqual(read.material.staticFriction, 0.80F),
                "read static friction") &&
         expect(nearlyEqual(read.material.dynamicFriction, 0.50F),
                "read dynamic friction") &&
         expect(nearlyEqual(read.material.restitution, 0.25F),
                "read restitution") &&
         expect(nearlyEqual(read.material.dampingMultiplier, 0.95F),
                "read damping") &&
         expect(read.material.weightClass ==
                    iggy3d::PhysicsWeightClass::Medium,
                "read weight") &&
         expect(iggy3d::hasPhysicsMaterialFlag(
                    read.material.flags,
                    iggy3d::kPhysicsMaterialFlagWalkable),
                "read walkable flag") &&
         expect(iggy3d::hasPhysicsMaterialFlag(
                    read.material.flags,
                    iggy3d::kPhysicsMaterialFlagPushable),
                "read pushable flag");
}

bool invalidAddDoesNotMutateTable() {
  iggy3d::PhysicsMaterialTable table;
  iggy3d::PhysicsMaterialDescriptor invalid = material("invalid_material");
  invalid.restitution = -0.1F;

  const iggy3d::PhysicsMaterialTableResult added = table.add(invalid);

  return expect(!added.ok, "invalid add rejected") &&
         expect(added.reasonCode == "physics_material_invalid_restitution",
                "invalid add reason") &&
         expect(table.empty(), "table still empty");
}

bool duplicateAndLookupFailuresAreStable() {
  iggy3d::PhysicsMaterialTable table;
  const iggy3d::PhysicsMaterialTableResult added = table.add(material());
  const iggy3d::PhysicsMaterialTableResult duplicate = table.add(material());
  const iggy3d::PhysicsMaterialTableResult byId =
      iggy3d::readPhysicsMaterial(&table, added.id);
  const iggy3d::PhysicsMaterialTableResult byKey =
      iggy3d::findPhysicsMaterialByKey(&table, "test_material");
  const iggy3d::PhysicsMaterialTableResult missingTable =
      iggy3d::findPhysicsMaterialByKey(nullptr, "test_material");
  const iggy3d::PhysicsMaterialTableResult invalidKey =
      table.findByKey("bad key");
  const iggy3d::PhysicsMaterialTableResult missingId = table.read({99U});
  const iggy3d::PhysicsMaterialTableResult invalidId = table.read({0U});
  const iggy3d::PhysicsMaterialTableResult missingKey =
      table.findByKey("missing_material");

  return expect(added.ok, "add ok") &&
         expect(!duplicate.ok, "duplicate rejected") &&
         expect(duplicate.reasonCode == "physics_material_duplicate_key",
                "duplicate reason") &&
         expect(byId.ok, "lookup by id ok") &&
         expect(byId.material.key == "test_material", "lookup by id key") &&
         expect(byKey.ok, "lookup by key ok") &&
         expect(byKey.id.value == added.id.value, "lookup by key id") &&
         expect(!missingTable.ok, "missing table rejected") &&
         expect(missingTable.reasonCode == "physics_material_missing_table",
                "missing table reason") &&
         expect(!invalidKey.ok, "invalid key lookup rejected") &&
         expect(invalidKey.reasonCode == "physics_material_invalid_key",
                "invalid key lookup reason") &&
         expect(!missingId.ok, "missing id rejected") &&
         expect(missingId.reasonCode == "physics_material_not_found",
                "missing id reason") &&
         expect(!invalidId.ok, "invalid id rejected") &&
         expect(invalidId.reasonCode == "physics_material_not_found",
                "invalid id reason") &&
         expect(!missingKey.ok, "missing key rejected") &&
         expect(missingKey.reasonCode == "physics_material_not_found",
                "missing key reason");
}

bool resetClearsTableAndRestartsIds() {
  iggy3d::PhysicsMaterialTable table;
  (void)table.add(material());
  const iggy3d::PhysicsMaterialTableResult reset = table.reset();
  const iggy3d::PhysicsMaterialTableResult added = table.add(material());

  return expect(reset.ok, "reset ok") &&
         expect(reset.reasonCode == "physics_material_store_reset",
                "reset reason") &&
         expect(table.size() == 1U, "one material after reset add") &&
         expect(added.id.value == 1U, "id restarts after reset");
}

bool builtInTableContainsRequiredMaterials() {
  const iggy3d::PhysicsMaterialTable table =
      iggy3d::makeBuiltInPhysicsMaterialTable();
  const iggy3d::PhysicsMaterialTableResult debugFloor =
      table.findByKey("debug_floor");
  const iggy3d::PhysicsMaterialTableResult debugWall =
      table.findByKey("debug_wall");
  const iggy3d::PhysicsMaterialTableResult stone = table.findByKey("stone");
  const iggy3d::PhysicsMaterialTableResult wood = table.findByKey("wood");
  const iggy3d::PhysicsMaterialTableResult metal = table.findByKey("metal");
  const iggy3d::PhysicsMaterialTableResult ice = table.findByKey("ice");
  const iggy3d::PhysicsMaterialTableResult rubber = table.findByKey("rubber");
  const iggy3d::PhysicsMaterialTableResult cloth = table.findByKey("cloth");
  const iggy3d::PhysicsMaterialTableResult trigger = table.findByKey("trigger");

  return expect(table.size() == 9U, "built-in count") &&
         expect(debugFloor.ok, "debug floor present") &&
         expect(debugWall.ok, "debug wall present") &&
         expect(stone.ok, "stone present") &&
         expect(wood.ok, "wood present") &&
         expect(metal.ok, "metal present") &&
         expect(ice.ok, "ice present") &&
         expect(rubber.ok, "rubber present") &&
         expect(cloth.ok, "cloth present") &&
         expect(trigger.ok, "trigger present") &&
         expect(debugFloor.id.value == 1U, "debug floor id") &&
         expect(trigger.id.value == 9U, "trigger id") &&
         expect(nearlyEqual(stone.material.staticFriction, 0.90F),
                "stone static friction") &&
         expect(nearlyEqual(ice.material.dynamicFriction, 0.03F),
                "ice dynamic friction") &&
         expect(nearlyEqual(rubber.material.restitution, 0.85F),
                "rubber restitution") &&
         expect(wood.material.weightClass ==
                    iggy3d::PhysicsWeightClass::Medium,
                "wood weight") &&
         expect(metal.material.weightClass ==
                    iggy3d::PhysicsWeightClass::Heavy,
                "metal weight") &&
         expect(iggy3d::hasPhysicsMaterialFlag(
                    debugFloor.material.flags,
                    iggy3d::kPhysicsMaterialFlagWalkable),
                "debug floor walkable") &&
         expect(iggy3d::hasPhysicsMaterialFlag(
                    debugWall.material.flags,
                    iggy3d::kPhysicsMaterialFlagBlocksActor),
                "debug wall blocks actor") &&
         expect(iggy3d::hasPhysicsMaterialFlag(
                    wood.material.flags,
                    iggy3d::kPhysicsMaterialFlagFlammable),
                "wood flammable") &&
         expect(iggy3d::hasPhysicsMaterialFlag(
                    rubber.material.flags,
                    iggy3d::kPhysicsMaterialFlagBouncy),
                "rubber bouncy") &&
         expect(iggy3d::hasPhysicsMaterialFlag(
                    cloth.material.flags,
                    iggy3d::kPhysicsMaterialFlagSilent),
                "cloth silent") &&
         expect(iggy3d::hasPhysicsMaterialFlag(
                    trigger.material.flags,
                    iggy3d::kPhysicsMaterialFlagTrigger),
                "trigger flag");
}

bool pairCombineIsSymmetricAndUsesMaterialPolicy() {
  const iggy3d::PhysicsMaterialTable table =
      iggy3d::makeBuiltInPhysicsMaterialTable();
  const iggy3d::PhysicsMaterialView stone =
      table.findByKey("stone").material;
  const iggy3d::PhysicsMaterialView ice = table.findByKey("ice").material;
  const iggy3d::PhysicsMaterialView rubber =
      table.findByKey("rubber").material;
  const iggy3d::PhysicsMaterialView trigger =
      table.findByKey("trigger").material;

  const iggy3d::PhysicsMaterialPairTraits stoneIce =
      iggy3d::combinePhysicsMaterialPair(stone, ice);
  const iggy3d::PhysicsMaterialPairTraits iceStone =
      iggy3d::combinePhysicsMaterialPair(ice, stone);
  const iggy3d::PhysicsMaterialPairTraits stoneStone =
      iggy3d::combinePhysicsMaterialPair(stone, stone);
  const iggy3d::PhysicsMaterialPairTraits stoneRubber =
      iggy3d::combinePhysicsMaterialPair(stone, rubber);
  const iggy3d::PhysicsMaterialPairTraits stoneTrigger =
      iggy3d::combinePhysicsMaterialPair(stone, trigger);

  return expect(stoneIce.firstMaterialId.value == iceStone.firstMaterialId.value,
                "symmetric first id") &&
         expect(stoneIce.secondMaterialId.value ==
                    iceStone.secondMaterialId.value,
                "symmetric second id") &&
         expect(nearlyEqual(stoneIce.staticFriction,
                            iceStone.staticFriction),
                "symmetric static friction") &&
         expect(nearlyEqual(stoneIce.dynamicFriction,
                            iceStone.dynamicFriction),
                "symmetric dynamic friction") &&
         expect(nearlyEqual(stoneIce.restitution, iceStone.restitution),
                "symmetric restitution") &&
         expect(stoneIce.staticFriction < stoneStone.staticFriction,
                "ice lowers stone static friction") &&
         expect(stoneIce.dynamicFriction < stoneStone.dynamicFriction,
                "ice lowers stone dynamic friction") &&
         expect(stoneRubber.restitution > stoneStone.restitution,
                "rubber raises restitution") &&
         expect(iggy3d::hasPhysicsMaterialFlag(
                    stoneTrigger.flags,
                    iggy3d::kPhysicsMaterialFlagTrigger),
                "trigger flag carried to pair") &&
         expect(stoneTrigger.trigger, "trigger pair") &&
         expect(!stoneTrigger.solveContact, "trigger pair non-solving");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  validationRejectsInvalidDescriptorFacts() &&
                  addAndReadMaterialPreservesSoAFacts() &&
                  invalidAddDoesNotMutateTable() &&
                  duplicateAndLookupFailuresAreStable() &&
                  resetClearsTableAndRestartsIds() &&
                  builtInTableContainsRequiredMaterials() &&
                  pairCombineIsSymmetricAndUsesMaterialPolicy();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
