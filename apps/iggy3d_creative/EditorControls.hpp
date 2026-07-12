#pragma once

#include "app/iggy3d/creative/input/ControlProfile.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/ui/UiWidgets.hpp"
#include "render/FrameInput.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace iggy3d {

class SdlWindow;

}  // namespace iggy3d

namespace iggy3d_creative_app {

struct CreativeEditorState;

enum class CreativeEditorControlPersistenceStatus : std::uint8_t {
  Missing,
  Loaded,
  Saved,
  Invalid,
  IoError,
};

struct CreativeEditorControlPersistenceReceipt {
  CreativeEditorControlPersistenceStatus status =
      CreativeEditorControlPersistenceStatus::Missing;
  std::size_t bindingCount = 0;
  bool accepted = false;
};

struct CreativeEditorControlsState {
  bool open = false;
  iggy3d::creative::CreativeControlDevice activeDevice =
      iggy3d::creative::CreativeControlDevice::KeyboardMouse;
  std::size_t selectedIndex = 0;
  std::size_t scrollOffset = 0;
  iggy3d::creative::CreativeUiWidgetId focusedWidgetId =
      iggy3d::creative::kInvalidCreativeUiWidgetId;
  iggy3d::creative::CreativeControlConflictPolicy conflictPolicy =
      iggy3d::creative::CreativeControlConflictPolicy::Swap;
  iggy3d::creative::CreativeControlBindingList bindingList;

  bool capturing = false;
  bool captureWaitingForRelease = false;
  std::uint16_t captureGroup = 0;
  iggy3d::creative::CreativeControlDevice captureDevice =
      iggy3d::creative::CreativeControlDevice::KeyboardMouse;
  iggy3d::creative::CreativeControlRebindReceipt lastRebind;
  std::string statusLabel;

  iggy3d::creative::CreativeUiRepeatState repeatState;
};

struct CreativeEditorControlsFrameRequest {
  iggy3d::SdlWindow& window;
  CreativeEditorState& editor;
  const iggy3d::creative::CreativeInputRouteResult& routedInput;
  const iggy3d::creative::CreativeInputFrame& inputFrame;
  const std::filesystem::path& settingsPath;
  const std::filesystem::path& toolWheelSettingsPath;
  std::uint64_t monotonicTimeNanoseconds = 0;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
};

struct CreativeEditorControlsFrameResult {
  bool blockWorldActions = false;
  bool openChanged = false;
  bool profileChanged = false;
  bool profileSaved = false;
  bool toolWheelChanged = false;
  bool toolWheelSaved = false;
};

[[nodiscard]] CreativeEditorControlPersistenceReceipt
loadCreativeEditorControlProfile(
    iggy3d::creative::CreativeControlProfile& profile,
    const std::filesystem::path& path);
[[nodiscard]] CreativeEditorControlPersistenceReceipt
saveCreativeEditorControlProfile(
    const iggy3d::creative::CreativeControlProfile& profile,
    const std::filesystem::path& path);

[[nodiscard]] CreativeEditorControlsFrameResult
processCreativeEditorControlsFrame(
    const CreativeEditorControlsFrameRequest& request);

[[nodiscard]] bool selectCreativeEditorControlsTab(
    CreativeEditorState& editor,
    iggy3d::creative::CreativeControlDevice device) noexcept;

void appendCreativeEditorControlsOverlay(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs);

}  // namespace iggy3d_creative_app
