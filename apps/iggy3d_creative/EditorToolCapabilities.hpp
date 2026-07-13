#pragma once

#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"

#include <cstdint>
#include <span>
#include <type_traits>

namespace iggy3d_creative_app {

enum class CreativeEditorToolOptionFilterProfile : std::uint8_t {
  Standard,
  MaterialPlacement,
  Count,
};

enum class CreativeEditorToolCommandProfile : std::uint8_t {
  None,
  MaterialBrush,
  ObjectGroup,
  ObjectMove,
  Count,
};

enum class CreativeEditorQuickEditProfile : std::uint8_t {
  None,
  Generic,
  TerrainControl,
  TerrainGrade,
  TerrainSculpt,
  TerrainProfile,
  TerrainPath,
  TerrainRegion,
  Count,
};

enum class CreativeEditorActionHintProfile : std::uint8_t {
  None,
  Material,
  MaterialBrush,
  ConnectedFill,
  SurfaceExtrude,
  TerrainControl,
  TerrainPaint,
  TerrainGrade,
  TerrainSculpt,
  TerrainProfile,
  TerrainPath,
  TerrainRegion,
  ObjectSelect,
  ObjectMove,
  ObjectGroup,
  VolumeSelect,
  DirectShapeVolume,
  VolumeOperation,
  LinearArray,
  Count,
};

enum class CreativeEditorToolDisplayProfile : std::uint8_t {
  Standard,
  Material,
  Selection,
  Count,
};

// Presentation-only companion to CreativeHeldItemDefinition. It selects UI
// profiles but never owns world behavior, mutation rules, or control bindings.
struct CreativeEditorToolCapability {
  iggy3d::creative::CreativeHeldItemKind kind =
      iggy3d::creative::CreativeHeldItemKind::Count;
  CreativeEditorToolOptionFilterProfile optionFilterProfile =
      CreativeEditorToolOptionFilterProfile::Standard;
  CreativeEditorToolCommandProfile commandProfile =
      CreativeEditorToolCommandProfile::None;
  CreativeEditorQuickEditProfile quickEditProfile =
      CreativeEditorQuickEditProfile::None;
  CreativeEditorActionHintProfile actionHintProfile =
      CreativeEditorActionHintProfile::None;
  CreativeEditorToolDisplayProfile displayProfile =
      CreativeEditorToolDisplayProfile::Standard;
  bool keyboardQuickEditHints = false;
};

static_assert(std::is_trivially_copyable_v<CreativeEditorToolCapability>);

[[nodiscard]] std::span<const CreativeEditorToolCapability>
creativeEditorToolCapabilities() noexcept;
[[nodiscard]] const CreativeEditorToolCapability&
describeCreativeEditorToolCapability(
    iggy3d::creative::CreativeHeldItemKind kind) noexcept;

}  // namespace iggy3d_creative_app
