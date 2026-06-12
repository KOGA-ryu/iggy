#pragma once

#include <filesystem>
#include <string>

namespace iggy::runtime {

enum class RuntimeSaveSlotKind {
	Manual,
	Auto,
	Quick,
};

struct RuntimeSaveSlotId {
	RuntimeSaveSlotKind kind = RuntimeSaveSlotKind::Manual;
	std::string name;
};

enum class RuntimeSaveSlotPathStatus {
	Valid,
	EmptyBaseDirectory,
	EmptySlotName,
	InvalidSlotName,
};

struct RuntimeSaveSlotPathResult {
	RuntimeSaveSlotPathStatus status = RuntimeSaveSlotPathStatus::Valid;
	std::filesystem::path path;
	RuntimeSaveSlotId slot;
};

struct RuntimeSaveSlotPathPolicyConfig {
	std::filesystem::path baseDirectory;
	std::string extension = ".igsave";
};

class RuntimeSaveSlotPathPolicy {
public:
	[[nodiscard]] RuntimeSaveSlotPathResult pathFor(
		const RuntimeSaveSlotPathPolicyConfig &config,
		RuntimeSaveSlotId slot) const;
};

} // namespace iggy::runtime
