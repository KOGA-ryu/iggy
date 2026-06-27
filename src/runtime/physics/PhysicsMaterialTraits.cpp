#include "runtime/physics/PhysicsMaterialTraits.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace iggy3d {
namespace {

constexpr std::size_t kMissingIndex = std::numeric_limits<std::size_t>::max();

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1091
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

bool validWeightClass(PhysicsWeightClass weightClass) {
  const auto index = static_cast<std::size_t>(weightClass);
  return index < 5U;
}

bool finiteNonNegative(float value) {
  return std::isfinite(value) && value >= 0.0F;
}

bool finiteUnitRange(float value) {
  return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
}

bool validMaterialKeyChar(char value) {
  return (value >= 'a' && value <= 'z') ||
         (value >= '0' && value <= '9') || value == '_';
}

PhysicsMaterialValidationResult validationResult(
    PhysicsMaterialStatus status,
    bool ok) {
  PhysicsMaterialValidationResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsMaterialStatusName(status);
  return result;
}

PhysicsMaterialTableResult tableResult(PhysicsMaterialStatus status, bool ok) {
  PhysicsMaterialTableResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsMaterialStatusName(status);
  return result;
}

PhysicsMaterialDescriptor material(std::string key,
                                   float staticFriction,
                                   float dynamicFriction,
                                   float restitution,
                                   float dampingMultiplier,
                                   PhysicsWeightClass weightClass,
                                   std::uint32_t flags) {
  PhysicsMaterialDescriptor descriptor;
  descriptor.key = std::move(key);
  descriptor.staticFriction = staticFriction;
  descriptor.dynamicFriction = dynamicFriction;
  descriptor.restitution = restitution;
  descriptor.dampingMultiplier = dampingMultiplier;
  descriptor.weightClass = weightClass;
  descriptor.flags = flags;
  return descriptor;
}

}  // namespace

std::string_view physicsMaterialStatusName(PhysicsMaterialStatus status) {
  static constexpr std::array<std::string_view, 13> kNames{
      "physics_material_valid",
      "physics_material_missing_descriptor",
      "physics_material_missing_table",
      "physics_material_invalid_key",
      "physics_material_duplicate_key",
      "physics_material_invalid_static_friction",
      "physics_material_invalid_dynamic_friction",
      "physics_material_invalid_restitution",
      "physics_material_invalid_damping_multiplier",
      "physics_material_invalid_weight_class",
      "physics_material_added",
      "physics_material_not_found",
      "physics_material_store_reset",
  };
  return enumName(status, kNames, "physics_material_invalid_key");
}

bool hasPhysicsMaterialFlag(std::uint32_t flags, std::uint32_t flag) {
  return (flags & flag) != 0U;
}

bool isValidPhysicsMaterialKey(std::string_view key) {
  // branch-gate: BG-1091
  if (key.empty()) {
    return false;
  }
  for (char value : key) {
    // branch-gate: BG-1091
    if (!validMaterialKeyChar(value)) {
      return false;
    }
  }
  return true;
}

PhysicsMaterialValidationResult validatePhysicsMaterialDescriptor(
    const PhysicsMaterialDescriptor* descriptor) {
  // branch-gate: BG-1091
  if (descriptor == nullptr) {
    return validationResult(PhysicsMaterialStatus::MissingDescriptor, false);
  }
  // branch-gate: BG-1091
  if (!isValidPhysicsMaterialKey(descriptor->key)) {
    return validationResult(PhysicsMaterialStatus::InvalidKey, false);
  }
  // branch-gate: BG-1091
  if (!finiteNonNegative(descriptor->staticFriction)) {
    return validationResult(PhysicsMaterialStatus::InvalidStaticFriction,
                            false);
  }
  // branch-gate: BG-1091
  if (!finiteNonNegative(descriptor->dynamicFriction)) {
    return validationResult(PhysicsMaterialStatus::InvalidDynamicFriction,
                            false);
  }
  // branch-gate: BG-1091
  if (!finiteUnitRange(descriptor->restitution)) {
    return validationResult(PhysicsMaterialStatus::InvalidRestitution, false);
  }
  // branch-gate: BG-1091
  if (!finiteUnitRange(descriptor->dampingMultiplier)) {
    return validationResult(PhysicsMaterialStatus::InvalidDampingMultiplier,
                            false);
  }
  // branch-gate: BG-1091
  if (!validWeightClass(descriptor->weightClass)) {
    return validationResult(PhysicsMaterialStatus::InvalidWeightClass, false);
  }
  return validationResult(PhysicsMaterialStatus::Valid, true);
}

