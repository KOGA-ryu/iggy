#include "runtime/RuntimeAutosaveRotationPolicy.hpp"

#include <string>

namespace iggy::runtime {
namespace {

[[nodiscard]] RuntimeSaveSlotId autosaveSlot(std::size_t index)
{
	return { RuntimeSaveSlotKind::Auto, "autosave_" + std::to_string(index) };
}

} // namespace

RuntimeAutosaveRotationResult RuntimeAutosaveRotationPolicy::plan(std::size_t maxSlots) const
{
	RuntimeAutosaveRotationResult result;
	if (maxSlots == 0) {
		result.issues.push_back({ RuntimeAutosaveRotationIssueCode::InvalidMaxSlots, maxSlots });
		return result;
	}

	result.plan.valid = true;
	result.plan.writeSlot = autosaveSlot(0);
	if (maxSlots == 1)
		return result;

	result.plan.deleteSlots.push_back(autosaveSlot(maxSlots - 1));
	for (std::size_t index = maxSlots - 1; index > 0; --index)
		result.plan.renames.push_back({ autosaveSlot(index - 1), autosaveSlot(index) });

	return result;
}

} // namespace iggy::runtime
