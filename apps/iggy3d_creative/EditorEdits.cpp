#include "EditorEdits.hpp"

#include <SDL3/SDL_log.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Object.hpp"

namespace iggy3d_creative_app {
namespace creative = iggy3d::creative;

void clearUndoStack(StandaloneUndoStack& undoStack,
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

void pushUndoSnapshot(StandaloneUndoStack& undoStack,
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

bool undoLastSnapshot(cr::CreativeAppState& appState,
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

void discardUndoSnapshot(StandaloneUndoStack& undoStack,
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

creative::CreativeDocumentRemoveReceipt deleteSelectedObject(
    creative::CreativeAppState& appState,
    std::string_view source,
    StandaloneUndoStack* undoStack) {
  const creative::Id selectedId =
      appState.facade.selectionState().selectedTarget.value;
  const std::uint64_t objectCountBefore =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  if (selectedId == 0U) {
    SDL_Log("iggy3d_creative: DELETE no selection source='%s' "
            "objectCount=%llu",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectCountBefore));
    return {};
  }

  const auto objectId = static_cast<creative::CreativeObjectId>(selectedId);
  const creative::CreativeObject* object = appState.facade.findObject(objectId);
  if (object == nullptr) {
    SDL_Log("iggy3d_creative: DELETE missing selection source='%s' "
            "objectId=%llu objectCount=%llu",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId),
            static_cast<unsigned long long>(objectCountBefore));
    return {};
  }

  const creative::CreativeObjectKind kind = object->kind;
  const std::size_t undoDepthBefore =
      undoStack != nullptr ? undoStack->documents.size() : 0U;
  if (undoStack != nullptr) {
    pushUndoSnapshot(*undoStack, appState.facade, source);
  }
  creative::CreativeDocumentRemoveReceipt receipt =
      appState.facade.removeDocumentObject(objectId);
  if ((!receipt.accepted || !receipt.objectRemoved) && undoStack != nullptr &&
      undoStack->documents.size() > undoDepthBefore) {
    undoStack->documents.pop_back();
    SDL_Log("iggy3d_creative: UNDO discarded source='%s' depth=%zu "
            "reasonCode='%s'",
            std::string(source).c_str(), undoStack->documents.size(),
            std::string(receipt.reasonCode).c_str());
  }
  const std::uint64_t objectCountAfter =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  const creative::Id selectionAfter =
      appState.facade.selectionState().selectedTarget.value;
  SDL_Log("iggy3d_creative: DELETE removed objectId=%llu kind='%s' "
          "accepted=%d changed=%d removed=%d status='%s' reasonCode='%s' "
          "objectCountBefore=%llu objectCountAfter=%llu selectionAfter=%u",
          static_cast<unsigned long long>(objectId),
          std::string(creative::toString(kind)).c_str(),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          receipt.objectRemoved ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          std::string(receipt.reasonCode).c_str(),
          static_cast<unsigned long long>(objectCountBefore),
          static_cast<unsigned long long>(objectCountAfter), selectionAfter);
  return receipt;
}

}  // namespace iggy3d_creative_app
