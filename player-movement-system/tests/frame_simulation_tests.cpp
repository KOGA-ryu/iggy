#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/RuntimeEventStreamDelta.hpp"
#include "app/RuntimeFrameCompletionReportRecorder.hpp"
#include "app/RuntimeFrameEventDeltaCollector.hpp"
#include "app/RuntimeFrameEventReportRecorder.hpp"
#include "app/RuntimeFramePolicyReportRecorder.hpp"
#include "app/RuntimeFramePolicyResolver.hpp"
#include "app/RuntimeFrameSimulationPhaseRunner.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSimulationFrameUpdater.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "session/GameSession.hpp"
#include "session/SessionEventRecorder.hpp"
#include "simulation/SimulationFramePolicyDescriber.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

void TestRuntimeFrameEventReportRecorderReplacesFrameAndSummaryEvents()
{
	dev::RuntimeFrameReport frame;
	dev::RuntimeRunSummary summary;
	summary.lastFrameEvents.emit({
	    .type = dev::MovementEventType::StepCommitted,
	    .playerId = 1,
	    .tile = { 1, 1 },
	});

	dev::SimulationFrameEvents events;
	events.emit({
	    .type = dev::MovementEventType::CommandAccepted,
	    .playerId = 2,
	    .tile = { 4, 5 },
	    .commandType = dev::MovementCommandType::WalkTo,
	});
	events.emit({
	    .type = dev::CombatEventType::Hit,
	    .damage = 3,
	});

	dev::RuntimeFrameEventReportRecorder {}.record(events, frame, summary);

	Expect(frame.frameEvents.movementEvents().size() == 1, "runtime frame event report recorder should replace frame movement events");
	Expect(frame.frameEvents.movementEvents().size() == 1 && frame.frameEvents.movementEvents()[0].playerId == 2, "runtime frame event report recorder should preserve current frame movement event payload");
	Expect(frame.frameEvents.combatEvents().size() == 1, "runtime frame event report recorder should preserve current frame combat events");
	Expect(summary.lastFrameEvents.movementEvents().size() == 1, "runtime frame event report recorder should replace summary movement events with latest frame");
	Expect(summary.lastFrameEvents.movementEvents().size() == 1 && summary.lastFrameEvents.movementEvents()[0].playerId == 2, "runtime frame event report recorder should drop older summary movement events");
	Expect(summary.lastFrameEvents.combatEvents().size() == 1, "runtime frame event report recorder should mirror frame combat events into summary");
}

void TestRuntimeFrameCompletionReportRecorderStoresFrameAndCountsRun()
{
	dev::GameLoopResult result;
	result.summary.framesRun = 2;
	result.frameReports.push_back({});

	dev::RuntimeFrameReport frame;
	frame.rawInputEventsRouted = 3;
	frame.movementCommandsQueued = 1;

	std::vector<dev::SessionEvent> sessionEvents {
	    {
	        .type = dev::SessionEventType::ModeChanged,
	        .commandType = dev::SessionCommandType::SetMode,
	        .mode = dev::GameSessionMode::Inventory,
	    },
	};
	std::vector<dev::InventoryEvent> inventoryEvents {
	    {
	        .type = dev::InventoryEventType::Equipped,
	        .commandType = dev::InventoryCommandType::EquipItem,
	        .commandResult = dev::InventoryCommandResultType::Applied,
	        .equipmentResult = dev::EquipmentResultType::Equipped,
	        .itemId = 8,
	    },
	};

	dev::RuntimeFrameCompletionReportRecorder {}.record(sessionEvents, inventoryEvents, frame, result);

	Expect(result.summary.framesRun == 3, "runtime frame completion report recorder should increment finished frame count");
	Expect(result.frameReports.size() == 2, "runtime frame completion report recorder should append one frame report");
	const dev::RuntimeFrameReport &completedFrame = result.frameReports.back();
	Expect(completedFrame.rawInputEventsRouted == 3, "runtime frame completion report recorder should preserve existing frame report fields");
	Expect(completedFrame.movementCommandsQueued == 1, "runtime frame completion report recorder should preserve queued movement counts");
	Expect(completedFrame.sessionEvents.size() == 1 && completedFrame.sessionEvents[0].mode == dev::GameSessionMode::Inventory, "runtime frame completion report recorder should attach session event deltas");
	Expect(completedFrame.inventoryEvents.size() == 1 && completedFrame.inventoryEvents[0].itemId == 8, "runtime frame completion report recorder should attach inventory event deltas");
}

