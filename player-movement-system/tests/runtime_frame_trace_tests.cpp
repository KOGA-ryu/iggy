#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/GameLoop.hpp"
#include "app/RuntimeCombatText.hpp"
#include "app/RuntimeEffectText.hpp"
#include "app/RuntimeFrameTrace.hpp"
#include "app/RuntimeFrameTraceSections.hpp"
#include "app/RuntimeInventoryScriptText.hpp"
#include "app/RuntimeInventoryText.hpp"
#include "app/RuntimeMovementEventText.hpp"
#include "app/RuntimeMovementInputBlockSummary.hpp"
#include "app/RuntimeMovementScriptText.hpp"
#include "app/RuntimePlayerActionText.hpp"
#include "app/RuntimeSessionText.hpp"
#include "commands/MovementCommandSource.hpp"
#include "inventory/InventoryCommandLog.hpp"
#include "inventory/InventoryCommandLogFileStore.hpp"
#include "inventory/InventoryCommandSource.hpp"
#include "inventory/InventoryScriptSource.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool ContainsLineFragment(const std::vector<std::string> &lines, std::string_view fragment)
{
	for (const std::string &line : lines) {
		if (line.find(fragment) != std::string::npos)
			return true;
	}
	return false;
}

void TestRuntimeFrameTraceFormatsReadableLines()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_trace_test";
	const std::filesystem::path scriptPath = root / "trace_inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 955,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "runtime frame trace test should create inventory script");

	dev::QueuedInventoryScriptSource inventoryScripts;
	inventoryScripts.enqueue(scriptPath);
	dev::QueuedInventoryCommandSource inventoryCommands;
	inventoryCommands.enqueue({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});
	dev::QueuedMovementCommandSource movementCommands;
	movementCommands.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = {
		        .movementCommandSources = { &movementCommands },
		        .inventoryCommandSources = { &inventoryCommands },
		        .inventoryScriptSources = { &inventoryScripts },
		    },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.capacity = 2;
	loop.session().world().players[0].inventory.items.push_back({
	    .id = 955,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::GameLoopResult result = loop.runForResult();
	Expect(result.frameReports.size() == 1, "runtime frame trace test should create one frame report");
	if (result.frameReports.empty()) {
		std::filesystem::remove_all(root);
		return;
	}

	std::vector<std::string> lines = dev::RuntimeFrameTrace {}.format(result.frameReports[0]);

	Expect(!lines.empty(), "runtime frame trace should produce readable lines");
	Expect(ContainsLineFragment(lines, "frame rawInput=0"), "runtime frame trace should include summary line");
	Expect(ContainsLineFragment(lines, "inventoryScripts=1"), "runtime frame trace should include inventory script count");
	Expect(ContainsLineFragment(lines, "inventoryResults=2"), "runtime frame trace should include inventory result count");
	Expect(ContainsLineFragment(lines, "movementQueued=1"), "runtime frame trace should include movement queue count");
	Expect(ContainsLineFragment(lines, "policy mode=Gameplay acceptCommands=1 updatePlayers=1 updateEnemies=1"), "runtime frame trace should include frame policy gates");
	Expect(ContainsLineFragment(lines, "reason=accept live input and advance all actors"), "runtime frame trace should include frame policy reason");
	Expect(ContainsLineFragment(lines, "inventoryScript[0] status=Completed results=1 applied=1 rejected=0"), "runtime frame trace should include inventory script detail");
	Expect(ContainsLineFragment(lines, "inventoryResult[0] type=Applied command=EquipItem equipment=Equipped item=955 slot=Weapon"), "runtime frame trace should include equip result detail");
	Expect(ContainsLineFragment(lines, "inventoryResult[1] type=Applied command=UnequipSlot equipment=Unequipped item=955 slot=Weapon"), "runtime frame trace should include unequip result detail");
	Expect(ContainsLineFragment(lines, "inventoryEvent[0] type=Equipped command=EquipItem result=Applied equipment=Equipped item=955 slot=Weapon"), "runtime frame trace should include inventory event detail");
	Expect(ContainsLineFragment(lines, "movementEvent[0] type=CommandAccepted"), "runtime frame trace should include movement event detail");

	std::filesystem::remove_all(root);
}

void TestRuntimeFrameTraceSectionsFormatsRuntimeSourcesInOrder()
{
	dev::RuntimeFrameReport report;
	report.movementInputBlockReasons.push_back(dev::PlayerActionBlockReason::Focus);
	report.sessionCommandResults.push_back({
	    .type = dev::SessionCommandResultType::Applied,
	    .command = { .type = dev::SessionCommandType::SetMode, .mode = dev::GameSessionMode::Inventory },
	});
	report.inventoryScriptResults.push_back({
	    .status = dev::InventoryScriptRunStatus::Completed,
	});
	report.inventoryCommandResults.push_back({
	    .type = dev::InventoryCommandResultType::Applied,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 10 },
	    .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 10, .slot = dev::EquipmentSlot::Weapon },
	});
	report.movementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::Completed,
	});

	const std::vector<std::string> lines = dev::RuntimeFrameTraceSections {}.formatRuntimeSources(report);
	const std::vector<std::string> expected {
		"sessionResult[0] type=Applied command=SetMode",
		"movementInputBlock[0] reason=Focus",
		"inventoryScript[0] status=Completed results=0 applied=0 rejected=0",
		"inventoryResult[0] type=Applied command=EquipItem equipment=Equipped item=10 slot=Weapon",
		"movementScript[0] status=Completed results=0 accepted=0 rejected=0",
	};

	Expect(lines == expected, "runtime frame trace sections should format runtime source lines in trace order");
}

