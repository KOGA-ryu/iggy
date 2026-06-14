#include <cstdlib>
#include <vector>

#include "scene/ai/AiMap2D.hpp"
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

void TestEmptyInputBuildsValidEmptyMap()
{
	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build({});

	Expect(result.built, "empty ai map input should build");
	Expect(result.issues.empty(), "empty ai map input should have no issues");
	Expect(result.map.nodes.empty(), "empty ai map input should produce empty map");
	Expect(!result.map.contains(Id("ai:missing")), "empty ai map should not contain missing id");
	Expect(result.map.find(Id("ai:missing")) == nullptr, "empty ai map should return null for missing id");
	Expect(result.map.nodesContaining({ 0.0F, 0.0F }).empty(), "empty ai map should contain no positions");
}

void TestSuccessfulBuildPreservesOrderAndFields()
{
	const std::vector<iggy::AiMapNode2D> nodes {
		Node("ai:patrol", { 1.0F, 2.0F }, 2.0F, 1.0F, 0.25F, 0.0F, 0.5F, { Id("tag:patrol"), Id("") }, { Id("ai:cover") }, true),
		Node("ai:cover", { -3.0F, 4.0F }, 1.5F, 0.1F, 3.0F, 0.2F, 0.0F, { Id("tag:cover") }, { Id("ai:patrol") }, false),
		Node("plain", { 0.0F, -1.0F }, 0.0F, 0.0F, 0.0F, 2.0F, 1.0F, {}, {}, true),
	};

	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);

	Expect(result.built, "valid ai map nodes should build");
	Expect(result.issues.empty(), "valid ai map nodes should have no issues");
	Expect(result.map.nodes.size() == nodes.size(), "valid ai map should preserve node count");
	if (result.map.nodes.size() == nodes.size()) {
		ExpectNode(result.map.nodes[0], nodes[0], "first ai map node should preserve fields");
		ExpectNode(result.map.nodes[1], nodes[1], "second ai map node should preserve fields");
		ExpectNode(result.map.nodes[2], nodes[2], "third ai map node should preserve fields");
	}
}

void TestFindAndContainsUseExactNodeIds()
{
	const std::vector<iggy::AiMapNode2D> nodes {
		Node("ai:patrol"),
		Node("ai:cover"),
	};
	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);

	Expect(result.built, "ai map lookup setup should build");
	Expect(result.map.contains(Id("ai:patrol")), "ai map should contain exact id");
	Expect(!result.map.contains(Id("ai:missing")), "ai map should not contain missing id");
	const iggy::AiMapNode2D *node = result.map.find(Id("ai:cover"));
	Expect(node != nullptr, "ai map should find exact id");
	if (node != nullptr)
		Expect(node->id == Id("ai:cover"), "ai map find should return matching node");
	Expect(result.map.find(Id("ai:missing")) == nullptr, "ai map find should return null for missing id");
}

void TestEmptyNodeIdFails()
{
	const iggy::AiMapNode2D node = Node("");

	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build({ node });

	Expect(!result.built, "empty ai node id should fail build");
	Expect(result.map.nodes.empty(), "failed empty-node-id build should not publish map");
	Expect(result.issues.size() == 1, "empty ai node id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::AiMap2DIssueCode::EmptyNodeId, "empty node id issue should use EmptyNodeId");
		Expect(result.issues[0].nodeIndex == 0, "empty node id issue should preserve node index");
		ExpectNode(result.issues[0].node, node, "empty node id issue should preserve node payload");
	}
}

void TestDuplicateNodeIdFailsForLaterNode()
{
	const std::vector<iggy::AiMapNode2D> nodes {
		Node("ai:node", { 0.0F, 0.0F }),
		Node("ai:node", { 2.0F, 0.0F }),
	};

	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);

	Expect(!result.built, "duplicate ai node id should fail build");
	Expect(result.map.nodes.empty(), "failed duplicate ai node build should not publish map");
	Expect(result.issues.size() == 1, "duplicate ai node id should report one issue for later node");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::AiMap2DIssueCode::DuplicateNodeId, "duplicate node id issue should use DuplicateNodeId");
		Expect(result.issues[0].nodeIndex == 1, "duplicate node id issue should preserve later node index");
		ExpectNode(result.issues[0].node, nodes[1], "duplicate node id issue should preserve later node payload");
	}
}