PhysicsMaterialPairTraits combinePhysicsMaterialPair(
    const PhysicsMaterialView& first,
    const PhysicsMaterialView& second) {
  PhysicsMaterialPairTraits result;
  result.firstMaterialId = first.id;
  result.secondMaterialId = second.id;
  // branch-gate: BG-1091
  if (result.secondMaterialId.value < result.firstMaterialId.value) {
    std::swap(result.firstMaterialId, result.secondMaterialId);
  }
  result.staticFriction =
      std::sqrt(first.staticFriction * second.staticFriction);
  result.dynamicFriction =
      std::sqrt(first.dynamicFriction * second.dynamicFriction);
  result.restitution = std::max(first.restitution, second.restitution);
  result.dampingMultiplier =
      std::min(first.dampingMultiplier, second.dampingMultiplier);
  result.flags = first.flags | second.flags;
  result.trigger = hasPhysicsMaterialFlag(result.flags,
                                          kPhysicsMaterialFlagTrigger);
  result.solveContact = !result.trigger;
  return result;
}

std::size_t PhysicsMaterialTable::size() const {
  return ids_.size();
}

bool PhysicsMaterialTable::empty() const {
  return ids_.empty();
}

const std::vector<PhysicsMaterialId>& PhysicsMaterialTable::ids() const {
  return ids_;
}

const std::vector<std::string>& PhysicsMaterialTable::keys() const {
  return keys_;
}

const std::vector<float>& PhysicsMaterialTable::staticFrictions() const {
  return staticFrictions_;
}

const std::vector<float>& PhysicsMaterialTable::dynamicFrictions() const {
  return dynamicFrictions_;
}

const std::vector<float>& PhysicsMaterialTable::restitutions() const {
  return restitutions_;
}

const std::vector<float>& PhysicsMaterialTable::dampingMultipliers() const {
  return dampingMultipliers_;
}

const std::vector<PhysicsWeightClass>& PhysicsMaterialTable::weightClasses()
    const {
  return weightClasses_;
}

const std::vector<std::uint32_t>& PhysicsMaterialTable::flags() const {
  return flags_;
}

PhysicsMaterialTableResult PhysicsMaterialTable::add(
    const PhysicsMaterialDescriptor& descriptor) {
  const PhysicsMaterialValidationResult validation =
      validatePhysicsMaterialDescriptor(&descriptor);
  // branch-gate: BG-1091
  if (!validation.ok) {
    PhysicsMaterialTableResult result;
    result.status = validation.status;
    result.reasonCode = validation.reasonCode;
    return result;
  }
  // branch-gate: BG-1091
  if (findKeyIndex(descriptor.key) != kMissingIndex) {
    return tableResult(PhysicsMaterialStatus::DuplicateKey, false);
  }

  const PhysicsMaterialId id{nextId_++};
  ids_.push_back(id);
  keys_.push_back(descriptor.key);
  staticFrictions_.push_back(descriptor.staticFriction);
  dynamicFrictions_.push_back(descriptor.dynamicFriction);
  restitutions_.push_back(descriptor.restitution);
  dampingMultipliers_.push_back(descriptor.dampingMultiplier);
  weightClasses_.push_back(descriptor.weightClass);
  flags_.push_back(descriptor.flags);

  PhysicsMaterialTableResult result =
      tableResult(PhysicsMaterialStatus::MaterialAdded, true);
  result.id = id;
  result.index = ids_.size() - 1U;
  result.material = materialAt(result.index);
  return result;
}

PhysicsMaterialTableResult PhysicsMaterialTable::read(
    PhysicsMaterialId id) const {
  const std::size_t index = findIndex(id);
  // branch-gate: BG-1091
  if (index == kMissingIndex) {
    PhysicsMaterialTableResult result =
        tableResult(PhysicsMaterialStatus::MaterialNotFound, false);
    result.id = id;
    return result;
  }

  PhysicsMaterialTableResult result =
      tableResult(PhysicsMaterialStatus::Valid, true);
  result.id = id;
  result.index = index;
  result.material = materialAt(index);
  return result;
}

PhysicsMaterialTableResult PhysicsMaterialTable::findByKey(
    std::string_view key) const {
  // branch-gate: BG-1091
  if (!isValidPhysicsMaterialKey(key)) {
    return tableResult(PhysicsMaterialStatus::InvalidKey, false);
  }

  const std::size_t index = findKeyIndex(key);
  // branch-gate: BG-1091
  if (index == kMissingIndex) {
    return tableResult(PhysicsMaterialStatus::MaterialNotFound, false);
  }

  PhysicsMaterialTableResult result =
      tableResult(PhysicsMaterialStatus::Valid, true);
  result.id = ids_[index];
  result.index = index;
  result.material = materialAt(index);
  return result;
}

PhysicsMaterialTableResult PhysicsMaterialTable::reset() {
  ids_.clear();
  keys_.clear();
  staticFrictions_.clear();
  dynamicFrictions_.clear();
  restitutions_.clear();
  dampingMultipliers_.clear();
  weightClasses_.clear();
  flags_.clear();
  nextId_ = 1U;
  return tableResult(PhysicsMaterialStatus::StoreReset, true);
}

