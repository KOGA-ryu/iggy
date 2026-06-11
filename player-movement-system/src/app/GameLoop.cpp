#include "GameLoop.hpp"

namespace dev {

int GameLoop::run()
{
	while (false) {
		pollInput();
		processCommands();
		updateSimulation();
		renderDebugView();
	}
	return 0;
}

void GameLoop::pollInput() {}
void GameLoop::processCommands() {}
void GameLoop::updateSimulation() {}
void GameLoop::renderDebugView() {}

} // namespace dev

