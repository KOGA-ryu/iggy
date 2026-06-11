#include "CombatRegistry.hpp"

#include <algorithm>

namespace dev {

void CombatRegistry::add(Combatant combatant)
{
	combatants_.push_back(combatant);
}

Combatant *CombatRegistry::find(const Target &target)
{
	for (Combatant &combatant : combatants_) {
		if (combatant.target.type == target.type && combatant.target.id == target.id)
			return &combatant;
	}
	return nullptr;
}

const Combatant *CombatRegistry::find(const Target &target) const
{
	for (const Combatant &combatant : combatants_) {
		if (combatant.target.type == target.type && combatant.target.id == target.id)
			return &combatant;
	}
	return nullptr;
}

bool CombatRegistry::remove(const Target &target)
{
	const auto before = combatants_.size();
	combatants_.erase(
	    std::remove_if(combatants_.begin(), combatants_.end(), [&target](const Combatant &combatant) {
		    return combatant.target.type == target.type && combatant.target.id == target.id;
	    }),
	    combatants_.end());
	return combatants_.size() != before;
}

void CombatRegistry::replaceAll(std::vector<Combatant> combatants)
{
	combatants_ = std::move(combatants);
}

const std::vector<Combatant> &CombatRegistry::combatants() const
{
	return combatants_;
}

} // namespace dev
