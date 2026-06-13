#include <cstdlib>
#include <string_view>
#include <vector>

#include "scene/draft/DraftBuildingCompile2D.hpp"
#include "scene/draft/DraftLevelGeometryPlan2D.hpp"
#include "scene/level/LevelCombinedDerivedCacheBuilder2D.hpp"
#include "scene/level/LevelCollisionSource2D.hpp"
#include "scene/level/LevelGridQuery.hpp"
#include "servers/physics2d/CollisionOverlap2D.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::DraftSymbol2D Symbol(
	const char *id,
	iggy::DraftSymbol2DKind kind,
	iggy::Vec2 position = { 0.0F, 0.0F },
	iggy::Vec2 size = { 1.0F, 1.0F },
	float rotationRadians = 0.0F,
	const char *assetId = "asset:symbol",
	const char *definitionId = "definition:symbol")
{
	return {
		Id(id),
		kind,
		position,
		size,
		rotationRadians,
		Id(assetId),
		Id(definitionId),
		true,
	};
}

iggy::DraftDocument2D Document(std::vector<iggy::DraftSymbol2D> symbols)
{
	return { symbols };
}

iggy::LevelRuntimeState RuntimeLevel(std::initializer_list<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(std::vector<std::string_view>(rows));
	return level;
}

iggy::LevelCombinedDerivedCacheBuildConfig2D CollisionConfig()
{
	iggy::LevelCombinedDerivedCacheBuildConfig2D config;
	config.buildCollisionCache = true;
	return config;
}

struct PipelineResult {
	iggy::DraftBuildingCompile2DResult building;
	iggy::DraftLevelGeometryPlan2DResult geometry;
	iggy::LevelCollisionSource2DBuildResult source;
	iggy::LevelCombinedDerivedCacheBuildResult2D combined;
};

PipelineResult RunPipeline(
	const iggy::DraftDocument2D &document,
	const iggy::LevelRuntimeState &level,
	const iggy::DraftLevelGeometryPlan2DConfig &geometryConfig = {},
	const iggy::LevelCollisionSource2DConfig &sourceConfig = {})
{
	PipelineResult result;
	result.building = iggy::DraftBuildingCompiler2D {}.compile(document);
	result.geometry = iggy::DraftLevelGeometryPlanner2D {}.plan(result.building, geometryConfig);
	result.source = iggy::LevelCollisionSource2DBuilder {}.build(result.geometry, sourceConfig);
	const iggy::LevelCollisionSource2D source = result.source.built ? result.source.source : iggy::LevelCollisionSource2D {};
	result.combined = iggy::LevelCombinedDerivedCacheBuilder2D {}.build(level, source, CollisionConfig());
	return result;
}

iggy::physics2d::CollisionOverlap2DResult Query(
	const iggy::physics2d::CollisionWorld2D &world,
	iggy::Aabb2 bounds)
{
	return iggy::physics2d::CollisionOverlap2D {}.queryAabb(world, bounds);
}

iggy::Aabb2 SmallQuery(iggy::Vec2 center)
{
	return {
		{ center.x - 0.1F, center.y - 0.1F },
		{ center.x + 0.1F, center.y + 0.1F },
	};
}

bool SameSymbols(const std::vector<iggy::DraftSymbol2D> &actual, const std::vector<iggy::DraftSymbol2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].id != expected[index].id
			|| actual[index].kind != expected[index].kind
			|| !NearVec(actual[index].position, expected[index].position)
			|| !NearVec(actual[index].size, expected[index].size)
			|| actual[index].rotationRadians != expected[index].rotationRadians
			|| actual[index].assetId != expected[index].assetId
			|| actual[index].definitionId != expected[index].definitionId
			|| actual[index].enabled != expected[index].enabled) {
			return false;
		}
	}
	return true;
}

bool SameSource(const iggy::LevelCollisionSource2D &actual, const iggy::LevelCollisionSource2D &expected)
{
	if (actual.boxes.size() != expected.boxes.size())
		return false;
	for (std::size_t index = 0; index < actual.boxes.size(); ++index) {
		const iggy::LevelCollisionSourceBox2D &left = actual.boxes[index];
		const iggy::LevelCollisionSourceBox2D &right = expected.boxes[index];
		if (left.sourceWallId != right.sourceWallId
			|| left.sourceWallIndex != right.sourceWallIndex
			|| !NearVec(left.center, right.center)
			|| !NearVec(left.size, right.size)
			|| left.rotationRadians != right.rotationRadians
			|| left.assetId != right.assetId
			|| left.definitionId != right.definitionId) {
			return false;
		}
	}
	return true;
}

