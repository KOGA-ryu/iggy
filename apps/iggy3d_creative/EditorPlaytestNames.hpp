#pragma once

// Snapshot-time entity name resolution for the Play Monitor. The map is
// captured AT PLAY TIME from the same document state the snapshot was
// written from, and lives on the owner beside the process handle (replaced
// per spawn) -- NEVER resolved against the live document, which Ace can be
// editing while the child runs. Unknown or unmapped ids render raw.
//
// Determinism bridge (verified two slices ago, re-verified here): session
// entity ids are assigned 1..N in seed order (SessionStateTransitions
// createWorld), and the sandbox seed order is player first (id 1, the
// local-player contract), then normalized NPC plans in object-id order, then
// interactables -- all reproducible editor-side from the activation payload.

#include <cstdint>
#include <string>
#include <unordered_map>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/play/PlayPreparation.hpp"

namespace iggy3d_creative_app {

using PlaytestEntityNameMap = std::unordered_map<std::uint64_t, std::string>;

// Pure: replicates the sandbox seed order over the activation payload and
// labels each entity id from the document -- object name if set, else
// "<kind> <objectId>". Entity 1 is always "player".
[[nodiscard]] PlaytestEntityNameMap buildPlaytestNameMap(
    const iggy3d::creative::CreativePlayActivationPayload& payload,
    const iggy3d::creative::CreativeDocument& document);

}  // namespace iggy3d_creative_app
