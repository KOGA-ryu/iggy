#pragma once

// Playtest launch: the editor's Play command validates the document, writes
// an immutable snapshot save, and spawns the i3dp process on it. Everything
// except the actual spawn is pure and headless-testable.

#include <filesystem>
#include <string>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "content/assets/StaticMeshAsset.hpp"

namespace iggy3d_creative_app {

// The snapshot lives in a SUBDIRECTORY of the editor save root so the
// editor's whole-root save scans never see it (SaveFileStore's directory
// iteration is non-recursive). One scratch slot, overwritten per Play.
inline constexpr std::string_view kPlaytestSnapshotDirName = "playtest";
inline constexpr std::string_view kPlaytestSnapshotSaveId = "snapshot";

struct PlaytestLaunchPlan {
  bool valid = false;
  std::string reasonCode = "playtest_launch_plan_not_built";
  std::filesystem::path binaryPath;
  std::vector<std::string> argv;  // argv[0] == binaryPath
};

// Pure: joins <basePath>/i3dp and orders the argv exactly as i3dp requires.
// Refuses empty basePath/saveRoot and ids outside [A-Za-z0-9_-]+.
[[nodiscard]] PlaytestLaunchPlan buildPlaytestLaunchPlan(
    const std::filesystem::path& basePath,
    const std::filesystem::path& saveRoot,
    const std::string& saveId);

struct PlaytestSnapshotResult {
  bool ok = false;
  std::string reasonCode = "playtest_snapshot_not_written";
  std::filesystem::path path;
};

// create_directories(<saveRoot>/playtest) then saveCreativeWorld the current
// document as the snapshot slot. The document is copied; the editor's
// live document is untouched.
[[nodiscard]] PlaytestSnapshotResult writePlaytestSnapshot(
    const iggy3d::creative::CreativeDocument& document,
    const std::filesystem::path& editorSaveRoot);

struct PlaytestLaunchPreparation {
  bool accepted = false;
  // On refusal: the validation/snapshot reason (surfaced in the dispatch
  // message). Invalid documents never write a snapshot nor build a plan.
  std::string reasonCode = "playtest_launch_not_prepared";
  PlaytestSnapshotResult snapshot;
  PlaytestLaunchPlan plan;
};

// validate (prepareCreativePlay) -> snapshot -> plan. NO spawn: the caller
// owns the actual process creation so tests can observe the plan instead.
[[nodiscard]] PlaytestLaunchPreparation preparePlaytestLaunch(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* staticMeshAssetCatalog,
    const std::filesystem::path& editorSaveRoot,
    const std::filesystem::path& basePath);

// Fire-and-forget SDL_CreateProcess of a valid plan. Returns false (with
// reason) when SDL refuses; the child owns its own window and lifetime.
[[nodiscard]] bool spawnPlaytestProcess(const PlaytestLaunchPlan& plan,
                                        std::string& reasonCode);

}  // namespace iggy3d_creative_app
