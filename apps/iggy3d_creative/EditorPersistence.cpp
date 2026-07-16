#include "EditorPersistence.hpp"

#include <cmath>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "EditorPlacement.hpp"

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
        std::fabs(a.z - b.z) >= kEps) {
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
    const cr::Facade& facade,
    const std::filesystem::path& saveRoot,
    const std::string& saveId) {
  cr::CreativeDocument docCopy = facade.document();  // Copy: save drains.
  iggy3d::CreativeWorldSaveRequest request;
  request.saveRoot = saveRoot;
  request.saveId = saveId;
  request.document = &docCopy;  // Mutable pointer at the local copy.
  request.worldTitle = "standalone";
  request.saveTitle = "scene";
  const iggy3d::CreativeWorldSaveResult result =
      iggy3d::saveCreativeWorld(request);
  SDL_Log("iggy3d_creative: SAVE accepted=%d saved=%d objectCount=%llu "
          "path='%s' reasonCode='%s'",
          result.accepted ? 1 : 0, result.saved ? 1 : 0,
          static_cast<unsigned long long>(result.objectCount),
          result.path.generic_string().c_str(), result.reasonCode.c_str());
  return result;
}

bool loadStandaloneScene(cr::CreativeAppState& appState,
                         const std::filesystem::path& saveRoot,
                         const std::string& saveId) {
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
  return installReceipt.accepted;
}

void clearToBlankScene(cr::CreativeAppState& appState) {
  cr::CreativeDocument blank = cr::CreativeDocument::create("blank");
  (void)blank.assignId(1);
  const cr::CreativeFacadeDocumentInstallReceipt installReceipt =
      appState.facade.installDocument(std::move(blank));
  SDL_Log("iggy3d_creative: NEW/CLEAR install accepted=%d objectCount=%llu",
          installReceipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(
              appState.facade.document().objectCount()));
}

}  // namespace iggy3d_creative_app