void TestRuntimeMovementInputBlockSummaryCountsReasons()
{
	const std::vector<dev::PlayerActionBlockReason> reasons {
		dev::PlayerActionBlockReason::Focus,
		dev::PlayerActionBlockReason::Focus,
		dev::PlayerActionBlockReason::Paused,
		dev::PlayerActionBlockReason::AnimationLocked,
		dev::PlayerActionBlockReason::AnimationCommitment,
		dev::PlayerActionBlockReason::Stunned,
		dev::PlayerActionBlockReason::None,
	};

	const dev::RuntimeMovementInputBlockSummary summary = dev::RuntimeMovementInputBlockSummaryBuilder {}.summarize(reasons);

	Expect(!summary.empty(), "runtime movement input block summary should report non-empty reasons");
	Expect(summary.total == 7, "runtime movement input block summary should count total blocks");
	Expect(summary.count(dev::PlayerActionBlockReason::Focus) == 2, "runtime movement input block summary should count focus blocks");
	Expect(summary.count(dev::PlayerActionBlockReason::Paused) == 1, "runtime movement input block summary should count paused blocks");
	Expect(summary.count(dev::PlayerActionBlockReason::AnimationLocked) == 1, "runtime movement input block summary should count app animation lock blocks");
	Expect(summary.count(dev::PlayerActionBlockReason::AnimationCommitment) == 1, "runtime movement input block summary should count animation commitment blocks");
	Expect(summary.count(dev::PlayerActionBlockReason::Stunned) == 1, "runtime movement input block summary should count stunned blocks");
	Expect(summary.count(dev::PlayerActionBlockReason::None) == 1, "runtime movement input block summary should count no-reason blocks");
}

void TestRuntimeMovementInputBlockSummaryTextFormatsReasonCounts()
{
	dev::RuntimeMovementInputBlockSummary summary;
	summary.total = 3;
	summary.focus = 1;
	summary.paused = 1;
	summary.stunned = 1;

	const std::string line = dev::RuntimeMovementInputBlockSummaryText {}.format("movementInputBlockReasons", summary);

	Expect(line == "movementInputBlockReasons total=3 focus=1 paused=1 animationLocked=0 animationCommitment=0 stunned=1 none=0", "runtime movement input block summary text should format reason counts");
}

void TestRuntimePlayerActionTextFormatsMovementBlockReasons()
{
	dev::RuntimePlayerActionText formatter;

	Expect(formatter.formatMovementBlockReason("movementInputBlock[0]", dev::PlayerActionBlockReason::Focus) == "movementInputBlock[0] reason=Focus", "runtime player action text should format focus block reason");
	Expect(formatter.formatMovementBlockReason("movementInputBlock[1]", dev::PlayerActionBlockReason::Paused) == "movementInputBlock[1] reason=Paused", "runtime player action text should format paused block reason");
	Expect(formatter.formatMovementBlockReason("movementInputBlock[2]", dev::PlayerActionBlockReason::AnimationLocked) == "movementInputBlock[2] reason=AnimationLocked", "runtime player action text should format app animation lock reason");
	Expect(formatter.formatMovementBlockReason("movementInputBlock[3]", dev::PlayerActionBlockReason::AnimationCommitment) == "movementInputBlock[3] reason=AnimationCommitment", "runtime player action text should format animation commitment reason");
	Expect(formatter.formatMovementBlockReason("movementInputBlock[4]", dev::PlayerActionBlockReason::Stunned) == "movementInputBlock[4] reason=Stunned", "runtime player action text should format stunned reason");
	Expect(formatter.formatMovementBlockReason("movementInputBlock[5]", dev::PlayerActionBlockReason::None) == "movementInputBlock[5] reason=None", "runtime player action text should format no block reason");
}

