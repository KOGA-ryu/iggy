#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/document/Object.hpp"

#include "EditorEdits.hpp"

#include <cstddef>
#include <string_view>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

[[nodiscard]] cr::CreativeDocumentMutationReceipt movePathObjectWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    cr::CreativeVec3 delta,
    std::string_view source);

[[nodiscard]] cr::CreativeDocumentMutationReceipt movePathPointWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    std::size_t pointIndex,
    cr::CreativeVec3 delta,
    std::string_view source);

}  // namespace iggy3d_creative_app
