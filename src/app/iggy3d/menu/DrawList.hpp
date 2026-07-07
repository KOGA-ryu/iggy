#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"

namespace iggy3d {

struct ProductSaveBridgeResult;
struct WorldSetupDraft;

enum class ProductUiPrimitiveKind : std::uint8_t {
  Panel,
  Rect,
  Text,
  Border,
  Highlight,
};

enum class ProductUiTone : std::uint8_t {
  Surface,
  SurfaceRaised,
  TextPrimary,
  TextMuted,
  Accent,
  Selected,
  Disabled,
  Border,
  Status,
};

struct ProductUiRect {
  float x = 0.0F;
  float y = 0.0F;
  float width = 0.0F;
  float height = 0.0F;
};

struct ProductUiColor {
  float r = 0.0F;
  float g = 0.0F;
  float b = 0.0F;
  float a = 1.0F;
};

// L2 theme (docs/ui/ui_architecture.md): the SAME semantic tones resolve to a
// different palette per surface. `System` is the dark UI shell (the starter menu
// and system chrome); `Journal` is the cream/leather/graphite Moleskine skin worn
// by the in-game, diegetic surfaces (the recon notebook and, once migrated, the
// in-game/pause menu). A draw list is tagged with the theme that built it, and the
// render seam resolves tones through that theme.
enum class ProductUiThemeId : std::uint8_t {
  System,
  Journal,
};

// A theme is a palette: one colour per ProductUiTone, indexed by the tone's value.
struct ProductUiTheme {
  std::array<ProductUiColor, 9> tones;
};

struct ProductUiPrimitive {
  ProductUiPrimitiveKind kind = ProductUiPrimitiveKind::Rect;
  ProductUiTone tone = ProductUiTone::Surface;
  ProductUiRect rect;
  std::string semanticId;
  std::string text;
  FrontendAction action = FrontendAction::None;
  bool selected = false;
  bool enabled = true;
};

// The input lane's view of an interactive widget: computed from the SAME rect
// that produced a draw primitive, so drawing and hit-testing cannot drift. The
// widget layer (src/app/iggy3d/ui/Widget.hpp) emits these; the draw list carries
// them alongside the primitives. NB: input-router consumption — replacing the
// hand-derived hit rects in OpeningMenuView — is a later step (see
// docs/ui/ui_architecture.md). Today they are captured and receipt-guarded here,
// not yet routed.
enum class UiHitKind : std::uint8_t {
  None,
  Button,
  Row,
  Slider,
  Toggle,
  Viewport,
};

struct UiHitRegion {
  std::string semanticId;
  ProductUiRect rect;
  UiHitKind kind = UiHitKind::None;
  FrontendAction action = FrontendAction::None;
  bool enabled = true;
};

struct ProductUiDrawList {
  bool ready = false;
  bool partial = false;
  std::string status = "product_ui_draw_list_not_ready";
  std::string reasonCode = "product_ui_draw_list_not_ready";
  std::uint32_t virtualWidth = 1280;
  std::uint32_t virtualHeight = 720;
  ProductUiThemeId theme = ProductUiThemeId::System;
  std::vector<ProductUiPrimitive> primitives;
  std::vector<UiHitRegion> hitRegions;
  std::uint64_t hitRegionCount = 0;
  std::uint64_t primitiveCount = 0;
  std::uint64_t textCount = 0;
  std::uint64_t rectCount = 0;
  std::uint64_t rowCount = 0;
  std::uint64_t disabledRowCount = 0;
  std::string selectedAction = "none";
};

// The ProductAppWindowState-owned scalars the starter UI request needs, passed as a
// small value type so this header (and menu/DrawList.*) never has to include the
// ~421-field ProductAppWindowState god-struct from ReceiptBuilder.hpp. Both the frame
// draw path and the hit-test path fill it from their own window state.
struct ProductStarterUiDraftState {
  bool dungeonDraftEditMode = false;
  bool dungeonDraftModified = false;
  std::uint64_t dungeonDraftCursorRow = 0;
  std::uint64_t dungeonDraftCursorColumn = 0;
  std::string dungeonDraftSelectedGlyph = ".";
  std::string dungeonDraftLastGlyph = "none";
  std::string selectedSaveId;
  // The save the delete-confirm panel is about to delete (window.saveSession.saveDelete.candidateId).
  std::string deleteCandidateId{};
};

// Build this via buildProductStarterUiDrawListRequest() — the single request
// constructor shared by the draw path and the hit-test path, so the two cannot
// diverge. Do not hand-roll a starter request in production.
struct ProductUiDrawListRequest {
  const FrontendState* frontend = nullptr;
  std::uint64_t compatibleSaveCount = 0;
  std::uint32_t virtualWidth = 1280;
  std::uint32_t virtualHeight = 720;
  const WorldSetupDraft* worldSetupDraft = nullptr;
  bool dungeonDraftEditMode = false;
  bool dungeonDraftModified = false;
  std::uint64_t dungeonDraftCursorRow = 0;
  std::uint64_t dungeonDraftCursorColumn = 0;
  std::string dungeonDraftSelectedGlyph = ".";
  std::string dungeonDraftLastGlyph = "none";
  FrontendSettingsTab settingsTab = FrontendSettingsTab::Input;
  const ProductSaveBridgeResult* saves = nullptr;
  // The currently selected save id tracked by the input layer. Threaded into
  // the save-browser model so the drawn row highlight matches the row the
  // player navigated to instead of always defaulting to the first slot.
  std::string selectedSaveId;
  // The save the delete-confirm panel is about to delete (window.saveSession.saveDelete.candidateId),
  // resolved to a real title/status via resolveProductDeleteConfirmModel. Default member
  // initializer keeps positional aggregate-init sites (hand-built test requests) warning-free.
  std::string deleteCandidateId{};
  bool gameplayActive = false;
  bool saveRootWritable = false;
  bool developerToolsEnabled = true;
  bool activeRoomEditable = false;
  bool roomEditingReady = false;
};

// The delete-confirmation panel's dynamic text, resolved once and rendered identically by both
// presentation lanes (the draw-list lane and the SDL view) so a destructive confirmation always
// names the world it will delete.
struct ProductDeleteConfirmModel {
  std::string mapTitle;
  std::string statusText;
};

// Resolve the delete-confirm panel text from the LIVE catalog + the tracked candidate id (the
// single source both lanes consume). Finds the slot with id == candidateId and shows its title;
// a deterministic fallback ("NO MAP SELECTED") when the candidate is empty/"none"/not found so
// the panel is never blank or garbage.
ProductDeleteConfirmModel resolveProductDeleteConfirmModel(
    std::string_view candidateId, const ProductSaveBridgeResult& saves);

std::string_view productUiPrimitiveKindName(ProductUiPrimitiveKind kind);
std::string_view productUiToneName(ProductUiTone tone);

// Resolve a tone to a colour. The single-argument form uses the System theme (the
// long-standing default — every existing receipt keeps its colours). The theme
// forms let a surface (e.g. the notebook) resolve the same tones to a different
// palette. `productUiTheme(id)` returns the built-in palette for a theme id.
ProductUiColor productUiToneColor(ProductUiTone tone);
ProductUiColor productUiToneColor(ProductUiTone tone, const ProductUiTheme& theme);
const ProductUiTheme& productUiTheme(ProductUiThemeId id);

ProductUiDrawList buildProductStarterUiDrawList(
    const ProductUiDrawListRequest& request);

// The single constructor for a starter-menu draw-list request, shared by the frame
// draw path (FramePresenter) and the opening-menu hit-test (OpeningMenuView) so both
// necessarily build the same request from the same state. virtualWidth/Height are
// fixed at 1280x720 (as the draw path always has).
ProductUiDrawListRequest buildProductStarterUiDrawListRequest(
    const FrontendState& frontend,
    const ProductSaveBridgeResult& saves,
    const WorldSetupDraft& worldSetupDraft,
    FrontendSettingsTab settingsTab,
    const ProductStarterUiDraftState& draft);

}  // namespace iggy3d
