#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "EditorObjectActionOutcome.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/SelectionTransformCommands.hpp"

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorWorldLayoutState;

struct CreativeEditorDeleteSelectionAction {};

struct CreativeEditorDuplicateSelectionAction {
  iggy3d::creative::CreativeDuplicateCommandRequest request;
};

struct CreativeEditorTransformSelectionAction {
  iggy3d::creative::CreativeTransformCommandRequest request;
};

struct CreativeEditorSetObjectTransformAction {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeTransform transform;
  bool setPosition = false;
  bool setRotation = false;
  bool setScale = false;
};

using CreativeEditorSceneObjectAction =
    std::variant<CreativeEditorDeleteSelectionAction,
                 CreativeEditorDuplicateSelectionAction,
                 CreativeEditorTransformSelectionAction,
                 CreativeEditorSetObjectTransformAction>;

struct CreativeEditorSceneObjectActionRequest {
  CreativeEditorSceneObjectAction action;
  std::string_view historySource;
};

enum class CreativeEditorObjectActionIntegrationImpact : std::uint16_t {
  None = 0U,
  SelectionSynchronized = 1U << 0U,
  WorldLayoutSourceChanged = 1U << 1U,
  WorldLayoutSourceDeleted = 1U << 2U,
  WorldLayoutSourceDuplicated = 1U << 3U,
  WorldLayoutAdoptionRequired = 1U << 4U,
  SceneRefreshRequired = 1U << 5U,
};

using CreativeEditorObjectActionIntegrationImpactFlags = std::uint16_t;

[[nodiscard]] constexpr CreativeEditorObjectActionIntegrationImpactFlags
creativeEditorObjectActionIntegrationImpactFlag(
    CreativeEditorObjectActionIntegrationImpact impact) noexcept {
  return static_cast<CreativeEditorObjectActionIntegrationImpactFlags>(impact);
}

struct CreativeEditorObjectActionExecution {
  CreativeEditorObjectActionOutcome outcome;
  CreativeEditorObjectActionIntegrationImpactFlags impacts = 0U;
  bool selectionSynchronizationAttempted = false;
  bool selectionSynchronizationAccepted = false;
  bool selectionSynchronizationChanged = false;
  std::string status;
};

[[nodiscard]] constexpr bool creativeEditorObjectActionHasIntegrationImpact(
    const CreativeEditorObjectActionExecution& execution,
    CreativeEditorObjectActionIntegrationImpact impact) noexcept {
  return (execution.impacts &
          creativeEditorObjectActionIntegrationImpactFlag(impact)) != 0U;
}

struct CreativeEditorObjectActionExecutionContext {
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorWorldLayoutState* worldLayout = nullptr;
};

[[nodiscard]] CreativeEditorObjectActionExecution
executeCreativeEditorSceneObjectAction(
    const CreativeEditorObjectActionExecutionContext& context,
    const CreativeEditorSceneObjectActionRequest& request);

}  // namespace iggy3d_creative_app
