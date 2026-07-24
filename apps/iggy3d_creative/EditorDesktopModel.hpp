#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

// UI-4A pure desktop model. ImGui-free and mutation-free: it projects document
// objects into the rows the Project panel renders, filters them, plans
// selection transitions, resolves selection against document truth, and
// validates Inspector transform drafts. Widgets own no derivation, and nothing
// here retains a CreativeObject pointer — callers key caches on document id +
// revision (see CreativeDesktopOutlinerModel).

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

// Why a row is not where its parentId claims it should be.
enum class CreativeDesktopHierarchyRecovery : std::uint8_t {
  None,
  MissingParent,  // parentId names an object the document does not contain.
  Cycle,          // unreachable from any valid root (parent cycle).
  DepthLimit,     // deeper than the displayed depth cap; clamped, not dropped.
};

struct CreativeDesktopOutlinerRow {
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  // kInvalidObjectId when the object has no parentId.
  cr::CreativeObjectId parentObjectId = cr::kInvalidObjectId;
  cr::CreativeObjectKind kind = cr::CreativeObjectKind::Unknown;
  std::string name;
  std::uint32_t depth = 0U;
  bool hasChildren = false;
  // Local flags are editable; effective flags include every ancestor.
  bool visible = true;
  bool locked = false;
  bool effectivelyVisible = true;
  bool effectivelyLocked = false;
  CreativeDesktopHierarchyRecovery recovery =
      CreativeDesktopHierarchyRecovery::None;
};

struct CreativeDesktopOutlinerModel {
  cr::CreativeDocumentId documentId = cr::kInvalidDocumentId;
  std::uint64_t documentRevision = 0U;
  std::vector<CreativeDesktopOutlinerRow> rows;
  std::size_t recoveredRowCount = 0U;
};

struct CreativeDesktopSelectionPlan {
  std::vector<cr::CreativeObjectId> objectIds;
  cr::CreativeObjectId primaryObjectId = cr::kInvalidObjectId;
  cr::CreativeObjectId nextAnchorObjectId = cr::kInvalidObjectId;
  bool accepted = false;
};

enum class CreativeDesktopSelectionGesture : std::uint8_t {
  Replace,
  Toggle,
  VisibleRange,
};

enum class CreativeDesktopHierarchySelectionScope : std::uint8_t {
  Parent,
  DirectChildren,
  Subtree,
};

// Modifier policy, pure so precedence is pinned by tests rather than by ImGui:
// range (Shift) wins when both modifiers are down; toggle is Cmd on macOS or
// Ctrl elsewhere; neither means a plain replace.
[[nodiscard]] CreativeDesktopSelectionGesture creativeDesktopSelectionGestureFor(
    bool rangeModifier,
    bool toggleModifier) noexcept;

// Valid selection resolved against current document truth. Stale ids (objects
// removed since the selection was made) are discarded, so the Inspector never
// trusts a cached id across a revision.
struct CreativeDesktopSelectionResolution {
  std::vector<cr::CreativeObjectId> objectIds;
  cr::CreativeObjectId primaryObjectId = cr::kInvalidObjectId;
};

// The persistent selection projected from the selection tool's target refs
// into desktop object ids, before document-truth filtering.
struct CreativeDesktopLiveSelection {
  std::vector<cr::CreativeObjectId> objectIds;
  cr::CreativeObjectId primaryObjectId = cr::kInvalidObjectId;
};

[[nodiscard]] CreativeDesktopLiveSelection creativeDesktopLiveSelection(
    const cr::CreativeSelectionState& selectionState);

// Deepest displayed depth. Deeper descendants stay as rows clamped to this
// depth and carry DepthLimit recovery rather than disappearing.
inline constexpr std::uint32_t kCreativeDesktopMaxOutlinerDepth = 64U;

// Builds the deterministic hierarchy: one id index and one child adjacency in
// source order, an iterative parent-before-child traversal of valid roots, then
// any still-unvisited object appended once as a recovered root in source order.
// Document order is preserved among roots and among siblings; every object is
// emitted exactly once.
[[nodiscard]] CreativeDesktopOutlinerModel buildCreativeDesktopOutlinerModel(
    cr::CreativeDocumentId documentId,
    std::uint64_t documentRevision,
    std::span<const cr::CreativeObject> objects);
[[nodiscard]] CreativeDesktopOutlinerModel buildCreativeDesktopOutlinerModel(
    const cr::CreativeDocument& document);

// Row indices (in model order) matching an ASCII case-insensitive query over
// object name, object-kind label, and decimal object id. An empty query returns
// every row. A matching descendant keeps its whole ancestor path so the result
// retains hierarchy context.
[[nodiscard]] std::vector<std::size_t> filterCreativeDesktopOutlinerRows(
    const CreativeDesktopOutlinerModel& model,
    std::string_view query);