void ExpectRuntimeLevelUnchanged(
	const iggy::LevelRuntimeState &actual,
	const iggy::LevelRuntimeState &expected,
	const char *message)
{
	Expect(actual.map.id == expected.map.id, message);
	Expect(actual.map.width == expected.map.width && actual.map.height == expected.map.height, message);
	Expect(actual.map.tiles.size() == expected.map.tiles.size(), message);
	for (std::size_t index = 0; index < actual.map.tiles.size() && index < expected.map.tiles.size(); ++index)
		Expect(actual.map.tiles[index].walkable == expected.map.tiles[index].walkable, message);
	Expect(actual.npcAgents.size() == expected.npcAgents.size(), message);
}

void TestEmptyDraftBuildsEmptyValidPipeline()
{
	const iggy::DraftDocument2D document;
	const iggy::LevelRuntimeState level;

	const PipelineResult result = RunPipeline(document, level);

	Expect(!result.building.hasIssues(), "empty draft should compile without building issues");
	Expect(result.building.wallCuts.segments.empty(), "empty draft should produce no wall cut segments");
	Expect(result.geometry.collisionBoxes.empty() && !result.geometry.hasIssues(), "empty draft should produce empty valid geometry plan");
	Expect(result.source.built, "empty geometry plan should build empty collision source");
	Expect(result.source.source.boxes.empty(), "empty geometry plan should produce empty source boxes");
	Expect(result.combined.built, "empty source should build combined derived cache");
	Expect(result.combined.state.hasCollisionCache, "empty combined cache should still publish requested collision cache");
	Expect(result.combined.state.collision.world.objects().empty(), "empty combined cache should have empty collision world");
}

void TestOneWallCompilesThroughQueryableCombinedCollision()
{
	const iggy::DraftDocument2D document = Document({
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 8.0F, 2.0F }, 0.0F, "asset:wall", "definition:wall"),
	});
	const iggy::LevelRuntimeState level = RuntimeLevel({ "..." });

	const PipelineResult result = RunPipeline(document, level);

	Expect(!result.building.hasIssues(), "single wall draft should compile without issues");
	Expect(result.building.wallCuts.segments.size() == 1, "single wall draft should produce one wall segment");
	Expect(result.geometry.collisionBoxes.size() == 1, "single wall draft should produce one geometry collision box");
	Expect(result.source.built && result.source.source.boxes.size() == 1, "single wall draft should produce one source box");
	Expect(result.combined.built, "single wall draft should build combined collision cache");
	Expect(result.combined.state.collision.world.objects().size() == 1, "single wall draft should produce one collision object");
	if (result.combined.state.collision.world.objects().size() == 1) {
		Expect(result.combined.state.collision.world.objects()[0].id == Id("draft:wall"), "single wall collision object should preserve source wall id");
		ExpectBounds(result.combined.state.collision.world.objects()[0].shape.bounds, { { -4.0F, -1.0F }, { 4.0F, 1.0F } }, "single wall collision object should preserve wall bounds");
	}
	Expect(Query(result.combined.state.collision.world, SmallQuery({ 0.0F, 0.0F })).hits.size() == 1, "single wall collision should be queryable");
}

void TestWallWithCenteredDoorProducesSplitCollisionAndGap()
{
	const iggy::DraftDocument2D document = Document({
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 10.0F, 2.0F }),
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 0.0F, 0.0F }, { 2.0F, 2.0F }),
	});
	const iggy::LevelRuntimeState level = RuntimeLevel({ "..." });

	const PipelineResult result = RunPipeline(document, level);

	Expect(!result.building.hasIssues(), "centered door draft should compile without issues");
	Expect(result.building.attachments.attachments.size() == 1, "centered door should attach to wall");
	Expect(result.building.wallCuts.segments.size() == 2, "centered door should split wall into two segments");
	Expect(result.geometry.collisionBoxes.size() == 2, "centered door should produce two geometry boxes");
	Expect(result.source.source.boxes.size() == 2, "centered door should produce two source boxes");
	Expect(result.combined.built, "centered door should build combined cache");
	Expect(result.combined.state.collision.world.objects().size() == 2, "centered door should produce two collision objects");
	Expect(Query(result.combined.state.collision.world, SmallQuery({ -3.0F, 0.0F })).hits.size() == 1, "left wall segment should be queryable");
	Expect(Query(result.combined.state.collision.world, SmallQuery({ 3.0F, 0.0F })).hits.size() == 1, "right wall segment should be queryable");
	Expect(Query(result.combined.state.collision.world, SmallQuery({ 0.0F, 0.0F })).hits.empty(), "door gap should not collide");
}

