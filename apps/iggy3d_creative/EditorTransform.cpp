#include "EditorTransform.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorState.hpp"
#include "app/platform/SdlWindow.hpp"

namespace iggy3d_creative_app {
namespace {

constexpr float kTau = 6.28318530717958647692F;

[[nodiscard]] bool finiteVec3(cr::CreativeVec3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

[[nodiscard]] bool sameVec3(cr::CreativeVec3 lhs,
                            cr::CreativeVec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] cr::CreativeVec3 subtract(cr::CreativeVec3 lhs,
                                        cr::CreativeVec3 rhs) noexcept {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] std::vector<cr::CreativeObjectId> sourceObjectIds(
    const CreativeEditorSelectionTransformState& state) {
  std::vector<cr::CreativeObjectId> ids;
  ids.reserve(state.sourceClipboard.objects.size());
  for (const cr::CreativeObject& object : state.sourceClipboard.objects) {
    ids.push_back(object.id);
  }
  return ids;
}

[[nodiscard]] bool moveSourceAvailable(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard) noexcept {
  const cr::CreativeDocument& document = appState.facade.document();
  if (clipboard.sourceDocumentId != document.id() ||
      clipboard.sourceRevision != document.revision() ||
      cr::creativeClipboardEmpty(clipboard)) {
    return false;
  }
  return std::all_of(
      clipboard.objects.begin(), clipboard.objects.end(),
      [&document](const cr::CreativeObject& object) {
        return document.containsObject(object.id);
      });
}

void refreshTransformPlan(const cr::CreativeAppState& appState,
                          CreativeEditorSelectionTransformState& state) {
  if (!state.active || !state.targetPositionable) {
    state.plan = {};
    return;
  }
  state.request.mode = state.mode;
  state.request.sourceAnchor = state.sourceClipboard.placementAnchor;
  state.plan = cr::planCreativeSelectionPlacement(
      state.sourceClipboard.objects, state.request);
  state.moveAvailable = moveSourceAvailable(appState, state.sourceClipboard);
  if (state.mode == cr::CreativeSelectionPlacementMode::Move &&
      !state.moveAvailable) {
    state.plan.accepted = false;
    state.plan.status = cr::CreativeSelectionPlacementStatus::InvalidSource;
    state.plan.reasonCode = "editor_transform_move_source_stale";
  }
}

[[nodiscard]] bool beginTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorTransformSource transformSource,
    cr::CreativeSelectionPlacementMode mode,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  state = {};
  if (cr::creativeClipboardEmpty(clipboard) || !clipboard.hasPlacementAnchor ||
      !finiteVec3(clipboard.placementAnchor)) {
    SDL_Log("iggy3d_creative: TRANSFORM preview rejected source='%s' "
            "objectCount=%zu hasAnchor=%d",
            std::string(source).c_str(), clipboard.objects.size(),
            clipboard.hasPlacementAnchor ? 1 : 0);
    return false;
  }
  state.active = true;
  state.source = transformSource;
  state.mode = mode;
  state.sourceClipboard = clipboard;
  state.request.mode = mode;
  state.request.sourceAnchor = clipboard.placementAnchor;
  state.moveAvailable = moveSourceAvailable(appState, clipboard);
  if (mode == cr::CreativeSelectionPlacementMode::Move &&
      !state.moveAvailable) {
    state = {};
    return false;
  }
  SDL_Log("iggy3d_creative: TRANSFORM preview begun source='%s' mode='%s' "
          "objectCount=%zu anchor=(%.3f, %.3f, %.3f)",
          std::string(source).c_str(),
          std::string(cr::toString(mode)).c_str(), clipboard.objects.size(),
          clipboard.placementAnchor.x, clipboard.placementAnchor.y,
          clipboard.placementAnchor.z);
  return true;
}

[[nodiscard]] cr::CreativeSelectionPlacementReceipt placeObjectsWithHistory(
    cr::CreativeAppState& appState,
    std::span<const cr::CreativeObjectId> objectIds,
    const cr::CreativeSelectionPlacementRequest& request,
    std::string_view source) {
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  cr::CreativeSelectionPlacementReceipt receipt =
      appState.facade.placeObjects(objectIds, request);
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.reasonCode));
  SDL_Log("iggy3d_creative: TRANSFORM move source='%s' accepted=%d changed=%d "
          "status='%s' objects=%llu reasonCode='%s'",
          std::string(source).c_str(), receipt.accepted ? 1 : 0,
          receipt.changed ? 1 : 0,
          std::string(cr::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.objectCount),
          receipt.reasonCode.c_str());
  return receipt;
}

