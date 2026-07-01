#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/menu/DrawList.hpp"  // ProductUiDrawList, ProductUiRect

namespace iggy3d {

// The in-game recon journal ("Moleskine") — the surface where the player records
// what they scouted: building layouts, entry points, patrols, garrison, hazards.
// This is the DRAW-LIST builder + data model only; it emits a ProductUiDrawList
// through the L1 widget layer (deterministic, receipt-testable), exactly like the
// starter UI. Runtime wiring (opening it in-game, tab/page input, audio) is a
// separate step. Full design: docs/ui/notebook_design.md (pending).
//
// v1 reality (docs/ui/ui_architecture.md): L0 is solid rects + glyph text only.
// So the floor plan is a BLOCKY, rectilinear ASCII grid rendered as small rects
// (Thief-blueprint), not hand-drawn line-art — curves/circles/arrows need a Line
// primitive, and interactive sketching needs the Viewport (both later). Colours
// are mapped onto the existing semantic tones; the cream/leather journal palette
// arrives with the L2 theme.

enum class ProductNotebookTab : std::uint8_t {
  Maps,
  Bestiary,
  Jobs,
  Spells,
};

// One open recon spread: the building sketched on the left page, the notes on the
// right. The floor plan is an ASCII grid — the same vocabulary the room-authoring
// surface uses — so scout data can eventually be derived from a room and refined.
struct ProductNotebookReconPage {
  std::string title;                     // e.g. "WARDEN'S KEEP - GROUND"
  std::vector<std::string> floorPlan;    // ASCII rows: '#' wall, ' '/'.' floor,
                                         // 'D' door, 'W' window, 'V' vent, 'G' guard
  std::vector<std::string> garrison;     // "2 x watchman - shortblade"
  std::vector<std::string> patrols;      // "loop: hall -> gallery -> yard"
  std::vector<std::string> hazards;      // "tripwire - east corridor"
  std::uint64_t pageNumber = 1;
};

struct ProductNotebookUiRequest {
  ProductNotebookTab tab = ProductNotebookTab::Maps;
  const ProductNotebookReconPage* page = nullptr;
  std::uint32_t virtualWidth = 1280;
  std::uint32_t virtualHeight = 720;
};

// Sound-event ids the notebook INTERACTION layer will queue (placeholder — no
// audio yet). These fire on the page-turn / sketch ACTIONS, not during draw, so
// they are not part of the pure draw-list builder below; they live here so the
// action layer and any test share one spelling.
inline constexpr std::string_view kNotebookSoundPageFlip = "notebook.page_flip";
inline constexpr std::string_view kNotebookSoundPencil = "notebook.pencil";

std::string_view productNotebookTabName(ProductNotebookTab tab);

// Pure (request -> draw list). Emits the book shell, the left sketch page (blocky
// ASCII floor plan), the right ruled notes page, the fore-edge tabs, and the page
// number, all as ProductUiPrimitives via the widget layer.
ProductUiDrawList buildProductNotebookUiDrawList(
    const ProductNotebookUiRequest& request);

}  // namespace iggy3d
