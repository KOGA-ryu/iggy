#include "runtime/world/WorldState.hpp"

#include <algorithm>
#include <utility>

namespace iggy3d {

namespace {

bool isValidName(const std::string& name) {
  return !name.empty();
}

std::vector<EntityState>::iterator findEntityIterator(
    std::vector<EntityState>& entities,
    EntityId id) {
  return std::find_if(entities.begin(), entities.end(), [id](const EntityState& entity) {
    return entity.id == id;
  });
}

std::vector<EntityState>::const_iterator findEntityIterator(
    const std::vector<EntityState>& entities,
    EntityId id) {
  return std::find_if(entities.begin(), entities.end(), [id](const EntityState& entity) {
    return entity.id == id;
  });
}

EntityId nextAfter(EntityId id) {
  return nextEntityId(id);
}

bool hasDuplicateId(const std::vector<EntityState>& entities, EntityId id, EntityId existingIdToIgnore) {
  if (!isValid(id)) {
    return false;
  }
  for (const EntityState& entity : entities) {
    if (entity.id != existingIdToIgnore && entity.id == id) {
      return true;
    }
  }
  return false;
}

WorldStatus validateEntityForInsert(
    const std::vector<EntityState>& entities,
    const EntityState& entity,
    EntityId existingIdToIgnore) {
  if (!isValidName(entity.stableName)) {
    return WorldStatus::InvalidName;
  }
  for (const EntityState& existing : entities) {
    if (existing.id != existingIdToIgnore && existing.stableName == entity.stableName) {
      return WorldStatus::DuplicateStableName;
    }
  }
  if (hasDuplicateId(entities, entity.id, existingIdToIgnore)) {
    return WorldStatus::InvalidEntityId;
  }
  if (!isValidEntityKind(entity.kind)) {
    return WorldStatus::InvalidKind;
  }
  if (!isFinite(entity.transform) || !hasPositiveFiniteScale(entity.transform)) {
    return WorldStatus::InvalidTransform;
  }
  if (!isValid(entity.localBounds)) {
    return WorldStatus::InvalidBounds;
  }
  return WorldStatus::Ok;
}

WorldEntityResult entityResult(WorldStatus status, EntityId id, std::size_t index = 0) {
  return {status, id, index};
}

WorldMutationResult mutationResult(WorldStatus status, EntityId id) {
  return {status, id};
}

void advanceCursor(EntityId& cursor, EntityId id) {
  if (nextAfter(id).value > cursor.value) {
    cursor = nextAfter(id);
  }
}

}  // namespace

WorldState::WorldState() = default;

const std::vector<EntityState>& WorldState::entities() const {
  return entities_;
}

EntityId WorldState::nextEntityId() const {
  return nextEntityId_;
}

bool WorldState::empty() const {
  return entities_.empty();
}

std::size_t WorldState::size() const {
  return entities_.size();
}

WorldEntityResult WorldState::addEntity(EntityState entity) {
  if (!isValid(entity.id)) {
    entity.id = nextEntityId_;
  } else if (hasDuplicateId(entities_, entity.id, kInvalidEntityId)) {
    return entityResult(WorldStatus::InvalidEntityId, entity.id);
  }
  const WorldStatus status = validateEntityForInsert(entities_, entity, kInvalidEntityId);
  if (status != WorldStatus::Ok) {
    return entityResult(status, entity.id);
  }
  const std::size_t index = entities_.size();
  entities_.push_back(std::move(entity));
  advanceCursor(nextEntityId_, entities_.back().id);
  return entityResult(WorldStatus::Ok, entities_.back().id, index);
}

WorldEntityResult WorldState::seedEntity(EntityState entity) {
  if (!isValid(entity.id)) {
    return entityResult(WorldStatus::InvalidEntityId, entity.id);
  }
  const WorldStatus status = validateEntityForInsert(entities_, entity, kInvalidEntityId);
  if (status != WorldStatus::Ok) {
    return entityResult(status, entity.id);
  }
  const std::size_t index = entities_.size();
  entities_.push_back(std::move(entity));
  advanceCursor(nextEntityId_, entities_.back().id);
  return entityResult(WorldStatus::Ok, entities_.back().id, index);
}

WorldEntityResult WorldState::upsertEntity(EntityState entity) {
  if (!isValid(entity.id)) {
    return addEntity(std::move(entity));
  }
  auto existing = findEntityIterator(entities_, entity.id);
  if (existing == entities_.end()) {
    return seedEntity(std::move(entity));
  }
  const WorldStatus status = validateEntityForInsert(entities_, entity, entity.id);
  if (status != WorldStatus::Ok) {
    return entityResult(status, entity.id);
  }
  const std::size_t index = static_cast<std::size_t>(existing - entities_.begin());
  *existing = std::move(entity);
  return entityResult(WorldStatus::Ok, entities_[index].id, index);
}

const EntityState* WorldState::findById(EntityId id) const {
  const auto found = findEntityIterator(entities_, id);
  return found == entities_.end() ? nullptr : &*found;
}

const EntityState* WorldState::findByStableName(const std::string& stableName) const {
  const auto found = std::find_if(entities_.begin(), entities_.end(),
                                  [&stableName](const EntityState& entity) {
                                    return entity.stableName == stableName;
                                  });
  return found == entities_.end() ? nullptr : &*found;
}

WorldMutationResult WorldState::updateTransform(EntityId id, const Transform3& transform) {
  if (!isValid(id)) {
    return mutationResult(WorldStatus::InvalidEntityId, id);
  }
  if (!isFinite(transform) || !hasPositiveFiniteScale(transform)) {
    return mutationResult(WorldStatus::InvalidTransform, id);
  }
  auto found = findEntityIterator(entities_, id);
  if (found == entities_.end()) {
    return mutationResult(WorldStatus::MissingEntity, id);
  }
  found->transform = transform;
  return mutationResult(WorldStatus::Ok, id);
}

WorldMutationResult WorldState::setActive(EntityId id, bool active) {
  if (!isValid(id)) {
    return mutationResult(WorldStatus::InvalidEntityId, id);
  }
  auto found = findEntityIterator(entities_, id);
  if (found == entities_.end()) {
    return mutationResult(WorldStatus::MissingEntity, id);
  }
  found->active = active;
  return mutationResult(WorldStatus::Ok, id);
}

void WorldState::clear() {
  entities_.clear();
  nextEntityId_ = EntityId{1};
}

void WorldState::resetFromBaseline(const WorldState& baseline) {
  entities_ = baseline.entities_;
  nextEntityId_ = baseline.nextEntityId_;
}

}  // namespace iggy3d