[[nodiscard]] cr::CreativeClipboardPasteRequest clipboardPasteRequest(
    const CreativeEditorSelectionTransformState& state) noexcept {
  cr::CreativeClipboardPasteRequest request;
  request.offset = subtract(state.request.targetAnchor,
                            state.request.sourceAnchor);
  request.quarterTurns = state.request.quarterTurns;
  request.mirrorX = state.request.mirrorX;
  request.mirrorZ = state.request.mirrorZ;
  return request;
}

void moveControlSelection(CreativeEditorSelectionTransformState& state,
                          std::int32_t direction) noexcept {
  if (direction == 0) {
    return;
  }
  const std::int64_t count =
      static_cast<std::int64_t>(kCreativeEditorTransformControlCount);
  const std::int64_t current =
      static_cast<std::int64_t>(state.selectedControl);
  state.selectedControl = static_cast<std::size_t>(
      ((current + static_cast<std::int64_t>(direction)) % count + count) %
      count);
}

void selectControlDirection(CreativeEditorSelectionTransformState& state,
                            float x,
                            float y,
                            float deadzone) noexcept {
  if (!std::isfinite(x) || !std::isfinite(y) ||
      std::hypot(x, y) <= deadzone) {
    return;
  }
  float angle = std::atan2(x, y);
  if (angle < 0.0F) {
    angle += kTau;
  }
  const float scaled = angle / kTau *
                       static_cast<float>(kCreativeEditorTransformControlCount);
  state.selectedControl = static_cast<std::size_t>(std::round(scaled)) %
                          kCreativeEditorTransformControlCount;
}

}  // namespace

std::string_view toString(CreativeEditorTransformControl control) noexcept {
  switch (control) {
    case CreativeEditorTransformControl::RotatePositive: return "RotatePositive";
    case CreativeEditorTransformControl::MirrorX: return "MirrorX";
    case CreativeEditorTransformControl::ToggleMode: return "ToggleMode";
    case CreativeEditorTransformControl::Confirm: return "Confirm";
    case CreativeEditorTransformControl::Cancel: return "Cancel";
    case CreativeEditorTransformControl::MirrorZ: return "MirrorZ";
    case CreativeEditorTransformControl::RotateNegative: return "RotateNegative";
    case CreativeEditorTransformControl::Reset: return "Reset";
    case CreativeEditorTransformControl::Count: return "Count";
  }
  return "Unknown";
}

bool beginCreativeEditorClipboardTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  return beginTransformPreview(
      appState, clipboard, CreativeEditorTransformSource::Clipboard,
      cr::CreativeSelectionPlacementMode::Copy, state, source);
}

bool beginCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  cr::CreativeClipboard selection;
  const cr::CreativeClipboardCopyReceipt copied =
      appState.facade.copySelectedObjectsToClipboard(selection);
  if (!copied.accepted) {
    return false;
  }
  return beginTransformPreview(
      appState, selection, CreativeEditorTransformSource::Selection,
      cr::CreativeSelectionPlacementMode::Move, state, source);
}

bool requestCreativeEditorSelectionTransformCommit(
    CreativeEditorSelectionTransformState& state) noexcept {
  if (!state.active || state.commitRequested) {
    return false;
  }
  state.commitRequested = true;
  return true;
}