PhysicsMaterialView PhysicsMaterialTable::materialAt(
    std::size_t index) const {
  PhysicsMaterialView material;
  material.id = ids_[index];
  material.key = keys_[index];
  material.staticFriction = staticFrictions_[index];
  material.dynamicFriction = dynamicFrictions_[index];
  material.restitution = restitutions_[index];
  material.dampingMultiplier = dampingMultipliers_[index];
  material.weightClass = weightClasses_[index];
  material.flags = flags_[index];
  return material;
}

std::size_t PhysicsMaterialTable::findIndex(PhysicsMaterialId id) const {
  // branch-gate: BG-1091
  if (!isValidPhysicsMaterialId(id)) {
    return kMissingIndex;
  }
  for (std::size_t index = 0U; index < ids_.size(); ++index) {
    // branch-gate: BG-1091
    if (ids_[index].value == id.value) {
      return index;
    }
  }
  return kMissingIndex;
}

std::size_t PhysicsMaterialTable::findKeyIndex(std::string_view key) const {
  for (std::size_t index = 0U; index < keys_.size(); ++index) {
    // branch-gate: BG-1091
    if (keys_[index] == key) {
      return index;
    }
  }
  return kMissingIndex;
}

PhysicsMaterialTableResult readPhysicsMaterial(
    const PhysicsMaterialTable* table,
    PhysicsMaterialId id) {
  // branch-gate: BG-1091
  if (table == nullptr) {
    return tableResult(PhysicsMaterialStatus::MissingTable, false);
  }
  return table->read(id);
}

PhysicsMaterialTableResult findPhysicsMaterialByKey(
    const PhysicsMaterialTable* table,
    std::string_view key) {
  // branch-gate: BG-1091
  if (table == nullptr) {
    return tableResult(PhysicsMaterialStatus::MissingTable, false);
  }
  return table->findByKey(key);
}

PhysicsMaterialTable makeBuiltInPhysicsMaterialTable() {
  static const std::array<PhysicsMaterialDescriptor, 9> kBuiltIns{
      material("debug_floor", 0.70F, 0.55F, 0.05F, 0.98F,
               PhysicsWeightClass::Static, kPhysicsMaterialFlagWalkable),
      material("debug_wall", 0.80F, 0.60F, 0.05F, 0.98F,
               PhysicsWeightClass::Static,
               kPhysicsMaterialFlagBlocksActor |
                   kPhysicsMaterialFlagBlocksProjectile),
      material("stone", 0.90F, 0.70F, 0.08F, 0.98F,
               PhysicsWeightClass::Static,
               kPhysicsMaterialFlagBlocksActor |
                   kPhysicsMaterialFlagBlocksProjectile),
      material("wood", 0.65F, 0.45F, 0.12F, 0.96F,
               PhysicsWeightClass::Medium,
               kPhysicsMaterialFlagBlocksActor |
                   kPhysicsMaterialFlagBlocksProjectile |
                   kPhysicsMaterialFlagPushable |
                   kPhysicsMaterialFlagBreakable |
                   kPhysicsMaterialFlagFlammable),
      material("metal", 0.55F, 0.35F, 0.18F, 0.96F,
               PhysicsWeightClass::Heavy,
               kPhysicsMaterialFlagBlocksActor |
                   kPhysicsMaterialFlagBlocksProjectile |
                   kPhysicsMaterialFlagConductive |
                   kPhysicsMaterialFlagMagnetic),
      material("ice", 0.08F, 0.03F, 0.02F, 0.99F,
               PhysicsWeightClass::Static,
               kPhysicsMaterialFlagWalkable | kPhysicsMaterialFlagSlippery),
      material("rubber", 1.20F, 1.00F, 0.85F, 0.92F,
               PhysicsWeightClass::Medium,
               kPhysicsMaterialFlagBlocksActor |
                   kPhysicsMaterialFlagBlocksProjectile |
                   kPhysicsMaterialFlagBouncy),
      material("cloth", 0.60F, 0.50F, 0.02F, 0.90F,
               PhysicsWeightClass::Light,
               kPhysicsMaterialFlagCarryable |
                   kPhysicsMaterialFlagBreakable |
                   kPhysicsMaterialFlagFlammable |
                   kPhysicsMaterialFlagSilent),
      material("trigger", 0.0F, 0.0F, 0.0F, 1.0F,
               PhysicsWeightClass::Static, kPhysicsMaterialFlagTrigger),
  };

  PhysicsMaterialTable table;
  for (const PhysicsMaterialDescriptor& descriptor : kBuiltIns) {
    (void)table.add(descriptor);
  }
  return table;
}

}  // namespace iggy3d
