#include "EditorPersistence.hpp"

#include <cmath>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "EditorPlacement.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/world/DocumentSection.hpp"

namespace iggy3d_creative_app {
namespace {

bool samePathPoints(const std::vector<cr::CreativePathPoint>& lhs,
                    const std::vector<cr::CreativePathPoint>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  constexpr double kEps = 1.0e-6;
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    const cr::CreativeVec3& a = lhs[index].position;
    const cr::CreativeVec3& b = rhs[index].position;
    if (std::fabs(a.x - b.x) >= kEps || std::fabs(a.y - b.y) >= kEps ||
        std::fabs(a.z - b.z) >= kEps ||
        lhs[index].dwellSeconds != rhs[index].dwellSeconds ||
        lhs[index].outgoingSpeedMultiplier !=
            rhs[index].outgoingSpeedMultiplier) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::vector<ObjectSnapshotEntry> snapshotDocument(
    const cr::CreativeDocument& document) {
  std::vector<ObjectSnapshotEntry> out;
  out.reserve(document.objectCount());
  for (const cr::CreativeObject& obj : document.objects()) {
    ObjectSnapshotEntry e;
    e.id = obj.id;
    e.kind = obj.kind;
    e.position = obj.transform.position;
    e.boundsMin = obj.bounds.min;
    e.boundsMax = obj.bounds.max;
    e.pathPoints = obj.pathPoints;
    e.movingPlatform = obj.movingPlatform;
    out.push_back(e);
  }
  return out;
}

void logDocumentSnapshot(const char* phase,
                         const std::vector<ObjectSnapshotEntry>& snapshot) {
  SDL_Log("iggy3d_creative: SNAPSHOT %s objectCount=%zu", phase,
          snapshot.size());
  for (const ObjectSnapshotEntry& e : snapshot) {
    SDL_Log("iggy3d_creative: SNAPSHOT %s   id=%llu kind='%s' "
            "pos=(%.3f, %.3f, %.3f) "
            "bounds=[(%.3f,%.3f,%.3f)..(%.3f,%.3f,%.3f)] "
            "pathPointCount=%zu pathPoints='%s'",
            phase, static_cast<unsigned long long>(e.id),
            std::string(cr::toString(e.kind)).c_str(), e.position.x,
            e.position.y, e.position.z, e.boundsMin.x, e.boundsMin.y,
            e.boundsMin.z, e.boundsMax.x, e.boundsMax.y, e.boundsMax.z,
            e.pathPoints.size(), pathPointsSummary(e.pathPoints).c_str());
  }
}

bool snapshotsMatch(const std::vector<ObjectSnapshotEntry>& before,
                    const std::vector<ObjectSnapshotEntry>& after) {
  if (before.size() != after.size()) {
    return false;
  }
  const auto vecEq = [](const cr::CreativeVec3& a,
                        const cr::CreativeVec3& b) {
    constexpr double kEps = 1.0e-6;
    return std::fabs(a.x - b.x) < kEps && std::fabs(a.y - b.y) < kEps &&
           std::fabs(a.z - b.z) < kEps;
  };
  for (std::size_t i = 0; i < before.size(); ++i) {
    if (before[i].kind != after[i].kind ||
        !vecEq(before[i].position, after[i].position) ||
        !vecEq(before[i].boundsMin, after[i].boundsMin) ||
        !vecEq(before[i].boundsMax, after[i].boundsMax) ||
        !samePathPoints(before[i].pathPoints, after[i].pathPoints) ||
        before[i].movingPlatform != after[i].movingPlatform) {
      return false;
    }
  }
  return true;
}

iggy3d::CreativeWorldSaveResult saveStandaloneScene(
    cr::Facade& facade,
    const std::filesystem::path& saveRoot,
    const std::string& saveId,
    const cr::CreativeWorldLayout* worldLayout,
    bool worldLayoutSynchronized) {
  if (worldLayout != nullptr && !worldLayoutSynchronized) {
    iggy3d::CreativeWorldSaveResult rejected;
    rejected.status = "creative_world_save_layout_not_generated";
    rejected.reasonCode = "creative_world_layout_not_generated";
    rejected.saveId = saveId;
    SDL_Log("iggy3d_creative: SAVE rejected unsynchronized world layout");
    return rejected;
  }
  const cr::CreativeDocumentId documentId = facade.document().id();
  const std::uint64_t documentRevision = facade.document().revision();
  cr::CreativeDocument docCopy = facade.document();  // Copy: save drains.
  iggy3d::CreativeWorldSaveRequest request;
  request.saveRoot = saveRoot;
  request.saveId = saveId;
  request.document = &docCopy;  // Mutable pointer at the local copy.
  request.worldLayout = worldLayout;
  request.worldTitle = "standalone";
  request.saveTitle = "scene";
  iggy3d::CreativeWorldSaveResult result =
      iggy3d::saveCreativeWorld(request);
  if (result.accepted && result.saved) {
    const cr::CreativeFacadeDocumentSaveAcknowledgeReceipt acknowledged =
        facade.acknowledgeDocumentSaved(documentId, documentRevision);
    result.dirtyFlagsDrained = acknowledged.dirtyFlagsDrained;
    result.dirtyFlagsAfter = acknowledged.dirtyFlagsAfter;
    if (!acknowledged.accepted) {
      result.accepted = false;
      result.status =
          "creative_world_save_live_acknowledgement_failed";
      result.reasonCode = std::string(acknowledged.reasonCode);
    }
  }
  SDL_Log("iggy3d_creative: SAVE accepted=%d saved=%d objectCount=%llu "
          "path='%s' reasonCode='%s'",
          result.accepted ? 1 : 0, result.saved ? 1 : 0,
          static_cast<unsigned long long>(result.objectCount),
          result.path.generic_string().c_str(), result.reasonCode.c_str());
  return result;
}

bool loadStandaloneScene(cr::CreativeAppState& appState,
                         const std::filesystem::path& saveRoot,
                         const std::string& saveId,
                         cr::CreativeWorldLayout* worldLayout) {
  iggy3d::CreativeWorldOpenRequest request;
  request.saveRoot = saveRoot;
  request.saveId = saveId;
  iggy3d::CreativeWorldOpenResult result = iggy3d::openCreativeWorld(request);
  SDL_Log("iggy3d_creative: LOAD accepted=%d objectCount=%llu reasonCode='%s'",
          result.accepted ? 1 : 0,
          static_cast<unsigned long long>(result.objectCount),
          result.reasonCode.c_str());
  if (!result.accepted) {
    return false;
  }
  const cr::CreativeFacadeDocumentInstallReceipt installReceipt =
      appState.facade.installDocument(std::move(result.document));
  SDL_Log("iggy3d_creative: LOAD install accepted=%d objectCount=%llu",
          installReceipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(
              appState.facade.document().objectCount()));
  if (!installReceipt.accepted) {
    return false;
  }
  if (worldLayout != nullptr) {
    *worldLayout = result.worldLayoutPresent
                       ? std::move(result.worldLayout)
                       : cr::CreativeWorldLayout{};
  }
  return true;
}

cr::CreativeFacadeDocumentInstallReceipt clearToBlankScene(
    cr::CreativeAppState& appState) {
  cr::CreativeDocument blank = cr::CreativeDocument::create("blank");
  (void)blank.assignId(1);
  cr::CreativeFacadeDocumentInstallReceipt installReceipt =
      appState.facade.installDocument(std::move(blank));
  SDL_Log("iggy3d_creative: NEW/CLEAR install accepted=%d objectCount=%llu",
          installReceipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(
              appState.facade.document().objectCount()));
  return installReceipt;
}

bool creativeEditorDocumentDirty(
    const CreativeEditorPersistenceState& state,
    const cr::CreativeDocument& document) {
  if (!state.hasSavePoint || state.documentId != document.id()) {
    return true;
  }
  if (!state.cachedFingerprintValid ||
      state.cachedDocumentId != document.id() ||
      state.cachedRevision != document.revision()) {
    state.cachedDocumentId = document.id();
    state.cachedRevision = document.revision();
    state.cachedFingerprint =
        iggy3d::fingerprintSaveCreativeDocumentSection(document);
    state.cachedFingerprintValid = true;
  }
  return state.cachedFingerprint == 0U ||
         state.cachedFingerprint != state.savedFingerprint;
}

void markCreativeEditorDocumentSaved(
    CreativeEditorPersistenceState& state,
    const cr::CreativeDocument& document) {
  const std::uint64_t fingerprint =
      iggy3d::fingerprintSaveCreativeDocumentSection(document);
  state.documentId = document.id();
  state.savedRevision = document.revision();
  state.savedFingerprint = fingerprint;
  state.hasSavePoint = document.isValid() &&
                       document.id() != cr::kInvalidDocumentId &&
                       fingerprint != 0U;
  state.cachedDocumentId = document.id();
  state.cachedRevision = document.revision();
  state.cachedFingerprint = fingerprint;
  state.cachedFingerprintValid = true;
}

void clearCreativeEditorDocumentSavePoint(
    CreativeEditorPersistenceState& state,
    cr::CreativeDocumentId documentId) noexcept {
  state.documentId = documentId;
  state.savedRevision = 0U;
  state.savedFingerprint = 0U;
  state.hasSavePoint = false;
  state.cachedDocumentId = cr::kInvalidDocumentId;
  state.cachedRevision = 0U;
  state.cachedFingerprint = 0U;
  state.cachedFingerprintValid = false;
}

}  // namespace iggy3d_creative_app
