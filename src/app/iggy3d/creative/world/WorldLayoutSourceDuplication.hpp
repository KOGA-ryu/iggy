#pragma once

#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace iggy3d::creative {

enum class CreativeWorldLayoutSourceDuplicatePolicy : std::uint8_t {
  Unsupported,
  SpecializedOwner,
  OffsetCopy,
};

enum class CreativeWorldLayoutSourceDuplicateStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidSource,
  UnsupportedSource,
  CoordinateOverflow,
  NoValidPlacement,
  Ready,
};

struct CreativeWorldLayoutSourceDuplicateRequest {
  CreativeWorldLayoutTable table = CreativeWorldLayoutTable::None;
  std::size_t sourceIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeGridSettings grid;
  std::uint64_t nextStableOrdinal = 1U;
  bool useExplicitOffset = false;
  std::int64_t deltaXCells = 0;
  std::int64_t deltaZCells = 0;
};

struct CreativeWorldLayoutSourceDuplicateResult {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeWorldLayoutSourceDuplicateStatus status =
      CreativeWorldLayoutSourceDuplicateStatus::NotRequested;
  CreativeWorldLayoutSourceRef source;
  CreativeWorldLayoutSourceRef duplicate;
  std::uint64_t nextStableOrdinal = 1U;
  CreativeWorldLayout edited;
  std::string reasonCode =
      "creative_world_layout_source_duplicate_not_requested";
};

[[nodiscard]] CreativeWorldLayoutSourceDuplicatePolicy
creativeWorldLayoutSourceDuplicatePolicy(
    CreativeWorldLayoutTable table) noexcept;

// Duplicates one independently placeable source into a deterministic valid
// location. Building and level duplication remain with their existing
// whole-subgraph owners; derived and coupled sources reject explicitly.
[[nodiscard]] CreativeWorldLayoutSourceDuplicateResult
duplicateCreativeWorldLayoutSource(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutSourceDuplicateRequest& request);

}  // namespace iggy3d::creative
