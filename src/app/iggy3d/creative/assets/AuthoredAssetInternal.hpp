#pragma once

#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"

namespace iggy3d::creative::authored_asset_internal {

[[nodiscard]] std::string makeSourceFingerprintTag(
    std::uint64_t fingerprint);
[[nodiscard]] CreativeBounds translateAssetBounds(
    CreativeBounds bounds,
    CreativeVec3 offset) noexcept;
[[nodiscard]] bool applyContentTransform(
    CreativeClipboard& clipboard,
    CreativeVec3 anchor,
    CreativeTransform transform,
    bool inverse);

}  // namespace iggy3d::creative::authored_asset_internal
