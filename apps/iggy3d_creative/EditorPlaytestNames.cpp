#include "EditorPlaytestNames.hpp"

#include <string_view>

namespace iggy3d_creative_app {
namespace {

namespace cr = iggy3d::creative;

constexpr std::string_view kStableObjectPrefix = "creative_object_";

// "creative_object_17" -> 17; 0 when the name is not of that shape.
[[nodiscard]] cr::CreativeObjectId objectIdFromStableName(
    std::string_view stableName) noexcept {
  if (stableName.substr(0, kStableObjectPrefix.size()) !=
      kStableObjectPrefix) {
    return cr::kInvalidObjectId;
  }
  const std::string_view digits =
      stableName.substr(kStableObjectPrefix.size());
  if (digits.empty()) {
    return cr::kInvalidObjectId;
  }
  cr::CreativeObjectId id = 0;
  for (const char value : digits) {
    if (value < '0' || value > '9') {
      return cr::kInvalidObjectId;
    }
    id = id * 10U + static_cast<cr::CreativeObjectId>(value - '0');
  }
  return id;
}

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
  // Then npc/monster anchors, in payload anchor order (== bake order).
  for (const iggy3d::RoomAnchorAsset& anchor : payload.room.anchors) {
    if (anchor.kind != "npc" && anchor.kind != "monster") {
      continue;
    }
    const std::string label = labelForObjectId(
        document, objectIdFromStableName(anchor.runtimeStableName));
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
