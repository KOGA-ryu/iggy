#pragma once

#include "ProductTestSupport.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/ascii_room/Activation.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"
#include "runtime/session/Session.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace iggy3d::test {

struct ProductAsciiRoomWindowFixtureRequest {
  std::string_view roomText;
  std::string_view roomId;
  std::string_view sourceName;
  std::string_view activationSuccessMessage = "ascii room activation ok";
  std::optional<float> cameraYawDegrees = std::nullopt;
  std::optional<float> cameraPitchDegrees = std::nullopt;
  std::optional<ProductInteractionMode> interactionMode = std::nullopt;
};

inline ProductAppWindowState activateAsciiRoomWindowForTest(
    std::optional<Session>& session,
    const ProductAsciiRoomWindowFixtureRequest& request) {
  ProductAppWindowState window;
  window.creativeAuthoring.asciiRoomDraft.text = std::string{request.roomText};
  window.creativeAuthoring.asciiRoomDraft.roomId = std::string{request.roomId};
  window.creativeAuthoring.asciiRoomDraft.sourceName =
      std::string{request.sourceName};

  if (request.cameraYawDegrees.has_value()) {
    window.viewport.cameraYawDegrees = request.cameraYawDegrees.value();
  }
  if (request.cameraPitchDegrees.has_value()) {
    window.viewport.cameraPitchDegrees = request.cameraPitchDegrees.value();
  }

  const ProductAsciiRoomActivationResult activation =
      activateProductAsciiRoomPreview(session, window);
  expect(activation.ok, request.activationSuccessMessage);

  if (request.interactionMode.has_value()) {
    window.inputDevice.interactionMode = request.interactionMode.value();
  }

  return window;
}

}  // namespace iggy3d::test
