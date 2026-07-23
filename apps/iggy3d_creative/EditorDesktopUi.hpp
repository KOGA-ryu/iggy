#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/world/WorldLayoutArchitecture.hpp"
#include "render/FrameInput.hpp"

#include "EditorDesktopModel.hpp"
#include "EditorMapValidationDiagnostics.hpp"
#include "EditorWorldLayoutHierarchy.hpp"
#include "EditorWorldLayoutState.hpp"

namespace iggy3d {
class VulkanBackend;
}

namespace iggy3d_creative_app {

// UI-4A transient Project-panel state: the revision-owned hierarchy model, the
// search box and its applied query, the filtered row indices, and the
// shift-range anchor. Caches only — never document truth, never persisted.
struct CreativeDesktopOutlinerState {
  CreativeDesktopOutlinerModel model;
  bool modelValid = false;
  // Indices into model.rows, in visible order.
  std::vector<std::size_t> filteredRows;
  std::string appliedQuery;
  bool filteredRowsValid = false;
  std::array<char, 128> searchBuffer{};
  iggy3d::creative::CreativeObjectId selectionAnchor =
      iggy3d::creative::kInvalidObjectId;
};

// World Layout's source tree is a transient revision-owned projection. Search
// filtering never rebuilds source ownership and none of this state persists.
struct CreativeDesktopWorldLayoutHierarchyState {
  CreativeEditorWorldLayoutHierarchyCache cache;
  std::vector<std::size_t> filteredRows;
  std::string appliedQuery;
  bool filteredRowsValid = false;
  std::array<char, 128> searchBuffer{};
  iggy3d::creative::CreativeWorldLayoutTable pendingTable =
      iggy3d::creative::CreativeWorldLayoutTable::None;
  std::size_t pendingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string pendingStableKey;
  std::string pendingLabel;
  std::array<char, 128> renameBuffer{};
};

// UI-4A transient Inspector draft, keyed by document id + object id. Rotation is
// held in DEGREES; the document stays radians. Never persisted.
struct CreativeDesktopInspectorDraft {
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::uint64_t sourceRevision = 0U;
  bool valid = false;
  std::string name;
  std::array<double, 3> position{0.0, 0.0, 0.0};
  std::array<double, 3> rotationDegrees{0.0, 0.0, 0.0};
  std::array<double, 3> scale{1.0, 1.0, 1.0};
  iggy3d::creative::CreativeMovingPlatformSettings movingPlatform;
  iggy3d::creative::CreativePlayerSpawnSettings playerSpawn;
  std::size_t movingPlatformWaypointIndex = 0U;
  double movingPlatformWaypointDwellSeconds = 0.0;
  // A draft field is being edited this frame, so a revision bump must not
  // overwrite it.
  bool editing = false;
  std::string validation;  // concise inline error; empty when the draft is ok.
};

// Desktop shell state — app-window UI state, NOT document truth (it lives in
// CreativeEditorState next to the other panel sub-states, never inside
// creative::CreativeAppState). No ImGui types here (plan DL-2); RenderContentViewport
// is a plain render struct, not an ImGui type.
// Transient draft for the Create tab's Building Blockout section: UI state,
// not document truth, never persisted. Footprint fields are integer grid-line
// coordinates with exclusive maximums; every non-footprint shell value keeps
// the CreativeEditorWorldLayoutRoomSettings default.
[[nodiscard]] inline CreativeEditorWorldLayoutBuildingBlockoutSettings
makeCreativeEditorWorldLayoutBlockoutDraft() noexcept {
  CreativeEditorWorldLayoutBuildingBlockoutSettings draft;
  draft.shell.footprint.minimum = {0, 0};
  draft.shell.footprint.maximum = {8, 8};
  return draft;
}

struct CreativeEditorDesktopBlockoutEditDraft {
  bool active = false;
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
};

struct CreativeEditorDesktopArchitectureDraft {
  bool active = false;
  bool previewReady = false;
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  iggy3d::creative::CreativeWorldLayoutArchitecturalProfile profile;
};

enum class CreativeDesktopDockLayoutMode : std::uint8_t {
  Standard,
  WorldLayoutSplit,
  Count,
};

struct CreativeDesktopDockLayoutSpec {
  CreativeDesktopDockLayoutMode mode =
      CreativeDesktopDockLayoutMode::Standard;
  bool worldLayoutPanelVisible = false;
  bool threeDimensionalViewportVisible = true;
  float worldLayoutPanelFraction = 0.0F;
};

enum class CreativeDesktopPointerCaptureMode : std::uint8_t {
  None,
  FlyLook,
  Orbit,
  Pan,
  Count,
};

struct CreativeEditorDesktopUiState {
  bool shellEnabled = false;  // true only for explicit desktop UI launches
  bool frameActive = false;   // NewFrame issued this frame, Render still owed
  // The drawable-pixel sub-rectangle the 3D scene occupies (the ImGui central
  // dock node). The all-zero sentinel means full-frame; it stays full-frame
  // until docked panels shrink the central node (UI-3).
  iggy3d::RenderContentViewport contentViewport;
  // Relative-pointer ownership for fly-look and bounded viewport gestures.
  // The resting state is free. Plain viewport click enters persistent
  // FlyLook; Option+drag enters Orbit and Shift+Option+drag enters Pan until
  // the primary button is released.
  CreativeDesktopPointerCaptureMode viewportPointerCaptureMode =
      CreativeDesktopPointerCaptureMode::None;
  // SDL may report the cursor warp that enters relative mode as mouse motion.
  // Consume that transition sample before applying camera look.
  bool discardNextViewportMouseDelta = false;

