#include <cstdlib>
#include <vector>

#include "runtime/RuntimeNpcAiMovementQueueStep.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::runtime::GameplayCommandFrame2D MoveFrame(const char *actorId, iggy::Vec2 point)
{
	return { { iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(Id(actorId), point) } };
}

bool SameCommand(const iggy::runtime::GameplayCommand2D &actual, const iggy::runtime::GameplayCommand2D &expected)
{
	return actual.type == expected.type
		&& actual.actorId == expected.actorId
		&& NearVec(actual.targetPoint, expected.targetPoint)
		&& actual.targetTile == expected.targetTile
		&& actual.targetId == expected.targetId;
}

bool SameFrame(const iggy::runtime::GameplayCommandFrame2D &actual, const iggy::runtime::GameplayCommandFrame2D &expected)
{
	if (actual.commands.size() != expected.commands.size())
		return false;
	for (std::size_t index = 0; index < actual.commands.size(); ++index) {
		if (!SameCommand(actual.commands[index], expected.commands[index]))
			return false;
	}
	return true;
}

bool SameQueue(const iggy::runtime::RuntimeCommandQueueState &actual, const iggy::runtime::RuntimeCommandQueueState &expected)
{
	if (actual.frames.size() != expected.frames.size())
		return false;
	for (std::size_t index = 0; index < actual.frames.size(); ++index) {
		if (!SameFrame(actual.frames[index], expected.frames[index]))
			return false;
	}
	return true;
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

iggy::NpcAiMovementProposal2DResult ValidProposal(const char *npcId, iggy::Vec2 point)
{
	return Proposal(iggy::NpcAiMovementProposal2DStatus::Proposed, true, Id(npcId), point);
}

iggy::NpcAiMovementProposal2DResult NoMovementProposal()
{
	return Proposal(iggy::NpcAiMovementProposal2DStatus::NoPath, false, Id("npc:no-path"), {});
}

iggy::NpcAiMovementProposal2DResult MissingNpcProposal()
{
	return Proposal(iggy::NpcAiMovementProposal2DStatus::Proposed, true, {}, { 3.0F, 4.0F });
}

void TestValidProposalsQueueCommandsInOrder()
{
	const std::vector<iggy::NpcAiMovementProposal2DResult> proposals {
		ValidProposal("npc:one", { 1.0F, 2.0F }),
		ValidProposal("npc:two", { 3.0F, 4.0F }),
	};

	const iggy::runtime::RuntimeNpcAiMovementQueueResult result =
		iggy::runtime::RuntimeNpcAiMovementQueueStep {}.push({}, {}, proposals);

	Expect(result.status == iggy::runtime::RuntimeNpcAiMovementQueueStatus::Queued, "valid proposals should queue");
	Expect(result.mapping.frame.commands.size() == 2, "valid proposals should map to two commands");
	Expect(!result.mapping.hasIssues(), "valid proposals should have no mapping issues");
	Expect(result.queuePush.status == iggy::runtime::RuntimeNpcAiCommandQueueStatus::Queued, "valid proposal queue push should succeed");
	Expect(result.queuePush.push.status == iggy::runtime::RuntimeCommandQueueStatus::Accepted, "valid proposal command frame should be accepted by queue");
	Expect(result.queue.frames.size() == 1, "valid proposals should append one command frame");
	Expect(SameFrame(result.queue.frames[0], result.mapping.frame), "queued frame should match mapped frame");
	Expect(result.queue.frames[0].commands[0].actorId == Id("npc:one"), "first queued command should preserve first NPC id");
	Expect(NearVec(result.queue.frames[0].commands[0].targetPoint, { 1.0F, 2.0F }), "first queued command should preserve first target point");
	Expect(result.queue.frames[0].commands[1].actorId == Id("npc:two"), "second queued command should preserve second NPC id");
	Expect(NearVec(result.queue.frames[0].commands[1].targetPoint, { 3.0F, 4.0F }), "second queued command should preserve second target point");
}

void TestMixedProposalsQueueValidCommandsAndPreserveIssues()
{
	const std::vector<iggy::NpcAiMovementProposal2DResult> proposals {
		ValidProposal("npc:valid", { 1.0F, 1.0F }),
		NoMovementProposal(),
		MissingNpcProposal(),
	};

	const iggy::runtime::RuntimeNpcAiMovementQueueResult result =
		iggy::runtime::RuntimeNpcAiMovementQueueStep {}.push({}, {}, proposals);

	Expect(result.status == iggy::runtime::RuntimeNpcAiMovementQueueStatus::Queued, "mixed proposals should queue valid command frame");
	Expect(result.mapping.frame.commands.size() == 1, "mixed proposals should map only valid commands");
	Expect(result.queue.frames.size() == 1, "mixed proposals should append one frame");
	Expect(result.queue.frames[0].commands.size() == 1, "mixed proposals queued frame should contain only accepted commands");
	Expect(result.queue.frames[0].commands[0].actorId == Id("npc:valid"), "mixed proposals should preserve valid command actor");
	Expect(result.mapping.hasIssues(), "mixed proposals should preserve mapping issues");
	Expect(result.mapping.issues.size() == 2, "mixed proposals should preserve issue count");
	Expect(result.mapping.issues[0].proposalIndex == 1, "no-movement issue should preserve proposal index");
	Expect(result.mapping.issues[0].map.status == iggy::NpcAiMovementCommandMapper2DStatus::NoMovementProposal, "no-movement issue should preserve mapper status");
	Expect(result.mapping.issues[1].proposalIndex == 2, "missing-NPC issue should preserve proposal index");
	Expect(result.mapping.issues[1].map.status == iggy::NpcAiMovementCommandMapper2DStatus::MissingNpcId, "missing-NPC issue should preserve mapper status");
	Expect(result.queuePush.mapping.issues.size() == 2, "queue push diagnostics should preserve mapping issues");
}

void TestEmptyProposalsQueueEmptyFrame()
{
	const iggy::runtime::RuntimeNpcAiMovementQueueResult result =
		iggy::runtime::RuntimeNpcAiMovementQueueStep {}.push({}, {}, {});

	Expect(result.status == iggy::runtime::RuntimeNpcAiMovementQueueStatus::Queued, "empty proposals should queue");
	Expect(result.mapping.frame.commands.empty(), "empty proposals should map to empty frame");
	Expect(!result.mapping.hasIssues(), "empty proposals should have no mapping issues");
	Expect(result.queue.frames.size() == 1, "empty proposals should still append one frame");
	Expect(result.queue.frames[0].commands.empty(), "queued empty proposals frame should contain no commands");
}

void TestFullBoundedQueueRejectsAndPreservesOriginalQueue()
{
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(MoveFrame("npc:existing", { 9.0F, 9.0F }));
	const iggy::runtime::RuntimeCommandQueueState queueBefore = queue;
	const std::vector<iggy::NpcAiMovementProposal2DResult> proposals {
		ValidProposal("npc:new", { 1.0F, 1.0F }),
	};

	const iggy::runtime::RuntimeNpcAiMovementQueueResult result =
		iggy::runtime::RuntimeNpcAiMovementQueueStep {}.push(queue, { 1 }, proposals);

	Expect(result.status == iggy::runtime::RuntimeNpcAiMovementQueueStatus::RejectedFull, "full queue should reject NPC movement frame");
	Expect(result.mapping.frame.commands.size() == 1, "mapping should still run when queue is full");
	Expect(result.queuePush.status == iggy::runtime::RuntimeNpcAiCommandQueueStatus::RejectedFull, "nested queue push should preserve rejection");
	Expect(result.queuePush.push.status == iggy::runtime::RuntimeCommandQueueStatus::RejectedFull, "push diagnostics should preserve queue rejection");
	Expect(SameQueue(result.queue, queueBefore), "rejected movement queue step should preserve original queue");
	Expect(SameQueue(result.queuePush.queue, queueBefore), "nested queue push should preserve original queue");
}

void TestExistingFramesKeepFifoOrder()
{
	const iggy::runtime::GameplayCommandFrame2D first = MoveFrame("npc:first", { 1.0F, 1.0F });
	const iggy::runtime::GameplayCommandFrame2D second = MoveFrame("npc:second", { 2.0F, 2.0F });
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(first);
	queue.frames.push_back(second);
	const std::vector<iggy::NpcAiMovementProposal2DResult> proposals {
		ValidProposal("npc:third", { 3.0F, 3.0F }),
	};

	const iggy::runtime::RuntimeNpcAiMovementQueueResult result =
		iggy::runtime::RuntimeNpcAiMovementQueueStep {}.push(queue, {}, proposals);

	Expect(result.status == iggy::runtime::RuntimeNpcAiMovementQueueStatus::Queued, "non-full queue should accept NPC movement frame");
	Expect(result.queue.frames.size() == 3, "NPC movement queue push should append after existing frames");
	Expect(SameFrame(result.queue.frames[0], first), "first existing frame should remain first");
	Expect(SameFrame(result.queue.frames[1], second), "second existing frame should remain second");
	Expect(SameFrame(result.queue.frames[2], result.mapping.frame), "mapped NPC movement frame should append last");
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(MoveFrame("npc:existing", { 6.0F, 6.0F }));
	const iggy::runtime::RuntimeCommandQueueState queueBefore = queue;
	iggy::runtime::RuntimeCommandQueueConfig config { 3 };
	const iggy::runtime::RuntimeCommandQueueConfig configBefore = config;
	std::vector<iggy::NpcAiMovementProposal2DResult> proposals {
		ValidProposal("npc:immutable", { 1.0F, 1.0F }),
		NoMovementProposal(),
	};
	const std::vector<iggy::NpcAiMovementProposal2DResult> proposalsBefore = proposals;

	const iggy::runtime::RuntimeNpcAiMovementQueueResult result =
		iggy::runtime::RuntimeNpcAiMovementQueueStep {}.push(queue, config, proposals);

	Expect(result.status == iggy::runtime::RuntimeNpcAiMovementQueueStatus::Queued, "immutability setup should queue");
	Expect(SameQueue(queue, queueBefore), "movement queue step should not mutate input queue");
	Expect(config.maxFrames == configBefore.maxFrames, "movement queue step should not mutate queue config");
	Expect(proposals.size() == proposalsBefore.size(), "movement queue step should not mutate proposal vector size");
	for (std::size_t index = 0; index < proposals.size(); ++index) {
		Expect(proposals[index].status == proposalsBefore[index].status, "movement queue step should not mutate proposal status");
		Expect(proposals[index].npcId == proposalsBefore[index].npcId, "movement queue step should not mutate proposal NPC id");
		Expect(NearVec(proposals[index].proposedPosition, proposalsBefore[index].proposedPosition), "movement queue step should not mutate proposal point");
		Expect(proposals[index].requestsMovement == proposalsBefore[index].requestsMovement, "movement queue step should not mutate proposal movement flag");
	}
}

} // namespace

int main()
{
	TestValidProposalsQueueCommandsInOrder();
	TestMixedProposalsQueueValidCommandsAndPreserveIssues();
	TestEmptyProposalsQueueEmptyFrame();
	TestFullBoundedQueueRejectsAndPreservesOriginalQueue();
	TestExistingFramesKeepFifoOrder();
	TestInputsAreNotMutated();

	return Failures;
}