void TestNegativeRadiusFailsButZeroRadiusIsValid()
{
	const iggy::AiMapNode2D negative = Node("ai:negative", { 0.0F, 0.0F }, -0.01F);
	const iggy::AiMapNode2D zero = Node("ai:zero", { 1.0F, 1.0F }, 0.0F);

	const iggy::AiMap2DBuildResult negativeResult = iggy::AiMap2DBuilder {}.build({ negative });
	const iggy::AiMap2DBuildResult zeroResult = iggy::AiMap2DBuilder {}.build({ zero });

	Expect(!negativeResult.built, "negative ai node radius should fail build");
	Expect(negativeResult.issues.size() == 1, "negative ai node radius should report one issue");
	if (negativeResult.issues.size() == 1)
		Expect(negativeResult.issues[0].code == iggy::AiMap2DIssueCode::NegativeRadius, "negative radius issue should use NegativeRadius");
	Expect(zeroResult.built, "zero ai node radius should be valid point node data");
	Expect(zeroResult.issues.empty(), "zero ai node radius should not report issues");
}

void TestMissingLinkTargetFails()
{
	const iggy::AiMapNode2D node = Node("ai:start", { 0.0F, 0.0F }, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, {}, { Id("ai:missing") });

	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build({ node });

	Expect(!result.built, "missing ai map link target should fail build");
	Expect(result.issues.size() == 1, "missing ai map link target should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::AiMap2DIssueCode::MissingLinkTarget, "missing link issue should use MissingLinkTarget");
		Expect(result.issues[0].nodeIndex == 0, "missing link issue should preserve node index");
		Expect(result.issues[0].linkIndex == 0, "missing link issue should preserve link index");
		Expect(result.issues[0].linkId == Id("ai:missing"), "missing link issue should preserve link id");
	}
}

void TestDuplicateLinksFailForLaterLink()
{
	const std::vector<iggy::AiMapNode2D> nodes {
		Node("ai:start", { 0.0F, 0.0F }, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, {}, { Id("ai:end"), Id("ai:end") }),
		Node("ai:end"),
	};

	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);

	Expect(!result.built, "duplicate ai map links should fail build");
	Expect(result.issues.size() == 1, "duplicate ai map link should report one issue for later link");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::AiMap2DIssueCode::DuplicateLink, "duplicate link issue should use DuplicateLink");
		Expect(result.issues[0].nodeIndex == 0, "duplicate link issue should preserve source node index");
		Expect(result.issues[0].linkIndex == 1, "duplicate link issue should preserve later link index");
		Expect(result.issues[0].linkId == Id("ai:end"), "duplicate link issue should preserve link id");
	}
}

void TestDisabledNodesAreValidAndLinksToDisabledNodesAreValid()
{
	const std::vector<iggy::AiMapNode2D> nodes {
		Node("ai:enabled", { 0.0F, 0.0F }, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, {}, { Id("ai:disabled") }, true),
		Node("ai:disabled", { 1.0F, 0.0F }, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, {}, {}, false),
	};

	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);

	Expect(result.built, "disabled ai nodes and links to them should be valid data");
	Expect(result.issues.empty(), "disabled ai nodes should not report issues");
	Expect(result.map.nodes.size() == 2, "disabled ai nodes should be preserved");
	if (result.map.nodes.size() == 2)
		ExpectNode(result.map.nodes[1], nodes[1], "disabled ai node should preserve payload");
}

void TestMultipleIssuesAreReportedInDeterministicOrder()
{
	const std::vector<iggy::AiMapNode2D> nodes {
		Node("ai:node"),
		Node("", { 0.0F, 0.0F }, -1.0F, 0.0F, 0.0F, 0.0F, 0.0F, {}, { Id("ai:missing"), Id("ai:missing") }),
		Node("ai:node", { 2.0F, 0.0F }, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, {}, { Id("") }),
	};

	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);

	Expect(!result.built, "multi-issue ai map should fail build");
	Expect(result.map.nodes.empty(), "failed multi-issue ai map should not publish partial map");
	Expect(result.issues.size() == 6, "multi-issue ai map should report all issues");
	if (result.issues.size() == 6) {
		Expect(result.issues[0].code == iggy::AiMap2DIssueCode::EmptyNodeId && result.issues[0].nodeIndex == 1, "empty id should be first issue for second node");
		Expect(result.issues[1].code == iggy::AiMap2DIssueCode::NegativeRadius && result.issues[1].nodeIndex == 1, "negative radius should follow empty id");
		Expect(result.issues[2].code == iggy::AiMap2DIssueCode::MissingLinkTarget && result.issues[2].nodeIndex == 1 && result.issues[2].linkIndex == 0, "first missing link should follow node issues");
		Expect(result.issues[3].code == iggy::AiMap2DIssueCode::MissingLinkTarget && result.issues[3].nodeIndex == 1 && result.issues[3].linkIndex == 1, "second missing link should preserve link order");
		Expect(result.issues[4].code == iggy::AiMap2DIssueCode::DuplicateLink && result.issues[4].nodeIndex == 1 && result.issues[4].linkIndex == 1, "duplicate link should follow missing issue for same link");
		Expect(result.issues[5].code == iggy::AiMap2DIssueCode::DuplicateNodeId && result.issues[5].nodeIndex == 2, "duplicate node id should be reported for third node");
	}
}

