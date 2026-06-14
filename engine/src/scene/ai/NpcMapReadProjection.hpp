#pragma once

#include "scene/ai/NpcMapRead.hpp"
#include "scene/ai/NpcRead.hpp"

namespace iggy {

class NpcMapReadProjection {
public:
	[[nodiscard]] NpcRead toRead(const NpcMapRead &mapRead) const;
};

} // namespace iggy
