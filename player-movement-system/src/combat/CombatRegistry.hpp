#pragma once

#include <optional>
#include <vector>

#include "combat/Combatant.hpp"

namespace dev {

class CombatRegistry {
public:
	void add(Combatant combatant);
	Combatant *find(const Target &target);
	const Combatant *find(const Target &target) const;
	void replaceAll(std::vector<Combatant> combatants);

	[[nodiscard]] const std::vector<Combatant> &combatants() const;

private:
	std::vector<Combatant> combatants_;
};

} // namespace dev
