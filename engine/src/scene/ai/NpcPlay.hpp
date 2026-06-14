#pragma once

#include "scene/ai/NpcRead.hpp"

namespace iggy {

enum class NpcPlayStatus {
	Played,
	NoPlayableEnt,
};

struct NpcPlay {
	NpcPlayStatus status = NpcPlayStatus::NoPlayableEnt;
	NpcRead read;
	NpcReadEnt selected;

	[[nodiscard]] bool hasPlay() const;
};

class NpcPlaySelector {
public:
	[[nodiscard]] NpcPlay play(const NpcRead &read) const;
};

} // namespace iggy
