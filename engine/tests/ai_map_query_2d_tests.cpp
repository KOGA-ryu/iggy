#include <cstdlib>
#include <vector>

#include "scene/ai/AiMapQuery2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::AiMapNode2D Node(
	const char *id,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float radius = 0.0F,
	float patrolWeight = 0.0F,
	float coverWeight = 0.0F,
	float dangerWeight = 0.0F,
	float interestWeight = 0.0F,
	std::vector<iggy::ResourceId> tags = {},
	std::vector<iggy::ResourceId> links = {},
	bool enabled = true)
{
	return {
		Id(id),
		position,
		radius,
		patrolWeight,
		coverWeight,
		dangerWeight,
		interestWeight,
		tags,
		links,
		enabled,
	};
}

iggy::AiMap2D Map(std::vector<iggy::AiMapNode2D> nodes)
{
	return { nodes };
}

void ExpectNode(const iggy::AiMapNode2D &actual, const iggy::AiMapNode2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.radius == expected.radius, message);
	Expect(actual.patrolWeight == expected.patrolWeight, message);
	Expect(actual.coverWeight == expected.coverWeight, message);
	Expect(actual.dangerWeight == expected.dangerWeight, message);
	Expect(actual.interestWeight == expected.interestWeight, message);
	Expect(actual.tags == expected.tags, message);
	Expect(actual.links == expected.links, message);
	Expect(actual.enabled == expected.enabled, message);
}

bool SameNodes(const std::vector<iggy::AiMapNode2D> &actual, const std::vector<iggy::AiMapNode2D> &expected)
{
	if (actual.size() != expected.size())
		return false;

	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].id != expected[index].id
			|| !NearVec(actual[index].position, expected[index].position)
			|| actual[index].radius != expected[index].radius
			|| actual[index].patrolWeight != expected[index].patrolWeight
			|| actual[index].coverWeight != expected[index].coverWeight
			|| actual[index].dangerWeight != expected[index].dangerWeight
			|| actual[index].interestWeight != expected[index].interestWeight
			|| actual[index].tags != expected[index].tags
			|| actual[index].links != expected[index].links
			|| actual[index].enabled != expected[index].enabled) {
			return false;
		}
	}
	return true;
}

void TestEmptyMapQueryHasNoMatch()
{
	const iggy::AiMapQuery2DResult result = iggy::AiMapQuery2D {}.query({}, { 1.0F, 2.0F });

	Expect(result.status == iggy::AiMapQuery2DStatus::NoMatch, "empty ai map query should report NoMatch");
	Expect(!result.hasMatches(), "empty ai map query should have no matches");
	Expect(NearVec(result.position, { 1.0F, 2.0F }), "empty ai map query should preserve requested position");
	Expect(result.entries.empty(), "empty ai map query should have no entries");
	Expect(result.tags.empty(), "empty ai map query should have no tags");
	Expect(result.patrolWeight == 0.0F && result.coverWeight == 0.0F && result.dangerWeight == 0.0F && result.interestWeight == 0.0F, "empty ai map query should have zero weights");
}

void TestSingleMatchingNodeCopiesNodeWeightsAndTags()
{
	const iggy::AiMapNode2D node =
		Node("ai:patrol", { 0.0F, 0.0F }, 2.0F, 1.0F, 2.0F, 3.0F, 4.0F, { Id("tag:patrol"), Id("tag:cover") });
	const iggy::AiMap2D map = Map({ node });

	const iggy::AiMapQuery2DResult result = iggy::AiMapQuery2D {}.query(map, { 1.0F, 0.0F });

	Expect(result.status == iggy::AiMapQuery2DStatus::Matched, "single matching ai node should report Matched");
	Expect(result.hasMatches(), "single matching ai node should report matches");
	Expect(result.entries.size() == 1, "single matching ai node should produce one entry");
	if (result.entries.size() == 1) {
		ExpectNode(result.entries[0].node, node, "single query entry should copy node payload");
		Expect(result.entries[0].distance == 1.0F, "single query entry should preserve distance to node center");
	}
	Expect(result.patrolWeight == 1.0F, "single query should copy patrol weight");
	Expect(result.coverWeight == 2.0F, "single query should copy cover weight");
	Expect(result.dangerWeight == 3.0F, "single query should copy danger weight");
	Expect(result.interestWeight == 4.0F, "single query should copy interest weight");
	Expect(result.tags == std::vector<iggy::ResourceId>({ Id("tag:patrol"), Id("tag:cover") }), "single query should copy tags in order");
}