void TestNamespacedAndUnqualifiedNodeIdsAreDistinct()
{
	const std::vector<iggy::AiMapNode2D> nodes {
		Node("patrol"),
		Node("ai:patrol"),
	};

	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);

	Expect(result.built, "namespaced and unqualified ai node ids should build as distinct ids");
	Expect(result.issues.empty(), "namespaced and unqualified ai node ids should not produce duplicate issues");
	Expect(result.map.contains(Id("patrol")), "ai map should contain unqualified node id");
	Expect(result.map.contains(Id("ai:patrol")), "ai map should contain namespaced node id");
	const iggy::AiMapNode2D *unqualified = result.map.find(Id("patrol"));
	const iggy::AiMapNode2D *namespaced = result.map.find(Id("ai:patrol"));
	Expect(unqualified != nullptr && namespaced != nullptr && unqualified != namespaced, "distinct ai node ids should resolve to distinct nodes");
}

void TestNodesContainingExcludesDisabledNodes()
{
	const std::vector<iggy::AiMapNode2D> nodes {
		Node("ai:near", { 0.0F, 0.0F }, 2.0F, 1.0F, 0.0F, 0.0F, 0.0F, { Id("tag:near") }),
		Node("ai:disabled", { 0.0F, 0.0F }, 3.0F, 0.0F, 1.0F, 0.0F, 0.0F, {}, {}, false),
		Node("ai:far", { 10.0F, 0.0F }, 1.0F),
		Node("ai:point", { 5.0F, 5.0F }, 0.0F),
	};
	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);

	Expect(result.built, "ai map position lookup setup should build");
	const std::vector<const iggy::AiMapNode2D *> originMatches = result.map.nodesContaining({ 1.0F, 0.0F });
	Expect(originMatches.size() == 1, "position lookup should include enabled containing node and exclude disabled containing node");
	if (originMatches.size() == 1)
		Expect(originMatches[0]->id == Id("ai:near"), "position lookup should return containing enabled node");
	const std::vector<const iggy::AiMapNode2D *> pointMatches = result.map.nodesContaining({ 5.0F, 5.0F });
	Expect(pointMatches.size() == 1 && pointMatches[0]->id == Id("ai:point"), "zero-radius point node should contain exact center");
	Expect(result.map.nodesContaining({ 5.1F, 5.0F }).empty(), "zero-radius point node should not contain nearby off-center position");
}

void TestInputVectorIsNotMutated()
{
	std::vector<iggy::AiMapNode2D> nodes {
		Node("ai:start", { 0.0F, 0.0F }, 1.0F, 1.0F, 2.0F, 3.0F, 4.0F, { Id("tag:start") }, { Id("ai:end") }),
		Node("ai:end", { 2.0F, 0.0F }, 1.0F),
	};
	const std::vector<iggy::AiMapNode2D> before = nodes;

	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);

	Expect(result.built, "ai map immutability setup should build");
	Expect(SameNodes(nodes, before), "ai map builder should not mutate input nodes");
}

} // namespace

int main()
{
	TestEmptyInputBuildsValidEmptyMap();
	TestSuccessfulBuildPreservesOrderAndFields();
	TestFindAndContainsUseExactNodeIds();
	TestEmptyNodeIdFails();
	TestDuplicateNodeIdFailsForLaterNode();
	TestNegativeRadiusFailsButZeroRadiusIsValid();
	TestMissingLinkTargetFails();
	TestDuplicateLinksFailForLaterLink();
	TestDisabledNodesAreValidAndLinksToDisabledNodesAreValid();
	TestMultipleIssuesAreReportedInDeterministicOrder();
	TestNamespacedAndUnqualifiedNodeIdsAreDistinct();
	TestNodesContainingExcludesDisabledNodes();
	TestInputVectorIsNotMutated();

	return Failures;
}
