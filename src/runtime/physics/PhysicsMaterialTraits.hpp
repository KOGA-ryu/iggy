#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/physics/PhysicsTypes.hpp"

namespace iggy3d {

inline constexpr std::uint32_t kPhysicsMaterialFlagWalkable = 1U << 0U;
inline constexpr std::uint32_t kPhysicsMaterialFlagBlocksActor = 1U << 1U;
inline constexpr std::uint32_t kPhysicsMaterialFlagBlocksProjectile =
    1U << 2U;
inline constexpr std::uint32_t kPhysicsMaterialFlagTrigger = 1U << 3U;
inline constexpr std::uint32_t kPhysicsMaterialFlagPushable = 1U << 4U;
inline constexpr std::uint32_t kPhysicsMaterialFlagCarryable = 1U << 5U;
inline constexpr std::uint32_t kPhysicsMaterialFlagBreakable = 1U << 6U;
inline constexpr std::uint32_t kPhysicsMaterialFlagFlammable = 1U << 7U;
inline constexpr std::uint32_t kPhysicsMaterialFlagConductive = 1U << 8U;
inline constexpr std::uint32_t kPhysicsMaterialFlagMagnetic = 1U << 9U;
inline constexpr std::uint32_t kPhysicsMaterialFlagSlippery = 1U << 10U;
inline constexpr std::uint32_t kPhysicsMaterialFlagBouncy = 1U << 11U;
inline constexpr std::uint32_t kPhysicsMaterialFlagSilent = 1U << 12U;

enum class PhysicsMaterialStatus : std::uint8_t {
  Valid,
  MissingDescriptor,
  MissingTable,
  InvalidKey,
  DuplicateKey,
  InvalidStaticFriction,
  InvalidDynamicFriction,
  InvalidRestitution,
  InvalidDampingMultiplier,
  InvalidWeightClass,
  MaterialAdded,
  MaterialNotFound,
  StoreReset,
};

struct PhysicsMaterialDescriptor {
  std::string key;
  float staticFriction = 0.0F;
  float dynamicFriction = 0.0F;
  float restitution = 0.0F;
  float dampingMultiplier = 1.0F;
  PhysicsWeightClass weightClass = PhysicsWeightClass::Medium;
  std::uint32_t flags = 0U;
};

struct PhysicsMaterialView {
  PhysicsMaterialId id;
  std::string key;
  float staticFriction = 0.0F;
  float dynamicFriction = 0.0F;
  float restitution = 0.0F;
  float dampingMultiplier = 1.0F;
  PhysicsWeightClass weightClass = PhysicsWeightClass::Medium;
  std::uint32_t flags = 0U;
};

struct PhysicsMaterialValidationResult {
  bool ok = false;
  PhysicsMaterialStatus status = PhysicsMaterialStatus::MissingDescriptor;
  std::string_view reasonCode = "physics_material_missing_descriptor";
};

struct PhysicsMaterialTableResult {
  bool ok = false;
  PhysicsMaterialStatus status = PhysicsMaterialStatus::MaterialNotFound;
  std::string_view reasonCode = "physics_material_not_found";
  PhysicsMaterialId id;
  std::size_t index = 0U;
  PhysicsMaterialView material;
};

struct PhysicsMaterialPairTraits {
  PhysicsMaterialId firstMaterialId;
  PhysicsMaterialId secondMaterialId;
  float staticFriction = 0.0F;
  float dynamicFriction = 0.0F;
  float restitution = 0.0F;
  float dampingMultiplier = 1.0F;
  std::uint32_t flags = 0U;
  bool trigger = false;
  bool solveContact = true;
};

std::string_view physicsMaterialStatusName(PhysicsMaterialStatus status);
bool hasPhysicsMaterialFlag(std::uint32_t flags, std::uint32_t flag);
bool isValidPhysicsMaterialKey(std::string_view key);
PhysicsMaterialValidationResult validatePhysicsMaterialDescriptor(
    const PhysicsMaterialDescriptor* descriptor);
PhysicsMaterialPairTraits combinePhysicsMaterialPair(
    const PhysicsMaterialView& first,
    const PhysicsMaterialView& second);

class PhysicsMaterialTable {
 public:
  PhysicsMaterialTable() = default;

  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] bool empty() const;
  [[nodiscard]] const std::vector<PhysicsMaterialId>& ids() const;
  [[nodiscard]] const std::vector<std::string>& keys() const;
  [[nodiscard]] const std::vector<float>& staticFrictions() const;
  [[nodiscard]] const std::vector<float>& dynamicFrictions() const;
  [[nodiscard]] const std::vector<float>& restitutions() const;
  [[nodiscard]] const std::vector<float>& dampingMultipliers() const;
  [[nodiscard]] const std::vector<PhysicsWeightClass>& weightClasses() const;
  [[nodiscard]] const std::vector<std::uint32_t>& flags() const;

  PhysicsMaterialTableResult add(
      const PhysicsMaterialDescriptor& descriptor);
  PhysicsMaterialTableResult read(PhysicsMaterialId id) const;
  PhysicsMaterialTableResult findByKey(std::string_view key) const;
  PhysicsMaterialTableResult reset();

 private:
  [[nodiscard]] PhysicsMaterialView materialAt(std::size_t index) const;
  [[nodiscard]] std::size_t findIndex(PhysicsMaterialId id) const;
  [[nodiscard]] std::size_t findKeyIndex(std::string_view key) const;

  std::vector<PhysicsMaterialId> ids_;
  std::vector<std::string> keys_;
  std::vector<float> staticFrictions_;
  std::vector<float> dynamicFrictions_;
  std::vector<float> restitutions_;
  std::vector<float> dampingMultipliers_;
  std::vector<PhysicsWeightClass> weightClasses_;
  std::vector<std::uint32_t> flags_;
  std::uint32_t nextId_ = 1U;
};

PhysicsMaterialTableResult readPhysicsMaterial(
    const PhysicsMaterialTable* table,
    PhysicsMaterialId id);
PhysicsMaterialTableResult findPhysicsMaterialByKey(
    const PhysicsMaterialTable* table,
    std::string_view key);
PhysicsMaterialTable makeBuiltInPhysicsMaterialTable();

}  // namespace iggy3d