void TestOverlappingNodesAggregateWeightsAndMergeTags()
{
	const std::vector<iggy::AiMapNode2D> nodes {
		Node("ai:a", { 0.0F, 0.0F }, 3.0F, 1.0F, 0.5F, 0.25F, 2.0F, { Id("tag:shared"), Id("tag:a") }),
		Node("ai:b", { 1.0F, 0.0F }, 3.0F, 2.0F, 1.5F, 0.75F, 3.0F, { Id("tag:b"), Id("tag:shared"), Id("tag:c") }),
		Node("ai:outside", { 10.0F, 0.0F }, 1.0F, 9.0F, 9.0F, 9.0F, 9.0F, { Id("tag:outside") }),
	};
	const iggy::AiMap2D map = Map(nodes);

	const iggy::AiMapQuery2DResult result = iggy::AiMapQuery2D {}.query(map, { 1.0F, 0.0F });

	Expect(result.entries.size() == 2, "overlapping ai nodes should produce two entries in map order");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].node.id == Id("ai:a"), "first overlapping entry should preserve map order");
		Expect(result.entries[1].node.id == Id("ai:b"), "second overlapping entry should preserve map order");
		Expect(result.entries[0].distance == 1.0F, "first overlapping entry should preserve distance");
		Expect(result.entries[1].distance == 0.0F, "second overlapping entry should preserve distance");
	}
	Expect(result.patrolWeight == 3.0F, "overlapping query should sum patrol weights");
	Expect(result.coverWeight == 2.0F, "overlapping query should sum cover weights");
	Expect(result.dangerWeight == 1.0F, "overlapping query should sum danger weights");
	Expect(result.interestWeight == 5.0F, "overlapping query should sum interest weights");
	Expect(result.tags == std::vector<iggy::ResourceId>({ Id("tag:shared"), Id("tag:a"), Id("tag:b"), Id("tag:c") }), "overlapping query should merge and de-dupe tags in first-seen order");
}

void TestDisabledMatchingNodeIsExcluded()
{
	const std::vector<iggy::AiMapNode2D> nodes {
		Node("ai:disabled", { 0.0F, 0.0F }, 3.0F, 9.0F, 9.0F, 9.0F, 9.0F, { Id("tag:disabled") }, {}, false),
		Node("ai:enabled", { 0.0F, 0.0F }, 1.0F, 1.0F, 0.0F, 0.0F, 0.0F, { Id("tag:enabled") }),
	};

	const iggy::AiMapQuery2DResult result = iggy::AiMapQuery2D {}.query(Map(nodes), { 0.0F, 0.0F });

	Expect(result.entries.size() == 1, "query should exclude disabled matching node");
	if (result.entries.size() == 1)
		Expect(result.entries[0].node.id == Id("ai:enabled"), "query should preserve enabled matching node");
	Expect(result.patrolWeight == 1.0F, "query should exclude disabled node weights");
	Expect(result.tags == std::vector<iggy::ResourceId>({ Id("tag:enabled") }), "query should exclude disabled node tags");
}

void TestZeroRadiusPointNodeMatchesExactly()
{
	const iggy::AiMap2D map = Map({
		Node("ai:point", { 5.0F, 5.0F }, 0.0F, 1.0F),
	});

	const iggy::AiMapQuery2DResult exact = iggy::AiMapQuery2D {}.query(map, { 5.0F, 5.0F });
	const iggy::AiMapQuery2DResult nearby = iggy::AiMapQuery2D {}.query(map, { 5.01F, 5.0F });

	Expect(exact.status == iggy::AiMapQuery2DStatus::Matched && exact.entries.size() == 1, "zero-radius point node should match exact center");
	Expect(nearby.status == iggy::AiMapQuery2DStatus::NoMatch && nearby.entries.empty(), "zero-radius point node should not match nearby position");
}

void TestDuplicateTagsDedupedInFirstSeenOrder()
{
	const iggy::AiMap2D map = Map({
		Node("ai:a", { 0.0F, 0.0F }, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, { Id("tag:a"), Id("tag:a"), Id("tag:b") }),
		Node("ai:b", { 0.0F, 0.0F }, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, { Id("tag:b"), Id("tag:c") }),
	});

	const iggy::AiMapQuery2DResult result = iggy::AiMapQuery2D {}.query(map, { 0.0F, 0.0F });

	Expect(result.tags == std::vector<iggy::ResourceId>({ Id("tag:a"), Id("tag:b"), Id("tag:c") }), "duplicate tags should be de-duped in first-seen order");
}

void TestNamespacedAndUnqualifiedTagsRemainDistinct()
{
	const iggy::AiMap2D map = Map({
		Node("ai:a", { 0.0F, 0.0F }, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, { Id("cover"), Id("tag:cover"), Id("cover") }),
	});

	const iggy::AiMapQuery2DResult result = iggy::AiMapQuery2D {}.query(map, { 0.0F, 0.0F });

	Expect(result.tags == std::vector<iggy::ResourceId>({ Id("cover"), Id("tag:cover") }), "namespaced and unqualified tags should remain distinct");
}

void TestInputMapIsNotMutated()
{
	iggy::AiMap2D map = Map({
		Node("ai:a", { 0.0F, 0.0F }, 2.0F, 1.0F, 2.0F, 3.0F, 4.0F, { Id("tag:a") }, { Id("ai:b") }),
		Node("ai:b", { 4.0F, 0.0F }, 1.0F),
	});
	const std::vector<iggy::AiMapNode2D> before = map.nodes;

	const iggy::AiMapQuery2DResult result = iggy::AiMapQuery2D {}.query(map, { 0.0F, 0.0F });

	Expect(result.hasMatches(), "ai map query immutability setup should produce matches");
	Expect(SameNodes(map.nodes, before), "ai map query should not mutate input map");
}

} // namespace

int main()
{
	TestEmptyMapQueryHasNoMatch();
	TestSingleMatchingNodeCopiesNodeWeightsAndTags();
	TestOverlappingNodesAggregateWeightsAndMergeTags();
	TestDisabledMatchingNodeIsExcluded();
	TestZeroRadiusPointNodeMatchesExactly();
	TestDuplicateTagsDedupedInFirstSeenOrder();
	TestNamespacedAndUnqualifiedTagsRemainDistinct();
	TestInputMapIsNotMutated();

	return Failures;
}
