#include "app/iggy3d/receipt/ReceiptFields.hpp"

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

void appendProductTailFields(RenderReceipt& receipt, const ProductAppOptions& options, const ProductWorldTemplate& world, const ProductAppWindowState& window, const ProductSaveBridgeResult& saves) {
  appendReceiptField(receipt, "event_poll_count", window.frontendShell.eventPollCount);
  appendReceiptField(receipt, "frames", static_cast<std::uint64_t>(options.frames));
  appendReceiptField(receipt, "frames_presented", window.frontendShell.framesPresented);
  appendReceiptField(receipt, "window_status", window.frontendShell.status);
  appendReceiptField(receipt, "save_root", saves.saveRoot.generic_string());
  appendReceiptField(receipt, "save_count",
                     static_cast<std::uint64_t>(saves.slots.slots.size()));
  appendReceiptField(receipt, "compatible_save_count", saves.slots.compatibleCount);
  appendReceiptField(receipt, "selected_package_id", world.packageId);
  appendReceiptField(receipt, "selected_scenario_id", world.scenarioId);
  appendReceiptField(receipt, "world_template_source", world.source);
  appendReceiptField(receipt, "dev_package_override", !options.devPackageOverride.empty());
  appendReceiptField(receipt, "normal_package_cli", false);
}

}  // namespace iggy3d
