#pragma once

#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"

#include <span>
#include <string_view>

namespace iggy3d::creative {

struct CreativeWorldLayoutBuildingBlockoutMaterializationOptions {
  std::string_view buildingName;
  std::span<const std::string_view> buildingTags;
};

// Appends one complete blockout building to a copied candidate. This is the
// shared semantic owner used by both interactive frontends and deterministic
// map recipes; callers remain responsible for publishing the returned layout.
[[nodiscard]] CreativeWorldLayoutBuildingEditResult
materializeCreativeWorldLayoutBuildingBlockout(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingBlockoutRecipe& recipe,
    std::uint64_t nextStableOrdinal = 1U,
    CreativeWorldLayoutBuildingBlockoutMaterializationOptions options = {});

}  // namespace iggy3d::creative
