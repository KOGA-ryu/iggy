#include "scene/player/PlayerAgentState.hpp"

namespace iggy {

TileCoord playerTile(const PlayerAgentState &state)
{
	return tileForPoint(state.position);
}

} // namespace iggy
