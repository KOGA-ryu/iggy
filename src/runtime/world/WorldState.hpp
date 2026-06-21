#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d {

enum class WorldStatus : std::uint8_t {
  Ok,
  InvalidEntityId,
  DuplicateStableName,
  MissingEntity,
  InvalidTransform,
  InvalidBounds,
  InvalidName,
  InvalidKind,
};

struct WorldEntityResult {
  WorldStatus status = WorldStatus::Ok;
  EntityId id;
  std::size_t index = 0;
};

struct WorldMutationResult {
  WorldStatus status = WorldStatus::Ok;
  EntityId id;
};

class WorldState {
public:
  WorldState();

  const std::vector<EntityState>& entities() const;
  EntityId nextEntityId() const;
  bool empty() const;
  std::size_t size() const;

  WorldEntityResult addEntity(EntityState entity);
  WorldEntityResult seedEntity(EntityState entity);
  WorldEntityResult upsertEntity(EntityState entity);

  const EntityState* findById(EntityId id) const;
  const EntityState* findByStableName(const std::string& stableName) const;

  WorldMutationResult updateTransform(EntityId id, const Transform3& transform);
  WorldMutationResult setActive(EntityId id, bool active);

  void clear();
  void resetFromBaseline(const WorldState& baseline);

private:
  std::vector<EntityState> entities_;
  EntityId nextEntityId_ = EntityId{1};
};

}  // namespace iggy3d
