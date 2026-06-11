#pragma once

namespace dev {

class GameLoop {
public:
	int run();

private:
	void pollInput();
	void processCommands();
	void updateSimulation();
	void renderDebugView();
};

} // namespace dev

