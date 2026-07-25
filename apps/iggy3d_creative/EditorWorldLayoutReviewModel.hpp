#pragma once

#include <cstddef>
#include "EditorDesktopCommands.hpp"
#include "EditorWorldLayoutState.hpp"

namespace iggy3d::creative {
struct CreativeCatalogEntry;
struct CreativeCatalogState;
}

namespace iggy3d_creative_app {

struct CreativeEditorWorldLayoutAssetRepairProjection {
  bool visible = false;
  bool refreshBoundsAvailable = false;
  bool proceduralInsertAvailable = false;
  std::size_t compatibleReplacementCount = 0U;
};

[[nodiscard]] bool
creativeEditorWorldLayoutAssetRepairReplacementCompatible(
    const CreativeEditorWorldLayoutDiagnostic& issue,
    const CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeCatalogEntry& entry) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutAssetRepairProjection
projectCreativeEditorWorldLayoutAssetRepair(
    const CreativeEditorWorldLayoutDiagnostic& issue,
    const CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeCatalogState& catalog);

struct CreativeEditorWorldLayoutRecipeChangeProjection {
  std::size_t visibleChangeCount = 0U;
};

[[nodiscard]] bool creativeEditorWorldLayoutRecipeChangeVisible(
    iggy3d::creative::CreativeWorldLayoutRecipeChangeKind kind) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutRecipeChangeProjection
projectCreativeEditorWorldLayoutRecipeChanges(
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics);

struct CreativeEditorWorldLayoutConflictReviewSynchronization {
  bool replace = false;
  CreativeEditorWorldLayoutConflictReviewState review;
};

[[nodiscard]] CreativeEditorWorldLayoutConflictReviewSynchronization
planCreativeEditorWorldLayoutConflictReviewSynchronization(
    const CreativeEditorWorldLayoutConflictReviewState& current,
    std::uint64_t diagnosticBuildCount,
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics);

struct CreativeEditorWorldLayoutConflictDecisionProjection {
  const iggy3d::creative::CreativeWorldLayoutRecipeMemberConflict* conflict =
      nullptr;
  bool groupDecision = false;
  bool concurrent = false;
  bool sourceResolutionAvailable = false;
  iggy3d::creative::CreativeWorldLayoutConflictResolution sourceResolution =
      iggy3d::creative::CreativeWorldLayoutConflictResolution::Block;
  iggy3d::creative::CreativeWorldLayoutConflictResolution outputResolution =
      iggy3d::creative::CreativeWorldLayoutConflictResolution::Block;
};

[[nodiscard]] CreativeEditorWorldLayoutConflictDecisionProjection
projectCreativeEditorWorldLayoutConflictDecision(
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics,
    const iggy3d::creative::CreativeWorldLayoutConflictDecision& decision)
    noexcept;

[[nodiscard]] const iggy3d::creative::CreativeWorldLayoutTerrainConflict*
findCreativeEditorWorldLayoutTerrainConflict(
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics,
    const iggy3d::creative::CreativeWorldLayoutTerrainConflictDecision&
        decision) noexcept;

struct CreativeEditorWorldLayoutConflictReviewProjection {
  bool objectConflicts = false;
  bool terrainConflicts = false;
  bool objectResolved = false;
  bool terrainResolved = false;
  bool allResolved = false;
};

[[nodiscard]] CreativeEditorWorldLayoutConflictReviewProjection
projectCreativeEditorWorldLayoutConflictReview(
    const CreativeEditorWorldLayoutConflictReviewState& review,
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics) noexcept;

[[nodiscard]] CreativeDesktopCommand
planCreativeEditorWorldLayoutDiagnosticFocusCommand(
    const CreativeEditorWorldLayoutDiagnostic& issue);

[[nodiscard]] CreativeDesktopCommand
planCreativeEditorWorldLayoutBuildingRepairCommand(
    const CreativeEditorWorldLayoutDiagnostic& issue);

[[nodiscard]] CreativeDesktopCommand
planCreativeEditorWorldLayoutAssetRepairCommand(
    const CreativeEditorWorldLayoutDiagnostic& issue,
    CreativeDesktopWorldLayoutAssetRepairOperation operation,
    std::string replacementAssetId = {});

[[nodiscard]] CreativeDesktopCommand
planCreativeEditorWorldLayoutTerrainDetachCommand(
    const iggy3d::creative::CreativeWorldLayoutTerrainConflict& conflict);

[[nodiscard]] CreativeDesktopCommand
planCreativeEditorWorldLayoutConflictConfirmCommand(
    const CreativeEditorWorldLayoutConflictReviewState& review);

}  // namespace iggy3d_creative_app
