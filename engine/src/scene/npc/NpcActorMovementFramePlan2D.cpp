#include "scene/npc/NpcActorMovementFramePlan2D.hpp"

namespace {

bool FilterPreparesRequest(const iggy::NpcActorPathStepOccupancyFilter2D &filter)
{
	return filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed
		|| filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc;
}

void CountEntry(
	iggy::NpcActorMovementFramePlan2DResult &result,
	const iggy::NpcActorMovementFramePlan2DEntry &entry)
{
	switch (entry.status) {
	case iggy::NpcActorMovementFramePlan2DEntryStatus::RequestPrepared:
		++result.preparedCount;
		if (entry.filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc) {
			++result.blockedRequestCount;
		}
		return;
	case iggy::NpcActorMovementFramePlan2DEntryStatus::NoMovementIntent:
		++result.noMovementIntentCount;
		return;
	case iggy::NpcActorMovementFramePlan2DEntryStatus::RouteTargetFailed:
		++result.routeFailedCount;
		return;
	case iggy::NpcActorMovementFramePlan2DEntryStatus::EscapeRouteTargetFailed:
		++result.escapeRouteFailedCount;
		return;
	case iggy::NpcActorMovementFramePlan2DEntryStatus::NavigationRequestFailed:
		++result.navigationFailedCount;
		return;
	case iggy::NpcActorMovementFramePlan2DEntryStatus::PathNotFound:
		++result.pathFailedCount;
		return;
	case iggy::NpcActorMovementFramePlan2DEntryStatus::StepNotProposed:
		++result.stepNotProposedCount;
		return;
	}
}

} // namespace

namespace iggy {

bool NpcActorMovementFramePlan2DResult::hasRequests() const
{
	return requestCount > 0;
}

NpcActorMovementFramePlan2DResult NpcActorMovementFramePlanner2D::plan(
	const NpcActorState2DRegistry &actors,
	const NpcActorControlState2DRegistry &controls,
	const LevelTileMap &map,
	const NpcActorMovementFramePlan2DConfig &config) const
{
	NpcActorMovementFramePlan2DResult result;
	result.frameState = NpcActorFrameStateProjector2D {}.project(actors, controls);
	result.movementIntents = NpcActorMovementFrameIntentProjector2D {}.project(result.frameState, config.intent);
	result.occupancy = NpcActorOccupancyProjector2D {}.project(actors, config.occupancy);

	for (const NpcActorMovementFrameIntent2DEntry &intentEntry : result.movementIntents.entries) {
		NpcActorMovementFramePlan2DEntry entry;
		entry.frameIndex = intentEntry.frameIndex;
		entry.intent = intentEntry.intent;

		if (!entry.intent.ready() || !entry.intent.requestsMovement) {
			entry.status = NpcActorMovementFramePlan2DEntryStatus::NoMovementIntent;
			CountEntry(result, entry);
			result.entries.push_back(entry);
			continue;
		}

		if (entry.intent.type == NpcActorMovementIntent2DType::MoveAwayFrom) {
			entry.escapeRoute = NpcActorEscapeRouteTargetProjector2D {}.project(entry.intent, map, config.escapeRoute);
			entry.route = entry.escapeRoute.route;
			if (!entry.escapeRoute.ready() || !entry.escapeRoute.requestsRoute) {
				entry.status = NpcActorMovementFramePlan2DEntryStatus::EscapeRouteTargetFailed;
				CountEntry(result, entry);
				result.entries.push_back(entry);
				continue;
			}
		} else {
			entry.route = NpcActorRouteTargetProjector2D {}.project(entry.intent, config.route);
			if (!entry.route.ready() || !entry.route.requestsRoute) {
				entry.status = NpcActorMovementFramePlan2DEntryStatus::RouteTargetFailed;
				CountEntry(result, entry);
				result.entries.push_back(entry);
				continue;
			}
		}

		entry.navigation = NpcActorNavigationRequestBuilder2D {}.build(entry.route, map);
		if (!entry.navigation.ready() || !entry.navigation.requestsPath) {
			entry.status = NpcActorMovementFramePlan2DEntryStatus::NavigationRequestFailed;
			CountEntry(result, entry);
			result.entries.push_back(entry);
			continue;
		}

		entry.path = NpcActorPathReporter2D {}.findPath(entry.navigation, map);
		if (!entry.path.hasPath() || !entry.path.requestsStep) {
			entry.status = NpcActorMovementFramePlan2DEntryStatus::PathNotFound;
			CountEntry(result, entry);
			result.entries.push_back(entry);
			continue;
		}

		entry.step = NpcActorPathStepper2D {}.step(entry.path, config.pathStep);
		entry.filter = NpcActorPathStepOccupancyFilterProjector2D {}.filter(
			entry.step,
			result.occupancy,
			config.occupancyFilter);
		if (!FilterPreparesRequest(entry.filter)) {
			entry.status = NpcActorMovementFramePlan2DEntryStatus::StepNotProposed;
			CountEntry(result, entry);
			result.entries.push_back(entry);
			continue;
		}

		entry.status = NpcActorMovementFramePlan2DEntryStatus::RequestPrepared;
		entry.requestPrepared = true;
		entry.requestIndex = result.requests.size();
		entry.request.filter = entry.filter;
		result.requests.push_back(entry.request);
		CountEntry(result, entry);
		result.entries.push_back(entry);
	}

	result.entryCount = result.entries.size();
	result.requestCount = result.requests.size();
	result.status = result.requestCount > 0
		? NpcActorMovementFramePlan2DStatus::Planned
		: NpcActorMovementFramePlan2DStatus::NoRequests;
	return result;
}

} // namespace iggy
