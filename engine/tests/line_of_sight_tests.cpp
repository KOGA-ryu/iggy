#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "modules/line_of_sight/LineOfSight.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool Near(float actual, float expected, float tolerance = 0.0001F)
{
	return std::fabs(actual - expected) <= tolerance;
}

bool SameTile(iggy::line_of_sight::TileCoord actual, int x, int y)
{
	return actual.x == x && actual.y == y;
}

iggy::LevelTileMap MapFromRows(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map;
	map.height = static_cast<int>(rows.size());
	map.width = rows.empty() ? 0 : static_cast<int>(rows.front().size());
	for (std::string_view row : rows) {
		for (char cell : row)
			map.tiles.push_back({ cell != '#' });
	}
	return map;
}

void TestClearHorizontalAndVerticalLines()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});

	Expect(iggy::line_of_sight::CanSee(map, { 0.5F, 1.5F }, { 4.5F, 1.5F }), "clear horizontal line should be visible");
	Expect(iggy::line_of_sight::CanSee(map, { 2.5F, 0.5F }, { 2.5F, 2.5F }), "clear vertical line should be visible");
}

void TestBlockedLineReportsHitTile()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		"..#..",
		".....",
	});
	const iggy::line_of_sight::LineOfSightTrace trace = iggy::line_of_sight::Trace(map, { 0.5F, 1.5F }, { 4.5F, 1.5F });

	Expect(!trace.visible(), "blocking tile should prevent visibility");
	Expect(trace.blocked(), "blocking tile should set blocked state");
	Expect(trace.status == iggy::line_of_sight::LineOfSightStatus::Blocked, "blocking tile should report blocked status");
	Expect(SameTile(trace.hitTile, 2, 1), "blocked line should report blocking tile");
	Expect(Near(trace.distance, 1.5F), "blocked line should report distance to blocking tile");
}

void TestSameTileVisibility()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
		"...",
	});
	const iggy::line_of_sight::LineOfSightTrace trace = iggy::line_of_sight::Trace(map, { 1.2F, 1.2F }, { 1.8F, 1.7F });

	Expect(trace.visible(), "points in same valid walkable tile should be visible");
	Expect(SameTile(trace.startTile, 1, 1), "same-tile trace should report start tile");
	Expect(SameTile(trace.endTile, 1, 1), "same-tile trace should report end tile");
}

void TestOutOfBoundsEndpoint()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
		"...",
	});
	const iggy::line_of_sight::LineOfSightTrace trace = iggy::line_of_sight::Trace(map, { 1.5F, 1.5F }, { 3.1F, 1.5F });

	Expect(!trace.visible(), "out-of-bounds endpoint should not be visible");
	Expect(!trace.valid(), "out-of-bounds endpoint should report invalid trace");
	Expect(trace.status == iggy::line_of_sight::LineOfSightStatus::EndOutOfBounds, "out-of-bounds endpoint should report end status");
}

void TestDiagonalCrossesBlockingTile()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		".#.",
		"...",
	});
	const iggy::line_of_sight::LineOfSightTrace trace = iggy::line_of_sight::Trace(map, { 0.5F, 0.5F }, { 2.5F, 2.5F });

	Expect(trace.status == iggy::line_of_sight::LineOfSightStatus::Blocked, "diagonal crossing blocker should be blocked");
	Expect(SameTile(trace.hitTile, 1, 1), "diagonal crossing blocker should report center tile");
	Expect(Near(trace.distance, std::sqrt(0.5F)), "diagonal crossing blocker should report deterministic entry distance");
}

} // namespace

int main()
{
	TestClearHorizontalAndVerticalLines();
	TestBlockedLineReportsHitTile();
	TestSameTileVisibility();
	TestOutOfBoundsEndpoint();
	TestDiagonalCrossesBlockingTile();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
