#include "runtime3d/Runtime3DCommand.hpp"

namespace iggy::runtime3d {

const char *Runtime3DCommandKindName(Runtime3DCommandKind kind)
{
	switch (kind) {
	case Runtime3DCommandKind::Move:
		return "move";
	case Runtime3DCommandKind::Interact:
		return "interact";
	case Runtime3DCommandKind::Inspect:
		return "inspect";
	case Runtime3DCommandKind::Wait:
		return "wait";
	case Runtime3DCommandKind::ToggleTacticalMode:
		return "toggle_tactical_mode";
	case Runtime3DCommandKind::StepTacticalTick:
		return "step_tactical_tick";
	case Runtime3DCommandKind::Retry:
		return "retry";
	case Runtime3DCommandKind::Reset:
		return "reset";
	case Runtime3DCommandKind::Save:
		return "save";
	case Runtime3DCommandKind::Load:
		return "load";
	}

	return "unknown";
}

} // namespace iggy::runtime3d
