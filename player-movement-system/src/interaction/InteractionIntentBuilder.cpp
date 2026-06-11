#include "InteractionIntentBuilder.hpp"

namespace dev {

InteractionIntent InteractionIntentBuilder::build(Target target, bool standGround) const
{
	switch (target.type) {
	case TargetType::EmptyTile:
		return { standGround ? InteractionIntentType::Attack : InteractionIntentType::Move, target };
	case TargetType::Item:
		return { InteractionIntentType::Pickup, target };
	case TargetType::Enemy:
	case TargetType::Player:
		return { InteractionIntentType::Attack, target };
	case TargetType::Npc:
		return { InteractionIntentType::Talk, target };
	case TargetType::Object:
		return { InteractionIntentType::Interact, target };
	}
	return { InteractionIntentType::Move, target };
}

} // namespace dev
