#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "render/FrameInput.hpp"

#include "EditorDesktopModel.hpp"

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
  // A draft field is being edited this frame, so a revision bump must not
  // overwrite it.
  bool editing = false;
  std::string validation;  // concise inline error; empty when the draft is ok.
};

// Desktop shell state — app-window UI state, NOT document truth (it lives in
// CreativeEditorState next to the other panel sub-states, never inside
// creative::CreativeAppState). No ImGui types here (plan DL-2); RenderContentViewport
// is a plain render struct, not an ImGui type.
struct CreativeEditorDesktopUiState {
  bool shellEnabled = false;  // false under --capture (plan DL-1)
  bool frameActive = false;   // NewFrame issued this frame, Render still owed
  // The drawable-pixel sub-rectangle the 3D scene occupies (the ImGui central
  // dock node). The all-zero sentinel means full-frame; it stays full-frame
  // until docked panels shrink the central node (UI-3).
  iggy3d::RenderContentViewport contentViewport;
  // Whether the viewport owns the pointer (relative-mouse fly-look). The
  // resting state in desktop mode is a free cursor; a click on the viewport
  // captures it and leaving the viewport context releases it (plan DD-9).
  bool viewportPointerCaptured = false;

  // View-menu panel visibility. The panels themselves land in UI-2b/UI-3;
  // these toggles are wired now so the menu is complete and the panels honor
  // them when they arrive.
  bool showOutliner = true;    // Project panel (left)
  bool showInspector = true;   // Inspector panel (right)
  bool showDiagnostics = true; // Diagnostics panel (bottom)
  bool showToolSettings = true;
  bool showAssetLibrary = true;
  bool showHistory = true;

  // Docked workspace layout. Built once via DockBuilder, rebuilt on Reset
  // Layout. The default arrangement follows the target proportions: Project
  // left, Inspector right, Diagnostics bottom, toolbar above the central 3D
  // viewport (plan §Workspace Skeleton).
  bool dockLayoutBuilt = false;
  bool resetLayoutRequested = false;

  // Save As modal (in-app, no native dialog — plan DD-14).
  bool saveAsModalOpen = false;
  std::array<char, 96> saveAsNameBuffer{};

  // Last dispatched command message, surfaced in the status bar (plan DD-11).
  std::string statusMessage;
  // Document revision at the last successful save; dirty = current != this.
  std::uint64_t lastSavedRevision = 0;

  // Inspector/Diagnostics cache. Logic topology is document-revision owned,
  // so idle UI frames reuse one bounded report instead of rescanning links.
  iggy3d::creative::CreativeDocumentId logicDiagnosticDocumentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t logicDiagnosticRevision = 0U;
  iggy3d::creative::CreativeLogicDiagnosticReport logicDiagnostics;
  bool logicDiagnosticsCached = false;

  // UI-4A Project/Inspector transient state (caches + drafts only).
  CreativeDesktopOutlinerState outliner;
  CreativeDesktopInspectorDraft inspectorDraft;
};

// Result of the free-pointer capture policy: the new capture state and whether
// it changed (so the caller only touches relative-mouse mode on a transition).
struct CreativeDesktopPointerDecision {
  bool captured = false;
  bool changed = false;
};

// Pure DD-9 policy. Free cursor is the resting state in desktop mode; a primary
// click that landed on the viewport (not an ImGui panel) with no modal open
// captures the pointer for fly-look; leaving the viewport context (a modal
// opened — e.g. Esc routed to Controls — or focus lost) releases it. Shell-off
// forces released, so capture mode / non-desktop runs are unaffected.
[[nodiscard]] CreativeDesktopPointerDecision decideCreativeDesktopPointerCapture(
    bool shellEnabled,
    bool currentlyCaptured,
    bool primaryPressedOverViewport,
    bool viewportContext,
    bool windowFocused) noexcept;

// Whether the desktop shell should claim the keyboard/mouse this frame, so the
// editor input context resolves to DesktopUi. While the viewport owns the
// pointer (fly-look) the app owns the mouse and ImGui only sees a warped,
// wandering cursor, so its want-capture must be ignored — otherwise the shell
// reclaims the context and releases the capture the instant the camera moves
// (plan DD-9). Pure so the regression is pinned directly.
[[nodiscard]] bool creativeDesktopUiWantsInput(
    bool viewportPointerCaptured,
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
