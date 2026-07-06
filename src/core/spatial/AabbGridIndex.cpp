#include "core/spatial/AabbGridIndex.hpp"

#include <algorithm>
#include <cmath>

namespace iggy3d {

namespace {

// A cell coordinate that keeps the whole AABB span inside the representable band. Returns false if
// the coordinate falls outside [-kCellBias, kCellBias).
constexpr std::int64_t kCellUpper = (std::int64_t{1} << 20);  // == kCellBias

}  // namespace

AabbGridIndex::AabbGridIndex(float cellSizeMeters)
    : cellSizeMeters_(std::isfinite(cellSizeMeters) && cellSizeMeters > 0.0F
                          ? cellSizeMeters
                          : 8.0F) {}

std::uint64_t AabbGridIndex::packCell(std::int64_t x, std::int64_t y,
                                      std::int64_t z) noexcept {
  const auto bx = static_cast<std::uint64_t>(x + kCellBias) & 0x1FFFFFULL;
  const auto by = static_cast<std::uint64_t>(y + kCellBias) & 0x1FFFFFULL;
  const auto bz = static_cast<std::uint64_t>(z + kCellBias) & 0x1FFFFFULL;
  return (bx << 42) | (by << 21) | bz;
}

bool AabbGridIndex::cellRange(const Aabb3& bounds, std::int64_t& minX,
                              std::int64_t& minY, std::int64_t& minZ,
                              std::int64_t& maxX, std::int64_t& maxY,
                              std::int64_t& maxZ) const noexcept {
  if (boundsStatus(bounds) != AabbGridBoundsStatus::Representable) {
    return false;
  }

  const auto cell = [this](float value) -> std::int64_t {
    return static_cast<std::int64_t>(
        std::floor(static_cast<double>(value) / cellSizeMeters_));
  };

  minX = cell(bounds.min.x);
  minY = cell(bounds.min.y);
  minZ = cell(bounds.min.z);
  maxX = cell(bounds.max.x);
  maxY = cell(bounds.max.y);
  maxZ = cell(bounds.max.z);
  return true;
}

void AabbGridIndex::eraseFromCells(ItemId id, const Aabb3& bounds) {
  std::int64_t minX = 0, minY = 0, minZ = 0, maxX = 0, maxY = 0, maxZ = 0;
  if (!cellRange(bounds, minX, minY, minZ, maxX, maxY, maxZ)) {
    return;
  }
  for (std::int64_t x = minX; x <= maxX; ++x) {
    for (std::int64_t y = minY; y <= maxY; ++y) {
      for (std::int64_t z = minZ; z <= maxZ; ++z) {
        const auto found = cells_.find(packCell(x, y, z));
        if (found == cells_.end()) {
          continue;
        }
        std::vector<ItemId>& bucket = found->second;
        bucket.erase(std::remove(bucket.begin(), bucket.end(), id),
                     bucket.end());
        if (bucket.empty()) {
          cells_.erase(found);
        }
      }
    }
  }
}

bool AabbGridIndex::insert(ItemId id, const Aabb3& bounds) {
  std::int64_t minX = 0, minY = 0, minZ = 0, maxX = 0, maxY = 0, maxZ = 0;
  if (!cellRange(bounds, minX, minY, minZ, maxX, maxY, maxZ)) {
    return false;
  }

  const auto existing = items_.find(id);
  if (existing != items_.end()) {
    eraseFromCells(id, existing->second);
  }

  for (std::int64_t x = minX; x <= maxX; ++x) {
    for (std::int64_t y = minY; y <= maxY; ++y) {
      for (std::int64_t z = minZ; z <= maxZ; ++z) {
        cells_[packCell(x, y, z)].push_back(id);
      }
    }
  }
  items_[id] = bounds;
  return true;
}

bool AabbGridIndex::remove(ItemId id) {
  const auto found = items_.find(id);
  if (found == items_.end()) {
    return false;
  }
  eraseFromCells(id, found->second);
  items_.erase(found);
  return true;
}

void AabbGridIndex::clear() noexcept {
  items_.clear();
  cells_.clear();
}

std::size_t AabbGridIndex::rebuildFrom(std::span<const AabbGridItem> items) {
  clear();
  std::size_t indexed = 0;
  for (const AabbGridItem& item : items) {
    if (insert(item.id, item.bounds)) {
      ++indexed;
    }
  }
  return indexed;
}

AabbGridBoundsStatus AabbGridIndex::boundsStatus(
    const Aabb3& bounds) const noexcept {
  if (!isFinite(bounds) || !isValid(bounds)) {
    return AabbGridBoundsStatus::InvalidBounds;
  }

  const auto cell = [this](float value) -> double {
    return std::floor(static_cast<double>(value) / cellSizeMeters_);
  };

  const double minX = cell(bounds.min.x);
  const double minY = cell(bounds.min.y);
  const double minZ = cell(bounds.min.z);
  const double maxX = cell(bounds.max.x);
  const double maxY = cell(bounds.max.y);
  const double maxZ = cell(bounds.max.z);

  const double lo = static_cast<double>(-kCellUpper);
  const double hi = static_cast<double>(kCellUpper - 1);
  if (minX < lo || minY < lo || minZ < lo || maxX > hi || maxY > hi ||
      maxZ > hi) {
    return AabbGridBoundsStatus::OutOfRange;
  }

  return AabbGridBoundsStatus::Representable;
}

AabbGridQueryResult AabbGridIndex::queryChecked(
    const Aabb3& queryBounds) const {
  AabbGridQueryResult result;
  std::int64_t minX = 0, minY = 0, minZ = 0, maxX = 0, maxY = 0, maxZ = 0;
  if (!cellRange(queryBounds, minX, minY, minZ, maxX, maxY, maxZ)) {
    result.boundsStatus = boundsStatus(queryBounds);
    return result;
  }
  result.boundsStatus = AabbGridBoundsStatus::Representable;

  for (std::int64_t x = minX; x <= maxX; ++x) {
    for (std::int64_t y = minY; y <= maxY; ++y) {
      for (std::int64_t z = minZ; z <= maxZ; ++z) {
        const auto found = cells_.find(packCell(x, y, z));
        if (found == cells_.end()) {
          continue;
        }
        result.candidates.insert(result.candidates.end(), found->second.begin(),
                                 found->second.end());
      }
    }
  }

  std::sort(result.candidates.begin(), result.candidates.end());
  result.candidates.erase(
      std::unique(result.candidates.begin(), result.candidates.end()),
      result.candidates.end());
  return result;
}

std::vector<AabbGridIndex::ItemId> AabbGridIndex::query(
    const Aabb3& queryBounds) const {
  return queryChecked(queryBounds).candidates;
}

AabbGridIndexStats AabbGridIndex::stats() const noexcept {
  AabbGridIndexStats out;
  out.itemCount = items_.size();
  out.cellCount = cells_.size();
  for (const auto& entry : cells_) {
    out.cellSpanCount += entry.second.size();
  }
  return out;
}

}  // namespace iggy3d
