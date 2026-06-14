#include <cstdlib>
#include <vector>

#include "runtime/RuntimeNpcAiCommandQueueStep.hpp"
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

bool SameQueue(
	const iggy::runtime::RuntimeCommandQueueState &actual,
	const iggy::runtime::RuntimeCommandQueueState &expected)
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

iggy::NpcAiCommandFrameMapper2DResult Mapping(const std::vector<iggy::NpcAiMovementProposal2DResult> &proposals)
{
	return iggy::NpcAiCommandFrameMapper2D {}.map(proposals);
}

void TestValidMappedFrameQueuesCommandFrame()
{
	const iggy::NpcAiCommandFrameMapper2DResult mapping =
		Mapping({
			ValidProposal("npc:one", { 1.0F, 2.0F }),
			ValidProposal("npc:two", { 3.0F, 4.0F }),
		});

	const iggy::runtime::RuntimeNpcAiCommandQueueResult result =
		iggy::runtime::RuntimeNpcAiCommandQueueStep {}.push({}, {}, mapping);

	Expect(result.status == iggy::runtime::RuntimeNpcAiCommandQueueStatus::Queued, "valid NPC AI frame should queue");
	Expect(result.push.status == iggy::runtime::RuntimeCommandQueueStatus::Accepted, "valid NPC AI frame push should be accepted");
	Expect(result.queue.frames.size() == 1, "valid NPC AI frame should append one queue frame");
	Expect(SameFrame(result.queue.frames[0], mapping.frame), "queued frame should match mapped NPC AI frame");
	Expect(result.queue.frames[0].commands.size() == 2, "queued NPC AI frame should preserve command count");
	Expect(result.queue.frames[0].commands[0].actorId == Id("npc:one"), "queued frame should preserve first NPC id");
	Expect(NearVec(result.queue.frames[0].commands[1].targetPoint, { 3.0F, 4.0F }), "queued frame should preserve second target point");
}

void TestEmptyMappedFrameQueuesEmptyFrame()
{
	const iggy::NpcAiCommandFrameMapper2DResult mapping = Mapping({});

	const iggy::runtime::RuntimeNpcAiCommandQueueResult result =
		iggy::runtime::RuntimeNpcAiCommandQueueStep {}.push({}, {}, mapping);

	Expect(result.status == iggy::runtime::RuntimeNpcAiCommandQueueStatus::Queued, "empty NPC AI frame should queue");
	Expect(result.mapping.frame.commands.empty(), "empty mapping should remain empty");
	Expect(!result.mapping.hasIssues(), "empty mapping should preserve no issues");
	Expect(result.queue.frames.size() == 1, "empty NPC AI frame should still append one queue frame");
	Expect(result.queue.frames[0].commands.empty(), "queued empty NPC AI frame should have no commands");
}

void TestMappingIssuesArePreservedWhileAcceptedCommandsQueue()
{
	const iggy::NpcAiCommandFrameMapper2DResult mapping =
		Mapping({
			ValidProposal("npc:valid", { 1.0F, 1.0F }),
			NoMovementProposal(),
			MissingNpcProposal(),
		});

	const iggy::runtime::RuntimeNpcAiCommandQueueResult result =
		iggy::runtime::RuntimeNpcAiCommandQueueStep {}.push({}, {}, mapping);

	Expect(result.status == iggy::runtime::RuntimeNpcAiCommandQueueStatus::Queued, "mixed NPC AI mapping should still queue accepted commands");
	Expect(result.queue.frames.size() == 1, "mixed NPC AI mapping should queue one frame");
	Expect(result.queue.frames[0].commands.size() == 1, "mixed NPC AI mapping should queue only accepted commands");
	Expect(result.queue.frames[0].commands[0].actorId == Id("npc:valid"), "mixed NPC AI mapping should preserve accepted command actor");
	Expect(result.mapping.hasIssues(), "mixed NPC AI mapping should preserve issues");
	Expect(result.mapping.issues.size() == 2, "mixed NPC AI mapping should preserve issue count");
	Expect(result.mapping.issues[0].proposalIndex == 1, "first preserved issue should keep original index");
	Expect(result.mapping.issues[0].map.status == iggy::NpcAiMovementCommandMapper2DStatus::NoMovementProposal, "first preserved issue should keep mapper status");
	Expect(result.mapping.issues[1].proposalIndex == 2, "second preserved issue should keep original index");
	Expect(result.mapping.issues[1].map.status == iggy::NpcAiMovementCommandMapper2DStatus::MissingNpcId, "second preserved issue should keep mapper status");
}