  // View-menu panel visibility. The panels themselves land in UI-2b/UI-3;
  // these toggles are wired now so the menu is complete and the panels honor
  // them when they arrive.
  bool showOutliner = true;    // Project panel (left)
  bool showInspector = true;   // Inspector panel (right)
  bool showDiagnostics = true; // Diagnostics panel (bottom)
  bool showToolSettings = true;
  bool showAssetLibrary = true;
  bool showHistory = true;
  bool showWorldLayout = true;
  bool terrainGeneratorFocusRequested = false;
  std::size_t worldLayoutDeleteLevelIndex =
      std::numeric_limits<std::size_t>::max();

  // Docked workspace layout. Built once via DockBuilder, rebuilt on Reset
  // Layout. The default arrangement follows the target proportions: Project
  // left, Inspector right, Diagnostics bottom, toolbar above the central 3D
  // viewport (plan §Workspace Skeleton).
  bool dockLayoutBuilt = false;
  bool resetLayoutRequested = false;

  // Save As modal (in-app, no native dialog — plan DD-14).
  bool saveAsModalOpen = false;
  std::array<char, 96> saveAsNameBuffer{};
  // Explicit destructive confirmations. The pending building index/mode is
  // transient UI routing state; document/template truth remains elsewhere.
  bool regenerateBuilderEstateModalOpen = false;
  bool buildingTemplateRebuildModalOpen = false;
  std::size_t pendingBuildingTemplateRebuildIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  iggy3d::creative::CreativeWorldLayoutBuildingTemplateRefreshMode
      pendingBuildingTemplateRebuildMode =
          iggy3d::creative::CreativeWorldLayoutBuildingTemplateRefreshMode::
              SelectedInstance;

  // Last dispatched command message, surfaced in the status bar (plan DD-11).
  std::string statusMessage;
  // True while a playtest child process is running (polled each frame).
  bool playtestRunning = false;
  // Document revision at the last successful save; dirty = current != this.
  std::uint64_t lastSavedRevision = 0;

  // Inspector/Diagnostics cache. Logic topology is document-revision owned,
  // so idle UI frames reuse one bounded report instead of rescanning links.
  iggy3d::creative::CreativeDocumentId logicDiagnosticDocumentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t logicDiagnosticRevision = 0U;
  iggy3d::creative::CreativeLogicDiagnosticReport logicDiagnostics;
  bool logicDiagnosticsCached = false;

  // Whole-map validation is an explicit, potentially expensive authoring
  // action. Preserve its last result and expose staleness without rebaking on
  // document-edit frames.
  CreativeEditorMapValidationCache mapValidation;

  // UI-4A Project/Inspector transient state (caches + drafts only).
  CreativeDesktopOutlinerState outliner;
  CreativeDesktopWorldLayoutHierarchyState worldLayoutHierarchy;
  CreativeDesktopInspectorDraft inspectorDraft;

  // Building Blockout draft for the Create tab (UI-BLOCKOUT-1).
  CreativeEditorWorldLayoutBuildingBlockoutSettings worldLayoutBlockoutDraft =
      makeCreativeEditorWorldLayoutBlockoutDraft();

