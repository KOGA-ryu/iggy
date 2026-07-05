#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/input/InputAction.hpp"
#include "app/input/KeyboardInput.hpp"
#include "app/input/MouseInput.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d {

struct ActionState;
struct ProductAppWindowState;

namespace creative {
struct CreativeAppState;
}  // namespace creative

// Held-button pointer gesture lifecycle state (TL-3). Persists across frames so
// the window layer can synthesize the full Press -> Move* -> Release lifecycle
// from raw mouse button + position state. Reset on tool switch / mode exit so a
// stranded held-flag cannot fire a phantom release later.
struct ProductCreativePointerLifecycleState {
  bool primaryButtonHeld = false;
  float lastPointerX = 0.0F;
  float lastPointerY = 0.0F;
};

// One raw mouse snapshot: is the primary button down THIS frame and where.
struct ProductCreativePointerSample {
  bool primaryButtonDown = false;
  float x = 0.0F;
  float y = 0.0F;
};

enum class ProductCreativePointerLifecyclePhase : std::uint8_t {
  None,     // button up last frame, up now (or held+unmoved): nothing to emit
  Press,    // button-down edge (up last frame, down now)
  Move,     // primary button held and the pointer position changed
  Release,  // button-up edge (down last frame, up now)
};

struct ProductCreativePointerLifecycleEvent {
  ProductCreativePointerLifecyclePhase phase =
      ProductCreativePointerLifecyclePhase::None;
  float x = 0.0F;
  float y = 0.0F;
  // Move-drag destination resolved from the pointer's grid XZ (TD-7). The
  // window fills it for Move/Release phases so the tool's snapped commit has a
  // destination anchor; the tool core carries it, the facade snaps + commits.
  bool hasWorldDestination = false;
  creative::CreativeToolWorldPoint worldDestination;
};

struct ProductCreativeInputFrameRequest {
  ProductAppWindowState* window = nullptr;
  creative::CreativeAppState* creative = nullptr;
  InputAction action = InputAction::None;
  bool toolKeyRequested = false;
  creative::Tool toolKey = creative::Tool::Select;
  MouseClick click;
  creative::TargetRef pointerTarget;
  // TL-3 pointer lifecycle continuation of the Press carried by `click`: a Move
  // while the primary button is held, or the Release that ends the gesture. The
  // pick chain resolves Press's target; Move/Release carry the current pointer
  // position (raw window-pixel space, same as `click`) so a drag/measure can
  // update and END. None phase = no lifecycle packet dispatched this frame.
  ProductCreativePointerLifecycleEvent pointerLifecycle;
  creative::TargetRef pointerLifecycleTarget;
};

struct ProductCreativeInputActionsRequest {
  ProductAppWindowState* window = nullptr;
  creative::CreativeAppState* creative = nullptr;
  const ActionState* actions = nullptr;
  KeyboardCreativeToolKeyPresses toolKeys;
  MouseClick click;
  creative::TargetRef pointerTarget;
  ProductCreativePointerLifecycleEvent pointerLifecycle;
  creative::TargetRef pointerLifecycleTarget;
};

struct ProductCreativeInputFrameReceipt {
  bool requested = false;
  bool active = false;
  bool facadeAvailable = false;
  bool actionHandled = false;
  bool toolChanged = false;
  bool pointerDispatched = false;
  bool pointerMoveDispatched = false;
  bool pointerReleaseDispatched = false;
  bool cancelDispatched = false;
  bool accepted = false;
  bool changed = false;
  std::string_view status = "product_creative_input_not_requested";
  std::string_view reasonCode = "product_creative_input_not_requested";
  creative::Tool activeToolBefore = creative::Tool::Select;
  creative::Tool activeToolAfter = creative::Tool::Select;
  creative::CreativeToolInputKind inputKind =
      creative::CreativeToolInputKind::Unknown;
  std::size_t emittedIntentCount = 0;
};

[[nodiscard]] bool productCreativeInputActiveForWindow(
    const ProductAppWindowState& window) noexcept;
[[nodiscard]] bool productCreativeToolKeyTarget(
    const KeyboardCreativeToolKeyPresses& presses,
    creative::Tool& out) noexcept;
[[nodiscard]] creative::CreativeToolInputPacket productCreativePointerPressPacket(
    const MouseClick& click,
    creative::TargetRef target = {}) noexcept;
[[nodiscard]] creative::CreativeToolInputPacket productCreativePointerMovePacket(
    float x,
    float y,
    creative::TargetRef target = {},
    bool hasWorldDestination = false,
    creative::CreativeToolWorldPoint worldDestination = {}) noexcept;
[[nodiscard]] creative::CreativeToolInputPacket
productCreativePointerReleasePacket(
    float x,
    float y,
    creative::TargetRef target = {},
    bool hasWorldDestination = false,
    creative::CreativeToolWorldPoint worldDestination = {}) noexcept;
// Pure lifecycle resolver: given the persistent held-state and this frame's raw
// sample, advance the state and report which lifecycle phase (if any) to emit.
// Press does NOT flow through here in the frame (the pick chain owns Press so it
// can resolve a target); Move/Release are the window-layer additions of TV1-F.
[[nodiscard]] ProductCreativePointerLifecycleEvent
resolveProductCreativePointerLifecycle(
    ProductCreativePointerLifecycleState& state,
    const ProductCreativePointerSample& sample) noexcept;
// Clear held-state so a stranded held-flag cannot fire a phantom Release after a
// tool switch or a creative-mode exit.
void resetProductCreativePointerLifecycle(
    ProductCreativePointerLifecycleState& state) noexcept;
[[nodiscard]] ProductCreativeInputFrameReceipt processProductCreativeInputFrame(
    const ProductCreativeInputFrameRequest& request);
[[nodiscard]] ProductCreativeInputFrameReceipt processProductCreativeInputActions(
    const ProductCreativeInputActionsRequest& request);

}  // namespace iggy3d
