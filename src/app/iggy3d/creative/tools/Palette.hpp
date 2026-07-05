#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/math/Vec3.hpp"
#include "runtime/object/ObjectTraits.hpp"

namespace iggy3d::creative {

enum class Group : std::uint8_t {
  Structure,
  Movement,
  Physics,
  Markers,
  TestLab,
  Unknown,
};

struct Slot {
  Group group = Group::Unknown;
  std::uint32_t slotIndex = 0;
  std::string assetId;
  std::string displayName;
  Vec3 defaultSizeMeters = {1.0F, 1.0F, 1.0F};
  bool placeable = false;
  bool rotatable = false;
  bool scalable = false;
  bool blocksActor = false;
  bool blocksVision = false;
  bool enabled = false;
  std::string reasonCode = "slot_not_built";
};

struct PalView {
  bool ok = false;
  std::string status = "palette_not_built";
  std::string reasonCode = "palette_not_built";
  std::vector<Slot> slots;
  std::uint64_t structureCount = 0;
  std::uint64_t movementCount = 0;
  std::uint64_t physicsCount = 0;
  std::uint64_t markerCount = 0;
  std::uint64_t testLabCount = 0;
  std::uint64_t unknownCount = 0;
  std::uint64_t disabledCount = 0;
};

std::string_view groupName(Group group);

PalView buildPalViewFromCatalog(const ObjectAssetCatalog& catalog);
PalView buildPalView();

const Slot* findSlot(const PalView& palette, std::string_view assetId);

}  // namespace iggy3d::creative