  // Selected-building Edit draft (UI-BLOCKOUT-2): filled from the backend
  // read contract, applied through the typed update command. sourceRevision
  // records the world-layout revision at read time so the drawer can show
  // whether the draft is still in sync with the source.
  CreativeEditorDesktopBlockoutEditDraft worldLayoutBlockoutEdit;
  CreativeEditorDesktopArchitectureDraft worldLayoutArchitecture;
};

// Result of the free-pointer capture policy: the new capture state and whether
// it changed (so the caller only touches relative-mouse mode on a transition).
struct CreativeDesktopPointerDecision {
  CreativeDesktopPointerCaptureMode mode =
      CreativeDesktopPointerCaptureMode::None;
  bool captured = false;
  bool changed = false;
  // The primary press that enters fly-look is pointer ownership, not a world
  // edit. The caller must consume that press before downstream input handling.
  bool consumePrimaryPress = false;
  // A newly acquired relative pointer must discard its first motion sample.
  bool discardNextMouseDelta = false;
};

// The full IDE shell is opt-in. Scripted capture remains UI-free even when a
// caller accidentally supplies both launch modes.
[[nodiscard]] bool creativeDesktopShellEnabledForLaunch(
    bool desktopUiRequested,
    bool captureMode) noexcept;

// World Layout is a split inspection workspace: its 2D plan/elevation canvas
// occupies a resizable sibling dock while the central passthrough node remains
// the live 3D viewport. Closing World Layout restores the standard workspace.
[[nodiscard]] CreativeDesktopDockLayoutSpec creativeDesktopDockLayoutSpec(
    bool showWorldLayout) noexcept;
[[nodiscard]] bool creativeDesktopDockLayoutRebuildRequired(
    bool dockLayoutBuilt, bool resetLayoutRequested) noexcept;

[[nodiscard]] bool creativeDesktopPointerCaptured(
    CreativeDesktopPointerCaptureMode mode) noexcept;
[[nodiscard]] bool creativeDesktopPointerModeOwnsPrimaryAction(
    CreativeDesktopPointerCaptureMode mode) noexcept;
[[nodiscard]] bool creativeDesktopMouseLookActive(
    bool shellEnabled,
    CreativeDesktopPointerCaptureMode mode) noexcept;
[[nodiscard]] bool creativeDesktopViewportDollyRequested(
    CreativeDesktopPointerCaptureMode mode,
    iggy3d::creative::CreativeInputModifierMask modifiers,
    float wheelDelta,
    bool viewportContext) noexcept;

// Pure DD-9 policy. Free cursor is the resting state in desktop mode; a primary
// click that landed on the viewport (not an ImGui panel) with no modal open
// captures the pointer for fly-look; leaving the viewport context (a modal
// opened — e.g. Esc routed to Controls — or focus lost) releases it. Shell-off
// forces released, so capture mode / non-desktop runs are unaffected.
[[nodiscard]] CreativeDesktopPointerDecision decideCreativeDesktopPointerCapture(
    bool shellEnabled,
    CreativeDesktopPointerCaptureMode currentMode,
    bool primaryPressedOverViewport,
    bool primaryDown,
    iggy3d::creative::CreativeInputModifierMask modifiers,
    bool viewportContext,
    bool windowFocused) noexcept;

// Whether the desktop shell should claim the keyboard/mouse this frame, so the
// editor input context resolves to DesktopUi. While the viewport owns the
// pointer (fly-look) the app owns the mouse and ImGui only sees a warped,
// wandering cursor, so its want-capture must be ignored — otherwise the shell
// reclaims the context and releases the capture the instant the camera moves
// (plan DD-9). Pure so the regression is pinned directly.
[[nodiscard]] bool creativeDesktopUiWantsInput(
    CreativeDesktopPointerCaptureMode pointerCaptureMode,
    bool imguiWantsMouse,
    bool imguiWantsKeyboard) noexcept;

// Starts the ImGui frame (NewFrame) if the shell is enabled, returning whether
// it is now active. MUST run after event polling and BEFORE the input context
// is resolved, so the shell's WantCapture reflects this frame's events rather
// than the previous frame (NewFrame-before-context ordering). When this returns
// true, endCreativeEditorDesktopFrame MUST run before submit this frame.
bool beginCreativeEditorDesktopUiFrame(CreativeEditorDesktopUiState& desktopUi,
                                       iggy3d::VulkanBackend& backend);

// Builds the full-window passthru dockspace and resolves the central-node
// content rect. The ImGui frame must already be active (see
// beginCreativeEditorDesktopUiFrame); no-op otherwise.
void layoutCreativeEditorDesktopDockspace(
    CreativeEditorDesktopUiState& desktopUi);

// ImGui::Render(). Never called from the renderer's record hook — a skipped
// submit must still leave the frame Rendered or the next NewFrame asserts.
void endCreativeEditorDesktopFrame(CreativeEditorDesktopUiState& desktopUi);

}  // namespace iggy3d_creative_app