void TestRuntimeFrameTraceSectionsFormatsLifecycleEventsInOrder()
{
	dev::RuntimeFrameReport report;
	report.sessionEvents.push_back({
	    .type = dev::SessionEventType::ModeChanged,
	    .commandType = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Inventory,
	});
	report.inventoryEvents.push_back({
	    .type = dev::InventoryEventType::Equipped,
	    .commandType = dev::InventoryCommandType::EquipItem,
	    .commandResult = dev::InventoryCommandResultType::Applied,
	    .equipmentResult = dev::EquipmentResultType::Equipped,
	    .itemId = 10,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	const std::vector<std::string> lines = dev::RuntimeFrameTraceSections {}.formatLifecycleEvents(report);
	const std::vector<std::string> expected {
		"sessionEvent[0] type=ModeChanged command=SetMode",
		"inventoryEvent[0] type=Equipped command=EquipItem result=Applied equipment=Equipped item=10 slot=Weapon",
	};

	Expect(lines == expected, "runtime frame trace sections should format lifecycle event lines in trace order");
}

void TestRuntimeFrameTraceSectionsFormatsSimulationEventsInOrder()
{
	dev::RuntimeFrameReport report;
	report.frameEvents.emit(dev::MovementEvent {
	    .type = dev::MovementEventType::StepCommitted,
	    .playerId = 1,
	    .tile = { 2, 3 },
	    .commandType = dev::MovementCommandType::WalkTo,
	});
	report.frameEvents.emit(dev::CombatEvent {
	    .type = dev::CombatEventType::Hit,
	    .damage = 4,
	    .remainingHitPoints = 6,
	});
	report.frameEvents.emit(dev::EffectRequest {
	    .type = dev::EffectRequestType::HitImpact,
	    .tile = { 2, 3 },
	});

	const std::vector<std::string> lines = dev::RuntimeFrameTraceSections {}.formatSimulationEvents(report);
	const std::vector<std::string> expected {
		"movementEvent[0] type=StepCommitted player=1 tile=(2,3) command=WalkTo",
		"combatEvent[0] type=Hit damage=4 remainingHp=6",
		"effect[0] type=HitImpact tile=(2,3)",
	};

	Expect(lines == expected, "runtime frame trace sections should format simulation event lines in trace order");
}

void TestRuntimeSessionTextFormatsResultsAndEvents()
{
	dev::RuntimeSessionText formatter;
	dev::SessionCommandResult result {
		.type = dev::SessionCommandResultType::Applied,
		.command = { .type = dev::SessionCommandType::SetMode, .mode = dev::GameSessionMode::Inventory },
	};
	dev::SessionEvent event {
		.type = dev::SessionEventType::ModeChanged,
		.commandType = dev::SessionCommandType::SetMode,
		.mode = dev::GameSessionMode::Inventory,
	};

	Expect(formatter.formatResult("sessionResult[0]", result) == "sessionResult[0] type=Applied command=SetMode", "runtime session text should format command result lines");
	Expect(formatter.formatEvent("sessionEvent[0]", event) == "sessionEvent[0] type=ModeChanged command=SetMode", "runtime session text should format event lines");
}

void TestRuntimeInventoryTextFormatsResultsAndEvents()
{
	dev::RuntimeInventoryText formatter;
	dev::InventoryCommandResult result {
		.type = dev::InventoryCommandResultType::Applied,
		.command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 100 },
		.equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 100, .slot = dev::EquipmentSlot::Weapon },
	};
	dev::InventoryEvent event {
		.type = dev::InventoryEventType::Equipped,
		.commandType = dev::InventoryCommandType::EquipItem,
		.commandResult = dev::InventoryCommandResultType::Applied,
		.equipmentResult = dev::EquipmentResultType::Equipped,
		.itemId = 100,
		.slot = dev::EquipmentSlot::Weapon,
	};

	Expect(formatter.formatResult("inventoryResult[0]", result) == "inventoryResult[0] type=Applied command=EquipItem equipment=Equipped item=100 slot=Weapon", "runtime inventory text should format command result lines");
	Expect(formatter.formatEvent("inventoryEvent[0]", event) == "inventoryEvent[0] type=Equipped command=EquipItem result=Applied equipment=Equipped item=100 slot=Weapon", "runtime inventory text should format event lines");
}

