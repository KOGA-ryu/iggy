#include <cstdlib>
#include <initializer_list>

#include "core/resource/ResourceId.hpp"
#include "runtime/GameplayCommandFrame2DValidator.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId ActorId { "actor:player" };
const iggy::ResourceId TargetId { "target:lever" };

iggy::runtime::GameplayCommandFrame2D Frame(std::initializer_list<iggy::runtime::GameplayCommand2D> commands)
{
	iggy::runtime::GameplayCommandFrame2D frame;
	frame.commands.insert(frame.commands.end(), commands.begin(), commands.end());
	return frame;
}

void TestEmptyFrameReturnsEmptyValidResult()
{
	const iggy::runtime::GameplayCommandFrame2D frame;
	const iggy::runtime::GameplayCommandFrame2DValidationResult result = iggy::runtime::GameplayCommandFrame2DValidator {}.validate(frame);

	Expect(!result.hasInvalidCommands, "empty command frame should have no invalid commands");
	Expect(result.acceptedFrame.commands.empty(), "empty command frame should produce empty accepted frame");
	Expect(result.invalidCommands.empty(), "empty command frame should produce no invalid diagnostics");
}

void TestAllValidCommandsAreAcceptedInOrder()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.moveToPoint(ActorId, { 1.0F, 2.0F }),
		factory.moveToTile(ActorId, { -3, 4 }),
		factory.wait(ActorId),
		factory.interact(ActorId, TargetId),
	});

	const iggy::runtime::GameplayCommandFrame2DValidationResult result = iggy::runtime::GameplayCommandFrame2DValidator {}.validate(frame);

	Expect(!result.hasInvalidCommands, "all-valid frame should report no invalid commands");
	Expect(result.invalidCommands.empty(), "all-valid frame should produce no invalid diagnostics");
	Expect(result.acceptedFrame.commands.size() == frame.commands.size(), "all-valid frame should accept all commands");
	if (result.acceptedFrame.commands.size() == frame.commands.size()) {
		Expect(result.acceptedFrame.commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "accepted frame should preserve first command order");
		Expect(result.acceptedFrame.commands[1].targetTile == iggy::TileCoord { -3, 4 }, "accepted frame should preserve second command data");
		Expect(result.acceptedFrame.commands[2].type == iggy::runtime::GameplayCommand2DType::Wait, "accepted frame should preserve third command order");
		Expect(result.acceptedFrame.commands[3].targetId == TargetId, "accepted frame should preserve fourth command data");
	}
}

void TestInvalidInteractReportsOriginalIndexStatusAndCommand()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommand2D invalid = factory.interact(ActorId, {});
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.wait(ActorId),
		invalid,
	});

	const iggy::runtime::GameplayCommandFrame2DValidationResult result = iggy::runtime::GameplayCommandFrame2DValidator {}.validate(frame);

	Expect(result.hasInvalidCommands, "frame with invalid interact should report invalid commands");
	Expect(result.invalidCommands.size() == 1, "frame with one invalid interact should report one diagnostic");
	if (result.invalidCommands.size() == 1) {
		Expect(result.invalidCommands[0].index == 1, "invalid command diagnostic should preserve original index");
		Expect(result.invalidCommands[0].status == iggy::runtime::GameplayCommand2DStatus::MissingTarget, "invalid command diagnostic should preserve MissingTarget status");
		Expect(result.invalidCommands[0].command.type == iggy::runtime::GameplayCommand2DType::Interact && result.invalidCommands[0].command.actorId == ActorId, "invalid command diagnostic should preserve command copy");
	}
}

void TestInvalidCommandExcludedFromAcceptedFrame()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.moveToPoint(ActorId, { 3.0F, 4.0F }),
		factory.interact(ActorId, {}),
		factory.wait(ActorId),
	});

	const iggy::runtime::GameplayCommandFrame2DValidationResult result = iggy::runtime::GameplayCommandFrame2DValidator {}.validate(frame);

	Expect(result.acceptedFrame.commands.size() == 2, "invalid command should be excluded from accepted frame");
	if (result.acceptedFrame.commands.size() == 2) {
		Expect(result.acceptedFrame.commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "accepted frame should keep valid command before invalid command");
		Expect(result.acceptedFrame.commands[1].type == iggy::runtime::GameplayCommand2DType::Wait, "accepted frame should keep valid command after invalid command");
	}
}

