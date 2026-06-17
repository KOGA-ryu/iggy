#include "runtime/RuntimeGameplayAsciiSourcePlanTomlFileReader.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "support/CanonicalAuthoringFixtures.hpp"

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

namespace {

int Failures = 0;

struct CanonicalFixtureBudget {
	std::size_t maxFileBytes = 8192;
	std::size_t maxRows = 8;
	std::size_t maxColumns = 16;
	std::size_t maxGridCells = 128;
	std::size_t maxLegendEntries = 16;
	std::size_t maxAnnotatedCells = 16;
	std::size_t maxRegions = 8;
	std::size_t maxProfiles = 8;
	std::size_t maxAuthoredFrameIds = 8;
	std::size_t maxAuthoredControls = 8;
	std::size_t maxAuthoredPlayerCommands = 8;
	std::size_t maxInteractionTargets = 8;
	std::size_t maxItemDrops = 8;
	std::size_t maxExpectationTraceFrames = 8;
	std::size_t maxExpectationRows = 48;
};

struct FixtureScale {
	std::string name;
	std::size_t fileBytes = 0;
	std::size_t rows = 0;
	std::size_t columns = 0;
	std::size_t gridCells = 0;
	std::size_t legendEntries = 0;
	std::size_t annotatedCells = 0;
	std::size_t regions = 0;
	std::size_t profiles = 0;
	std::size_t authoredFrameIds = 0;
	std::size_t authoredControls = 0;
	std::size_t authoredPlayerCommands = 0;
	std::size_t interactionTargets = 0;
	std::size_t itemDrops = 0;
	std::size_t expectationTraceFrames = 0;
	std::size_t expectationRows = 0;
};

void Expect(bool condition, const std::string &message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

std::filesystem::path FixturePath(const char *name)
{
	return std::filesystem::path(IGGY_TEST_FIXTURE_DIR) / name;
}

std::string CountMessage(
	const FixtureScale &scale,
	const char *field,
	std::size_t actual,
	std::size_t limit)
{
	std::ostringstream stream;
	stream << scale.name << " exceeds " << field << " budget: " << actual
		   << " > " << limit;
	return stream.str();
}

std::vector<std::string> AuthoredFrameIds(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan &plan)
{
	std::vector<std::string> ids;
	const auto pushUnique = [&ids](const iggy::ResourceId &id) {
		const std::string value(id.value());
		for (const std::string &existing : ids) {
			if (existing == value) {
				return;
			}
		}
		ids.push_back(value);
	};

	for (const auto &control : plan.authoredControls) {
		if (control.hasFrameId) {
			pushUnique(control.frameId);
		}
	}
	for (const auto &command : plan.authoredPlayerCommands) {
		if (command.hasFrameId) {
			pushUnique(command.frameId);
		}
	}
	return ids;
}

std::size_t ExpectedRowCount(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanExpectations &expectations)
{
	std::size_t rows = expectations.finalRows.size();
	for (const auto &traceFrame : expectations.traceFrames) {
		rows += traceFrame.rows.size();
	}
	return rows;
}

FixtureScale MeasureFixture(const iggy::test::CanonicalAuthoringFixture &fixture)
{
	const std::filesystem::path path = FixturePath(fixture.name);
	const auto readResult =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);

	Expect(readResult.ok(), std::string("canonical fixture should parse: ") +
		fixture.name);

	FixtureScale scale;
	scale.name = fixture.name;
	scale.fileBytes = readResult.bytesRead;
	if (!readResult.ok()) {
		return scale;
	}

	const iggy::runtime::RuntimeGameplayAsciiSourcePlan &plan =
		readResult.text.plan;
	scale.rows = plan.grid.height;
	scale.columns = plan.grid.width;
	scale.gridCells = plan.grid.width * plan.grid.height;
	scale.legendEntries = plan.legendCount();
	scale.annotatedCells = plan.annotatedCellCount();
	scale.regions = plan.regionCount();
	scale.profiles = plan.authoredProfileCount();
	scale.authoredFrameIds = AuthoredFrameIds(plan).size();
	scale.authoredControls = plan.authoredControlCount();
	scale.authoredPlayerCommands = plan.authoredPlayerCommandCount();
	scale.interactionTargets = plan.authoredInteractionTargetCount();
	scale.itemDrops = plan.authoredItemDropCount();
	scale.expectationTraceFrames = plan.expectations.traceFrames.size();
	scale.expectationRows = ExpectedRowCount(plan.expectations);
	return scale;
}

void ExpectWithinBudget(
	const FixtureScale &scale,
	const CanonicalFixtureBudget &budget)
{
	Expect(
		scale.fileBytes <= budget.maxFileBytes,
		CountMessage(scale, "file byte", scale.fileBytes, budget.maxFileBytes));
	Expect(
		scale.rows <= budget.maxRows,
		CountMessage(scale, "row", scale.rows, budget.maxRows));
	Expect(
		scale.columns <= budget.maxColumns,
		CountMessage(scale, "column", scale.columns, budget.maxColumns));
	Expect(
		scale.gridCells <= budget.maxGridCells,
		CountMessage(scale, "grid cell", scale.gridCells, budget.maxGridCells));
	Expect(
		scale.legendEntries <= budget.maxLegendEntries,
		CountMessage(
			scale,
			"legend entry",
			scale.legendEntries,
			budget.maxLegendEntries));
	Expect(
		scale.annotatedCells <= budget.maxAnnotatedCells,
		CountMessage(
			scale,
			"annotated cell",
			scale.annotatedCells,
			budget.maxAnnotatedCells));
	Expect(
		scale.regions <= budget.maxRegions,
		CountMessage(scale, "region", scale.regions, budget.maxRegions));
	Expect(
		scale.profiles <= budget.maxProfiles,
		CountMessage(scale, "profile", scale.profiles, budget.maxProfiles));
	Expect(
		scale.authoredFrameIds <= budget.maxAuthoredFrameIds,
		CountMessage(
			scale,
			"authored frame id",
			scale.authoredFrameIds,
			budget.maxAuthoredFrameIds));
	Expect(
		scale.authoredControls <= budget.maxAuthoredControls,
		CountMessage(
			scale,
			"authored control",
			scale.authoredControls,
			budget.maxAuthoredControls));
	Expect(
		scale.authoredPlayerCommands <= budget.maxAuthoredPlayerCommands,
		CountMessage(
			scale,
			"authored player command",
			scale.authoredPlayerCommands,
			budget.maxAuthoredPlayerCommands));
	Expect(
		scale.interactionTargets <= budget.maxInteractionTargets,
		CountMessage(
			scale,
			"interaction target",
			scale.interactionTargets,
			budget.maxInteractionTargets));
	Expect(
		scale.itemDrops <= budget.maxItemDrops,
		CountMessage(scale, "item drop", scale.itemDrops, budget.maxItemDrops));
	Expect(
		scale.expectationTraceFrames <= budget.maxExpectationTraceFrames,
		CountMessage(
			scale,
			"expectation trace frame",
			scale.expectationTraceFrames,
			budget.maxExpectationTraceFrames));
	Expect(
		scale.expectationRows <= budget.maxExpectationRows,
		CountMessage(
			scale,
			"expectation row",
			scale.expectationRows,
			budget.maxExpectationRows));
}

void TestCanonicalFixturesStayWithinAuthoringV1Budget()
{
	const CanonicalFixtureBudget budget;
	std::size_t measured = 0;

	for (const iggy::test::CanonicalAuthoringFixture &fixture :
		iggy::test::CanonicalAuthoringFixtures()) {
		if (fixture.expectedResult !=
			iggy::test::CanonicalAuthoringFixtureExpectedResult::Success) {
			continue;
		}

		const FixtureScale scale = MeasureFixture(fixture);
		ExpectWithinBudget(scale, budget);
		++measured;
	}

	Expect(measured == 11, "budget test should cover current canonical fixtures");
}

} // namespace

int main()
{
	TestCanonicalFixturesStayWithinAuthoringV1Budget();
	return Failures == 0 ? 0 : 1;
}
