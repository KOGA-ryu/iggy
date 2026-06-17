#include "runtime/RuntimeGameplayAsciiSourcePlanTomlReader.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "support/CanonicalAuthoringFixtures.hpp"

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

namespace {

int Failures = 0;

struct TableSurface {
	std::string name;
	bool repeated = false;
	std::set<std::string> keys;
};

struct ObservedKey {
	std::string table;
	bool repeated = false;
	std::string key;
	std::size_t line = 0;
};

void Expect(bool condition, const std::string &message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

std::string Trim(const std::string &value)
{
	std::size_t first = 0;
	while (first < value.size() &&
		std::isspace(static_cast<unsigned char>(value[first]))) {
		++first;
	}
	std::size_t last = value.size();
	while (last > first &&
		std::isspace(static_cast<unsigned char>(value[last - 1]))) {
		--last;
	}
	return value.substr(first, last - first);
}

std::string StripComment(const std::string &line)
{
	bool inString = false;
	bool escaped = false;
	for (std::size_t index = 0; index < line.size(); ++index) {
		const char character = line[index];
		if (escaped) {
			escaped = false;
			continue;
		}
		if (inString && character == '\\') {
			escaped = true;
			continue;
		}
		if (character == '"') {
			inString = !inString;
			continue;
		}
		if (!inString && character == '#') {
			return line.substr(0, index);
		}
	}
	return line;
}

const std::vector<TableSurface> &SupportedTomlSurface()
{
	static const std::vector<TableSurface> surface {
		{ "root", false, { "format_id", "source_id", "source_ref", "version" } },
		{ "grid", false, { "background", "height", "rows", "width" } },
		{
			"no_claims",
			false,
			{
				"file_parsing",
				"gameplay_execution",
				"profile_scenario_conversion",
				"runtime_truth",
			},
		},
		{
			"promotion",
			false,
			{
				"file_parsing",
				"profile_scenario_conversion",
				"ready",
				"runtime_execution",
			},
		},
		{
			"expect",
			false,
			{
				"accepted_command_count",
				"final_rows",
				"frame_count",
				"interaction_changed",
				"npc_moved_count",
				"picked_up_count",
			},
		},
		{
			"expect_trace_frames",
			true,
			{
				"accepted_command_count",
				"frame_id",
				"interaction_changed",
				"npc_moved_count",
				"picked_up_count",
				"rows",
			},
		},
		{ "expect_inventory_stacks", true, { "count", "item_id" } },
		{ "expect_interaction_targets", true, { "enabled", "target_id" } },
		{ "expect_actor_states", true, { "actor_id", "tile" } },
		{ "expect_player_state", false, { "tile" } },
		{
			"legend",
			true,
			{
				"glyph",
				"kind",
				"maps_to_scenario_marker",
				"role_id",
				"role_tags",
				"scenario_marker_kind",
				"target_marker_id",
				"target_profile_id",
			},
		},
		{
			"cells",
			true,
			{
				"cell_bounds",
				"column",
				"glyph",
				"id",
				"local_position",
				"local_tile",
				"marker_id",
				"profile_id",
				"role_tags",
				"row",
			},
		},
		{
			"regions",
			true,
			{
				"id",
				"max_column",
				"max_row",
				"min_column",
				"min_row",
				"role_tags",
			},
		},
		{
			"profiles",
			true,
			{
				"charisma",
				"constitution",
				"dexterity",
				"id",
				"intelligence",
				"strength",
				"wisdom",
			},
		},
		{
			"interaction_targets",
			true,
			{
				"drop_id",
				"effect",
				"effect_target_id",
				"enabled",
				"enabled_value",
				"event_id",
				"kind",
				"position",
				"radius",
				"required_item_id",
				"target_id",
				"text",
				"tile",
			},
		},
		{
			"item_drops",
			true,
			{
				"count",
				"drop_id",
				"enabled",
				"glyph",
				"item_id",
				"pickup_radius",
				"position",
				"tile",
			},
		},
		{
			"frame_controls",
			true,
			{ "behavior", "frame_id", "move_mode", "npc", "target" },
		},
		{
			"frame_player_commands",
			true,
			{ "command", "frame_id", "target_id", "x", "y" },
		},
	};
	return surface;
}

std::map<std::string, TableSurface> SupportedTomlSurfaceByName()
{
	std::map<std::string, TableSurface> byName;
	for (const TableSurface &table : SupportedTomlSurface()) {
		byName.emplace(table.name, table);
	}
	return byName;
}

std::string FixturePath(const char *name)
{
	return (std::filesystem::path(IGGY_TEST_FIXTURE_DIR) / name).string();
}

std::string ReadFile(const std::string &path)
{
	std::ifstream file(path);
	std::ostringstream stream;
	stream << file.rdbuf();
	return stream.str();
}

std::vector<ObservedKey> ObserveTomlKeys(const std::string &text)
{
	std::vector<ObservedKey> observed;
	std::string table = "root";
	bool repeated = false;
	bool inStringArray = false;

	std::istringstream stream(text);
	std::string rawLine;
	std::size_t lineNumber = 0;
	while (std::getline(stream, rawLine)) {
		++lineNumber;
		const std::string line = Trim(StripComment(rawLine));
		if (line.empty()) {
			continue;
		}
		if (inStringArray) {
			if (line == "]") {
				inStringArray = false;
			}
			continue;
		}
		if (line.size() >= 4 && line.substr(0, 2) == "[[" &&
			line.substr(line.size() - 2) == "]]") {
			table = Trim(line.substr(2, line.size() - 4));
			repeated = true;
			continue;
		}
		if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
			table = Trim(line.substr(1, line.size() - 2));
			repeated = false;
			continue;
		}

		const std::size_t equals = line.find('=');
		if (equals == std::string::npos) {
			continue;
		}
		const std::string key = Trim(line.substr(0, equals));
		const std::string value = Trim(line.substr(equals + 1));
		observed.push_back({ table, repeated, key, lineNumber });
		if (value == "[") {
			inStringArray = true;
		}
	}
	return observed;
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	const std::string &table,
	const std::string &key,
	const std::string &detail)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue &issue :
		result.issues) {
		if (issue.code ==
				iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::
					UnsupportedNestedShape &&
			issue.table == table && issue.key == key &&
			issue.detail.find(detail) != std::string::npos) {
			return true;
		}
	}
	return false;
}

