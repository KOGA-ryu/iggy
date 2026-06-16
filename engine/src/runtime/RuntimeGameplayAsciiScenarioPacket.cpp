#include "runtime/RuntimeGameplayAsciiScenarioPacket.hpp"

namespace iggy::runtime {

bool RuntimeGameplayAsciiScenarioPacket::hasRows() const
{
	return !rows.empty();
}

std::size_t RuntimeGameplayAsciiScenarioPacket::rowCount() const
{
	return rows.size();
}

std::size_t RuntimeGameplayAsciiScenarioPacket::markerCount() const
{
	return markers.size();
}

std::size_t RuntimeGameplayAsciiScenarioPacket::frameCount() const
{
	return frames.size();
}

} // namespace iggy::runtime