void TestRuntimeEventStreamDeltaCopiesEventsSinceOffset()
{
	std::vector<dev::SessionEvent> events {
	    {
	        .type = dev::SessionEventType::GameStarted,
	        .commandType = dev::SessionCommandType::StartNewGame,
	    },
	    {
	        .type = dev::SessionEventType::ModeChanged,
	        .commandType = dev::SessionCommandType::SetMode,
	        .mode = dev::GameSessionMode::Inventory,
	    },
	};

	std::vector<dev::SessionEvent> delta = dev::RuntimeEventStreamDelta {}.eventsSince(events, 1);
	std::vector<dev::SessionEvent> allEvents = dev::RuntimeEventStreamDelta {}.eventsSince(events, 0);
	std::vector<dev::SessionEvent> noneAtEnd = dev::RuntimeEventStreamDelta {}.eventsSince(events, events.size());
	std::vector<dev::SessionEvent> nonePastEnd = dev::RuntimeEventStreamDelta {}.eventsSince(events, events.size() + 4);

	Expect(delta.size() == 1, "runtime event stream delta should copy only events after offset");
	Expect(delta.size() == 1 && delta[0].mode == dev::GameSessionMode::Inventory, "runtime event stream delta should preserve event payloads after offset");
	Expect(allEvents.size() == 2, "runtime event stream delta should copy all events from zero offset");
	Expect(noneAtEnd.empty(), "runtime event stream delta should return empty delta at stream end");
	Expect(nonePastEnd.empty(), "runtime event stream delta should return empty delta past stream end");
}

void TestRuntimeFrameEventDeltaCollectorCapturesEventsSinceBeginFrame()
{
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	sessionEvents.emit({
	    .type = dev::SessionEventType::GameStarted,
	    .commandType = dev::SessionCommandType::StartNewGame,
	});
	inventoryEvents.emit({
	    .type = dev::InventoryEventType::Rejected,
	    .commandType = dev::InventoryCommandType::EquipItem,
	    .commandResult = dev::InventoryCommandResultType::Rejected,
	    .equipmentResult = dev::EquipmentResultType::MissingItem,
	    .itemId = 2,
	});

	dev::RuntimeFrameEventDeltaCollector collector;
	collector.beginFrame(sessionEvents, inventoryEvents);

	sessionEvents.emit({
	    .type = dev::SessionEventType::ModeChanged,
	    .commandType = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Inventory,
	});
	inventoryEvents.emit({
	    .type = dev::InventoryEventType::Equipped,
	    .commandType = dev::InventoryCommandType::EquipItem,
	    .commandResult = dev::InventoryCommandResultType::Applied,
	    .equipmentResult = dev::EquipmentResultType::Equipped,
	    .itemId = 3,
	});

	const dev::RuntimeFrameEventDeltas deltas = collector.collect(sessionEvents, inventoryEvents);

	Expect(deltas.sessionEvents.size() == 1, "runtime frame event delta collector should ignore session events before frame start");
	Expect(deltas.sessionEvents.size() == 1 && deltas.sessionEvents[0].mode == dev::GameSessionMode::Inventory, "runtime frame event delta collector should capture session events since frame start");
	Expect(deltas.inventoryEvents.size() == 1, "runtime frame event delta collector should ignore inventory events before frame start");
	Expect(deltas.inventoryEvents.size() == 1 && deltas.inventoryEvents[0].itemId == 3, "runtime frame event delta collector should capture inventory events since frame start");

	collector.beginFrame(sessionEvents, inventoryEvents);
	const dev::RuntimeFrameEventDeltas resetDeltas = collector.collect(sessionEvents, inventoryEvents);

	Expect(resetDeltas.sessionEvents.empty(), "runtime frame event delta collector should reset session offset at the next frame start");
	Expect(resetDeltas.inventoryEvents.empty(), "runtime frame event delta collector should reset inventory offset at the next frame start");
}

void TestRuntimeFramePolicyReportRecorderStoresCurrentFramePolicy()
{
	dev::RuntimeFrameReport frame;
	frame.framePolicy = dev::SimulationFramePolicyDescriber {}.describe(dev::SimulationMode::Paused);
	const dev::SimulationFramePolicyDescription inventory = dev::SimulationFramePolicyDescriber {}.describe(dev::SimulationMode::Inventory);

	dev::RuntimeFramePolicyReportRecorder {}.record(inventory, frame);

	Expect(frame.framePolicy.mode == dev::SimulationMode::Inventory, "runtime frame policy report recorder should replace frame policy mode");
	Expect(!frame.framePolicy.policy.updatePlayers, "runtime frame policy report recorder should preserve movement policy gate");
	Expect(std::string { frame.framePolicy.summary } == "hold world simulation while inventory owns input", "runtime frame policy report recorder should preserve policy summary");
}

