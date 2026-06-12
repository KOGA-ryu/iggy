#include "runtime/RuntimeSaveSlotPathPolicy.hpp"

namespace iggy::runtime {
namespace {

[[nodiscard]] bool isAllowedSlotNameCharacter(char value)
{
	return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') || (value >= '0' && value <= '9') || value == '_' || value == '-';
}

[[nodiscard]] bool isValidSlotName(const std::string &name)
{
	if (name.empty())
		return false;

	for (char value : name) {
		if (!isAllowedSlotNameCharacter(value))
			return false;
	}
	return true;
}

[[nodiscard]] const char *directoryName(RuntimeSaveSlotKind kind)
{
	switch (kind) {
	case RuntimeSaveSlotKind::Manual:
		return "manual";
	case RuntimeSaveSlotKind::Auto:
		return "auto";
	case RuntimeSaveSlotKind::Quick:
		return "quick";
	}
	return "manual";
}

} // namespace

RuntimeSaveSlotPathResult RuntimeSaveSlotPathPolicy::pathFor(
	const RuntimeSaveSlotPathPolicyConfig &config,
	RuntimeSaveSlotId slot) const
{
	RuntimeSaveSlotPathResult result;
	result.slot = slot;

	if (config.baseDirectory.empty()) {
		result.status = RuntimeSaveSlotPathStatus::EmptyBaseDirectory;
		return result;
	}

	if (slot.name.empty()) {
		result.status = RuntimeSaveSlotPathStatus::EmptySlotName;
		return result;
	}

	if (!isValidSlotName(slot.name)) {
		result.status = RuntimeSaveSlotPathStatus::InvalidSlotName;
		return result;
	}

	result.status = RuntimeSaveSlotPathStatus::Valid;
	result.path = config.baseDirectory / directoryName(slot.kind) / (slot.name + config.extension);
	return result;
}

} // namespace iggy::runtime
