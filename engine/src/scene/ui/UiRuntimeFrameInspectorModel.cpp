#include "scene/ui/UiRuntimeFrameInspectorModel.hpp"

#include <string>

namespace iggy::ui {
namespace {

std::string BoolText(bool value)
{
	return value ? "true" : "false";
}

void AddRow(UiRuntimeFrameInspectorModel &model, std::string key, std::string value)
{
	model.rows.push_back({ std::move(key), std::move(value) });
}

} // namespace

std::string uiRuntimeInteractionEffectFrameEventLabel(runtime::RuntimePlayerInputInteractionEffectFrameEvent event)
{
	switch (event) {
	case runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerCommandAccepted:
		return "player_command_accepted";
	case runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerIntentBlocked:
		return "player_intent_blocked";
	case runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerIntentRejected:
		return "player_intent_rejected";
	case runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued:
		return "command_frame_queued";
	case runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan:
		return "command_runner_ran";
	case runtime::RuntimePlayerInputInteractionEffectFrameEvent::InteractionReadyWithEffects:
		return "interaction_ready_with_effects";
	case runtime::RuntimePlayerInputInteractionEffectFrameEvent::InteractionReadyWithoutEffects:
		return "interaction_ready_without_effects";
	case runtime::RuntimePlayerInputInteractionEffectFrameEvent::InteractionBlocked:
		return "interaction_blocked";
	case runtime::RuntimePlayerInputInteractionEffectFrameEvent::EffectsRequested:
		return "effects_requested";
	}
	return "unknown";
}

UiRuntimeFrameInspectorModel buildUiRuntimeFrameInspectorModel(
	const runtime::RuntimeSessionState *session,
	const runtime::RuntimePlayerInputInteractionEffectFrameReport *report)
{
	UiRuntimeFrameInspectorModel model;
	model.hasSession = session != nullptr;
	if (session != nullptr) {
		model.tickIndex = session->tickIndex;
		model.hasPlayer = session->hasPlayer;
		model.hasRenderCache = session->hasRenderCache;
	}
	if (report != nullptr) {
		model.acceptedCommandCount = report->acceptedCommandCount;
		model.blockedIntentCount = report->blockedIntentCount;
		model.rejectedIntentCount = report->rejectedIntentCount;
		model.readyInteractionCount = report->readyInteractionCount;
		model.noEffectInteractionCount = report->noEffectInteractionCount;
		model.blockedInteractionCount = report->blockedInteractionCount;
		model.requestedEffectCount = report->requestedEffectCount;
	}

	AddRow(model, "has_session", BoolText(model.hasSession));
	AddRow(model, "tick_index", std::to_string(model.tickIndex));
	AddRow(model, "has_player", BoolText(model.hasPlayer));
	AddRow(model, "has_render_cache", BoolText(model.hasRenderCache));
	AddRow(model, "accepted_commands", std::to_string(model.acceptedCommandCount));
	AddRow(model, "blocked_intents", std::to_string(model.blockedIntentCount));
	AddRow(model, "rejected_intents", std::to_string(model.rejectedIntentCount));
	AddRow(model, "ready_interactions", std::to_string(model.readyInteractionCount));
	AddRow(model, "no_effect_interactions", std::to_string(model.noEffectInteractionCount));
	AddRow(model, "blocked_interactions", std::to_string(model.blockedInteractionCount));
	AddRow(model, "requested_effects", std::to_string(model.requestedEffectCount));
	return model;
}

} // namespace iggy::ui
