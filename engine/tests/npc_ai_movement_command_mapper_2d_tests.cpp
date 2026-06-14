#include <cstdlib>
#include <vector>

#include "scene/ai/NpcAiMovementCommandMapper2D.hpp"
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
	iggy::ResourceId npcId = Id("npc:mapper"),
	iggy::Vec2 proposedPosition = { 2.5F, 0.5F })
{
	iggy::NpcAiMovementProposal2DResult proposal;
	proposal.status = status;
	proposal.npcId = npcId;
	proposal.startPosition = { 0.5F, 0.5F };
	proposal.finalTargetPosition = { 3.5F, 0.5F };
	proposal.proposedPosition = proposedPosition;
	proposal.selectedWaypointIndex = 1;
	proposal.intentType = iggy::NpcAiBehaviorIntent2DType::Patrol;
	proposal.requestsMovement = requestsMovement;
	return proposal;
}

void ExpectNoCommand(
	const iggy::NpcAiMovementCommandMapper2DResult &result,
	iggy::NpcAiMovementCommandMapper2DStatus status,
	const char *message)
{
	Expect(result.status == status, message);
	Expect(!result.hasCommand(), message);
	Expect(result.command.type == iggy::runtime::GameplayCommand2DType::None, message);
}

void TestNoPathProposalMapsToNoMovementProposal()
{
	const iggy::NpcAiMovementProposal2DResult proposal =
		Proposal(iggy::NpcAiMovementProposal2DStatus::NoPath, false);

	const iggy::NpcAiMovementCommandMapper2DResult result =
		iggy::NpcAiMovementCommandMapper2D {}.map(proposal);

	ExpectNoCommand(result, iggy::NpcAiMovementCommandMapper2DStatus::NoMovementProposal, "NoPath proposal should not map a command");
	Expect(result.proposal.status == proposal.status, "NoPath mapper result should preserve copied proposal");
}

void TestHoldPositionProposalMapsToNoMovementProposal()
{
	const iggy::NpcAiMovementProposal2DResult proposal =
		Proposal(iggy::NpcAiMovementProposal2DStatus::HoldPosition, false);

	const iggy::NpcAiMovementCommandMapper2DResult result =
		iggy::NpcAiMovementCommandMapper2D {}.map(proposal);

	ExpectNoCommand(result, iggy::NpcAiMovementCommandMapper2DStatus::NoMovementProposal, "HoldPosition proposal should not map a command");
}

void TestAlreadyAtTargetProposalMapsToNoMovementProposal()
{
	const iggy::NpcAiMovementProposal2DResult proposal =
		Proposal(iggy::NpcAiMovementProposal2DStatus::AlreadyAtTarget, false);

	const iggy::NpcAiMovementCommandMapper2DResult result =
		iggy::NpcAiMovementCommandMapper2D {}.map(proposal);

	ExpectNoCommand(result, iggy::NpcAiMovementCommandMapper2DStatus::NoMovementProposal, "AlreadyAtTarget proposal should not map a command");
}

void TestProposedMovementMapsToMoveToPointCommand()
{
	const iggy::NpcAiMovementProposal2DResult proposal =
		Proposal(
			iggy::NpcAiMovementProposal2DStatus::Proposed,
			true,
			Id("npc:scout"),
			{ 4.25F, 1.75F });

	const iggy::NpcAiMovementCommandMapper2DResult result =
		iggy::NpcAiMovementCommandMapper2D {}.map(proposal);

	Expect(result.status == iggy::NpcAiMovementCommandMapper2DStatus::Mapped, "movement proposal should map to a command");
	Expect(result.hasCommand(), "mapped proposal should report command availability");
	Expect(result.command.type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "movement proposal should map to MoveToPoint");
	Expect(result.command.actorId == Id("npc:scout"), "movement command should preserve NPC id as actor id");
	Expect(NearVec(result.command.targetPoint, { 4.25F, 1.75F }), "movement command should preserve proposed waypoint");
	Expect(result.command.targetId.empty(), "movement command should leave unrelated target id defaulted");
	Expect(result.proposal.npcId == proposal.npcId, "mapped result should preserve proposal NPC id");
	Expect(NearVec(result.proposal.proposedPosition, proposal.proposedPosition), "mapped result should preserve copied proposal waypoint");
}

void TestMissingNpcIdFailsDeterministically()
{
	const iggy::NpcAiMovementProposal2DResult proposal =
		Proposal(iggy::NpcAiMovementProposal2DStatus::Proposed, true, {});

	const iggy::NpcAiMovementCommandMapper2DResult result =
		iggy::NpcAiMovementCommandMapper2D {}.map(proposal);

	ExpectNoCommand(result, iggy::NpcAiMovementCommandMapper2DStatus::MissingNpcId, "movement proposal with empty NPC id should not map a command");
	Expect(result.proposal.hasMovementProposal(), "missing-id failure should preserve copied movement proposal");
}

void TestInputProposalIsNotMutated()
{
	iggy::NpcAiMovementProposal2DResult proposal =
		Proposal(
			iggy::NpcAiMovementProposal2DStatus::Proposed,
			true,
			Id("npc:immutable"),
			{ 2.0F, 3.0F });
	const iggy::NpcAiMovementProposal2DStatus statusBefore = proposal.status;
	const iggy::ResourceId npcIdBefore = proposal.npcId;
	const iggy::Vec2 proposedBefore = proposal.proposedPosition;
	const std::size_t selectedBefore = proposal.selectedWaypointIndex;
	const bool requestsBefore = proposal.requestsMovement;

	const iggy::NpcAiMovementCommandMapper2DResult result =
		iggy::NpcAiMovementCommandMapper2D {}.map(proposal);

	Expect(result.status == iggy::NpcAiMovementCommandMapper2DStatus::Mapped, "immutability setup should map a command");
	Expect(proposal.status == statusBefore, "mapper should not mutate proposal status");
	Expect(proposal.npcId == npcIdBefore, "mapper should not mutate proposal NPC id");
	Expect(NearVec(proposal.proposedPosition, proposedBefore), "mapper should not mutate proposal point");
	Expect(proposal.selectedWaypointIndex == selectedBefore, "mapper should not mutate proposal waypoint index");
	Expect(proposal.requestsMovement == requestsBefore, "mapper should not mutate proposal movement flag");
}

} // namespace

int main()
{
	TestNoPathProposalMapsToNoMovementProposal();
	TestHoldPositionProposalMapsToNoMovementProposal();
	TestAlreadyAtTargetProposalMapsToNoMovementProposal();
	TestProposedMovementMapsToMoveToPointCommand();
	TestMissingNpcIdFailsDeterministically();
	TestInputProposalIsNotMutated();

	return Failures;
}