void TestRuntimeInventoryScriptTextFormatsResultsAndAggregates()
{
	dev::InventoryScriptRunResult completed {
		.status = dev::InventoryScriptRunStatus::Completed,
		.commandResults = {
		    {
		        .type = dev::InventoryCommandResultType::Applied,
		        .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 100 },
		    },
		    {
		        .type = dev::InventoryCommandResultType::Rejected,
		        .command = { .type = dev::InventoryCommandType::UnequipSlot, .slot = dev::EquipmentSlot::Weapon },
		    },
		},
	};
	std::vector<dev::InventoryScriptRunResult> results {
		completed,
		{ .status = dev::InventoryScriptRunStatus::LoadFailed },
		{ .status = dev::InventoryScriptRunStatus::NoActivePlayer },
	};

	dev::RuntimeInventoryScriptText formatter;

	Expect(formatter.formatResult("inventoryScript[0]", completed) == "inventoryScript[0] status=Completed results=2 applied=1 rejected=1", "runtime inventory script text should format one script result");
	Expect(formatter.formatAggregate("runtime inventoryScripts", results) == "runtime inventoryScripts=3 completed=1 loadFailed=1 noActivePlayer=1 applied=1 rejected=1", "runtime inventory script text should format aggregate script results");
}

void TestRuntimeMovementScriptTextFormatsResultsAndAggregates()
{
	dev::MovementScriptRunResult completed {
		.status = dev::MovementScriptRunStatus::Completed,
		.replayReport = {
		    .results = {
		        {
		            .type = dev::MovementCommandDispatchResultType::Accepted,
		            .command = {
		                .type = dev::MovementCommandType::WalkTo,
		                .playerId = 0,
		                .destination = { 1, 0 },
		            },
		        },
		        {
		            .type = dev::MovementCommandDispatchResultType::Rejected,
		            .command = {
		                .type = dev::MovementCommandType::MoveThenAct,
		                .playerId = 0,
		                .destination = { 1, 0 },
		                .destinationAction = std::nullopt,
		            },
		        },
		    },
		},
	};
	std::vector<dev::MovementScriptRunResult> results {
		completed,
		{ .status = dev::MovementScriptRunStatus::LoadFailed },
		{ .status = dev::MovementScriptRunStatus::NoActiveWorld },
	};

	dev::RuntimeMovementScriptText formatter;

	Expect(formatter.formatResult("movementScript[0]", completed) == "movementScript[0] status=Completed results=2 accepted=1 rejected=1", "runtime movement script text should format one replay result");
	Expect(formatter.formatAggregate("runtime movementScripts", results) == "runtime movementScripts=3 completed=1 loadFailed=1 noActiveWorld=1 accepted=1 rejected=1", "runtime movement script text should format aggregate replay results");
}

void TestRuntimeMovementEventTextFormatsMovementEvents()
{
	dev::RuntimeMovementEventText formatter;
	dev::MovementEvent basicEvent {
		.type = dev::MovementEventType::StepCommitted,
		.playerId = 1,
		.tile = { 2, 3 },
		.commandType = dev::MovementCommandType::WalkTo,
	};
	dev::MovementEvent enemyEvent {
		.type = dev::MovementEventType::EnemyAttackTransitioned,
		.playerId = 2,
		.tile = { 4, 5 },
		.commandType = dev::MovementCommandType::MoveThenAct,
		.enemyId = 90,
		.enemyPursuitStopReason = dev::EnemyPursuitStopReason::AttackRangeReached,
		.enemyPursuitStepsCommitted = 3,
		.enemyAttackTransition = dev::EnemyAttackTransition::WindupStarted,
	};

	Expect(formatter.formatEvent("movementEvent[0]", basicEvent) == "movementEvent[0] type=StepCommitted player=1 tile=(2,3) command=WalkTo", "runtime movement event text should format basic movement event lines");
	Expect(formatter.formatEvent("movementEvent[1]", enemyEvent) == "movementEvent[1] type=EnemyAttackTransitioned player=2 tile=(4,5) command=MoveThenAct enemy=90 pursuitStop=AttackRangeReached pursuitSteps=3 attackTransition=WindupStarted", "runtime movement event text should format enemy movement event details");
}