bool cancelCreativeEditorSelectionTransformPreview(
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  if (!state.active) {
    return false;
  }
  SDL_Log("iggy3d_creative: TRANSFORM preview cancelled source='%s'",
          std::string(source).c_str());
  state = {};
  return true;
}

bool applyCreativeEditorTransformControl(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformControl control) {
  if (!state.active || control == CreativeEditorTransformControl::Count) {
    return false;
  }
  bool changed = true;
  switch (control) {
    case CreativeEditorTransformControl::RotatePositive:
      state.request.quarterTurns =
          static_cast<std::uint8_t>((state.request.quarterTurns + 1U) % 4U);
      break;
    case CreativeEditorTransformControl::MirrorX:
      state.request.mirrorX = !state.request.mirrorX;
      break;
    case CreativeEditorTransformControl::ToggleMode:
      if (state.mode == cr::CreativeSelectionPlacementMode::Move) {
        state.mode = cr::CreativeSelectionPlacementMode::Copy;
      } else if (state.moveAvailable) {
        state.mode = cr::CreativeSelectionPlacementMode::Move;
      } else {
        changed = false;
      }
      break;
    case CreativeEditorTransformControl::Confirm:
      changed = requestCreativeEditorSelectionTransformCommit(state);
      state.controlsOpen = false;
      break;
    case CreativeEditorTransformControl::Cancel:
      return cancelCreativeEditorSelectionTransformPreview(
          state, "transform_control_cancel");
    case CreativeEditorTransformControl::MirrorZ:
      state.request.mirrorZ = !state.request.mirrorZ;
      break;
    case CreativeEditorTransformControl::RotateNegative:
      state.request.quarterTurns =
          static_cast<std::uint8_t>((state.request.quarterTurns + 3U) % 4U);
      break;
    case CreativeEditorTransformControl::Reset:
      changed = state.request.quarterTurns != 0U || state.request.mirrorX ||
                state.request.mirrorZ;
      state.request.quarterTurns = 0U;
      state.request.mirrorX = false;
      state.request.mirrorZ = false;
      break;
    case CreativeEditorTransformControl::Count:
      return false;
  }
  if (changed && state.active) {
    state.lastCommit = {};
    refreshTransformPlan(appState, state);
  }
  return changed;
}

CreativeEditorTransformCommitReceipt
processCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    bool targetPositionable,
    cr::CreativeVec3 targetAnchor,
    bool secondaryPressed,
    std::string_view source) {
  if (!state.active) {
    return {};
  }

  const bool targetChanged =
      state.targetPositionable != targetPositionable ||
      (targetPositionable && !sameVec3(state.request.targetAnchor, targetAnchor));
  state.targetPositionable = targetPositionable && finiteVec3(targetAnchor);
  if (state.targetPositionable) {
    state.request.targetAnchor = targetAnchor;
  }
  if (targetChanged) {
    state.lastCommit = {};
  }
  refreshTransformPlan(appState, state);
  if (secondaryPressed) {
    static_cast<void>(requestCreativeEditorSelectionTransformCommit(state));
  }
  if (!state.commitRequested) {
    return {};
  }
  state.commitRequested = false;

  CreativeEditorTransformCommitReceipt receipt;
  receipt.requested = true;
  receipt.mode = state.mode;
  if (!state.targetPositionable || !state.plan.accepted) {
    receipt.reasonCode = state.targetPositionable
                             ? state.plan.reasonCode
                             : "editor_transform_target_unavailable";
    state.lastCommit = receipt;
    return receipt;
  }

  if (state.mode == cr::CreativeSelectionPlacementMode::Copy) {
    receipt.copyReceipt = pasteClipboardWithHistory(
        appState, state.sourceClipboard, clipboardPasteRequest(state), source);
    receipt.accepted = receipt.copyReceipt.accepted;
    receipt.changed = receipt.copyReceipt.changed;
    receipt.reasonCode = receipt.copyReceipt.reasonCode;
  } else {
    const std::vector<cr::CreativeObjectId> ids = sourceObjectIds(state);
    receipt.moveReceipt =
        placeObjectsWithHistory(appState, ids, state.request, source);
    receipt.accepted = receipt.moveReceipt.accepted;
    receipt.changed = receipt.moveReceipt.changed;
    receipt.reasonCode = receipt.moveReceipt.reasonCode;
  }
  state.lastCommit = receipt;
  if (receipt.accepted && receipt.changed) {
    state.active = false;
    state.controlsOpen = false;
    state.targetPositionable = false;
  }
  return receipt;
}

