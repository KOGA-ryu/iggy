#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeWorldLayoutAdoptionStatus : std::uint8_t {
  NotRequested,
  InvalidGrid,
  SourceMissing,
  UnsupportedSource,
  ObjectMismatch,
  NonInvertibleTransform,
  UnrepresentableGeometry,
  Ready,
};

enum class CreativeWorldLayoutAdoptionMode : std::uint8_t {
  Exact,
  Canonicalized,
};

// A bounded inverse of generated world-layout output. Exact plans preserve the
// live object representation byte-for-byte after regeneration. Canonicalized
// plans preserve its axis-aligned world geometry while folding scale into the
// semantic source dimensions.
struct CreativeWorldLayoutAdoptionResult {
  bool accepted = false;
  bool changed = false;
  CreativeWorldLayoutAdoptionStatus status =
      CreativeWorldLayoutAdoptionStatus::NotRequested;
  CreativeWorldLayoutAdoptionMode mode =
      CreativeWorldLayoutAdoptionMode::Exact;
  CreativeWorldLayoutTable table = CreativeWorldLayoutTable::None;
  std::size_t index = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayout candidate;
  std::string_view reasonCode =
      "creative_world_layout_adoption_not_requested";
};

// Only direct object-library symbols and explicit box symbols are invertible
// here. Room-expanded walls, openings, stairs, ramps, and other generated
// fragments must be edited through their semantic source controls.
[[nodiscard]] CreativeWorldLayoutAdoptionResult
planCreativeWorldLayoutObjectAdoption(
    const CreativeWorldLayout& layout,
    const CreativeObject& object,
    CreativeGridSettings grid);

}  // namespace iggy3d::creative
