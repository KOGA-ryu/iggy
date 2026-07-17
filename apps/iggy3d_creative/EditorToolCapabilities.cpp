#include "EditorToolCapabilities.hpp"

#include <array>
#include <cstddef>

namespace iggy3d_creative_app {
namespace {
namespace cr = iggy3d::creative;

using OptionFilter = CreativeEditorToolOptionFilterProfile;
using CommandProfile = CreativeEditorToolCommandProfile;
using QuickEditProfile = CreativeEditorQuickEditProfile;
using HintProfile = CreativeEditorActionHintProfile;
using DisplayProfile = CreativeEditorToolDisplayProfile;

[[nodiscard]] constexpr std::size_t capabilityIndex(
    cr::CreativeHeldItemKind kind) noexcept {
  return static_cast<std::size_t>(kind);
}

[[nodiscard]] consteval auto makeToolCapabilities() {
  std::array<CreativeEditorToolCapability, cr::kCreativeHeldItemKindCount>
      rows{};
  for (std::size_t index = 0U; index < rows.size(); ++index) {
    rows[index].kind = static_cast<cr::CreativeHeldItemKind>(index);
    rows[index].quickEditProfile = QuickEditProfile::Generic;
  }

  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::Material)];
    row.optionFilterProfile = OptionFilter::MaterialPlacement;
    row.actionHintProfile = HintProfile::Material;
    row.displayProfile = DisplayProfile::Material;
  }
  {
    auto& row =
        rows[capabilityIndex(cr::CreativeHeldItemKind::MaterialBrush)];
    row.commandProfile = CommandProfile::MaterialBrush;
    row.actionHintProfile = HintProfile::MaterialBrush;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::ObjectSelect)];
    row.actionHintProfile = HintProfile::ObjectSelect;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::ObjectMove)];
    row.commandProfile = CommandProfile::ObjectMove;
    row.quickEditProfile = QuickEditProfile::None;
    row.actionHintProfile = HintProfile::ObjectMove;
    row.displayProfile = DisplayProfile::Selection;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::VolumeSelect)];
    row.actionHintProfile = HintProfile::VolumeSelect;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::VolumeFill)];
    row.actionHintProfile = HintProfile::DirectShapeVolume;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::VolumeHollow)];
    row.actionHintProfile = HintProfile::DirectShapeVolume;
  }
  {
    auto& row =
        rows[capabilityIndex(cr::CreativeHeldItemKind::VolumeReplace)];
    row.actionHintProfile = HintProfile::VolumeOperation;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::VolumeErase)];
    row.actionHintProfile = HintProfile::VolumeOperation;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::VolumeClone)];
    row.actionHintProfile = HintProfile::VolumeOperation;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::LinearArray)];
    row.actionHintProfile = HintProfile::LinearArray;
  }
  {
    auto& row =
        rows[capabilityIndex(cr::CreativeHeldItemKind::ConnectedFill)];
    row.actionHintProfile = HintProfile::ConnectedFill;
  }
  {
    auto& row =
        rows[capabilityIndex(cr::CreativeHeldItemKind::SurfaceExtrude)];
    row.actionHintProfile = HintProfile::SurfaceExtrude;
  }
  {
    auto& row =
        rows[capabilityIndex(cr::CreativeHeldItemKind::TerrainControl)];
    row.quickEditProfile = QuickEditProfile::TerrainControl;
    row.actionHintProfile = HintProfile::TerrainControl;
    row.keyboardQuickEditHints = true;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::TerrainPaint)];
    row.actionHintProfile = HintProfile::TerrainPaint;
    row.keyboardQuickEditHints = true;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::TerrainGrade)];
    row.quickEditProfile = QuickEditProfile::TerrainGrade;
    row.actionHintProfile = HintProfile::TerrainGrade;
    row.keyboardQuickEditHints = true;
  }
  {
    auto& row =
        rows[capabilityIndex(cr::CreativeHeldItemKind::TerrainSculpt)];
    row.quickEditProfile = QuickEditProfile::TerrainSculpt;
    row.actionHintProfile = HintProfile::TerrainSculpt;
    row.keyboardQuickEditHints = true;
  }
  {
    auto& row =
        rows[capabilityIndex(cr::CreativeHeldItemKind::TerrainProfile)];
    row.quickEditProfile = QuickEditProfile::TerrainProfile;
    row.actionHintProfile = HintProfile::TerrainProfile;
    row.keyboardQuickEditHints = true;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::TerrainPath)];
    row.quickEditProfile = QuickEditProfile::TerrainPath;
    row.actionHintProfile = HintProfile::TerrainPath;
    row.keyboardQuickEditHints = true;
  }
  {
    auto& row =
        rows[capabilityIndex(cr::CreativeHeldItemKind::TerrainRegion)];
    row.quickEditProfile = QuickEditProfile::TerrainRegion;
    row.actionHintProfile = HintProfile::TerrainRegion;
    row.keyboardQuickEditHints = true;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::ObjectGroup)];
    row.commandProfile = CommandProfile::ObjectGroup;
    row.actionHintProfile = HintProfile::ObjectGroup;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::LogicLink)];
    row.quickEditProfile = QuickEditProfile::LogicLink;
    row.actionHintProfile = HintProfile::LogicLink;
    row.keyboardQuickEditHints = true;
  }
  {
    auto& row = rows[capabilityIndex(cr::CreativeHeldItemKind::BuildingRoom)];
    row.actionHintProfile = HintProfile::BuildingRoom;
    row.keyboardQuickEditHints = true;
  }
  return rows;
}

constexpr auto kToolCapabilities = makeToolCapabilities();
static_assert(kToolCapabilities.size() == cr::kCreativeHeldItemKindCount);

constexpr CreativeEditorToolCapability kInvalidToolCapability{};

}  // namespace

std::span<const CreativeEditorToolCapability>
creativeEditorToolCapabilities() noexcept {
  return kToolCapabilities;
}

const CreativeEditorToolCapability& describeCreativeEditorToolCapability(
    cr::CreativeHeldItemKind kind) noexcept {
  const std::size_t index = capabilityIndex(kind);
  return index < kToolCapabilities.size() ? kToolCapabilities[index]
                                          : kInvalidToolCapability;
}

}  // namespace iggy3d_creative_app
