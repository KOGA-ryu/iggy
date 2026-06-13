#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionFrameStep.hpp"
#include "support/CommandFrameFixtures.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::CommandFrame;
using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::Near;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:input-interaction-frame" };

iggy::runtime::RuntimePlayerCommandExecutionConfig PlayerConfig()
{
	iggy::runtime::RuntimePlayerCommandExecutionConfig config;
	config.movement.maxStep = 1.0F;
	return config;
}

iggy::npc_ai::NpcAgentTickConfig NpcConfig()
{
	iggy::npc_ai::NpcAgentTickConfig config;
	config.maxDistance = 0.25F;
	config.awareness = { 8.0F, 0 };
	return config;
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = MapFromRows({ "..", ".." });
	session.level.map.id = iggy::ResourceId { "level:input-interaction-frame" };
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	session.tickIndex = 31;
	return session;
}

iggy::runtime::GameplayCommandFrame2D WaitFrame()
{
	return CommandFrame({ iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId) });
}

iggy::runtime::RuntimePlayerInputGatedFrameStepInput PlayerInput(
	iggy::runtime::RuntimeSessionState session,
	std::vector<iggy::PlayerInputIntent2D> intents,
	iggy::PlayerInputContext2D context = {},
	iggy::runtime::RuntimeCommandQueueState queue = {},
	iggy::runtime::RuntimeCommandQueueConfig queueConfig = {})
{
	return {
		{
			session,
			queue,
			queueConfig,
			PlayerId,
			context,
			intents,
			{ 1.5F, 1.5F },
			PlayerConfig(),
			NpcConfig(),
		},
	};
}

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind,
	iggy::Vec2 position,
	float radius,
	bool enabled = true)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "player input interaction frame test registry setup should build");
	return result.registry;
}

iggy::runtime::RuntimePlayerInputInteractionFrameInput Input(
	iggy::runtime::RuntimePlayerInputGatedFrameStepInput playerInput,
	iggy::InteractionTarget2DRegistry interactionTargets,
	iggy::InteractionReach2DConfig interactionReach = {})
{
	return { playerInput, interactionTargets, interactionReach };
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "player input interaction frame collision fixture should build");
	return build.world;
}

iggy::physics2d::CollisionWorld2D BlockingWorld()
{
	return World({
		Object(iggy::ResourceId { "wall:east" }, { { 0.5F, -0.5F }, { 1.5F, 0.5F } }),
	});
}

void SetCollisionCache(iggy::runtime::RuntimeSessionState &session, const iggy::physics2d::CollisionWorld2D &world)
{
	session.derivedCaches.hasCollisionCache = true;
	session.derivedCaches.collision.world = world;
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

void ExpectTarget(const iggy::InteractionTarget2D &actual, const iggy::InteractionTarget2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.kind == expected.kind, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.radius == expected.radius, message);
	Expect(actual.enabled == expected.enabled, message);
}

void TestNoInteractionCommandsStillRunsPlayerInput()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(PlayerInput(session, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }), Registry({})));

	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "no-interaction adapter should run player input");
	Expect(result.interactions.interactions.empty(), "no-interaction adapter should report no interaction entries");
	Expect(!result.interactions.hasInteractions(), "no-interaction adapter should report no interactions");
	Expect(result.interactions.readyCount == 0 && result.interactions.blockedCount == 0, "no-interaction adapter should have zero interaction counts");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "no-interaction adapter should preserve player movement result");
	Expect(SameQueue(result.queue, result.playerInput.queue), "adapter queue should come from player input result");
}

void TestReachableInteractIntentProducesReadyDiagnostic()
{
	const iggy::InteractionTarget2D target = Target("target:ready", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(target.id) }), Registry({ target })));

	Expect(result.interactions.interactions.size() == 1, "reachable interact should produce one interaction entry");
	Expect(result.interactions.readyCount == 1, "reachable interact should count one ready interaction");
	Expect(result.interactions.blockedCount == 0, "reachable interact should count zero blocked interactions");
	if (result.interactions.interactions.size() == 1) {
		Expect(result.interactions.interactions[0].commandIndex == 0, "reachable interact should preserve command index");
		Expect(result.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "reachable interact should report ready status");
		Expect(result.interactions.interactions[0].result.ready(), "reachable interact result should be ready");
		ExpectTarget(result.interactions.interactions[0].result.plan.query.target, target, "reachable interact should preserve target details");
	}
}

