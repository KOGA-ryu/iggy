#include "app/iggy3d/creative/tools/Palette.hpp"

#include <array>
#include <cstddef>
#include <string>

namespace iggy3d::creative {
namespace {

struct GroupDescriptor {
  Group group = Group::Unknown;
  std::string_view name = "unknown";
};

struct AssetGroupDescriptor {
  std::string_view assetId;
  Group group = Group::Unknown;
};

struct EditorGroupDescriptor {
  std::string_view editorGroup;
  Group group = Group::Unknown;
};

constexpr std::array kGroupDescriptors{
    GroupDescriptor{Group::Structure,
                                          "structure"},
    GroupDescriptor{Group::Movement,
                                          "movement"},
    GroupDescriptor{Group::Physics,
                                          "physics"},
    GroupDescriptor{Group::Markers,
                                          "markers"},
    GroupDescriptor{Group::TestLab,
                                          "test_lab"},
    GroupDescriptor{Group::Unknown,
                                          "unknown"},
};

constexpr std::array kAssetGroupDescriptors{
    AssetGroupDescriptor{"stone_block_proxy",
                                        Group::Structure},
    AssetGroupDescriptor{"stone_floor_slab",
                                        Group::Structure},
    AssetGroupDescriptor{"stone_wall_panel",
                                        Group::Structure},
    AssetGroupDescriptor{"wood_crate_proxy",
                                        Group::Physics},
};

constexpr std::array kEditorGroupDescriptors{
    EditorGroupDescriptor{"structure",
                                         Group::Structure},
    EditorGroupDescriptor{"movement",
                                         Group::Movement},
    EditorGroupDescriptor{"physics",
                                         Group::Physics},
    EditorGroupDescriptor{"markers",
                                         Group::Markers},
    EditorGroupDescriptor{"test_lab",
                                         Group::TestLab},
    EditorGroupDescriptor{"shapes",
                                         Group::Structure},
};

Group groupForEditorGroup(std::string_view editorGroup) {
  for (const EditorGroupDescriptor& descriptor :
       kEditorGroupDescriptors) {
    // branch-gate: BG-1221
    if (descriptor.editorGroup == editorGroup) {
      return descriptor.group;
    }
  }
  return Group::Unknown;
}

Group groupForAsset(const ObjectAssetDefinition& asset) {
  for (const AssetGroupDescriptor& descriptor :
       kAssetGroupDescriptors) {
    // branch-gate: BG-1221
    if (descriptor.assetId == asset.id.value) {
      return descriptor.group;
    }
  }
  return groupForEditorGroup(asset.editor.paletteGroup);
}

bool isKnownGroup(Group group) {
  return group != Group::Unknown;
}

std::uint64_t& groupCount(PalView& palette,
                          Group group) {
  // branch-gate: BG-1221
  switch (group) {
    case Group::Structure:
      return palette.structureCount;
    case Group::Movement:
      return palette.movementCount;
    case Group::Physics:
      return palette.physicsCount;
    case Group::Markers:
      return palette.markerCount;
    case Group::TestLab:
      return palette.testLabCount;
    case Group::Unknown:
      return palette.unknownCount;
  }
  return palette.unknownCount;
}

Slot slotFromAsset(const ObjectAssetDefinition& asset,
                                         std::uint32_t slotIndex) {
  Slot slot;
  slot.group = groupForAsset(asset);
  slot.slotIndex = slotIndex;
  slot.assetId = asset.id.value;
  slot.displayName = asset.editor.displayName;
  slot.defaultSizeMeters = asset.primitiveShape.sizeMeters;
  slot.placeable = asset.editor.placeable;
  slot.rotatable = asset.editor.rotatable;
  slot.scalable = asset.editor.scalable;
  slot.blocksActor = asset.collision.blocksMovement;
  slot.blocksVision = asset.collision.blocksVision;

  const ObjectValidationResult validation = validateObjectAssetDefinition(&asset);
  slot.enabled = validation.ok && slot.placeable &&
                 isKnownGroup(slot.group);
  // branch-gate: BG-1221
  if (!validation.ok) {
    slot.reasonCode = std::string(validation.reasonCode);
  } else if (!slot.placeable) {  // branch-gate: BG-1221
    slot.reasonCode = "not_placeable";
  } else if (!isKnownGroup(slot.group)) {  // branch-gate: BG-1221
    slot.reasonCode = "unknown_group";
  } else {
    slot.reasonCode = "slot_ready";
  }
  return slot;
}

}  // namespace

std::string_view groupName(
    Group group) {
  for (const GroupDescriptor& descriptor :
       kGroupDescriptors) {
    // branch-gate: BG-1221
    if (descriptor.group == group) {
      return descriptor.name;
    }
  }
  return "unknown";
}

PalView buildPalViewFromCatalog(
    const ObjectAssetCatalog& catalog) {
  PalView palette;
  palette.slots.reserve(catalog.assets.size());
  for (const ObjectAssetDefinition& asset : catalog.assets) {
    std::uint64_t& count = groupCount(palette, groupForAsset(asset));
    const auto slotIndex = static_cast<std::uint32_t>(count);
    Slot slot = slotFromAsset(asset, slotIndex);
    ++count;
    // branch-gate: BG-1221
    if (!slot.enabled) {
      ++palette.disabledCount;
    }
    palette.slots.push_back(std::move(slot));
  }
  palette.ok = true;
  palette.status = "palette_ready";
  palette.reasonCode = "palette_ready";
  return palette;
}

PalView buildPalView() {
  return buildPalViewFromCatalog(makeBuiltInObjectAssetCatalog());
}

const Slot* findSlot(
    const PalView& palette,
    std::string_view assetId) {
  for (const Slot& slot : palette.slots) {
    // branch-gate: BG-1221
    if (slot.assetId == assetId) {
      return &slot;
    }
  }
  return nullptr;
}

}  // namespace iggy3d::creative
