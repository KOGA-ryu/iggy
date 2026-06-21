#pragma once

#include <cstdint>
#include <string>

#include "runtime3d/Runtime3DEntityId.hpp"

namespace iggy::runtime3d {

enum class Runtime3DCommandKind {
	Move,
	Interact,
	Inspect,
	Wait,
	ToggleTacticalMode,
	StepTacticalTick,
	Retry,
	Reset,
	Save,
	Load,
};

enum class Runtime3DCommandAdmissionStatus {
	Pending,
	Accepted,
	Rejected,
};

struct Runtime3DCommandRecord {
	std::uint64_t commandId = 0;
	std::uint64_t sequence = 0;
	std::uint32_t playerSlot = 0;
	Runtime3DEntityId actorId;
	Runtime3DCommandKind kind = Runtime3DCommandKind::Wait;
	bool hasTarget = false;
	Runtime3DEntityId targetId;
	std::uint64_t issuedTick = 0;
	Runtime3DCommandAdmissionStatus admissionStatus = Runtime3DCommandAdmissionStatus::Pending;
	std::string admissionReason;
};

[[nodiscard]] const char *Runtime3DCommandKindName(Runtime3DCommandKind kind);

} // namespace iggy::runtime3d
