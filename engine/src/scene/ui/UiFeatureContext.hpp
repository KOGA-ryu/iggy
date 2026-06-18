#pragma once

#include <functional>
#include <string>

#include "core/resource/ResourceId.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimeGameplayAuthoringPreviewModel.hpp"
#include "runtime/RuntimeGameplayProductPlayMode.hpp"
#include "runtime/RuntimePlayerInputInteractionEffectFrameReport.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEvent2D.hpp"

namespace iggy::ui {

struct UiFeatureContext {
	const runtime::RuntimeSessionState *session = nullptr;
	const runtime::RuntimePlayerInputInteractionEffectFrameReport *latestFrameReport = nullptr;
	const InteractionEventRecorder2D *interactionEvents = nullptr;
	const runtime::RuntimeGameplayAuthoringPreviewModel *authoringPreview = nullptr;
	const runtime::RuntimeGameplayProductPlayModeBuildResult *productPlayModeBuild = nullptr;
	const runtime::RuntimeGameplayProductPlayModeState *productPlayModeState = nullptr;
	const runtime::RuntimeGameplayProductPlayModeFrameResult *latestProductPlayModeFrame = nullptr;
	ResourceId activeToolId;
	ResourceId selectedActorId;
	ResourceId selectedTargetId;
};

struct UiShellActions {
	std::function<void(runtime::GameplayCommandFrame2D)> queueCommandFrame;
	std::function<void(ResourceId)> setActiveTool;
	std::function<void(ResourceId)> selectActor;
	std::function<void(ResourceId)> selectTarget;
	std::function<void(std::string)> setStatusText;
	std::function<void()> requestSave;
};

[[nodiscard]] bool uiFeatureContextHasSession(const UiFeatureContext &context);
[[nodiscard]] bool uiFeatureContextHasFrameReport(const UiFeatureContext &context);
[[nodiscard]] bool uiFeatureContextHasInteractionEvents(const UiFeatureContext &context);
[[nodiscard]] bool uiFeatureContextHasAuthoringPreview(const UiFeatureContext &context);
[[nodiscard]] bool uiFeatureContextHasProductPlayMode(const UiFeatureContext &context);

} // namespace iggy::ui
