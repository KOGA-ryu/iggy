#include "app/iggy3d/automation/Automation.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/MenuInput.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/ascii_room/Preview.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/iggy3d/world/DungeonDraft.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/room_editor/AuthoringController.hpp"
#include "app/iggy3d/room_editor/ActionController.hpp"
#include "app/iggy3d/room_editor/Cursor.hpp"

namespace iggy3d {

// branch-gate-relocation: BG-1232 from=src/app/iggy3d/automation/Automation.cpp

  void markAutomationApplied(ProductAppWindowState& window,
                             const ProductAutomationCommand& command,
                             std::string_view action,
                             MenuOwner owner,
                             std::string_view result);

void recordWorldSetupDraftState(const WorldSetupDraft& draft,
                                ProductAppWindowState& window) {
  window.creativeAuthoring.worldSetup.title = draft.worldName;
  window.creativeAuthoring.worldSetup.dungeonTitle = draft.worldName;
  window.creativeAuthoring.worldSetup.dungeonCount = productBuiltinDungeonCatalog().size();
  const std::size_t dungeonIndex =
      productBuiltinDungeonIndexForRoomId(draft.asciiRoomId);
  window.creativeAuthoring.worldSetup.dungeonIndex = 0;
  // branch-gate: BG-1136
  if (dungeonIndex < productBuiltinDungeonCatalog().size()) {
    window.creativeAuthoring.worldSetup.dungeonIndex =
        static_cast<std::uint64_t>(dungeonIndex + 1U);
  }
  window.creativeAuthoring.worldSetup.asciiRoomEnabled = draft.asciiRoomEnabled;
  window.creativeAuthoring.worldSetup.asciiRoomTextPresent = !draft.asciiRoomText.empty();
  // branch-gate: BG-1004
  window.creativeAuthoring.worldSetup.asciiRoomId =
      draft.asciiRoomId.empty() ? "none" : draft.asciiRoomId;
  // branch-gate: BG-1004
  window.creativeAuthoring.worldSetup.asciiRoomSourceName =
      draft.asciiRoomSourceName.empty() ? "none" : draft.asciiRoomSourceName;
  window.creativeAuthoring.asciiRoomDraft.text = draft.asciiRoomText;
  // branch-gate: BG-1004
  window.creativeAuthoring.asciiRoomDraft.roomId =
      draft.asciiRoomId.empty() ? "ascii_preview" : draft.asciiRoomId;
  // branch-gate: BG-1004
  window.creativeAuthoring.asciiRoomDraft.sourceName =
      draft.asciiRoomSourceName.empty() ? "world_setup_ascii_room" :
                                          draft.asciiRoomSourceName;
  // branch-gate: BG-1004
  if (draft.asciiRoomEnabled && !draft.asciiRoomText.empty()) {
    buildProductAsciiRoomPreviewResult(window);
    return;
  }
  window.creativeAuthoring.asciiRoomPreview.status = "not_requested";
  window.creativeAuthoring.asciiRoomPreview.reasonCode = "not_requested";
  window.creativeAuthoring.asciiRoomPreview.failedStage = "not_started";
  window.creativeAuthoring.asciiRoomPreview.roomId = "none";
  window.creativeAuthoring.asciiRoomPreview.sourceName = "none";
  window.creativeAuthoring.asciiRoomPreview.ready = false;
  window.creativeAuthoring.asciiRoomPreview.width = 0;
  window.creativeAuthoring.asciiRoomPreview.height = 0;
  window.creativeAuthoring.asciiRoomPreview.floorCount = 0;
  window.creativeAuthoring.asciiRoomPreview.wallCount = 0;
  window.creativeAuthoring.asciiRoomPreview.markerCount = 0;
  window.creativeAuthoring.asciiRoomPreview.elevatedFloorCount = 0;
  window.creativeAuthoring.asciiRoomPreview.rampCount = 0;
  window.creativeAuthoring.asciiRoomPreview.blockedSlopeCount = 0;
  window.creativeAuthoring.asciiRoomPreview.staticMeshCount = 0;
  window.creativeAuthoring.asciiRoomPreview.anchorCount = 0;
  window.creativeAuthoring.asciiRoomPreview.spatialSurfaceCount = 0;
  window.creativeAuthoring.asciiRoomPreview.assetTextWritten = false;
  window.creativeAuthoring.asciiRoomPreview.assetTextBytes = 0;
}

ProductDungeonDraftCursor dungeonDraftCursorFromWindow(
    const ProductAppWindowState& window) {
  return ProductDungeonDraftCursor{
      static_cast<std::size_t>(window.creativeAuthoring.worldSetup.dungeonDraftCursorRow),
      static_cast<std::size_t>(window.creativeAuthoring.worldSetup.dungeonDraftCursorColumn),
  };
}

void recordDungeonDraftOperation(ProductAppWindowState& window,
                                 const ProductDungeonDraftOperationResult& result) {
  window.creativeAuthoring.worldSetup.dungeonDraftStatus = std::string(result.status);
  window.creativeAuthoring.worldSetup.dungeonDraftReasonCode = std::string(result.reasonCode);
  window.creativeAuthoring.worldSetup.dungeonDraftCursorRow =
      static_cast<std::uint64_t>(result.cursor.row);
  window.creativeAuthoring.worldSetup.dungeonDraftCursorColumn =
      static_cast<std::uint64_t>(result.cursor.column);
  // branch-gate: BG-1004
  window.creativeAuthoring.worldSetup.dungeonDraftLastGlyph =
      result.glyph == '\0' ? std::string{"none"} : std::string(1U, result.glyph);
  // branch-gate: BG-1004
  if (result.modified) {
    window.creativeAuthoring.worldSetup.dungeonDraftModified = true;
  }
}

void resetDungeonDraftWindowCursor(const WorldSetupDraft& draft,
                                   ProductAppWindowState& window) {
  const ProductDungeonDraftCursor cursor =
      clampProductDungeonDraftCursor(draft, ProductDungeonDraftCursor{});
  window.creativeAuthoring.worldSetup.dungeonDraftCursorRow = static_cast<std::uint64_t>(cursor.row);
  window.creativeAuthoring.worldSetup.dungeonDraftCursorColumn =
      static_cast<std::uint64_t>(cursor.column);
  window.creativeAuthoring.worldSetup.dungeonDraftLastGlyph = "none";
}

bool applyDungeonDraftPaintGlyph(WorldSetupDraft& worldSetupDraft,
                                 ProductAppWindowState& window,
                                 char glyph) {
  // branch-gate: BG-1004
  if (!window.creativeAuthoring.worldSetup.dungeonDraftEditMode) {
    window.creativeAuthoring.worldSetup.dungeonDraftStatus = "dungeon_draft_edit_mode_off";
    window.creativeAuthoring.worldSetup.dungeonDraftReasonCode = "dungeon_draft_edit_mode_off";
    return false;
  }
  ProductDungeonDraftOperationResult painted = paintProductDungeonDraftCell(
      worldSetupDraft, dungeonDraftCursorFromWindow(window), glyph);
  recordDungeonDraftOperation(window, painted);
  recordWorldSetupDraftState(worldSetupDraft, window);
  return painted.ok;
}

bool selectDungeonDraftPaintGlyph(ProductAppWindowState& window, char glyph) {
  // branch-gate: BG-1146
  if (!window.creativeAuthoring.worldSetup.dungeonDraftEditMode) {
    window.creativeAuthoring.worldSetup.dungeonDraftStatus = "dungeon_draft_edit_mode_off";
    window.creativeAuthoring.worldSetup.dungeonDraftReasonCode = "dungeon_draft_edit_mode_off";
    return false;
  }
  // branch-gate: BG-1146
  if (!isProductDungeonDraftGlyph(glyph)) {
    window.creativeAuthoring.worldSetup.dungeonDraftStatus = "dungeon_draft_invalid_glyph";
    window.creativeAuthoring.worldSetup.dungeonDraftReasonCode = "dungeon_draft_invalid_glyph";
    return false;
  }
  window.creativeAuthoring.worldSetup.dungeonDraftSelectedGlyph = std::string(1U, glyph);
  window.creativeAuthoring.worldSetup.dungeonDraftStatus = "dungeon_draft_paint_tool_selected";
  window.creativeAuthoring.worldSetup.dungeonDraftReasonCode = "dungeon_draft_paint_tool_selected";
  return true;
}

namespace {

std::string readAutomationTextFile(std::string_view path) {
  std::ifstream input{std::string(path)};
  // branch-gate: BG-1165
  if (!input) {
    return {};
  }
  return std::string((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
}

std::string titleFromAutomationAsciiRoomPath(std::string_view path) {
  std::string title = std::filesystem::path(std::string(path)).stem().string();
  // branch-gate: BG-1165
  if (title.empty()) {
    return std::string{"Custom Draft"};
  }
  for (char& ch : title) {
    // branch-gate: BG-1165
    if (ch == '_' || ch == '-') {
      ch = ' ';
    }
  }
  return title;
}

}  // namespace

ProductAutomationExecutionResult applyProductWorldSetupAutomationCommand(
    const ProductAutomationCommand& command,
    std::string_view canonicalKey,
    ProductAutomationWorldSetupContext& context) {
  const std::string_view value{command.value};

  // branch-gate: BG-1004
  if (canonicalKey == "world.create") {
    bool boolValue = false;
    // branch-gate: BG-1004
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return {true, false};
    }
    // branch-gate: BG-1004
    if (!boolValue) {
      markAutomationApplied(context.window, command, "world.create",
                            context.currentOwner(), "ignored");
      return {true, true};
    }
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControl.status = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.create",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const bool editMode = context.window.creativeAuthoring.worldSetup.dungeonDraftEditMode;
    context.window.creativeAuthoring.worldSetup.dungeonDraftEditMode = false;
    const bool routed = context.routeInput(InputAction::MenuConfirm);
    context.window.creativeAuthoring.worldSetup.dungeonDraftEditMode = editMode;
    markAutomationApplied(context.window, command,
                          inputActionName(InputAction::MenuConfirm),
                          context.window.automationControl.lastOwner,
                          // branch-gate: BG-1004
                          routed ? "applied" : "failed");
    return {true, routed};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.title") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControl.status = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.title",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    context.worldSetupDraft.worldName = std::string(value);
    recordWorldSetupDraftState(context.worldSetupDraft, context.window);
    context.window.creativeAuthoring.worldSetup.status = "world_setup_title_updated";
    markAutomationApplied(context.window, command, "world.title",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.dungeon_id") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControl.status = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.dungeon_id",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductNonEmptyStringAutomationResult dungeonId =
        resolveProductNonEmptyStringAutomation(value);
    const std::size_t index = productBuiltinDungeonIndexForRoomId(dungeonId.value);
    const auto fail = [&]() -> ProductAutomationExecutionResult {
      context.window.automationControl.status = "invalid_value";
      markAutomationApplied(context.window, command, "world.dungeon_id",
                            context.currentOwner(), "failed");
      return {true, false};
    };
    const auto apply = [&]() -> ProductAutomationExecutionResult {
      const bool applied =
          applyProductBuiltinDungeonToDraft(index, context.worldSetupDraft);
      // branch-gate: BG-1004
      if (!applied) {
        return fail();
      }
      recordWorldSetupDraftState(context.worldSetupDraft, context.window);
      context.window.creativeAuthoring.worldSetup.dungeonDraftModified = false;
      context.window.creativeAuthoring.worldSetup.dungeonDraftEditMode = false;
      resetDungeonDraftWindowCursor(context.worldSetupDraft, context.window);
      context.window.creativeAuthoring.worldSetup.status = "world_setup_dungeon_selected";
      markAutomationApplied(context.window, command, "world.dungeon_id",
                            context.currentOwner(), "applied");
      return {true, true};
    };
    // branch-gate: BG-1004
    return dungeonId.valid ? apply() : fail();
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.draft_edit_mode") {
    bool boolValue = false;
    // branch-gate: BG-1004
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return {true, false};
    }
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControl.status = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.draft_edit_mode",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    context.window.creativeAuthoring.worldSetup.dungeonDraftEditMode = boolValue;
    // branch-gate: BG-1004
    context.worldSetupDraft.selectedField =
        boolValue ? WorldSetupField::AsciiRoom : WorldSetupField::Create;
    // branch-gate: BG-1004
    context.window.creativeAuthoring.worldSetup.dungeonDraftStatus =
        boolValue ? "dungeon_draft_edit_mode_on" : "dungeon_draft_edit_mode_off";
    context.window.creativeAuthoring.worldSetup.dungeonDraftReasonCode =
        context.window.creativeAuthoring.worldSetup.dungeonDraftStatus;
    resetDungeonDraftWindowCursor(context.worldSetupDraft, context.window);
    markAutomationApplied(context.window, command, "world.draft_edit_mode",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.draft_move") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControl.status = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.draft_move",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    // branch-gate: BG-1004
    if (!context.window.creativeAuthoring.worldSetup.dungeonDraftEditMode) {
      context.window.creativeAuthoring.worldSetup.dungeonDraftStatus = "dungeon_draft_edit_mode_off";
      context.window.creativeAuthoring.worldSetup.dungeonDraftReasonCode =
          "dungeon_draft_edit_mode_off";
      context.window.automationControl.status = "command_failed";
      markAutomationApplied(context.window, command, "world.draft_move",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductDungeonDraftDirectionAutomationResult direction =
        resolveProductDungeonDraftDirectionAutomation(value);
    // branch-gate: BG-1004
    if (!direction.valid) {
      context.window.automationControl.status = "invalid_value";
      markAutomationApplied(context.window, command, "world.draft_move",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductDungeonDraftOperationResult moved =
        moveProductDungeonDraftCursor(context.worldSetupDraft,
                                      dungeonDraftCursorFromWindow(context.window),
                                      direction.direction);
    recordDungeonDraftOperation(context.window, moved);
    // branch-gate: BG-1004
    markAutomationApplied(context.window, command, "world.draft_move",
                          context.currentOwner(),
                          moved.ok ? "applied" : "failed");
    // branch-gate: BG-1004
    if (!moved.ok) {
      context.window.automationControl.status = "command_failed";
    }
    return {true, moved.ok};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.draft_paint") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControl.status = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.draft_paint",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductDungeonDraftPaintAutomationResult paint =
        resolveProductDungeonDraftPaintAutomation(value);
    // branch-gate: BG-1004
    if (!paint.valid) {
      context.window.automationControl.status = "invalid_value";
      markAutomationApplied(context.window, command, "world.draft_paint",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const bool painted =
        applyDungeonDraftPaintGlyph(context.worldSetupDraft, context.window,
                                    paint.glyph.front());
    // branch-gate: BG-1004
    markAutomationApplied(context.window, command, "world.draft_paint",
                          context.currentOwner(),
                          painted ? "applied" : "failed");
    // branch-gate: BG-1004
    if (!painted) {
      context.window.automationControl.status = "command_failed";
    }
    return {true, painted};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.draft_cell") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControl.status = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.draft_cell",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const std::vector<std::string_view> fields = splitProductAutomationCsv(value);
    const ProductDungeonDraftCellAutomationResult cell =
        fields.size() == 3U
            ? resolveProductDungeonDraftCellAutomation(fields[0], fields[1],
                                                      fields[2])
            : ProductDungeonDraftCellAutomationResult{};
    // branch-gate: BG-1004
    if (!cell.valid) {
      context.window.automationControl.status = "invalid_value";
      markAutomationApplied(context.window, command, "world.draft_cell",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    ProductDungeonDraftOperationResult painted =
        setProductDungeonDraftCell(context.worldSetupDraft, cell.row, cell.column,
                                   fields[2].front());
    recordDungeonDraftOperation(context.window, painted);
    recordWorldSetupDraftState(context.worldSetupDraft, context.window);
    // branch-gate: BG-1004
    markAutomationApplied(context.window, command, "world.draft_cell",
                          context.currentOwner(),
                          painted.ok ? "applied" : "failed");
    // branch-gate: BG-1004
    if (!painted.ok) {
      context.window.automationControl.status = "command_failed";
    }
    return {true, painted.ok};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.ascii_room_text") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControl.status = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.ascii_room_text",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    context.worldSetupDraft.asciiRoomEnabled = true;
    context.worldSetupDraft.asciiRoomText =
        decodeProductAsciiRoomAutomationText(value);
    recordWorldSetupDraftState(context.worldSetupDraft, context.window);
    context.window.creativeAuthoring.worldSetup.status = "world_setup_ascii_room_text_updated";
    markAutomationApplied(context.window, command, "world.ascii_room_text",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.ascii_room_id") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControl.status = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.ascii_room_id",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductNonEmptyStringAutomationResult asciiRoomId =
        resolveProductNonEmptyStringAutomation(value);
    // branch-gate: BG-1004
    if (!asciiRoomId.valid) {
      context.window.automationControl.status = "invalid_value";
      markAutomationApplied(context.window, command, "world.ascii_room_id",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    context.worldSetupDraft.asciiRoomEnabled = true;
    context.worldSetupDraft.asciiRoomId = std::string(asciiRoomId.value);
    recordWorldSetupDraftState(context.worldSetupDraft, context.window);
    context.window.creativeAuthoring.worldSetup.status = "world_setup_ascii_room_id_updated";
    markAutomationApplied(context.window, command, "world.ascii_room_id",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.ascii_room_source_name") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControl.status = "owner_unavailable";
      markAutomationApplied(context.window, command,
                            "world.ascii_room_source_name",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductNonEmptyStringAutomationResult asciiRoomSourceName =
        resolveProductNonEmptyStringAutomation(value);
    // branch-gate: BG-1004
    if (!asciiRoomSourceName.valid) {
      context.window.automationControl.status = "invalid_value";
      markAutomationApplied(context.window, command,
                            "world.ascii_room_source_name",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    context.worldSetupDraft.asciiRoomEnabled = true;
    context.worldSetupDraft.asciiRoomSourceName =
        std::string(asciiRoomSourceName.value);
    recordWorldSetupDraftState(context.worldSetupDraft, context.window);
    context.window.creativeAuthoring.worldSetup.status =
        "world_setup_ascii_room_source_name_updated";
    markAutomationApplied(context.window, command, "world.ascii_room_source_name",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1165
  if (canonicalKey == "world.ascii_room_file") {
    // branch-gate: BG-1165
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControl.status = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.ascii_room_file",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductNonEmptyStringAutomationResult asciiRoomFile =
        resolveProductNonEmptyStringAutomation(value);
    const auto fail = [&]() -> ProductAutomationExecutionResult {
      context.window.automationControl.status = "invalid_value";
      markAutomationApplied(context.window, command, "world.ascii_room_file",
                            context.currentOwner(), "failed");
      return {true, false};
    };
    // branch-gate: BG-1165
    if (!asciiRoomFile.valid) {
      return fail();
    }
    const std::string asciiRoomText =
        readAutomationTextFile(asciiRoomFile.value);
    // branch-gate: BG-1165
    if (asciiRoomText.empty()) {
      return fail();
    }
    context.worldSetupDraft.worldName =
        titleFromAutomationAsciiRoomPath(asciiRoomFile.value);
    context.worldSetupDraft.asciiRoomEnabled = true;
    context.worldSetupDraft.asciiRoomId =
        std::string(productCustomDungeonRoomId());
    context.worldSetupDraft.asciiRoomSourceName = std::string(asciiRoomFile.value);
    context.worldSetupDraft.asciiRoomText = asciiRoomText;
    context.window.creativeAuthoring.worldSetup.dungeonDraftModified = false;
    context.window.creativeAuthoring.worldSetup.dungeonDraftEditMode = false;
    resetDungeonDraftWindowCursor(context.worldSetupDraft, context.window);
    recordWorldSetupDraftState(context.worldSetupDraft, context.window);
    context.window.creativeAuthoring.worldSetup.status = "world_setup_ascii_room_file_loaded";
    markAutomationApplied(context.window, command, "world.ascii_room_file",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "ascii_room.text") {
    context.window.creativeAuthoring.asciiRoomDraft.text =
        decodeProductAsciiRoomAutomationText(value);
    context.window.creativeAuthoring.asciiRoomPreview.status = "ascii_room_text_updated";
    context.window.creativeAuthoring.asciiRoomPreview.reasonCode = "ascii_room_text_updated";
    context.window.creativeAuthoring.asciiRoomPreview.failedStage = "not_started";
    context.window.creativeAuthoring.asciiRoomPreview.ready = false;
    markAutomationApplied(context.window, command, "ascii_room.text",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "ascii_room.room_id") {
    const ProductNonEmptyStringAutomationResult roomId =
        resolveProductNonEmptyStringAutomation(value);
    // branch-gate: BG-1004
    if (!roomId.valid) {
      context.window.automationControl.status = "invalid_value";
      return {true, false};
    }
    context.window.creativeAuthoring.asciiRoomDraft.roomId = std::string(roomId.value);
    markAutomationApplied(context.window, command, "ascii_room.room_id",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "ascii_room.source_name") {
    const ProductNonEmptyStringAutomationResult sourceName =
        resolveProductNonEmptyStringAutomation(value);
    // branch-gate: BG-1004
    if (!sourceName.valid) {
      context.window.automationControl.status = "invalid_value";
      return {true, false};
    }
    context.window.creativeAuthoring.asciiRoomDraft.sourceName = std::string(sourceName.value);
    markAutomationApplied(context.window, command, "ascii_room.source_name",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "ascii_room.build") {
    bool boolValue = false;
    // branch-gate: BG-1004
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return {true, false};
    }
    // branch-gate: BG-1004
    if (!boolValue) {
      markAutomationApplied(context.window, command, "ascii_room.build",
                            context.currentOwner(), "ignored");
      return {true, true};
    }
    const bool previewBuilt = buildProductAsciiRoomPreview(context.window);
    // branch-gate: BG-1004
    markAutomationApplied(context.window, command, "ascii_room.build",
                          context.currentOwner(),
                          previewBuilt ? "applied" : "failed");
    return {true, previewBuilt};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "ascii_room.activate") {
    bool boolValue = false;
    // branch-gate: BG-1004
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return {true, false};
    }
    // branch-gate: BG-1004
    if (!boolValue) {
      markAutomationApplied(context.window, command, "ascii_room.activate",
                            context.currentOwner(), "ignored");
      return {true, true};
    }
    const bool activated = context.activateAsciiRoom();
    // branch-gate: BG-1004
    markAutomationApplied(context.window, command, "ascii_room.activate",
                          context.currentOwner(),
                          activated ? "applied" : "failed");
    return {true, activated};
  }

  return {};
}

}  // namespace iggy3d