void TestWallWithDoorAtOneEndProducesOneRemainingCollisionSegment()
{
	const iggy::DraftDocument2D document = Document({
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 10.0F, 2.0F }),
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { -4.0F, 0.0F }, { 2.0F, 2.0F }),
	});
	const iggy::LevelRuntimeState level = RuntimeLevel({ "..." });

	const PipelineResult result = RunPipeline(document, level);

	Expect(!result.building.hasIssues(), "end door draft should compile without issues");
	Expect(result.building.wallCuts.segments.size() == 1, "end door should produce one remaining wall segment");
	Expect(result.combined.built, "end door should build combined cache");
	Expect(result.combined.state.collision.world.objects().size() == 1, "end door should produce one collision object");
	Expect(Query(result.combined.state.collision.world, SmallQuery({ -4.5F, 0.0F })).hits.empty(), "door cut at end should be open");
	Expect(Query(result.combined.state.collision.world, SmallQuery({ 1.0F, 0.0F })).hits.size() == 1, "remaining wall segment should be queryable");
}

void TestDoorOutsideWallBlocksGeometryAndSourceByDefault()
{
	const iggy::DraftDocument2D document = Document({
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 4.0F, 2.0F }),
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 10.0F, 0.0F }, { 1.0F, 2.0F }),
	});
	const iggy::LevelRuntimeState level = RuntimeLevel({ "..." });

	const PipelineResult result = RunPipeline(document, level);

	Expect(result.building.hasIssues(), "outside door should make building compile issue-bearing");
	Expect(result.building.attachments.issues.size() == 1, "outside door should surface attachment issue");
	if (result.building.attachments.issues.size() == 1)
		Expect(result.building.attachments.issues[0].code == iggy::DraftWallDoorAttach2DIssueCode::NoContainingWall, "outside door issue should be NoContainingWall");
	Expect(result.geometry.hasIssues(), "default geometry plan should preserve building issue signal");
	Expect(result.geometry.collisionBoxes.empty(), "default geometry plan should block issue-bearing building output");
	Expect(!result.source.built, "default source builder should reject issue-bearing geometry plan");
	Expect(result.source.issues.size() == 1, "default source builder should report issue-bearing geometry plan");
	Expect(result.combined.built, "combined cache with no published source should still build tile-only collision");
	Expect(result.combined.state.collision.world.objects().empty(), "outside door default pipeline should publish no draft collision");
}

void TestIssueBearingCompileCanBeAllowedThroughGeometryAndSource()
{
	const iggy::DraftDocument2D document = Document({
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 4.0F, 2.0F }),
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 10.0F, 0.0F }, { 1.0F, 2.0F }),
	});
	const iggy::LevelRuntimeState level = RuntimeLevel({ "..." });
	const iggy::DraftLevelGeometryPlan2DConfig geometryConfig { true };
	const iggy::LevelCollisionSource2DConfig sourceConfig { true };

	const PipelineResult result = RunPipeline(document, level, geometryConfig, sourceConfig);

	Expect(result.building.hasIssues(), "allowed outside door setup should remain issue-bearing at building stage");
	Expect(result.geometry.hasIssues(), "allowed geometry plan should preserve issue signal");
	Expect(result.geometry.collisionBoxes.size() == 1, "allowed geometry plan should emit valid pass-through wall box");
	Expect(result.source.built, "allowed source builder should build source from valid geometry box");
	Expect(result.source.hasIssues(), "allowed source builder should preserve issue-bearing geometry signal");
	Expect(result.source.source.boxes.size() == 1, "allowed source builder should emit pass-through source box");
	Expect(result.combined.built, "allowed issue-bearing pipeline should build combined cache from valid source");
	Expect(result.combined.state.collision.world.objects().size() == 1, "allowed issue-bearing pipeline should publish pass-through wall collision");
	Expect(Query(result.combined.state.collision.world, SmallQuery({ 0.0F, 0.0F })).hits.size() == 1, "allowed pass-through wall collision should be queryable");
}

