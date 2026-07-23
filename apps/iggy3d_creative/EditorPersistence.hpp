#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

struct CreativeEditorPersistenceState;

// A minimal per-object snapshot captured from the live document: id + kind +
// position + bounds. Used to prove the standalone save/load round-trip is
// lossless without depending on ids re-minting exactly.
struct ObjectSnapshotEntry {
  cr::CreativeObjectId id = cr::kInvalidObjectId;
  cr::CreativeObjectKind kind = cr::CreativeObjectKind::Unknown;
  cr::CreativeVec3 position{};
  cr::CreativeVec3 boundsMin{};
  cr::CreativeVec3 boundsMax{};
  std::vector<cr::CreativePathPoint> pathPoints;
  cr::CreativeMovingPlatformSettings movingPlatform;
};

[[nodiscard]] std::vector<ObjectSnapshotEntry> snapshotDocument(
    const cr::CreativeDocument& document);

void logDocumentSnapshot(const char* phase,
                         const std::vector<ObjectSnapshotEntry>& snapshot);

[[nodiscard]] bool snapshotsMatch(
    const std::vector<ObjectSnapshotEntry>& before,
    const std::vector<ObjectSnapshotEntry>& after);

[[nodiscard]] iggy3d::CreativeWorldSaveResult saveStandaloneScene(
    cr::Facade& facade,
    const std::filesystem::path& saveRoot,
    const std::string& saveId,
    const cr::CreativeWorldLayout* worldLayout = nullptr,
    bool worldLayoutSynchronized = true);

[[nodiscard]] bool loadStandaloneScene(cr::CreativeAppState& appState,
                                       const std::filesystem::path& saveRoot,
                                       const std::string& saveId,
                                       cr::CreativeWorldLayout* worldLayout =
                                           nullptr);

[[nodiscard]] cr::CreativeFacadeDocumentInstallReceipt clearToBlankScene(
    cr::CreativeAppState& appState);

[[nodiscard]] bool creativeEditorDocumentDirty(
    const CreativeEditorPersistenceState& state,
    const cr::CreativeDocument& document);

void markCreativeEditorDocumentSaved(
    CreativeEditorPersistenceState& state,
    const cr::CreativeDocument& document);

void clearCreativeEditorDocumentSavePoint(
    CreativeEditorPersistenceState& state,
    cr::CreativeDocumentId documentId) noexcept;

}  // namespace iggy3d_creative_app