void TestBlockedInteractionTargetsProduceDiagnostics()
{
	const iggy::InteractionTarget2D disabled = Target("target:disabled", iggy::InteractionTarget2DKind::Talk, { 0.0F, 0.0F }, 2.0F, false);
	const iggy::InteractionTarget2D far = Target("target:far", iggy::InteractionTarget2DKind::Door, { 5.0F, 0.0F }, 1.0F);
	const iggy::runtime::RuntimePlayerInputInteractionFrameResult missing = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(iggy::ResourceId { "target:missing" }) }), Registry({ disabled, far })));
	const iggy::runtime::RuntimePlayerInputInteractionFrameResult disabledResult = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(disabled.id) }), Registry({ disabled, far })));
	const iggy::runtime::RuntimePlayerInputInteractionFrameResult farResult = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(far.id) }), Registry({ disabled, far })));

	Expect(missing.interactions.interactions.size() == 1 && missing.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::TargetNotFound, "missing interaction target should produce blocked not-found diagnostic");
	Expect(disabledResult.interactions.interactions.size() == 1 && disabledResult.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::TargetDisabled, "disabled interaction target should produce blocked disabled diagnostic");
	Expect(farResult.interactions.interactions.size() == 1 && farResult.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange, "out-of-range interaction target should produce blocked out-of-range diagnostic");
	Expect(missing.interactions.blockedCount == 1 && disabledResult.interactions.blockedCount == 1 && farResult.interactions.blockedCount == 1, "blocked interaction diagnostics should count blocked entries");
}

void TestMoveThenInteractUsesPostMovePlayerPosition()
{
	const iggy::InteractionTarget2D target = Target("target:after_move", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(
			PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), {
				iggy::playerMoveToPointIntent({ 1.0F, 0.0F }),
				iggy::playerInteractIntent(target.id),
			}),
			Registry({ target })));

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "move-then-interact adapter should preserve post-move session");
	Expect(result.interactions.interactions.size() == 1, "move-then-interact should produce one interaction entry");
	if (result.interactions.interactions.size() == 1) {
		Expect(result.interactions.interactions[0].commandIndex == 1, "move-then-interact should preserve interact command index");
		Expect(result.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "move-then-interact should use post-move player position");
		Expect(NearVec(result.interactions.interactions[0].result.plan.actorPosition, { 1.0F, 0.0F }), "move-then-interact plan should use post-move actor position");
		Expect(Near(result.interactions.interactions[0].result.plan.reach.distance, 0.0F), "move-then-interact reach should be computed from post-move position");
	}
}

void TestContextBlockedInteractDoesNotReachInteractionDiagnostics()
{
	iggy::PlayerInputContext2D context;
	context.interactionEnabled = false;
	const iggy::InteractionTarget2D target = Target("target:blocked", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F);

	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }, context), Registry({ target })));

	Expect(result.playerInput.command.intake.mapping.gateIssues.size() == 1, "context-blocked interact should preserve gate issue");
	Expect(result.playerInput.report.blockedIntentCount == 1, "context-blocked interact report should count blocked intent");
	Expect(result.playerInput.command.intake.mapping.frame.commands.empty(), "context-blocked interact should not enter mapped command frame");
	Expect(!result.interactions.hasInteractions(), "context-blocked interact should produce no interaction diagnostics");
}

void TestUnsupportedInspectIsMappingIssueNotInteractionDiagnostic()
{
	const iggy::InteractionTarget2D target = Target("target:inspect", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 1.0F);

	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(PlayerInput(SessionWithPlayer(), { iggy::playerInspectIntent(target.id) }), Registry({ target })));

	Expect(result.playerInput.command.intake.mapping.gateIssues.empty(), "unsupported inspect should not be gate blocked by default");
	Expect(result.playerInput.command.intake.mapping.mapping.issues.size() == 1, "unsupported inspect should preserve mapping issue");
	Expect(result.playerInput.report.rejectedIntentCount == 1, "unsupported inspect report should count rejected intent");
	Expect(result.playerInput.command.intake.mapping.frame.commands.empty(), "unsupported inspect should not enter mapped command frame");
	Expect(!result.interactions.hasInteractions(), "unsupported inspect should produce no interaction diagnostics");
}