void TestRuntimeCombatTextFormatsCombatEvents()
{
	dev::RuntimeCombatText formatter;
	dev::CombatEvent event {
		.type = dev::CombatEventType::Defeated,
		.damage = 7,
		.remainingHitPoints = 0,
	};

	Expect(formatter.formatEvent("combatEvent[0]", event) == "combatEvent[0] type=Defeated damage=7 remainingHp=0", "runtime combat text should format combat event lines");
}

void TestRuntimeEffectTextFormatsEffectRequests()
{
	dev::RuntimeEffectText formatter;
	dev::EffectRequest request {
		.type = dev::EffectRequestType::HitStop,
		.tile = { 6, 7 },
	};

	Expect(formatter.formatRequest("effect[0]", request) == "effect[0] type=HitStop tile=(6,7)", "runtime effect text should format effect request lines");
}

void TestRuntimeFrameTraceFormatsEnemyPursuitEvents()
{
	dev::RuntimeFrameReport report;
	report.frameEvents.emit(dev::MovementEvent {
	    .type = dev::MovementEventType::EnemyPursuitStopped,
	    .tile = { 3, 0 },
	    .enemyId = 90,
	    .enemyPursuitStopReason = dev::EnemyPursuitStopReason::AttackRangeReached,
	    .enemyPursuitStepsCommitted = 3,
	});

	std::vector<std::string> lines = dev::RuntimeFrameTrace {}.format(report);

	Expect(ContainsLineFragment(lines, "movementEvent[0] type=EnemyPursuitStopped"), "runtime frame trace should include enemy pursuit event type");
	Expect(ContainsLineFragment(lines, "enemy=90"), "runtime frame trace should include enemy id");
	Expect(ContainsLineFragment(lines, "pursuitStop=AttackRangeReached"), "runtime frame trace should include enemy pursuit stop reason");
	Expect(ContainsLineFragment(lines, "pursuitSteps=3"), "runtime frame trace should include enemy pursuit committed steps");
}

void TestRuntimeFrameTraceFormatsEnemyAttackEvents()
{
	dev::RuntimeFrameReport report;
	report.frameEvents.emit(dev::MovementEvent {
	    .type = dev::MovementEventType::EnemyAttackTransitioned,
	    .tile = { 1, 0 },
	    .enemyId = 91,
	    .enemyAttackTransition = dev::EnemyAttackTransition::WindupCompleted,
	});

	std::vector<std::string> lines = dev::RuntimeFrameTrace {}.format(report);

	Expect(ContainsLineFragment(lines, "movementEvent[0] type=EnemyAttackTransitioned"), "runtime frame trace should include enemy attack event type");
	Expect(ContainsLineFragment(lines, "enemy=91"), "runtime frame trace should include enemy attack id");
	Expect(ContainsLineFragment(lines, "attackTransition=WindupCompleted"), "runtime frame trace should include enemy attack transition");
}

} // namespace

int main()
{
	TestRuntimeFrameTraceFormatsReadableLines();
	TestRuntimeFrameTraceSectionsFormatsRuntimeSourcesInOrder();
	TestRuntimeMovementInputBlockSummaryCountsReasons();
	TestRuntimeMovementInputBlockSummaryTextFormatsReasonCounts();
	TestRuntimePlayerActionTextFormatsMovementBlockReasons();
	TestRuntimeFrameTraceSectionsFormatsLifecycleEventsInOrder();
	TestRuntimeFrameTraceSectionsFormatsSimulationEventsInOrder();
	TestRuntimeSessionTextFormatsResultsAndEvents();
	TestRuntimeInventoryTextFormatsResultsAndEvents();
	TestRuntimeInventoryScriptTextFormatsResultsAndAggregates();
	TestRuntimeMovementScriptTextFormatsResultsAndAggregates();
	TestRuntimeMovementEventTextFormatsMovementEvents();
	TestRuntimeCombatTextFormatsCombatEvents();
	TestRuntimeEffectTextFormatsEffectRequests();
	TestRuntimeFrameTraceFormatsEnemyPursuitEvents();
	TestRuntimeFrameTraceFormatsEnemyAttackEvents();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
