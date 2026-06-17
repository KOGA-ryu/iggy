#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy::runtime {

struct RuntimeGameplayTomlScenarioFacadeResult;

enum class RuntimeGameplayAuthoringDiagnosticLayer {
	File,
	Toml,
	SourcePlan,
	Adapter,
	Conversion,
	Profile,
	Scenario,
	Run,
};

struct RuntimeGameplayAuthoringDiagnosticEntry {
	RuntimeGameplayAuthoringDiagnosticLayer layer =
		RuntimeGameplayAuthoringDiagnosticLayer::File;
	std::string code;
	std::string table;
	std::string key;
	bool hasTableIndex = false;
	std::size_t tableIndex = 0;
	bool hasLine = false;
	std::size_t line = 0;
	bool hasColumn = false;
	std::size_t column = 0;
	bool hasIndex = false;
	std::size_t index = 0;
	bool hasRow = false;
	std::size_t row = 0;
	bool hasActorIndex = false;
	std::size_t actorIndex = 0;
	bool hasControlIndex = false;
	std::size_t controlIndex = 0;
	bool hasSubjectIndex = false;
	std::size_t subjectIndex = 0;
	char glyph = '\0';
	ResourceId id;
	std::string detail;
};

[[nodiscard]] const char *runtimeGameplayAuthoringDiagnosticLayerText(
	RuntimeGameplayAuthoringDiagnosticLayer layer);

[[nodiscard]] std::vector<RuntimeGameplayAuthoringDiagnosticEntry>
projectRuntimeGameplayAuthoringDiagnostics(
	const RuntimeGameplayTomlScenarioFacadeResult &result);

} // namespace iggy::runtime
