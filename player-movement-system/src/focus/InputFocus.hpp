#pragma once

namespace dev {

enum class InputOwner {
	Gameplay,
	Inventory,
	Menu,
	Dialogue,
	Cutscene,
};

struct FocusState {
	InputOwner owner = InputOwner::Gameplay;
	bool textEntryActive = false;
};

class InputFocus {
public:
	explicit InputFocus(const FocusState &state);

	bool gameplayOwnsMovement() const;
	bool gameplayOwnsActions() const;

private:
	const FocusState &state_;
};

} // namespace dev

