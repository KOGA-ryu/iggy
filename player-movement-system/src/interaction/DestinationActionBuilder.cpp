#include "DestinationActionBuilder.hpp"

namespace dev {

DestinationAction DestinationActionBuilder::build(const InteractionIntent &intent) const
{
	switch (intent.type) {
	case InteractionIntentType::Attack:
		return { DestinationActionType::Attack, intent.target, 1 };
	case InteractionIntentType::Pickup:
		return { DestinationActionType::Pickup, intent.target, 0 };
	case InteractionIntentType::Talk:
		return { DestinationActionType::Talk, intent.target, 1 };
	case InteractionIntentType::Interact:
		return { DestinationActionType::Interact, intent.target, 1 };
	case InteractionIntentType::Move:
		return {};
	}
	return {};
}

} // namespace dev