void TestExplicitWorldOverloadUsesResultingPlayerPosition()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::physics2d::CollisionWorld2D emptyWorld;
	const iggy::InteractionTarget2D target = Target("target:explicit", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 0.0F);

	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(
			PlayerInput(session, {
				iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
				iggy::playerInteractIntent(target.id),
			}),
			Registry({ target })),
		emptyWorld);

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "explicit world adapter should preserve movement override result");
	Expect(result.interactions.interactions.size() == 1, "explicit world adapter should produce interaction diagnostic");
	if (result.interactions.interactions.size() == 1) {
		Expect(result.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "explicit world adapter should evaluate interaction from moved position");
		Expect(NearVec(result.interactions.interactions[0].result.plan.actorPosition, { 1.0F, 0.0F }), "explicit world interaction plan should use moved actor position");
	}
}

void TestQueueRejectedStillComputesDiagnosticsFromMappedFrameWithoutExecuting()
{
	const iggy::InteractionTarget2D target = Target("target:queued", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });

	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(
			PlayerInput(
				session,
				{ iggy::playerInteractIntent(target.id) },
				{},
				fullQueue,
				{ 1 }),
			Registry({ target })));

	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue rejected adapter should preserve player input rejection");
	Expect(result.playerInput.report.tickResultCount == 0, "queue rejected adapter should not run command ticks");
	Expect(result.session.tickIndex == session.tickIndex, "queue rejected adapter should not advance session");
	ExpectPlayerAgent(result.session.player, session.player, "queue rejected adapter session");
	Expect(SameQueue(result.queue, fullQueue), "queue rejected adapter should preserve input queue");
	Expect(result.interactions.interactions.size() == 1, "queue rejected adapter should still evaluate mapped interact frame");
	if (result.interactions.interactions.size() == 1)
		Expect(result.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "queue rejected mapped interact should still produce ready diagnostic");
}

void TestInputsAreNotMutated()
{
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F);
	iggy::runtime::RuntimePlayerInputInteractionFrameInput input = Input(
		PlayerInput(
			SessionWithPlayer({ 0.0F, 0.0F }),
			{ iggy::playerInteractIntent(target.id), iggy::playerCancelIntent() },
			{},
			{ { WaitFrame() } },
			{ 3 }),
		Registry({ target }),
		{ 0.5F });
	const iggy::runtime::RuntimePlayerInputInteractionFrameInput before = input;

	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(input);

	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "immutability setup should run adapter");
	Expect(input.playerInput.commandInput.session.tickIndex == before.playerInput.commandInput.session.tickIndex, "adapter should not mutate input session tickIndex");
	ExpectPlayerAgent(input.playerInput.commandInput.session.player, before.playerInput.commandInput.session.player, "adapter input session");
	Expect(SameQueue(input.playerInput.commandInput.queue, before.playerInput.commandInput.queue), "adapter should not mutate input queue");
	Expect(input.playerInput.commandInput.intents.size() == before.playerInput.commandInput.intents.size(), "adapter should not mutate input intents");
	Expect(input.interactionTargets.targets().size() == before.interactionTargets.targets().size(), "adapter should not mutate interaction target registry");
	if (input.interactionTargets.targets().size() == before.interactionTargets.targets().size())
		ExpectTarget(input.interactionTargets.targets()[0], before.interactionTargets.targets()[0], "adapter should not mutate interaction target payload");
	Expect(input.interactionReach.extraReach == before.interactionReach.extraReach, "adapter should not mutate reach config");
}

} // namespace

int main()
{
	TestNoInteractionCommandsStillRunsPlayerInput();
	TestReachableInteractIntentProducesReadyDiagnostic();
	TestBlockedInteractionTargetsProduceDiagnostics();
	TestMoveThenInteractUsesPostMovePlayerPosition();
	TestContextBlockedInteractDoesNotReachInteractionDiagnostics();
	TestUnsupportedInspectIsMappingIssueNotInteractionDiagnostic();
	TestExplicitWorldOverloadUsesResultingPlayerPosition();
	TestQueueRejectedStillComputesDiagnosticsFromMappedFrameWithoutExecuting();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
