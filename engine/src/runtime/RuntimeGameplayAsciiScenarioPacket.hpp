#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayAsciiScenarioMarkerKind {
	Empty,
	Floor,
	Wall,
	Actor,
	PlayerStart,
	Unknown,
};

struct RuntimeGameplayAsciiScenarioMarkerDeclaration {
	char marker = '\0';
	RuntimeGameplayAsciiScenarioMarkerKind kind =
		RuntimeGameplayAsciiScenarioMarkerKind::Unknown;
	ResourceId actorId;
	ResourceId profileId;
	bool present = true;
};

struct RuntimeGameplayAsciiScenarioFrameDeclaration {
	bool hasFrameId = false;
	ResourceId frameId;
};

struct RuntimeGameplayAsciiScenarioPacket {
	bool hasSourceId = false;
	ResourceId sourceId;
	std::vector<std::string> rows;
	std::vector<RuntimeGameplayAsciiScenarioMarkerDeclaration> markers;
	std::vector<RuntimeGameplayAsciiScenarioFrameDeclaration> frames;

	[[nodiscard]] bool hasRows() const;
	[[nodiscard]] std::size_t rowCount() const;
	[[nodiscard]] std::size_t markerCount() const;
	[[nodiscard]] std::size_t frameCount() const;
};

} // namespace iggy::runtime
