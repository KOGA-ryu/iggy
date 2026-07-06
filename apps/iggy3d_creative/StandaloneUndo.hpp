#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"

#include <SDL3/SDL_log.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

using StandaloneUndoStack = cr::CreativeDocumentUndoStack;

inline void clearUndoStack(StandaloneUndoStack& undoStack,
                           std::string_view source) {
  const std::uint64_t depthBefore = cr::creativeUndoDepth(undoStack);
  cr::clearCreativeUndoStack(undoStack);
  if (depthBefore > 0U) {
    SDL_Log("iggy3d_creative: UNDO cleared source='%s' depthBefore=%zu "
            "depthAfter=0",
            std::string(source).c_str(),
            static_cast<std::size_t>(depthBefore));
  }
}

inline void pushUndoSnapshot(StandaloneUndoStack& undoStack,
                             const cr::Facade& facade,
                             std::string_view source) {
  const cr::CreativeDocument& document = facade.document();
  cr::pushCreativeUndoSnapshot(undoStack, document);
  SDL_Log("iggy3d_creative: UNDO pushed source='%s' depth=%zu "
          "objectCount=%llu revision=%llu dirtyFlags=%llu nextObjectId=%llu",
          std::string(source).c_str(),
          static_cast<std::size_t>(cr::creativeUndoDepth(undoStack)),
          static_cast<unsigned long long>(document.objectCount()),
          static_cast<unsigned long long>(document.revision()),
          static_cast<unsigned long long>(document.dirtyFlags()),
          static_cast<unsigned long long>(document.nextObjectId()));
}

inline bool undoLastSnapshot(cr::CreativeAppState& appState,
                             StandaloneUndoStack& undoStack,
                             std::string_view source) {
  const std::uint64_t objectCountBefore =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  if (!cr::creativeUndoAvailable(undoStack)) {
    SDL_Log("iggy3d_creative: UNDO empty source='%s' objectCount=%llu",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectCountBefore));
    return false;
  }

  const cr::CreativeDocumentUndoApplyReceipt receipt =
      cr::applyLastCreativeUndoSnapshot(appState.facade, undoStack);
  const cr::Id selectionAfter =
      appState.facade.selectionState().selectedTarget.value;
  SDL_Log("iggy3d_creative: UNDO applied source='%s' accepted=%d changed=%d "
          "depthBefore=%zu depthAfter=%zu objectCountBefore=%llu "
          "objectCountAfter=%llu revisionAfter=%llu dirtyFlagsAfter=%llu "
          "selectionAfter=%u reasonCode='%s'",
          std::string(source).c_str(), receipt.accepted ? 1 : 0,
          receipt.changed ? 1 : 0,
          static_cast<std::size_t>(receipt.depthBefore),
          static_cast<std::size_t>(receipt.depthAfter),
          static_cast<unsigned long long>(objectCountBefore),
          static_cast<unsigned long long>(receipt.objectCountAfter),
          static_cast<unsigned long long>(appState.facade.document().revision()),
          static_cast<unsigned long long>(appState.facade.document().dirtyFlags()),
          selectionAfter, receipt.reasonCode.c_str());
  return receipt.accepted;
}

inline void discardUndoSnapshot(StandaloneUndoStack& undoStack,
                                std::uint64_t depthBefore,
                                std::string_view source,
                                std::string_view reasonCode) {
  if (!cr::discardCreativeUndoSnapshot(undoStack, depthBefore)) {
    return;
  }
  SDL_Log("iggy3d_creative: UNDO discarded source='%s' depth=%zu "
          "reasonCode='%s'",
          std::string(source).c_str(),
          static_cast<std::size_t>(cr::creativeUndoDepth(undoStack)),
          std::string(reasonCode).c_str());
}

}  // namespace iggy3d_creative_app
