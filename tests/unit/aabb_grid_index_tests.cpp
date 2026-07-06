#include "core/spatial/AabbGridIndex.hpp"

#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {
using iggy3d::Aabb3;
using iggy3d::AabbGridIndex;
using iggy3d::AabbGridItem;
using iggy3d::intersects;
using iggy3d::makeAabb3;
using iggy3d::Vec3;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

Aabb3 box(float minX, float minY, float minZ, float maxX, float maxY,
          float maxZ) {
  return makeAabb3(Vec3{minX, minY, minZ}, Vec3{maxX, maxY, maxZ});
}

bool contains(const std::vector<AabbGridIndex::ItemId>& ids,
              AabbGridIndex::ItemId id) {
  return std::find(ids.begin(), ids.end(), id) != ids.end();
}

// A deterministic fixture spanning positive, negative, and cross-cell-boundary boxes.
std::vector<AabbGridItem> fixture() {
  return {
      {1, box(0.0F, 0.0F, 0.0F, 2.0F, 2.0F, 2.0F)},
      {2, box(1.0F, 1.0F, 1.0F, 3.0F, 3.0F, 3.0F)},     // overlaps 1
      {3, box(20.0F, 0.0F, 20.0F, 22.0F, 2.0F, 22.0F)}, // far away
      {4, box(-9.0F, -9.0F, -9.0F, -7.0F, -7.0F, -7.0F)},
      {5, box(7.5F, 0.0F, 7.5F, 9.5F, 2.0F, 9.5F)},     // straddles the 8 m cell edge
      {6, box(-1.0F, -1.0F, -1.0F, 0.0F, 0.0F, 0.0F)},  // touches 1 at a corner
  };
}

bool emptyIndexQueriesEmpty() {
  const AabbGridIndex index;
  const iggy3d::AabbGridIndexStats stats = index.stats();
  return expect(index.empty(), "fresh index empty") &&
         expect(index.query(box(0, 0, 0, 100, 100, 100)).empty(),
                "empty index yields no candidates") &&
         expect(stats.itemCount == 0 && stats.cellCount == 0,
                "empty index stats zero");
}

bool singleItemHitAndMiss() {
  AabbGridIndex index;
  const bool inserted = index.insert(42, box(0, 0, 0, 2, 2, 2));
  const std::vector<AabbGridIndex::ItemId> hit = index.query(box(1, 1, 1, 3, 3, 3));
  const std::vector<AabbGridIndex::ItemId> miss =
      index.query(box(50, 50, 50, 52, 52, 52));
  return expect(inserted, "single insert accepted") &&
         expect(index.size() == 1U, "size one after insert") &&
         expect(contains(hit, 42), "overlapping query hits") &&
         expect(miss.empty(), "distant query misses");
}

// THE broadphase invariant: the index candidate set is a SUPERSET of the true
// intersects() set -- it never drops a real overlap. Brute force is the oracle.
bool broadphaseNeverDropsTrueOverlap() {
  const std::vector<AabbGridItem> items = fixture();
  AabbGridIndex index;
  for (const AabbGridItem& item : items) {
    index.insert(item.id, item.bounds);
  }

  const Aabb3 queries[] = {
      box(0.5F, 0.5F, 0.5F, 1.5F, 1.5F, 1.5F),
      box(-10.0F, -10.0F, -10.0F, -6.0F, -6.0F, -6.0F),
      box(8.0F, 0.0F, 8.0F, 8.5F, 1.0F, 8.5F),
      box(-1.0F, -1.0F, -1.0F, 0.0F, 0.0F, 0.0F),
      box(100.0F, 100.0F, 100.0F, 101.0F, 101.0F, 101.0F),
  };

  bool ok = true;
  for (const Aabb3& queryBounds : queries) {
    const std::vector<AabbGridIndex::ItemId> candidates = index.query(queryBounds);
    for (const AabbGridItem& item : items) {
      if (intersects(item.bounds, queryBounds)) {
        ok = ok && expect(contains(candidates, item.id),
                          "index never drops a true overlap");
      }
    }
  }
  return ok;
}

bool queryResultsAscendingAndDeduped() {
  const std::vector<AabbGridItem> items = fixture();
  AabbGridIndex index;
  for (const AabbGridItem& item : items) {
    index.insert(item.id, item.bounds);
  }
  // A box big enough to gather items spanning multiple cells (so dedup matters).
  const std::vector<AabbGridIndex::ItemId> candidates =
      index.query(box(-2.0F, -2.0F, -2.0F, 10.0F, 3.0F, 10.0F));
  bool ascending = std::is_sorted(candidates.begin(), candidates.end());
  bool unique = std::adjacent_find(candidates.begin(), candidates.end()) ==
                candidates.end();
  return expect(ascending, "candidates ascending") &&
         expect(unique, "candidates de-duplicated");
}

bool insertReplaceMovesItem() {
  AabbGridIndex index;
  index.insert(7, box(0, 0, 0, 1, 1, 1));
  index.insert(7, box(40, 40, 40, 41, 41, 41));  // same id, new place
  return expect(index.size() == 1U, "replace keeps single item") &&
         expect(!contains(index.query(box(0, 0, 0, 1, 1, 1)), 7),
                "old location no longer returns item") &&
         expect(contains(index.query(box(40, 40, 40, 41, 41, 41)), 7),
                "new location returns item");
}

bool removeDropsItem() {
  AabbGridIndex index;
  index.insert(9, box(0, 0, 0, 2, 2, 2));
  const bool removed = index.remove(9);
  const bool removedAgain = index.remove(9);
  return expect(removed, "remove reports success") &&
         expect(!removedAgain, "second remove reports absent") &&
         expect(index.empty(), "index empty after remove") &&
         expect(index.query(box(0, 0, 0, 2, 2, 2)).empty(),
                "removed item not queryable");
}

bool rebuildMatchesIncrementalInserts() {
  const std::vector<AabbGridItem> items = fixture();
  AabbGridIndex incremental;
  for (const AabbGridItem& item : items) {
    incremental.insert(item.id, item.bounds);
  }
  AabbGridIndex rebuilt;
  const std::size_t indexed = rebuilt.rebuildFrom(items);

  const Aabb3 probe = box(-2, -2, -2, 25, 3, 25);
  return expect(indexed == items.size(), "rebuild indexes every valid item") &&
         expect(incremental.query(probe) == rebuilt.query(probe),
                "rebuild query matches incremental query") &&
         expect(incremental.size() == rebuilt.size(), "rebuild size matches");
}

bool invalidBoundsRejected() {
  AabbGridIndex index;
  const bool inverted = index.insert(1, box(2, 2, 2, 0, 0, 0));  // min > max
  const float inf = std::numeric_limits<float>::infinity();
  const bool nonFinite = index.insert(2, box(0, 0, 0, inf, 1, 1));
  return expect(!inverted, "inverted bounds rejected") &&
         expect(!nonFinite, "non-finite bounds rejected") &&
         expect(index.empty(), "index unchanged after rejected inserts");
}

}  // namespace

int main() {
  const bool ok = emptyIndexQueriesEmpty() && singleItemHitAndMiss() &&
                  broadphaseNeverDropsTrueOverlap() &&
                  queryResultsAscendingAndDeduped() && insertReplaceMovesItem() &&
                  removeDropsItem() && rebuildMatchesIncrementalInserts() &&
                  invalidBoundsRejected();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