void TestTileMapAndDraftSourceCollisionBothAppear()
{
	const iggy::DraftDocument2D document = Document({
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 5.0F, 0.0F }, { 2.0F, 2.0F }),
	});
	const iggy::LevelRuntimeState level = RuntimeLevel({
		".#.",
	});

	const PipelineResult result = RunPipeline(document, level);

	Expect(result.combined.built, "tile and draft source pipeline should build combined cache");
	Expect(result.combined.tileCollision.built, "tile and draft source pipeline should preserve tile collision result");
	Expect(result.combined.sourceCollision.built, "tile and draft source pipeline should preserve source collision result");
	Expect(result.combined.merge.built, "tile and draft source pipeline should preserve merge result");
	Expect(result.combined.state.collision.world.objects().size() == 2, "tile and draft source pipeline should produce both collision objects");
	if (result.combined.state.collision.world.objects().size() == 2) {
		Expect(result.combined.state.collision.world.objects()[0].id.empty(), "tile collision object should remain first");
		ExpectBounds(result.combined.state.collision.world.objects()[0].shape.bounds, iggy::tileBounds({ 1, 0 }), "tile collision object should preserve tile bounds");
		Expect(result.combined.state.collision.world.objects()[1].id == Id("draft:wall"), "draft source collision object should follow tile collision");
	}
	Expect(Query(result.combined.state.collision.world, { { 1.1F, 0.1F }, { 1.9F, 0.9F } }).hits.size() == 1, "tile collision should be queryable");
	Expect(Query(result.combined.state.collision.world, SmallQuery({ 5.0F, 0.0F })).hits.size() == 1, "draft source collision should be queryable");
}

void TestDuplicateSourceCollisionIdsFailCombinedCacheBuild()
{
	const iggy::DraftDocument2D document = Document({
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { -3.0F, 0.0F }, { 2.0F, 2.0F }),
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 3.0F, 0.0F }, { 2.0F, 2.0F }),
	});
	const iggy::LevelRuntimeState level = RuntimeLevel({ "..." });

	const PipelineResult result = RunPipeline(document, level);

	Expect(!result.building.hasIssues(), "duplicate source id setup uses direct draft document and should compile wall facts");
	Expect(result.source.built && result.source.source.boxes.size() == 2, "duplicate source id setup should produce two source boxes");
	Expect(result.combined.sourceCollision.built, "duplicate source id setup should build source collision world before merge");
	Expect(!result.combined.built, "duplicate non-empty source ids should fail combined cache build");
	Expect(!result.combined.state.hasCollisionCache, "duplicate non-empty source ids should publish no collision cache");
	Expect(result.combined.merge.issues.size() == 1, "duplicate non-empty source ids should report merge issue");
	if (result.combined.merge.issues.size() == 1) {
		Expect(result.combined.merge.issues[0].code == iggy::LevelCollisionWorldMerge2DIssueCode::DuplicateObjectId, "duplicate source id should fail with DuplicateObjectId");
		Expect(result.combined.merge.issues[0].objectIndex == 1, "duplicate source id should preserve later duplicate index");
	}
}

void TestInputsAreNotMutated()
{
	iggy::DraftDocument2D document = Document({
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 5.0F, 0.0F }, { 2.0F, 2.0F }),
	});
	iggy::LevelRuntimeState level = RuntimeLevel({
		".#.",
	});
	level.map.id = Id("level:test");
	const std::vector<iggy::DraftSymbol2D> symbolsBefore = document.symbols;
	const iggy::LevelRuntimeState levelBefore = level;

	PipelineResult result = RunPipeline(document, level);
	const iggy::LevelCollisionSource2D sourceBefore = result.source.source;
	const iggy::LevelCombinedDerivedCacheBuildResult2D rebuild =
		iggy::LevelCombinedDerivedCacheBuilder2D {}.build(level, result.source.source, CollisionConfig());

	Expect(result.combined.built && rebuild.built, "immutability pipeline should build combined caches");
	Expect(SameSymbols(document.symbols, symbolsBefore), "draft building collision pipeline should not mutate draft document");
	ExpectRuntimeLevelUnchanged(level, levelBefore, "draft building collision pipeline should not mutate level runtime state or tile map");
	Expect(SameSource(result.source.source, sourceBefore), "combined derived cache builder should not mutate produced collision source data");
}

} // namespace

int main()
{
	TestEmptyDraftBuildsEmptyValidPipeline();
	TestOneWallCompilesThroughQueryableCombinedCollision();
	TestWallWithCenteredDoorProducesSplitCollisionAndGap();
	TestWallWithDoorAtOneEndProducesOneRemainingCollisionSegment();
	TestDoorOutsideWallBlocksGeometryAndSourceByDefault();
	TestIssueBearingCompileCanBeAllowedThroughGeometryAndSource();
	TestTileMapAndDraftSourceCollisionBothAppear();
	TestDuplicateSourceCollisionIdsFailCombinedCacheBuild();
	TestInputsAreNotMutated();

	return Failures;
}
