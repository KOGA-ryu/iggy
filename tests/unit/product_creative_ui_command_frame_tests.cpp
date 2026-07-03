#include "app/iggy3d/window/CreativeUiCommandFrame.hpp"

#include "app/iggy3d/creative/Facade.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

iggy3d::ProductCreativeUiInputFrameReceipt commandInput(
    std::string_view semanticId,
    bool consumed = true,
    bool enabled = true) {
  iggy3d::ProductCreativeUiInputFrameReceipt receipt;
  receipt.consumed = consumed;
  receipt.enabled = enabled;
  receipt.semanticId = std::string(semanticId);
  return receipt;
}

iggy3d::ProductCreativeUiCommandFrameReceipt routeCommand(
    cr::Facade& facade,
    std::string_view semanticId = "creative.row.tools.active_tool") {
  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.facade = &facade;
  request.inputReceipt = commandInput(semanticId);
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

bool nullFacadeFailsClosed() {
  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.inputReceipt = commandInput("creative.row.tools.active_tool");
  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      iggy3d::routeProductCreativeUiCommandFrame(request);

  return expect(receipt.requested, "null requested") &&
         expect(!receipt.facadeAvailable, "null facade unavailable") &&
         expect(receipt.inputConsumed, "null consumed copied") &&
         expect(receipt.inputEnabled, "null enabled copied") &&
         expect(!receipt.accepted, "null not accepted") &&
         expect(!receipt.changed, "null unchanged") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::None,
                "null command none") &&
         expect(receipt.toolBefore == cr::Tool::Select,
                "null before default select") &&
         expect(receipt.toolAfter == cr::Tool::Select,
                "null after default select") &&
         expect(receipt.semanticId == "creative.row.tools.active_tool",
                "null semantic copied") &&
         expect(receipt.status ==
                    "product_creative_ui_command_facade_missing",
                "null status") &&
         expect(receipt.reasonCode ==
                    "product_creative_ui_command_facade_missing",
                "null reason");
}

bool notConsumedInputNoops() {
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.facade = &facade;
  request.inputReceipt =
      commandInput("creative.row.tools.active_tool", false, true);
  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      iggy3d::routeProductCreativeUiCommandFrame(request);

  return expect(receipt.requested, "not consumed requested") &&
         expect(receipt.facadeAvailable, "not consumed facade available") &&
         expect(!receipt.inputConsumed, "not consumed copied") &&
         expect(receipt.inputEnabled, "not consumed enabled copied") &&
         expect(!receipt.accepted, "not consumed not accepted") &&
         expect(!receipt.changed, "not consumed unchanged") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::None,
                "not consumed command none") &&
         expect(receipt.toolBefore == cr::Tool::Measure,
                "not consumed before") &&
         expect(receipt.toolAfter == cr::Tool::Measure,
                "not consumed after") &&
         expect(facade.toolState().activeTool == cr::Tool::Measure,
                "not consumed facade unchanged") &&
         expect(receipt.status ==
                    "product_creative_ui_command_not_consumed",
                "not consumed status");
}

bool consumedDisabledNoops() {
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.facade = &facade;
  request.inputReceipt =
      commandInput("creative.row.tools.active_tool", true, false);
  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      iggy3d::routeProductCreativeUiCommandFrame(request);

  return expect(receipt.inputConsumed, "disabled consumed copied") &&
         expect(!receipt.inputEnabled, "disabled enabled false") &&
         expect(!receipt.accepted, "disabled not accepted") &&
         expect(!receipt.changed, "disabled unchanged") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::None,
                "disabled command none") &&
         expect(receipt.toolBefore == cr::Tool::Inspect,
                "disabled before") &&
         expect(receipt.toolAfter == cr::Tool::Inspect,
                "disabled after") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "disabled facade unchanged") &&
         expect(receipt.status == "product_creative_ui_command_disabled",
                "disabled status");
}

