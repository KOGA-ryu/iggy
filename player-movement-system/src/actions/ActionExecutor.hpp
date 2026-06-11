#pragma once

#include "actions/ActionResult.hpp"
#include "actions/ActionRules.hpp"
#include "events/MovementEventSink.hpp"
#include "player/Player.hpp"

namespace dev {

class ActionExecutor {
public:
	explicit ActionExecutor(ActionRules rules = {}, MovementEventSink *eventSink = nullptr);

	ActionResult update(Player &player) const;

private:
	void applyAnimationCommitment(Player &player, const DestinationAction &action) const;

	ActionRules rules_;
	MovementEventSink *eventSink_;
};

} // namespace dev