void TestMixedFramePreservesValidRelativeOrder()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.interact(ActorId, {}),
		factory.moveToPoint(ActorId, { 5.0F, 6.0F }),
		factory.interact(ActorId, TargetId),
		factory.interact(ActorId, {}),
		factory.moveToTile(ActorId, { -7, -8 }),
	});

	const iggy::runtime::GameplayCommandFrame2DValidationResult result = iggy::runtime::GameplayCommandFrame2DValidator {}.validate(frame);

	Expect(result.hasInvalidCommands, "mixed frame should report invalid commands");
	Expect(result.acceptedFrame.commands.size() == 3, "mixed frame should accept only valid commands");
	if (result.acceptedFrame.commands.size() == 3) {
		Expect(result.acceptedFrame.commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint && NearVec(result.acceptedFrame.commands[0].targetPoint, { 5.0F, 6.0F }), "mixed frame should preserve first valid command");
		Expect(result.acceptedFrame.commands[1].type == iggy::runtime::GameplayCommand2DType::Interact && result.acceptedFrame.commands[1].targetId == TargetId, "mixed frame should preserve second valid command");
		Expect(result.acceptedFrame.commands[2].type == iggy::runtime::GameplayCommand2DType::MoveToTile && result.acceptedFrame.commands[2].targetTile == iggy::TileCoord { -7, -8 }, "mixed frame should preserve third valid command");
	}
}

void TestMultipleInvalidCommandsReportedInInputOrder()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.interact(ActorId, {}),
		factory.wait(ActorId),
		factory.interact({}, {}),
	});

	const iggy::runtime::GameplayCommandFrame2DValidationResult result = iggy::runtime::GameplayCommandFrame2DValidator {}.validate(frame);

	Expect(result.invalidCommands.size() == 2, "multiple invalid commands should produce multiple diagnostics");
	if (result.invalidCommands.size() == 2) {
		Expect(result.invalidCommands[0].index == 0 && result.invalidCommands[0].status == iggy::runtime::GameplayCommand2DStatus::MissingTarget, "first invalid diagnostic should preserve input order and status");
		Expect(result.invalidCommands[1].index == 2 && result.invalidCommands[1].status == iggy::runtime::GameplayCommand2DStatus::MissingTarget, "second invalid diagnostic should preserve input order and status");
	}
}

void TestEmptyActorIdsRemainAccepted()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.none(),
		factory.moveToPoint({}, { 1.0F, 2.0F }),
		factory.moveToTile({}, { -1, -2 }),
		factory.interact({}, TargetId),
		factory.wait(),
	});

	const iggy::runtime::GameplayCommandFrame2DValidationResult result = iggy::runtime::GameplayCommandFrame2DValidator {}.validate(frame);

	Expect(!result.hasInvalidCommands, "empty actor ids should remain accepted");
	Expect(result.acceptedFrame.commands.size() == 5, "empty actor id frame should accept all valid commands");
	Expect(result.invalidCommands.empty(), "empty actor id frame should produce no diagnostics");
}

void TestInputFrameIsNotMutated()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.moveToPoint(ActorId, { 9.0F, 10.0F }),
		factory.interact(ActorId, {}),
	});
	const iggy::runtime::GameplayCommandFrame2D original = frame;

	const iggy::runtime::GameplayCommandFrame2DValidationResult result = iggy::runtime::GameplayCommandFrame2DValidator {}.validate(frame);

	Expect(result.hasInvalidCommands, "input immutability setup should include invalid command");
	Expect(frame.commands.size() == original.commands.size(), "validator should not mutate input command count");
	Expect(frame.commands[0].type == original.commands[0].type && NearVec(frame.commands[0].targetPoint, original.commands[0].targetPoint), "validator should not mutate first input command");
	Expect(frame.commands[1].type == original.commands[1].type && frame.commands[1].targetId == original.commands[1].targetId, "validator should not mutate second input command");
}

void TestValidatorDoesNotTouchRuntimeSessionState()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.wait(ActorId),
	});

	const iggy::runtime::GameplayCommandFrame2DValidationResult result = iggy::runtime::GameplayCommandFrame2DValidator {}.validate(frame);

	Expect(!result.hasInvalidCommands, "validator should validate command frames without runtime session state");
	Expect(result.acceptedFrame.commands.size() == 1, "validator should operate only on command frame data");
}

} // namespace

int main()
{
	TestEmptyFrameReturnsEmptyValidResult();
	TestAllValidCommandsAreAcceptedInOrder();
	TestInvalidInteractReportsOriginalIndexStatusAndCommand();
	TestInvalidCommandExcludedFromAcceptedFrame();
	TestMixedFramePreservesValidRelativeOrder();
	TestMultipleInvalidCommandsReportedInInputOrder();
	TestEmptyActorIdsRemainAccepted();
	TestInputFrameIsNotMutated();
	TestValidatorDoesNotTouchRuntimeSessionState();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
