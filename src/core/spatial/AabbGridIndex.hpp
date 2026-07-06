#pragma once

#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

#include "core/math/Aabb3.hpp"

namespace iggy3d {

// One item's registration in the index: a stable id plus its world-space AABB.
struct AabbGridItem {
  std::uint64_t id = 0;
  Aabb3 bounds{};
};

struct AabbGridIndexStats {
  std::uint64_t itemCount = 0;      // distinct items held
  std::uint64_t cellCount = 0;      // occupied buckets
  std::uint64_t cellSpanCount = 0;  // total (item, cell) incidences
};

enum class AabbGridBoundsStatus {
  Representable,
  InvalidBounds,
  OutOfRange,
};

struct AabbGridQueryResult {
  AabbGridBoundsStatus boundsStatus = AabbGridBoundsStatus::InvalidBounds;
  std::vector<std::uint64_t> candidates;

  [[nodiscard]] bool queried() const noexcept {
    return boundsStatus == AabbGridBoundsStatus::Representable;
  }
};

// A coarse uniform-cell spatial hash over Aabb3 items -- the shared broadphase substrate the
// creative editor's four otherwise-separate lanes all reduce to: ray-pick candidate gather,
// snap-neighborhood search, overlap-at-placement validation, and frustum/cull. Same math as the
// occupancy grid, inverse direction (cell -> items). Buckets by a fixed COARSE cell size (default
// 8 m, NOT the 1 m occupancy resolution).
//
// Broadphase contract: query() returns a SUPERSET of the items whose AABB truly intersects the
// query box -- it never drops a real overlap. The caller does the exact intersects() narrow test.
// Results are de-duplicated and returned in ascending id order (deterministic, platform-stable).
//
// Maintenance mirrors the mutation receipt: insert()/remove() on dirty-flag deltas, rebuildFrom()
// on snapshot boundaries. Inverted (min > max) or non-finite bounds are rejected; zero-extent
// bounds are valid and index deterministically into their containing cell. Bounds whose cells fall
// outside the representable range are also rejected rather than silently truncated.
class AabbGridIndex {
 public:
  using ItemId = std::uint64_t;

  // Non-finite, zero, or negative cell sizes preserve the legacy invalid-input fallback and
  // normalize to the default 8 m cells.
  explicit AabbGridIndex(float cellSizeMeters = 8.0F);

  [[nodiscard]] float cellSizeMeters() const noexcept { return cellSizeMeters_; }
  [[nodiscard]] std::size_t size() const noexcept { return items_.size(); }
  [[nodiscard]] bool empty() const noexcept { return items_.empty(); }
  [[nodiscard]] bool contains(ItemId id) const noexcept {
    return items_.find(id) != items_.end();
  }

  // Insert a new item or replace an existing item's bounds. Returns false (leaving the index
  // unchanged for that id) when the bounds are invalid, non-finite, or out of representable range.
  bool insert(ItemId id, const Aabb3& bounds);
  bool remove(ItemId id);
  void clear() noexcept;

  // Snapshot-boundary rebuild: drop everything and re-insert the given items. Invalid items are
  // skipped; the return value is the number actually indexed.
  std::size_t rebuildFrom(std::span<const AabbGridItem> items);

  // Bounds contract shared by insert and query. Callers that must not confuse an empty query with
  // an unrepresentable query should use queryChecked().
  [[nodiscard]] AabbGridBoundsStatus boundsStatus(
      const Aabb3& bounds) const noexcept;
  [[nodiscard]] bool canRepresent(const Aabb3& bounds) const noexcept {
    return boundsStatus(bounds) == AabbGridBoundsStatus::Representable;
  }

  // Broadphase: candidate ids whose occupied cells overlap `query`, ascending + de-duplicated.
  [[nodiscard]] std::vector<ItemId> query(const Aabb3& query) const;
  [[nodiscard]] AabbGridQueryResult queryChecked(
      const Aabb3& query) const;

  [[nodiscard]] AabbGridIndexStats stats() const noexcept;

 private:
  // Cell coords are biased into [0, 2*kCellBias) and packed 21 bits each into a 64-bit key.
  static constexpr std::int64_t kCellBias = 1 << 20;

  // Conservative cell coverage: both min and max endpoints are floored, so an AABB whose max lies
  // exactly on a grid line also occupies the adjacent cell. This intentionally over-keeps
  // candidates for broadphase safety; exact intersects() remains the caller's narrow phase.
  [[nodiscard]] bool cellRange(const Aabb3& bounds, std::int64_t& minX,
                               std::int64_t& minY, std::int64_t& minZ,
                               std::int64_t& maxX, std::int64_t& maxY,
                               std::int64_t& maxZ) const noexcept;
  static std::uint64_t packCell(std::int64_t x, std::int64_t y,
                                std::int64_t z) noexcept;
  void eraseFromCells(ItemId id, const Aabb3& bounds);

  float cellSizeMeters_;
  std::unordered_map<ItemId, Aabb3> items_;
  std::unordered_map<std::uint64_t, std::vector<ItemId>> cells_;
};

}  // namespace iggy3d
