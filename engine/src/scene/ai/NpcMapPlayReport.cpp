#include "scene/ai/NpcMapPlayReport.hpp"

#include "scene/ai/NpcMapReadProjection.hpp"

namespace iggy {

bool NpcMapPlayReport::kept() const
{
	return fold.kept();
}

bool NpcMapPlayReport::folded() const
{
	return fold.folded();
}

NpcMapPlayReport NpcMapPlayReporter::report(
	const NpcHand &hand,
	const AiMapQuery2DResult &map,
	const NpcReadConfig &rawConfig,
	const NpcMapReadConfig &mapConfig,
	const NpcFoldConfig &foldConfig) const
{
	NpcMapPlayReport report;
	report.hand = hand;
	report.map = map;
	report.rawRead = NpcReader {}.read(hand, rawConfig);
	report.mapRead = NpcMapReader {}.read(hand, map, mapConfig);
	report.projectedMapRead = NpcMapReadProjection {}.toRead(report.mapRead);
	report.play = NpcPlaySelector {}.play(report.projectedMapRead);
	report.tell = NpcTeller {}.tell(report.play);
	report.fold = NpcFolder {}.fold(report.tell, foldConfig);
	report.rawRankedCount = report.rawRead.rankedEnts.size();
	report.mapRankedCount = report.mapRead.rankedEnts.size();

	report.hasRawSelection = !report.rawRead.rankedEnts.empty();
	if (report.hasRawSelection) {
		const NpcReadEnt &selected = report.rawRead.rankedEnts.front();
		report.rawSelectedActionTag = selected.ent.actionTag;
		report.rawSelectedBehaviorState = selected.ent.behaviorState;
	}

	report.hasMapSelection = report.play.hasPlay();
	if (report.hasMapSelection) {
		report.mapSelectedActionTag = report.play.selected.ent.actionTag;
		report.mapSelectedBehaviorState = report.play.selected.ent.behaviorState;
	}

	report.mapChangedSelection = report.hasRawSelection && report.hasMapSelection
		&& (report.rawSelectedActionTag != report.mapSelectedActionTag
			|| report.rawSelectedBehaviorState != report.mapSelectedBehaviorState);

	return report;
}

} // namespace iggy
