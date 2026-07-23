#include "EditorPlaytestNames.hpp"

namespace iggy3d_creative_app {
namespace {

namespace cr = iggy3d::creative;

[[nodiscard]] std::string labelForObjectId(const cr::CreativeDocument& document,
                                           cr::CreativeObjectId objectId) {
  const cr::CreativeObject* object = document.findObject(objectId);
  if (object == nullptr) {
    return {};  // unmapped: caller keeps the id raw
  }
  if (!object->name.empty()) {
    return object->name;
  }
  return std::string(toString(object->kind)) + " " + std::to_string(objectId);
}

}  // namespace

PlaytestEntityNameMap buildPlaytestNameMap(
    const cr::CreativePlayActivationPayload& payload,
    const cr::CreativeDocument& document) {
  PlaytestEntityNameMap names;
  // Seed order mirror: player is always entity 1 (local-player contract).
  std::uint64_t nextEntityId = 1U;
  names.emplace(nextEntityId, "player");
  ++nextEntityId;
  // Then explicit actor plans, in the same object-id order runtime consumes.
  for (const cr::CreativeNpcSpawnPlan& actor : payload.npcSpawns) {
    const std::string label = labelForObjectId(document, actor.objectId);
    if (!label.empty()) {
      names.emplace(nextEntityId, label);
    }
    ++nextEntityId;  // the entity exists either way; unmapped stays raw
  }
  // Then interactables, in payload order (they carry the object id).
  for (const cr::CreativeRuntimeInteractableDefinition& definition :
       payload.interactables) {
    const std::string label = labelForObjectId(document, definition.objectId);
    if (!label.empty()) {
      names.emplace(nextEntityId, label);
    }
    ++nextEntityId;
  }
  return names;
}

}  // namespace iggy3d_creative_app
