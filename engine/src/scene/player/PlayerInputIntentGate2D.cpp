#include "scene/player/PlayerInputIntentGate2D.hpp"

namespace iggy {
namespace {

bool RequiresWorldInput(PlayerInputIntent2DType type)
{
	return type == PlayerInputIntent2DType::MoveToPoint
		|| type == PlayerInputIntent2DType::MoveToTile
		|| type == PlayerInputIntent2DType::Interact
		|| type == PlayerInputIntent2DType::Inspect;
}

bool RequiresInteraction(PlayerInputIntent2DType type)
{
	return type == PlayerInputIntent2DType::Interact
		|| type == PlayerInputIntent2DType::Inspect;
}

PlayerInputIntentGate2DResult Blocked(
	PlayerInputIntent2D intent,
	PlayerInputIntent2DStatus intentStatus,
	PlayerInputIntentBlockReason reason)
{
	PlayerInputIntentGate2DResult result;
	result.accepted = false;
	result.reason = reason;
	result.intentStatus = intentStatus;
	result.intent = intent;
	return result;
}

} // namespace

PlayerInputIntentGate2DResult PlayerInputIntentGate2D::evaluate(const PlayerInputContext2D &context, PlayerInputIntent2D intent) const
{
	const PlayerInputIntent2DStatus intentStatus = validate(intent);
	if (intentStatus != PlayerInputIntent2DStatus::Valid)
		return Blocked(intent, intentStatus, PlayerInputIntentBlockReason::InvalidIntent);

	if (!context.playerControlEnabled)
		return Blocked(intent, intentStatus, PlayerInputIntentBlockReason::PlayerControlDisabled);

	if (!context.worldInputEnabled && RequiresWorldInput(intent.type))
		return Blocked(intent, intentStatus, PlayerInputIntentBlockReason::WorldInputDisabled);

	if (!context.interactionEnabled && RequiresInteraction(intent.type))
		return Blocked(intent, intentStatus, PlayerInputIntentBlockReason::InteractionDisabled);

	if (!context.cancelEnabled && intent.type == PlayerInputIntent2DType::Cancel)
		return Blocked(intent, intentStatus, PlayerInputIntentBlockReason::CancelDisabled);

	PlayerInputIntentGate2DResult result;
	result.accepted = true;
	result.reason = PlayerInputIntentBlockReason::None;
	result.intentStatus = intentStatus;
	result.intent = intent;
	return result;
}

} // namespace iggy
