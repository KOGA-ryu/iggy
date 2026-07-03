#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Tools.hpp"
#include "app/input/InputAction.hpp"
#include "app/input/MouseInput.hpp"

#include <cstddef>
#include <string_view>

namespace iggy3d {

struct ProductAppWindowState;

namespace creative {
class Facade;
}  // namespace creative

struct ProductCreativeInputFrameRequest {
  ProductAppWindowState* window = nullptr;
  creative::Facade* facade = nullptr;
  InputAction action = InputAction::None;
  MouseClick click;
};

struct ProductCreativeInputFrameReceipt {
  bool requested = false;
  bool active = false;
  bool facadeAvailable = false;
  bool actionHandled = false;
  bool toolChanged = false;
  bool pointerDispatched = false;
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
[[nodiscard]] creative::Tool nextProductCreativeTool(
    creative::Tool tool) noexcept;
[[nodiscard]] creative::Tool previousProductCreativeTool(
    creative::Tool tool) noexcept;
[[nodiscard]] bool productCreativeToolActionTarget(
    InputAction action,
    creative::Tool current,
    creative::Tool& out) noexcept;
[[nodiscard]] creative::CreativeToolInputPacket productCreativePointerPressPacket(
    const MouseClick& click) noexcept;
[[nodiscard]] ProductCreativeInputFrameReceipt processProductCreativeInputFrame(
    const ProductCreativeInputFrameRequest& request);

}  // namespace iggy3d
