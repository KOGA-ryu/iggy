#include <cstdlib>
#include <vector>

#include "scene/ai/NpcAiCommandFrameMapper2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcAiMovementProposal2DResult Proposal(
	iggy::NpcAiMovementProposal2DStatus status,
	bool requestsMovement,
	iggy::ResourceId npcId,
	iggy::Vec2 proposedPosition)
{
	iggy::NpcAiMovementProposal2DResult proposal;
	proposal.status = status;
	proposal.npcId = npcId;
	proposal.startPosition = { 0.5F, 0.5F };
	proposal.finalTargetPosition = { 5.5F, 0.5F };
	proposal.proposedPosition = proposedPosition;
	proposal.selectedWaypointIndex = 1;
	proposal.intentType = iggy::NpcAiBehaviorIntent2DType::Patrol;
	proposal.requestsMovement = requestsMovement;
	return proposal;
}

iggy::NpcAiMovementProposal2DResult ValidProposal(
	const char *npcId,
	iggy::Vec2 proposedPosition)
{
	return Proposal(
		iggy::NpcAiMovementProposal2DStatus::Proposed,
		true,
		Id(npcId),
		proposedPosition);
}

iggy::NpcAiMovementProposal2DResult NoMovementProposal()
{
	return Proposal(
		iggy::NpcAiMovementProposal2DStatus::NoPath,
		false,
		Id("npc:no-path"),
		{});
}

iggy::NpcAiMovementProposal2DResult MissingNpcProposal()
{
	return Proposal(
		iggy::NpcAiMovementProposal2DStatus::Proposed,
		true,
		{},
		{ 3.0F, 4.0F });
}

void ExpectProposalEquals(
	const iggy::NpcAiMovementProposal2DResult &actual,
	const iggy::NpcAiMovementProposal2DResult &expected,
	const char *message)
{
	Expect(actual.status == expected.status, message);
	Expect(actual.npcId == expected.npcId, message);
	Expect(NearVec(actual.startPosition, expected.startPosition), message);
	Expect(NearVec(actual.finalTargetPosition, expected.finalTargetPosition), message);
	Expect(NearVec(actual.proposedPosition, expected.proposedPosition), message);
	Expect(actual.selectedWaypointIndex == expected.selectedWaypointIndex, message);
	Expect(actual.intentType == expected.intentType, message);
	Expect(actual.requestsMovement == expected.requestsMovement, message);
}

void TestEmptyInputProducesEmptyFrameWithNoIssues()
{
	const iggy::NpcAiCommandFrameMapper2DResult result =
		iggy::NpcAiCommandFrameMapper2D {}.map({});

	Expect(result.frame.commands.empty(), "empty NPC proposal input should produce empty frame");
	Expect(result.issues.empty(), "empty NPC proposal input should produce no issues");
	Expect(!result.hasIssues(), "empty NPC proposal input should report no issues");
}

void TestMultipleValidProposalsProduceCommandsInOrder()
{
	const std::vector<iggy::NpcAiMovementProposal2DResult> proposals {
		ValidProposal("npc:one", { 1.0F, 2.0F }),
		ValidProposal("npc:two", { 3.0F, 4.0F }),
		ValidProposal("npc:three", { -5.0F, 6.0F }),
	};

	const iggy::NpcAiCommandFrameMapper2DResult result =
		iggy::NpcAiCommandFrameMapper2D {}.map(proposals);

	Expect(result.frame.commands.size() == 3, "valid NPC proposals should all append commands");
	Expect(result.issues.empty(), "valid NPC proposal batch should have no issues");
	Expect(!result.hasIssues(), "valid NPC proposal batch should report no issues");

	Expect(result.frame.commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "first command should be MoveToPoint");
	Expect(result.frame.commands[0].actorId == Id("npc:one"), "first command should preserve first NPC id");
	Expect(NearVec(result.frame.commands[0].targetPoint, { 1.0F, 2.0F }), "first command should preserve first proposed point");
	Expect(result.frame.commands[1].actorId == Id("npc:two"), "second command should preserve second NPC id");
	Expect(NearVec(result.frame.commands[1].targetPoint, { 3.0F, 4.0F }), "second command should preserve second proposed point");
	Expect(result.frame.commands[2].actorId == Id("npc:three"), "third command should preserve third NPC id");
	Expect(NearVec(result.frame.commands[2].targetPoint, { -5.0F, 6.0F }), "third command should preserve third proposed point");
}