void TestSupportedSurfaceSnapshotHasStableOrdering()
{
	const std::vector<TableSurface> &surface = SupportedTomlSurface();
	Expect(surface.size() == 18, "supported TOML surface should list every table");

	std::set<std::string> names;
	for (const TableSurface &table : surface) {
		Expect(names.insert(table.name).second,
			"supported TOML table should be listed once: " + table.name);
		Expect(!table.keys.empty(),
			"supported TOML table should list keys: " + table.name);
		Expect(std::is_sorted(table.keys.begin(), table.keys.end()),
			"supported TOML keys should remain sorted for " + table.name);
	}
}

void TestCanonicalFixturesUseOnlySupportedSurface()
{
	const std::map<std::string, TableSurface> surface =
		SupportedTomlSurfaceByName();

	for (const iggy::test::CanonicalAuthoringFixture &fixture :
		iggy::test::CanonicalAuthoringFixtures()) {
		const std::string path = FixturePath(fixture.name);
		const std::vector<ObservedKey> observed = ObserveTomlKeys(ReadFile(path));
		for (const ObservedKey &key : observed) {
			const auto table = surface.find(key.table);
			Expect(table != surface.end(),
				std::string(fixture.name) + " should use a supported TOML table at line " +
					std::to_string(key.line) + ": " + key.table);
			if (table == surface.end()) {
				continue;
			}
			Expect(table->second.repeated == key.repeated,
				std::string(fixture.name) +
					" should use the supported table shape at line " +
					std::to_string(key.line) + ": " + key.table);
			Expect(table->second.keys.count(key.key) == 1,
				std::string(fixture.name) + " should use a supported key at line " +
					std::to_string(key.line) + ": " + key.table + "." + key.key);
		}
	}
}

void TestUnsupportedTableAndKeysAreRejected()
{
	const std::string unsupportedTable = R"toml(
format_id = "iggy:ascii-source-plan"
version = 1

[grid]
width = 1
height = 1
background = "."
rows = ["."]

[scripts]
run = "open_door"
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult tableResult =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReader {}.read(
			unsupportedTable);
	Expect(!tableResult.ok(), "unsupported TOML table should fail");
	Expect(tableResult.status ==
			iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::
				UnsupportedSyntax,
		"unsupported TOML table should report unsupported syntax");

	const std::string unsupportedRootKey = R"toml(
format_id = "iggy:ascii-source-plan"
version = 1
schema = "v1"

[grid]
width = 1
height = 1
background = "."
rows = ["."]
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult rootResult =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReader {}.read(
			unsupportedRootKey);
	Expect(!rootResult.ok(), "unsupported root key should fail");
	Expect(HasIssue(rootResult, "root", "schema", "unsupported root key"),
		"unsupported root key should include table/key diagnostics");

	const std::string unsupportedRepeatedKey = R"toml(
format_id = "iggy:ascii-source-plan"
version = 1

[grid]
width = 1
height = 1
background = "."
rows = ["."]

[[frame_player_commands]]
frame_id = "frame:one"
command = "pickup"
target_id = "drop:key"
condition = "has_key"
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult
		repeatedResult =
			iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReader {}.read(
				unsupportedRepeatedKey);
	Expect(!repeatedResult.ok(), "unsupported repeated-table key should fail");
	Expect(HasIssue(
		repeatedResult,
		"frame_player_commands",
		"condition",
		"unsupported frame_player_commands key"),
		"unsupported repeated-table key should include table/key diagnostics");
}

} // namespace

int main()
{
	TestSupportedSurfaceSnapshotHasStableOrdering();
	TestCanonicalFixturesUseOnlySupportedSurface();
	TestUnsupportedTableAndKeysAreRejected();

	if (Failures != 0) {
		std::cerr << Failures
			<< " runtime gameplay ASCII source plan TOML schema snapshot test(s) failed\n";
		return 1;
	}
	return 0;
}
