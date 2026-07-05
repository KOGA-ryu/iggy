#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/spatial/Snap.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeGhostChangeKind : std::uint8_t {
  None,
  HideGhost,
  UpdatePreview,
};

struct CreativeGhostPoint2 {
  double x = 0.0;
  double y = 0.0;
};

struct CreativeGhostState {
  bool visible = false;
  Tool sourceTool = Tool::Select;
  CreativeGhostPoint2 rawPoint;
  CreativeGhostPoint2 snappedPoint;
  TargetRef target;
  bool snapAccepted = false;
  bool snapApplied = false;
  bool snapChanged = false;
  std::uint64_t updateCount = 0;
};

struct CreativeGhostReceipt {
  CreativeGhostChangeKind requestedChange = CreativeGhostChangeKind::None;
  CreativeGhostChangeKind appliedChange = CreativeGhostChangeKind::None;
  bool visibleBefore = false;
  bool visibleAfter = false;
  Tool sourceToolBefore = Tool::Select;
  Tool sourceToolAfter = Tool::Select;
  CreativeGhostPoint2 rawPointBefore;
  CreativeGhostPoint2 rawPointAfter;
  CreativeGhostPoint2 snappedPointBefore;
  CreativeGhostPoint2 snappedPointAfter;
  TargetRef targetBefore;
  TargetRef targetAfter;
  bool snapAccepted = false;
  bool snapApplied = false;
  bool snapChanged = false;
  std::uint64_t updateCountBefore = 0;
  std::uint64_t updateCountAfter = 0;
  bool changed = false;
  bool accepted = false;
  std::string_view message = "no_ghost_change";
};

[[nodiscard]] CreativeGhostState makeDefaultCreativeGhostState() noexcept;
[[nodiscard]] CreativeGhostReceipt hideGhost(
    CreativeGhostState& state) noexcept;
[[nodiscard]] CreativeGhostReceipt updateGhostPreview(
    CreativeGhostState& state,
    const CreativeToolPointerPacket& pointer,
    Tool sourceTool,
    CreativeSnapSettings snapSettings) noexcept;
[[nodiscard]] CreativeGhostReceipt applyGhostToolIntent(
    CreativeGhostState& state,
    const CreativeToolIntent& intent,
    CreativeSnapSettings snapSettings) noexcept;

}  // namespace iggy3d::creative
