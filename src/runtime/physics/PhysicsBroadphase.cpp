#include "runtime/physics/PhysicsBroadphase.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace iggy3d {
namespace {

struct CellKey {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::int32_t z = 0;
};

struct CellEntry {
  CellKey cell;
  std::size_t colliderIndex = 0U;
};

struct CandidateKey {
  std::size_t firstIndex = 0U;
  std::size_t secondIndex = 0U;
};

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1088
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsBroadphaseResult broadphaseResult(PhysicsBroadphaseStatus status,
                                         bool ok) {
  PhysicsBroadphaseResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsBroadphaseStatusName(status);
  return result;
}

bool validCellSize(float cellSizeMeters) {
  return std::isfinite(cellSizeMeters) && cellSizeMeters > 0.0F;
}

bool sameBody(const PhysicsAabbCollider& lhs,
              const PhysicsAabbCollider& rhs) {
  return lhs.bodyId.value == rhs.bodyId.value;
}

std::int32_t cellCoordinate(float value, float cellSizeMeters) {
  return static_cast<std::int32_t>(std::floor(value / cellSizeMeters));
}

CellEntry cellEntry(CellKey cell, std::size_t colliderIndex) {
  CellEntry entry;
  entry.cell = cell;
  entry.colliderIndex = colliderIndex;
  return entry;
}

bool cellLess(const CellKey& lhs, const CellKey& rhs) {
  return std::array<std::int32_t, 3>{lhs.x, lhs.y, lhs.z} <
         std::array<std::int32_t, 3>{rhs.x, rhs.y, rhs.z};
}

bool sameCell(const CellKey& lhs, const CellKey& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool cellEntryLess(const CellEntry& lhs, const CellEntry& rhs) {
  return cellLess(lhs.cell, rhs.cell) ||
         (sameCell(lhs.cell, rhs.cell) &&
          lhs.colliderIndex < rhs.colliderIndex);
}

CandidateKey candidateKey(std::size_t lhs, std::size_t rhs) {
  CandidateKey key;
  key.firstIndex = lhs;
  key.secondIndex = rhs;
  // branch-gate: BG-1088
  if (key.secondIndex < key.firstIndex) {
    std::swap(key.firstIndex, key.secondIndex);
  }
  return key;
}

bool candidateLess(const CandidateKey& lhs, const CandidateKey& rhs) {
  return std::array<std::size_t, 2>{lhs.firstIndex, lhs.secondIndex} <
         std::array<std::size_t, 2>{rhs.firstIndex, rhs.secondIndex};
}

bool sameCandidate(const CandidateKey& lhs, const CandidateKey& rhs) {
  return lhs.firstIndex == rhs.firstIndex &&
         lhs.secondIndex == rhs.secondIndex;
}

std::vector<CellEntry> buildCellEntries(
    const std::vector<PhysicsAabbCollider>& colliders,
    float cellSizeMeters) {
  std::vector<CellEntry> entries;
  for (std::size_t index = 0U; index < colliders.size(); ++index) {
    const PhysicsAabbCollider& collider = colliders[index];
    const std::int32_t minX =
        cellCoordinate(collider.bounds.min.x, cellSizeMeters);
    const std::int32_t minY =
        cellCoordinate(collider.bounds.min.y, cellSizeMeters);
    const std::int32_t minZ =
        cellCoordinate(collider.bounds.min.z, cellSizeMeters);
    const std::int32_t maxX =
        cellCoordinate(collider.bounds.max.x, cellSizeMeters);
    const std::int32_t maxY =
        cellCoordinate(collider.bounds.max.y, cellSizeMeters);
    const std::int32_t maxZ =
        cellCoordinate(collider.bounds.max.z, cellSizeMeters);
    for (std::int32_t x = minX; x <= maxX; ++x) {
      for (std::int32_t y = minY; y <= maxY; ++y) {
        for (std::int32_t z = minZ; z <= maxZ; ++z) {
          entries.push_back(cellEntry({x, y, z}, index));
        }
      }
    }
  }
  std::sort(entries.begin(), entries.end(), cellEntryLess);
  return entries;
}

void recordCellBucketStats(const std::vector<CellEntry>& entries,
                           PhysicsBroadphaseResult& result) {
  std::size_t index = 0U;
  while (index < entries.size()) {
    const CellKey cell = entries[index].cell;
    std::size_t end = index + 1U;
    // branch-gate: BG-1088
    while (end < entries.size() && sameCell(entries[end].cell, cell)) {
      ++end;
    }
    ++result.occupiedCellCount;
    result.maxBucketSize = std::max(result.maxBucketSize, end - index);
    index = end;
  }
}

std::vector<CandidateKey> collectCandidateKeys(
    const std::vector<CellEntry>& entries,
    PhysicsBroadphaseResult& result) {
  std::vector<CandidateKey> candidates;
  std::size_t index = 0U;
  while (index < entries.size()) {
    const CellKey cell = entries[index].cell;
    std::size_t end = index + 1U;
    // branch-gate: BG-1088
    while (end < entries.size() && sameCell(entries[end].cell, cell)) {
      ++end;
    }
    for (std::size_t lhs = index; lhs < end; ++lhs) {
      for (std::size_t rhs = lhs + 1U; rhs < end; ++rhs) {
        ++result.candidatePairCount;
        candidates.push_back(candidateKey(entries[lhs].colliderIndex,
                                          entries[rhs].colliderIndex));
      }
    }
    index = end;
  }
  std::sort(candidates.begin(), candidates.end(), candidateLess);
  const auto uniqueEnd =
      std::unique(candidates.begin(), candidates.end(), sameCandidate);
  result.duplicatePairRejectedCount =
      static_cast<std::size_t>(candidates.end() - uniqueEnd);
  candidates.erase(uniqueEnd, candidates.end());
  return candidates;
}

PhysicsBroadphasePair makePair(const PhysicsAabbCollider& lhs,
                               const PhysicsAabbCollider& rhs,
                               std::size_t lhsIndex,
                               std::size_t rhsIndex) {
  PhysicsBroadphasePair pair;
  pair.firstBodyId = lhs.bodyId;
  pair.secondBodyId = rhs.bodyId;
  pair.firstColliderIndex = lhsIndex;
  pair.secondColliderIndex = rhsIndex;
  pair.includesSensor = lhs.sensor || rhs.sensor;
  return pair;
}

}  // namespace

std::string_view physicsBroadphaseStatusName(PhysicsBroadphaseStatus status) {
  static constexpr std::array<std::string_view, 4> kNames{
      "physics_broadphase_pairs_collected",
      "physics_broadphase_missing_colliders",
      "physics_broadphase_invalid_collider",
      "physics_broadphase_invalid_grid_config",
  };
  return enumName(status, kNames, "physics_broadphase_invalid_collider");
}

PhysicsBroadphaseResult collectPhysicsBroadphasePairs(
    const PhysicsBroadphaseRequest& request) {
  // branch-gate: BG-1088
  if (request.colliders == nullptr) {
    return broadphaseResult(PhysicsBroadphaseStatus::MissingColliders, false);
  }
  // branch-gate: BG-1088
  if (!validCellSize(request.cellSizeMeters)) {
    return broadphaseResult(PhysicsBroadphaseStatus::InvalidGridConfig, false);
  }

  PhysicsBroadphaseResult result =
      broadphaseResult(PhysicsBroadphaseStatus::PairsCollected, true);
  result.colliderCount = request.colliders->size();

  for (std::size_t index = 0U; index < request.colliders->size(); ++index) {
    // branch-gate: BG-1088
    if (!isValidPhysicsAabbCollider((*request.colliders)[index])) {
      result.ok = false;
      result.status = PhysicsBroadphaseStatus::InvalidCollider;
      result.reasonCode = physicsBroadphaseStatusName(result.status);
      result.invalidColliderIndex = index;
      result.pairs.clear();
      result.overlappingPairCount = 0U;
      result.testedPairCount = 0U;
      return result;
    }
  }

  const std::vector<CellEntry> cellEntries =
      buildCellEntries(*request.colliders, request.cellSizeMeters);
  result.cellEntryCount = cellEntries.size();
  recordCellBucketStats(cellEntries, result);
  const std::vector<CandidateKey> candidates =
      collectCandidateKeys(cellEntries, result);

  for (const CandidateKey& candidate : candidates) {
    const PhysicsAabbCollider& first =
        (*request.colliders)[candidate.firstIndex];
    const PhysicsAabbCollider& second =
        (*request.colliders)[candidate.secondIndex];
    // branch-gate: BG-1088
    if (sameBody(first, second)) {
      continue;
    }
    ++result.testedPairCount;
    // branch-gate: BG-1088
    if (!physicsAabbOverlaps(first, second)) {
      continue;
    }
    result.pairs.push_back(makePair(first, second, candidate.firstIndex,
                                    candidate.secondIndex));
  }

  result.overlappingPairCount = result.pairs.size();
  return result;
}

}  // namespace iggy3d