void TestRuntimeFramePolicyResolverMapsSessionModeToSimulationPolicy()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_policy_resolver_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::RuntimeFramePolicyResolver resolver;

	dev::SimulationFramePolicyDescription gameplay = resolver.resolve(session);
	session.setMode(dev::GameSessionMode::Inventory);
	dev::SimulationFramePolicyDescription inventory = resolver.resolve(session);
	session.setMode(dev::GameSessionMode::Paused);
	dev::SimulationFramePolicyDescription paused = resolver.resolve(session);
	session.setMode(dev::GameSessionMode::Empty);
	dev::SimulationFramePolicyDescription empty = resolver.resolve(session);

	Expect(gameplay.mode == dev::SimulationMode::Gameplay, "runtime frame policy resolver should map gameplay sessions to gameplay simulation");
	Expect(gameplay.policy.updatePlayers && gameplay.policy.updateEnemies, "runtime frame policy resolver should advance actors during gameplay");
	Expect(inventory.mode == dev::SimulationMode::Inventory, "runtime frame policy resolver should map inventory sessions to inventory simulation");
	Expect(!inventory.policy.acceptCommands && !inventory.policy.updatePlayers, "runtime frame policy resolver should freeze command intake and player updates during inventory");
	Expect(paused.mode == dev::SimulationMode::Paused, "runtime frame policy resolver should map paused sessions to paused simulation");
	Expect(!paused.policy.acceptCommands && !paused.policy.updatePlayers, "runtime frame policy resolver should freeze paused simulation");
	Expect(empty.mode == dev::SimulationMode::Paused, "runtime frame policy resolver should map empty sessions to paused simulation");
	Expect(!empty.policy.updatePlayers && !empty.policy.updateEnemies, "runtime frame policy resolver should keep empty sessions inert");

	std::filesystem::remove_all(root);
}

void TestRuntimeSimulationFrameUpdaterAdvancesSessionWithFrameSettings()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_simulation_frame_updater_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	session.world().commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	const dev::SimulationFrameEvents events = dev::RuntimeSimulationFrameUpdater {}.update(
	    session,
	    dev::RuntimeFrameSettings {
	        .fixedDeltaSeconds = 1.0F / 60.0F,
	    });

	Expect(!events.movementEvents().empty(), "runtime simulation frame updater should return simulation frame events");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 1, 0 }, "runtime simulation frame updater should advance the active session world");

	std::filesystem::remove_all(root);
}

void TestRuntimeFrameSimulationPhaseRunnerRecordsPolicyAndEvents()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_simulation_phase_runner_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	session.world().commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::GameLoopResult result;
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };
	recorder.beginFrame();

	dev::RuntimeFrameSimulationPhaseRunner {}.run(
	    session,
	    dev::RuntimeFrameSettings {
	        .fixedDeltaSeconds = 1.0F / 60.0F,
	    },
	    recorder);
	recorder.finishFrame();

	Expect(result.frameReports.size() == 1, "runtime frame simulation phase runner should be recordable inside a frame");
	Expect(result.frameReports[0].framePolicy.mode == dev::SimulationMode::Gameplay, "runtime frame simulation phase runner should record current frame policy");
	Expect(!result.frameReports[0].frameEvents.movementEvents().empty(), "runtime frame simulation phase runner should record simulation events");
	Expect(!result.summary.lastFrameEvents.movementEvents().empty(), "runtime frame simulation phase runner should update summary frame events");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 1, 0 }, "runtime frame simulation phase runner should advance the active session world");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestRuntimeFrameEventReportRecorderReplacesFrameAndSummaryEvents();
	TestRuntimeFrameCompletionReportRecorderStoresFrameAndCountsRun();
	TestRuntimeEventStreamDeltaCopiesEventsSinceOffset();
	TestRuntimeFrameEventDeltaCollectorCapturesEventsSinceBeginFrame();
	TestRuntimeFramePolicyReportRecorderStoresCurrentFramePolicy();
	TestRuntimeFramePolicyResolverMapsSessionModeToSimulationPolicy();
	TestRuntimeSimulationFrameUpdaterAdvancesSessionWithFrameSettings();
	TestRuntimeFrameSimulationPhaseRunnerRecordsPolicyAndEvents();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
