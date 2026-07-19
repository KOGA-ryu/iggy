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
  bool visible = true;
  bool locked = false;
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

// Validates an Inspector draft (position meters, rotation degrees, scale) and
// converts it to a document transform. Rejects any non-finite component and any
// scale component <= 0.0.
[[nodiscard]] CreativeDesktopTransformDraft
validateCreativeDesktopTransformDraft(cr::CreativeVec3 position,
                                      cr::CreativeVec3 rotationDegrees,
                                      cr::CreativeVec3 scale) noexcept;

// Generated Object/Box outputs have a one-to-one inverse and may be refined
// then adopted. Structural and condensed outputs remain source-owned.
[[nodiscard]] bool creativeDesktopGeneratedSourceSupportsAdoption(
    const cr::CreativeWorldLayoutObjectProvenance& provenance) noexcept;

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

[[nodiscard]] CreativeDesktopGeneratedSourceScopeModel
buildCreativeDesktopGeneratedSourceScopeModel(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutObjectProvenance& provenance) noexcept;

[[nodiscard]] std::size_t findCreativeDesktopGeneratedSourceScope(
    const CreativeDesktopGeneratedSourceScopeModel& model,
    cr::CreativeWorldLayoutTable table,
    std::size_t index) noexcept;

}  // namespace iggy3d_creative_app
