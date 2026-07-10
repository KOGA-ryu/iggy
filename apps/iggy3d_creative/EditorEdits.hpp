#pragma once

#include <cstdint>
#include <string_view>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/tools/Transform.hpp"

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

using StandaloneUndoStack = cr::CreativeDocumentUndoStack;

void clearUndoStack(StandaloneUndoStack& undoStack, std::string_view source);

void pushUndoSnapshot(StandaloneUndoStack& undoStack,
                      const cr::Facade& facade,
                      std::string_view source);

[[nodiscard]] bool undoLastSnapshot(cr::CreativeAppState& appState,
                                    StandaloneUndoStack& undoStack,
                                    std::string_view source);

void discardUndoSnapshot(StandaloneUndoStack& undoStack,
                         std::uint64_t depthBefore,
                         std::string_view source,
                         std::string_view reasonCode);

[[nodiscard]] iggy3d::creative::CreativeDocumentRemoveReceipt
deleteSelectedObject(iggy3d::creative::CreativeAppState& appState,
                     std::string_view source,
                     StandaloneUndoStack* undoStack = nullptr);

[[nodiscard]] cr::CreativeTransformCommandReceipt
transformSelectedObjectsWithUndo(
    cr::CreativeAppState& appState,
    StandaloneUndoStack& undoStack,
    const cr::CreativeTransformCommandRequest& request,
    std::string_view source);

[[nodiscard]] cr::CreativeDuplicateCommandReceipt
duplicateSelectedObjectsWithUndo(
    cr::CreativeAppState& appState,
    StandaloneUndoStack& undoStack,
    const cr::CreativeDuplicateCommandRequest& request,
    std::string_view source);

}  // namespace iggy3d_creative_app
