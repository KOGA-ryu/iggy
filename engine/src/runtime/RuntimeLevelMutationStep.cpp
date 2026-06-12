#include "runtime/RuntimeLevelMutationStep.hpp"

namespace iggy::runtime {

namespace {

void MirrorLegacyRenderCache(RuntimeSessionState &session)
{
	session.hasRenderCache = session.derivedCaches.hasRenderCache;
	if (session.hasRenderCache) {
		session.renderCache = session.derivedCaches.render;
		return;
	}

	session.renderCache = {};
}

} // namespace

RuntimeLevelMutationResult RuntimeLevelMutationStep::apply(
	const RuntimeSessionState &session,
	const std::vector<LevelTileEdit> &edits) const
{
	RuntimeLevelMutationResult result;
	result.session = session;
	result.levelMutation = LevelMutationCacheUpdateStep {}.apply(session.level, session.derivedCaches, edits);

	if (result.levelMutation.status == LevelMutationCacheUpdateStatus::CacheUpdateFailed) {
		result.status = RuntimeLevelMutationStatus::Failed;
		return result;
	}

	result.status = result.levelMutation.status == LevelMutationCacheUpdateStatus::NoMutation
		? RuntimeLevelMutationStatus::NoMutation
		: RuntimeLevelMutationStatus::Applied;
	result.session.level = result.levelMutation.level;
	result.session.derivedCaches = result.levelMutation.derivedCaches;
	MirrorLegacyRenderCache(result.session);
	return result;
}

} // namespace iggy::runtime