void TestMixedProposalsProduceCommandsAndIssuesInOrder()
{
	const iggy::NpcAiMovementProposal2DResult validFirst = ValidProposal("npc:first", { 1.5F, 0.5F });
	const iggy::NpcAiMovementProposal2DResult noMovement = NoMovementProposal();
	const iggy::NpcAiMovementProposal2DResult missingNpc = MissingNpcProposal();
	const iggy::NpcAiMovementProposal2DResult validLast = ValidProposal("npc:last", { 4.5F, 0.5F });
	const std::vector<iggy::NpcAiMovementProposal2DResult> proposals {
		validFirst,
		noMovement,
		missingNpc,
		validLast,
	};

	const iggy::NpcAiCommandFrameMapper2DResult result =
		iggy::NpcAiCommandFrameMapper2D {}.map(proposals);

	Expect(result.frame.commands.size() == 2, "mixed NPC proposals should append only mapped commands");
	Expect(result.frame.commands[0].actorId == Id("npc:first"), "first valid command should preserve relative order");
	Expect(NearVec(result.frame.commands[0].targetPoint, { 1.5F, 0.5F }), "first valid command should preserve target point");
	Expect(result.frame.commands[1].actorId == Id("npc:last"), "last valid command should preserve relative order");
	Expect(NearVec(result.frame.commands[1].targetPoint, { 4.5F, 0.5F }), "last valid command should preserve target point");
	Expect(result.issues.size() == 2, "mixed NPC proposals should report excluded proposals");
	Expect(result.hasIssues(), "mixed NPC proposals should report issues");

	Expect(result.issues[0].proposalIndex == 1, "no-movement issue should preserve original index");
	Expect(result.issues[0].map.status == iggy::NpcAiMovementCommandMapper2DStatus::NoMovementProposal, "no-movement issue should preserve mapper status");
	ExpectProposalEquals(result.issues[0].proposal, noMovement, "no-movement issue should preserve original proposal");
	ExpectProposalEquals(result.issues[0].map.proposal, noMovement, "no-movement issue should preserve mapper proposal diagnostics");

	Expect(result.issues[1].proposalIndex == 2, "missing-NPC issue should preserve original index");
	Expect(result.issues[1].map.status == iggy::NpcAiMovementCommandMapper2DStatus::MissingNpcId, "missing-NPC issue should preserve mapper status");
	ExpectProposalEquals(result.issues[1].proposal, missingNpc, "missing-NPC issue should preserve original proposal");
	ExpectProposalEquals(result.issues[1].map.proposal, missingNpc, "missing-NPC issue should preserve mapper proposal diagnostics");
}

void TestInputProposalsAreNotMutated()
{
	std::vector<iggy::NpcAiMovementProposal2DResult> proposals {
		ValidProposal("npc:immutable-a", { 1.0F, 1.0F }),
		NoMovementProposal(),
		ValidProposal("npc:immutable-b", { 2.0F, 2.0F }),
	};
	const std::vector<iggy::NpcAiMovementProposal2DResult> proposalsBefore = proposals;

	const iggy::NpcAiCommandFrameMapper2DResult result =
		iggy::NpcAiCommandFrameMapper2D {}.map(proposals);

	Expect(result.frame.commands.size() == 2, "immutability setup should append valid commands");
	Expect(result.issues.size() == 1, "immutability setup should report excluded proposal");
	Expect(proposals.size() == proposalsBefore.size(), "frame mapper should not mutate proposal vector size");
	for (std::size_t index = 0; index < proposals.size(); ++index)
		ExpectProposalEquals(proposals[index], proposalsBefore[index], "frame mapper should not mutate proposal entries");
}

} // namespace

int main()
{
	TestEmptyInputProducesEmptyFrameWithNoIssues();
	TestMultipleValidProposalsProduceCommandsInOrder();
	TestMixedProposalsProduceCommandsAndIssuesInOrder();
	TestInputProposalsAreNotMutated();

	return Failures;
}
