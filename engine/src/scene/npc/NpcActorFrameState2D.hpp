#pragma once

#include <cstddef>
#include <vector>

#include "scene/npc/NpcActorControlState2D.hpp"
#include "scene/npc/NpcActorState2D.hpp"

namespace iggy {

struct NpcActorFrameState2D {
	NpcActorState2D actor;
	NpcActorControlState2D control;
	bool hasControl = false;
};

enum class NpcActorFrameState2DIssueCode {
	MissingControlState,
	OrphanControlState,
};

struct NpcActorFrameState2DIssue {
	NpcActorFrameState2DIssueCode code = NpcActorFrameState2DIssueCode::MissingControlState;
	std::size_t actorIndex = 0;
	std::size_t controlIndex = 0;
	NpcActorState2D actor;
	NpcActorControlState2D control;
};

struct NpcActorFrameState2DProjectionResult {
	std::vector<NpcActorFrameState2D> entries;
	std::vector<NpcActorFrameState2DIssue> issues;

	[[nodiscard]] bool hasIssues() const;
};

class NpcActorFrameStateProjector2D {
public:
	[[nodiscard]] NpcActorFrameState2DProjectionResult project(
		const NpcActorState2DRegistry &actors,
		const NpcActorControlState2DRegistry &controls) const;
};

} // namespace iggy