CreativeEditorTransformFrameResult processCreativeEditorTransformFrame(
    const CreativeEditorTransformFrameRequest& request) {
  CreativeEditorTransformFrameResult result;
  CreativeEditorSelectionTransformState& state = request.editor.transform;
  if (!state.active) {
    return result;
  }
  const bool wasOpen = state.controlsOpen;
  if (request.routedInput.context == cr::CreativeInputContext::TransformPreview) {
    for (const cr::CreativeInputActionEvent& event :
         request.routedInput.actionEvents()) {
      if (!state.active) {
        break;
      }
      switch (event.action) {
        case cr::CreativeInputActionId::ToggleTransformControls:
          state.controlsOpen = !state.controlsOpen;
          break;
        case cr::CreativeInputActionId::TransformControlPrevious:
          if (state.controlsOpen) {
            moveControlSelection(state, -1);
          }
          break;
        case cr::CreativeInputActionId::TransformControlNext:
          if (state.controlsOpen) {
            moveControlSelection(state, 1);
          }
          break;
        case cr::CreativeInputActionId::ConfirmActiveTool:
          if (state.controlsOpen) {
            static_cast<void>(applyCreativeEditorTransformControl(
                request.appState, state,
                static_cast<CreativeEditorTransformControl>(
                    state.selectedControl)));
          } else {
            static_cast<void>(
                requestCreativeEditorSelectionTransformCommit(state));
          }
          break;
        case cr::CreativeInputActionId::CancelActiveTool:
          if (state.controlsOpen) {
            state.controlsOpen = false;
          } else {
            static_cast<void>(cancelCreativeEditorSelectionTransformPreview(
                state, "transform_preview_cancel"));
          }
          break;
        default:
          break;
      }
    }
  }

  if (state.active && state.controlsOpen) {
    selectControlDirection(state, request.directionX, request.directionY, 0.35F);
    const iggy3d::SdlWindowEventState& events = request.window.eventState();
    if (events.pointerMoved) {
      const float scaleX = events.windowWidth > 0U
                               ? static_cast<float>(request.drawableWidth) /
                                     static_cast<float>(events.windowWidth)
                               : 1.0F;
      const float scaleY = events.windowHeight > 0U
                               ? static_cast<float>(request.drawableHeight) /
                                     static_cast<float>(events.windowHeight)
                               : 1.0F;
      selectControlDirection(
          state,
          events.pointerX * scaleX -
              static_cast<float>(request.drawableWidth) * 0.5F,
          static_cast<float>(request.drawableHeight) * 0.5F -
              events.pointerY * scaleY,
          24.0F);
    }
    if (events.primaryPointerPressed) {
      static_cast<void>(applyCreativeEditorTransformControl(
          request.appState, state,
          static_cast<CreativeEditorTransformControl>(state.selectedControl)));
    }
  }

  result.openChanged = wasOpen != (state.active && state.controlsOpen);
  if (result.openChanged) {
    const bool controlsOpen = state.active && state.controlsOpen;
    static_cast<void>(request.window.setTextInputActive(false));
    static_cast<void>(request.window.setRelativeMouseMode(!controlsOpen));
    if (controlsOpen) {
      request.window.centerPointer();
    }
  }
  result.blockWorldActions = wasOpen || (state.active && state.controlsOpen) ||
                             result.openChanged;
  return result;
}

}  // namespace iggy3d_creative_app
