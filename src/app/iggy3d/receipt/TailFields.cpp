#include "app/iggy3d/receipt/ReceiptFields.hpp"

#include <array>
#include <charconv>
#include <string>
#include <string_view>
#include <utility>

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/gameplay/MovementProof.hpp"
#include "app/iggy3d/debug/MovementDebugHud.hpp"
#include "app/iggy3d/debug/NpcBehaviorDebugHud.hpp"
#include "app/iggy3d/debug/PhysicsDebugHud.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"
#include "app/iggy3d/creative/bridge/ViewportPickFrame.hpp"
#include "app/iggy3d/creative/bridge/WireframeFrame.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"

namespace iggy3d {

namespace {

struct TailReceiptContext {
  const ProductAppOptions& options;
  const ProductWorldTemplate& world;
  const ProductAppWindowState& window;
  const ProductSaveBridgeResult& saves;
};

struct TailReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const TailReceiptContext& context,
                 std::string_view key);
};

const std::array<TailReceiptFieldRow, 12> kTailReceiptFields{{
    {"event_poll_count",
     [](RenderReceipt& receipt,
        const TailReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.frontendShell.eventPollCount);
     }},
    {"frames",
     [](RenderReceipt& receipt,
        const TailReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           static_cast<std::uint64_t>(context.options.frames));
     }},
    {"frames_presented",
     [](RenderReceipt& receipt,
        const TailReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.window.frontendShell.framesPresented);
     }},
    {"window_status",
     [](RenderReceipt& receipt,
        const TailReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.window.frontendShell.status);
     }},
    {"save_root",
     [](RenderReceipt& receipt,
        const TailReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.saves.saveRoot.generic_string());
     }},
    {"save_count",
     [](RenderReceipt& receipt,
        const TailReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           static_cast<std::uint64_t>(
               context.saves.slots.slots.size()));
     }},
    {"compatible_save_count",
     [](RenderReceipt& receipt,
        const TailReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.saves.slots.compatibleCount);
     }},
    {"selected_package_id",
     [](RenderReceipt& receipt,
        const TailReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.world.packageId);
     }},
    {"selected_scenario_id",
     [](RenderReceipt& receipt,
        const TailReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.world.scenarioId);
     }},
    {"world_template_source",
     [](RenderReceipt& receipt,
        const TailReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.world.source);
     }},
    {"dev_package_override",
     [](RenderReceipt& receipt,
        const TailReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           !context.options.devPackageOverride.empty());
     }},
    {"normal_package_cli",
     [](RenderReceipt& receipt,
        const TailReceiptContext&,
        std::string_view key) {
       appendReceiptField(receipt, key, false);
     }},
}};

}  // namespace

void appendProductTailFields(RenderReceipt& receipt,
                             const ProductAppOptions& options,
                             const ProductWorldTemplate& world,
                             const ProductAppWindowState& window,
                             const ProductSaveBridgeResult& saves) {
  const TailReceiptContext context{
      options,
      world,
      window,
      saves,
  };

  for (const TailReceiptFieldRow& row : kTailReceiptFields) {
    row.append(receipt, context, row.key);
  }
}

}  // namespace iggy3d
