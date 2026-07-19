#include "app/iggy3d/creative/document/TerrainHeightField.hpp"

#include <algorithm>
#include <limits>

namespace iggy3d::creative {
namespace {

[[nodiscard]] std::size_t fieldCellCount(
    CreativeTerrainHeightFieldBounds bounds) noexcept {
  return static_cast<std::size_t>(bounds.widthCells) * bounds.depthCells;
}

[[nodiscard]] bool validHeight(std::uint16_t height) noexcept {
  return height == kCreativeTerrainEmptyHeightCells ||
         (height >= kCreativeTerrainMinimumHeightCells &&
          height <= kCreativeTerrainMaximumHeightCells);
}

}  // namespace

bool isValidCreativeTerrainHeightFieldBounds(
    CreativeTerrainHeightFieldBounds bounds) noexcept {
  if (bounds.widthCells == 0U || bounds.depthCells == 0U ||
      fieldCellCount(bounds) > kCreativeTerrainHeightFieldCellCapacity) {
    return false;
  }
  const std::int64_t maximumX =
      static_cast<std::int64_t>(bounds.minimum.x) + bounds.widthCells - 1;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(bounds.minimum.z) + bounds.depthCells - 1;
  return maximumX <= std::numeric_limits<std::int32_t>::max() &&
         maximumZ <= std::numeric_limits<std::int32_t>::max();
}

std::string_view toString(
    CreativeTerrainHeightFieldReplaceStatus status) noexcept {
  switch (status) {
    case CreativeTerrainHeightFieldReplaceStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainHeightFieldReplaceStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainHeightFieldReplaceStatus::InvalidBounds:
      return "InvalidBounds";
    case CreativeTerrainHeightFieldReplaceStatus::InvalidHeights:
      return "InvalidHeights";
    case CreativeTerrainHeightFieldReplaceStatus::NoChange:
      return "NoChange";
    case CreativeTerrainHeightFieldReplaceStatus::Applied:
      return "Applied";
  }
  return "Unknown";
}

bool CreativeTerrainHeightField::isValid() const noexcept {
  return valid_;
}

bool CreativeTerrainHeightField::validateInvariants() const noexcept {
  if (!valid_) {
    return false;
  }
  if (heights_.empty()) {
    return bounds_ == CreativeTerrainHeightFieldBounds{};
  }
  return isValidCreativeTerrainHeightFieldBounds(bounds_) &&
         heights_.size() == fieldCellCount(bounds_) &&
         std::all_of(heights_.begin(), heights_.end(), validHeight);
}

std::uint64_t CreativeTerrainHeightField::revision() const noexcept {
  return revision_;
}

CreativeTerrainHeightFieldBounds CreativeTerrainHeightField::bounds()
    const noexcept {
  return bounds_;
}

std::uint64_t CreativeTerrainHeightField::cellCount() const noexcept {
  return heights_.size();
}

std::uint64_t CreativeTerrainHeightField::presentCellCount() const noexcept {
  return static_cast<std::uint64_t>(std::count_if(
      heights_.begin(), heights_.end(), [](std::uint16_t height) {
        return height != kCreativeTerrainEmptyHeightCells;
      }));
}

std::span<const std::uint16_t> CreativeTerrainHeightField::heights()
    const noexcept {
  return heights_;
}

bool CreativeTerrainHeightField::contains(
    CreativeTerrainCoord2 coord) const noexcept {
  if (heights_.empty()) {
    return false;
  }
  const std::int64_t offsetX =
      static_cast<std::int64_t>(coord.x) - bounds_.minimum.x;
  const std::int64_t offsetZ =
      static_cast<std::int64_t>(coord.z) - bounds_.minimum.z;
  return offsetX >= 0 && offsetX < bounds_.widthCells && offsetZ >= 0 &&
         offsetZ < bounds_.depthCells;
}

std::optional<std::uint16_t> CreativeTerrainHeightField::heightAt(
    CreativeTerrainCoord2 coord) const noexcept {
  if (!contains(coord)) {
    return std::nullopt;
  }
  const std::size_t offsetX = static_cast<std::size_t>(
      static_cast<std::int64_t>(coord.x) - bounds_.minimum.x);
  const std::size_t offsetZ = static_cast<std::size_t>(
      static_cast<std::int64_t>(coord.z) - bounds_.minimum.z);
  return heights_[offsetZ * bounds_.widthCells + offsetX];
}

CreativeTerrainHeightFieldReplaceReceipt CreativeTerrainHeightField::replace(
    CreativeTerrainHeightFieldBounds bounds,
    std::span<const std::uint16_t> heights) {
  CreativeTerrainHeightFieldReplaceReceipt receipt;
  receipt.requested = true;
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;
  receipt.cellCountBefore = heights_.size();
  receipt.cellCountAfter = heights_.size();
  if (!validateInvariants()) {
    receipt.status = CreativeTerrainHeightFieldReplaceStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_height_field_invalid";
    return receipt;
  }
  if (!isValidCreativeTerrainHeightFieldBounds(bounds)) {
    receipt.status = CreativeTerrainHeightFieldReplaceStatus::InvalidBounds;
    receipt.reasonCode = "creative_terrain_height_field_bounds_invalid";
    return receipt;
  }
  if (heights.size() != fieldCellCount(bounds) ||
      !std::all_of(heights.begin(), heights.end(), validHeight)) {
    receipt.status = CreativeTerrainHeightFieldReplaceStatus::InvalidHeights;
    receipt.reasonCode = "creative_terrain_height_field_heights_invalid";
    return receipt;
  }
  if (bounds == bounds_ && std::equal(heights.begin(), heights.end(),
                                     heights_.begin(), heights_.end())) {
    receipt.accepted = true;
    receipt.status = CreativeTerrainHeightFieldReplaceStatus::NoChange;
    receipt.reasonCode = "creative_terrain_height_field_no_change";
    return receipt;
  }

  bounds_ = bounds;
  heights_.assign(heights.begin(), heights.end());
  ++revision_;
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeTerrainHeightFieldReplaceStatus::Applied;
  receipt.revisionAfter = revision_;
  receipt.cellCountAfter = heights_.size();
  receipt.reasonCode = "creative_terrain_height_field_applied";
  return receipt;
}

void CreativeTerrainHeightField::clear() noexcept {
  bounds_ = {};
  heights_.clear();
  revision_ = 0U;
  valid_ = true;
}

}  // namespace iggy3d::creative