bool consumedUnknownSemanticNoops() {
  cr::Facade facade;
  facade.reset();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(facade, "creative.row.status.creative_status");

  return expect(receipt.facadeAvailable, "unknown facade available") &&
         expect(receipt.inputConsumed, "unknown consumed") &&
         expect(receipt.inputEnabled, "unknown enabled") &&
         expect(!receipt.accepted, "unknown not accepted") &&
         expect(!receipt.changed, "unknown unchanged") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::None,
                "unknown command none") &&
         expect(receipt.toolBefore == cr::Tool::Select,
                "unknown before") &&
         expect(receipt.toolAfter == cr::Tool::Select, "unknown after") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "unknown facade unchanged") &&
         expect(receipt.status ==
                    "product_creative_ui_command_unknown_semantic",
                "unknown status");
}

bool activeToolCommandCyclesSelectToInspect() {
  cr::Facade facade;
  facade.reset();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(facade);

  return expect(receipt.accepted, "cycle accepted") &&
         expect(receipt.changed, "cycle changed") &&
         expect(receipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::CycleNextTool,
                "cycle command kind") &&
         expect(receipt.toolBefore == cr::Tool::Select, "cycle before") &&
         expect(receipt.toolAfter == cr::Tool::Inspect, "cycle after") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "cycle facade tool") &&
         expect(receipt.status == "product_creative_ui_command_applied",
                "cycle status");
}

bool repeatedActiveToolCommandCyclesToolOrder() {
  cr::Facade facade;
  facade.reset();

  const iggy3d::ProductCreativeUiCommandFrameReceipt first =
      routeCommand(facade);
  const iggy3d::ProductCreativeUiCommandFrameReceipt second =
      routeCommand(facade);
  const iggy3d::ProductCreativeUiCommandFrameReceipt third =
      routeCommand(facade);

  return expect(first.toolBefore == cr::Tool::Select &&
                    first.toolAfter == cr::Tool::Inspect,
                "repeat select inspect") &&
         expect(second.toolBefore == cr::Tool::Inspect &&
                    second.toolAfter == cr::Tool::Measure,
                "repeat inspect measure") &&
         expect(third.toolBefore == cr::Tool::Measure &&
                    third.toolAfter == cr::Tool::Select,
                "repeat measure select") &&
         expect(first.changed && second.changed && third.changed,
                "repeat all changed") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "repeat facade select");
}

bool commandUpdatesOldStateAndDoesNotMutateDocument() {
  cr::Facade facade;
  facade.reset();
  const cr::CreativeObjectId roomId = facade.createRoom("Room");
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
      routeCommand(facade);

  return expect(roomId != cr::kInvalidObjectId, "state room created") &&
         expect(receipt.changed, "state changed") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "state tool state inspect") &&
         expect(facade.state().tool == cr::Tool::Inspect,
                "state old state inspect") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "state document unchanged");
}

bool nonToolRowsRemainUnknownNoop() {
  constexpr std::array<std::string_view, 3> kUnknownRows = {
      "creative.row.status.creative_status",
      "creative.row.selection.selected_target",
      "creative.row.snap.snap_settings",
  };

  bool ok = true;
  for (std::string_view semanticId : kUnknownRows) {
    cr::Facade facade;
    facade.reset();
    const iggy3d::ProductCreativeUiCommandFrameReceipt receipt =
        routeCommand(facade, semanticId);
    ok &= expect(receipt.semanticId == semanticId, "unknown row semantic") &&
          expect(receipt.status ==
                     "product_creative_ui_command_unknown_semantic",
                 "unknown row status") &&
          expect(receipt.commandKind ==
                     iggy3d::ProductCreativeUiCommandKind::None,
                 "unknown row command none") &&
          expect(!receipt.accepted, "unknown row not accepted") &&
          expect(!receipt.changed, "unknown row unchanged") &&
          expect(facade.toolState().activeTool == cr::Tool::Select,
                 "unknown row facade unchanged");
  }
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok &= nullFacadeFailsClosed();
  ok &= notConsumedInputNoops();
  ok &= consumedDisabledNoops();
  ok &= consumedUnknownSemanticNoops();
  ok &= activeToolCommandCyclesSelectToInspect();
  ok &= repeatedActiveToolCommandCyclesToolOrder();
  ok &= commandUpdatesOldStateAndDoesNotMutateDocument();
  ok &= nonToolRowsRemainUnknownNoop();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