void TestFullBoundedQueueRejectsAndPreservesOriginalQueue()
{
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(MoveFrame("npc:existing", { 9.0F, 9.0F }));
	const iggy::runtime::RuntimeCommandQueueState queueBefore = queue;
	const iggy::NpcAiCommandFrameMapper2DResult mapping =
		Mapping({ ValidProposal("npc:new", { 1.0F, 1.0F }) });

	const iggy::runtime::RuntimeNpcAiCommandQueueResult result =
		iggy::runtime::RuntimeNpcAiCommandQueueStep {}.push(queue, { 1 }, mapping);

	Expect(result.status == iggy::runtime::RuntimeNpcAiCommandQueueStatus::RejectedFull, "full queue should reject NPC AI command frame");
	Expect(result.push.status == iggy::runtime::RuntimeCommandQueueStatus::RejectedFull, "push diagnostics should preserve full rejection");
	Expect(SameQueue(result.queue, queueBefore), "rejected NPC AI push should preserve original queue");
	Expect(SameQueue(result.push.queue, queueBefore), "push result should preserve original queue on rejection");
	Expect(SameFrame(result.mapping.frame, mapping.frame), "rejected NPC AI push should preserve mapping diagnostics");
}

void TestExistingFramesKeepFifoOrderAfterPush()
{
	const iggy::runtime::GameplayCommandFrame2D first = MoveFrame("npc:first", { 1.0F, 1.0F });
	const iggy::runtime::GameplayCommandFrame2D second = MoveFrame("npc:second", { 2.0F, 2.0F });
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(first);
	queue.frames.push_back(second);
	const iggy::NpcAiCommandFrameMapper2DResult mapping =
		Mapping({ ValidProposal("npc:third", { 3.0F, 3.0F }) });

	const iggy::runtime::RuntimeNpcAiCommandQueueResult result =
		iggy::runtime::RuntimeNpcAiCommandQueueStep {}.push(queue, {}, mapping);

	Expect(result.status == iggy::runtime::RuntimeNpcAiCommandQueueStatus::Queued, "non-full queue should accept NPC AI frame");
	Expect(result.queue.frames.size() == 3, "successful NPC AI push should append after existing frames");
	Expect(SameFrame(result.queue.frames[0], first), "first existing frame should remain first");
	Expect(SameFrame(result.queue.frames[1], second), "second existing frame should remain second");
	Expect(SameFrame(result.queue.frames[2], mapping.frame), "NPC AI frame should append last");
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(MoveFrame("npc:existing", { 6.0F, 6.0F }));
	const iggy::runtime::RuntimeCommandQueueState queueBefore = queue;
	iggy::runtime::RuntimeCommandQueueConfig config { 3 };
	const iggy::runtime::RuntimeCommandQueueConfig configBefore = config;
	iggy::NpcAiCommandFrameMapper2DResult mapping =
		Mapping({
			ValidProposal("npc:immutable", { 1.0F, 1.0F }),
			NoMovementProposal(),
		});
	const iggy::NpcAiCommandFrameMapper2DResult mappingBefore = mapping;

	const iggy::runtime::RuntimeNpcAiCommandQueueResult result =
		iggy::runtime::RuntimeNpcAiCommandQueueStep {}.push(queue, config, mapping);

	Expect(result.status == iggy::runtime::RuntimeNpcAiCommandQueueStatus::Queued, "immutability setup should queue");
	Expect(SameQueue(queue, queueBefore), "NPC AI queue step should not mutate input queue");
	Expect(config.maxFrames == configBefore.maxFrames, "NPC AI queue step should not mutate queue config");
	Expect(SameFrame(mapping.frame, mappingBefore.frame), "NPC AI queue step should not mutate mapping frame");
	Expect(mapping.issues.size() == mappingBefore.issues.size(), "NPC AI queue step should not mutate mapping issues");
	Expect(mapping.issues[0].proposalIndex == mappingBefore.issues[0].proposalIndex, "NPC AI queue step should preserve mapping issue index");
	Expect(mapping.issues[0].map.status == mappingBefore.issues[0].map.status, "NPC AI queue step should preserve mapping issue status");
}

} // namespace

int main()
{
	TestValidMappedFrameQueuesCommandFrame();
	TestEmptyMappedFrameQueuesEmptyFrame();
	TestMappingIssuesArePreservedWhileAcceptedCommandsQueue();
	TestFullBoundedQueueRejectsAndPreservesOriginalQueue();
	TestExistingFramesKeepFifoOrderAfterPush();
	TestInputsAreNotMutated();

	return Failures;
}