// Plans the next selection from the currently filtered visible-order ids, the
// live selection, the clicked row, the stored anchor, and the gesture. Plain and
// toggle clicks move the anchor; a range retains it. A range whose anchor is
// absent from the visible rows falls back to a plain replace.
[[nodiscard]] CreativeDesktopSelectionPlan planCreativeDesktopSelection(
    std::span<const cr::CreativeObjectId> visibleObjectIds,
    std::span<const cr::CreativeObjectId> currentSelectedIds,
    cr::CreativeObjectId primaryObjectId,
    cr::CreativeObjectId clickedObjectId,
    cr::CreativeObjectId anchorObjectId,
    CreativeDesktopSelectionGesture gesture);

// Plans explicit hierarchy navigation in deterministic Outliner row order.
// Parent and direct-child scopes replace the selection with exactly that
// relationship; Subtree includes the requested root and every descendant.
// Results above the core selection capacity fail atomically.
[[nodiscard]] CreativeDesktopSelectionPlan
planCreativeDesktopHierarchySelection(
    const CreativeDesktopOutlinerModel& model,
    cr::CreativeObjectId objectId,
    CreativeDesktopHierarchySelectionScope scope);

// Keeps only ids the document still contains (order preserved) and resolves the
// primary: the requested primary when still valid, else the first valid id.
[[nodiscard]] CreativeDesktopSelectionResolution
resolveCreativeDesktopSelection(
    std::span<const cr::CreativeObject> objects,
    std::span<const cr::CreativeObjectId> selectedObjectIds,
    cr::CreativeObjectId primaryObjectId);
[[nodiscard]] CreativeDesktopSelectionResolution
resolveCreativeDesktopSelection(
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeObjectId> selectedObjectIds,
    cr::CreativeObjectId primaryObjectId);

// Degrees live only at the panel boundary; the document stays radians.
[[nodiscard]] cr::CreativeVec3 creativeDesktopRadiansToDegrees(
    cr::CreativeVec3 radians) noexcept;
[[nodiscard]] cr::CreativeVec3 creativeDesktopDegreesToRadians(
    cr::CreativeVec3 degrees) noexcept;

struct CreativeDesktopTransformDraft {
  bool valid = false;
  std::string message;             // concise inline error when !valid.
  cr::CreativeTransform transform;  // rotation in radians; valid only when valid.
};

enum class CreativeDesktopPropertyEditIntent : std::uint8_t {
  None,
  Preview,
  Commit,
  Cancel,
};

// ImGui item state is sampled at the widget boundary, then reduced here so
// every Inspector property family obeys the same preview/commit law.
struct CreativeDesktopPropertyEditActivity {
  bool changed = false;
  bool commitRequested = false;
};

constexpr void observeCreativeDesktopContinuousPropertyEdit(
    CreativeDesktopPropertyEditActivity& activity,
    bool changed,
    bool deactivatedAfterEdit) noexcept {
  activity.changed = activity.changed || changed;
  activity.commitRequested =
      activity.commitRequested || deactivatedAfterEdit;
}

constexpr void observeCreativeDesktopDiscretePropertyEdit(
    CreativeDesktopPropertyEditActivity& activity,
    bool changed) noexcept {
  activity.changed = activity.changed || changed;
  activity.commitRequested = activity.commitRequested || changed;
}

[[nodiscard]] constexpr CreativeDesktopPropertyEditIntent
resolveCreativeDesktopPropertyEditIntent(
    CreativeDesktopPropertyEditActivity activity,
    bool dirty,
    bool valid,
    bool previewActive,
    bool cancelRequested = false) noexcept {
  if (cancelRequested) {
    return previewActive ? CreativeDesktopPropertyEditIntent::Cancel
                         : CreativeDesktopPropertyEditIntent::None;
  }
  if (activity.commitRequested) {
    if (dirty && valid) {
      return CreativeDesktopPropertyEditIntent::Commit;
    }
    return previewActive ? CreativeDesktopPropertyEditIntent::Cancel
                         : CreativeDesktopPropertyEditIntent::None;
  }
  if (activity.changed) {
    if (dirty && valid) {
      return CreativeDesktopPropertyEditIntent::Preview;
    }
    return previewActive ? CreativeDesktopPropertyEditIntent::Cancel
                         : CreativeDesktopPropertyEditIntent::None;
  }
  return CreativeDesktopPropertyEditIntent::None;
}

// Validates an Inspector draft (position meters, rotation degrees, scale) and
// converts it to a document transform. Rejects any non-finite component and any
// scale component <= 0.0.
[[nodiscard]] CreativeDesktopTransformDraft
validateCreativeDesktopTransformDraft(cr::CreativeVec3 position,
                                      cr::CreativeVec3 rotationDegrees,
                                      cr::CreativeVec3 scale) noexcept;

inline constexpr std::size_t kCreativeDesktopGeneratedSourceScopeCapacity = 4U;

struct CreativeDesktopGeneratedSourceScopeEntry {
  cr::CreativeWorldLayoutTable table = cr::CreativeWorldLayoutTable::None;
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
  std::string_view stableKey;
  std::string_view name;
};

// Fixed ancestry for one generated object. The order is broadest to narrowest:
// Building -> Level -> Room/host -> direct source. Cross-level connectors omit
// Level and Room because selecting either endpoint would fabricate ownership.
struct CreativeDesktopGeneratedSourceScopeModel {
  std::array<CreativeDesktopGeneratedSourceScopeEntry,
             kCreativeDesktopGeneratedSourceScopeCapacity>
      entries{};
  std::size_t count = 0U;
  std::size_t directEntryIndex = 0U;
};

struct CreativeDesktopGeneratedSourceScopeTint {
  float r = 1.0F;
  float g = 0.82F;
  float b = 0.22F;
  float a = 1.0F;
};

struct CreativeDesktopGeneratedSourceScopeSummary {
  bool valid = false;
  cr::CreativeWorldLayoutTable table = cr::CreativeWorldLayoutTable::None;
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t objectCount = 0U;
  std::size_t visibleObjectCount = 0U;
  std::size_t hiddenObjectCount = 0U;
  bool hasBounds = false;
  cr::CreativeBounds worldBounds{};
};

struct CreativeDesktopGeneratedSourceScopeCache {
  bool populated = false;
  cr::CreativeDocumentId documentId = cr::kInvalidDocumentId;
  std::uint64_t documentRevision = 0U;
  std::uint64_t sourceEpoch = 0U;
  std::uint64_t sourceRevision = 0U;
  std::uint64_t generatedRevision = 0U;
  cr::CreativeWorldLayoutTable table = cr::CreativeWorldLayoutTable::None;
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeDesktopGeneratedSourceScopeSummary summary;
  std::vector<cr::CreativeObjectId> objectIds;
};

[[nodiscard]] CreativeDesktopGeneratedSourceScopeModel
buildCreativeDesktopGeneratedSourceScopeModel(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutObjectProvenance& provenance) noexcept;

[[nodiscard]] std::size_t findCreativeDesktopGeneratedSourceScope(
    const CreativeDesktopGeneratedSourceScopeModel& model,
    cr::CreativeWorldLayoutTable table,
    std::size_t index) noexcept;

// Resolves the 2D source selection against one generated object's ancestry.
// A selection outside that ancestry cannot steal scope and falls back to the
// object's direct source.
[[nodiscard]] std::size_t resolveCreativeDesktopGeneratedSourceActiveScope(
    const CreativeDesktopGeneratedSourceScopeModel& model,
    cr::CreativeWorldLayoutTable selectedTable,
    std::size_t selectedIndex) noexcept;

// Shared room edges belong to every contributing room. Other scopes follow
// the same no-fabricated-ancestry model used by the Inspector breadcrumb.
[[nodiscard]] bool creativeDesktopGeneratedObjectBelongsToSourceScope(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeObject& object,
    cr::CreativeWorldLayoutTable table,
    std::size_t index);

[[nodiscard]] CreativeDesktopGeneratedSourceScopeSummary
buildCreativeDesktopGeneratedSourceScopeSummary(
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& layout,
    cr::CreativeWorldLayoutTable table,
    std::size_t index);

// Returns true only when the revision/scope key changed and the member list was
// rebuilt. Object ids are sorted for allocation-free binary lookup on idle
// frames.
[[nodiscard]] bool refreshCreativeDesktopGeneratedSourceScopeCache(
    CreativeDesktopGeneratedSourceScopeCache& cache,
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& layout,
    std::uint64_t sourceEpoch,
    std::uint64_t sourceRevision,
    std::uint64_t generatedRevision,
    cr::CreativeWorldLayoutTable table,
    std::size_t index);

[[nodiscard]] bool creativeDesktopGeneratedSourceScopeCacheContains(
    const CreativeDesktopGeneratedSourceScopeCache& cache,
    cr::CreativeObjectId objectId) noexcept;

// Building, Level, Room, and direct-leaf selections use one visual language in
// the 3D viewport, 2D layout canvas, and Inspector breadcrumb.
[[nodiscard]] CreativeDesktopGeneratedSourceScopeTint
creativeDesktopGeneratedSourceScopeTint(
    cr::CreativeWorldLayoutTable table) noexcept;

}  // namespace iggy3d_creative_app
