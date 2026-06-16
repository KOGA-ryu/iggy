#include "scene/ai/NpcMapPlayControlExplainLedger.hpp"

namespace {

iggy::NpcMapPlayControlExplainRow Row(
	iggy::NpcMapPlayControlExplainLedger &ledger,
	iggy::NpcMapPlayControlExplainEvent event,
	const iggy::ResourceId &npcId = {},
	const iggy::ResourceId &profileId = {})
{
	iggy::NpcMapPlayControlExplainRow row;
	row.index = ledger.rows.size();
	row.event = event;
	row.npcId = npcId;
	row.profileId = profileId;
	return row;
}

void AddRow(
	iggy::NpcMapPlayControlExplainLedger &ledger,
	iggy::NpcMapPlayControlExplainRow row)
{
	row.index = ledger.rows.size();
	ledger.rows.push_back(row);
}

void AddProfileRows(
	iggy::NpcMapPlayControlExplainLedger &ledger,
	const iggy::NpcAiProfileTraitResolveResult &profile)
{
	for (const iggy::NpcAiProfileTraitResolveEntry &entry : profile.entries) {
		if (entry.status == iggy::NpcAiProfileTraitResolveStatus::ActorNotPresent) {
			iggy::NpcMapPlayControlExplainRow row = Row(
				ledger,
				iggy::NpcMapPlayControlExplainEvent::ProfileAbsentSkipped,
				entry.actor.npcId,
				entry.actor.aiProfileId);
			row.hasActorIndex = true;
			row.actorIndex = entry.actorIndex;
			AddRow(ledger, row);
			++ledger.profileAbsentSkippedCount;
			continue;
		}

		if (entry.status == iggy::NpcAiProfileTraitResolveStatus::EmptyActorProfileId) {
			iggy::NpcMapPlayControlExplainRow row = Row(
				ledger,
				iggy::NpcMapPlayControlExplainEvent::ProfileEmptyProfileId,
				entry.actor.npcId,
				entry.actor.aiProfileId);
			row.hasActorIndex = true;
			row.actorIndex = entry.actorIndex;
			AddRow(ledger, row);
			++ledger.profileEmptyIdCount;
			continue;
		}

		if (entry.status == iggy::NpcAiProfileTraitResolveStatus::MissingProfile) {
			iggy::NpcMapPlayControlExplainRow row = Row(
				ledger,
				iggy::NpcMapPlayControlExplainEvent::ProfileMissing,
				entry.actor.npcId,
				entry.actor.aiProfileId);
			row.hasActorIndex = true;
			row.actorIndex = entry.actorIndex;
			AddRow(ledger, row);
			++ledger.profileMissingCount;
			continue;
		}

		if (entry.resolved) {
			iggy::NpcMapPlayControlExplainRow row = Row(
				ledger,
				iggy::NpcMapPlayControlExplainEvent::ProfileResolved,
				entry.actor.npcId,
				entry.actor.aiProfileId);
			row.hasActorIndex = true;
			row.actorIndex = entry.actorIndex;
			AddRow(ledger, row);
			++ledger.profileResolvedCount;
		}
	}
}

void AddPlanEntryStatusRow(
	iggy::NpcMapPlayControlExplainLedger &ledger,
	const iggy::NpcMapPlayControlFramePlanEntry &entry,
	iggy::NpcMapPlayControlExplainEvent event)
{
	iggy::NpcMapPlayControlExplainRow row = Row(ledger, event, entry.frame.actor.npcId);
	row.hasFrameIndex = true;
	row.frameIndex = entry.frameIndex;
	AddRow(ledger, row);
}

void AddPlanRows(
	iggy::NpcMapPlayControlExplainLedger &ledger,
	const iggy::NpcMapPlayControlFramePlanResult &plan)
{
	for (const iggy::NpcMapPlayControlFramePlanEntry &entry : plan.entries) {
		switch (entry.status) {
		case iggy::NpcMapPlayControlFramePlanEntryStatus::MissingControl:
			AddPlanEntryStatusRow(
				ledger,
				entry,
				iggy::NpcMapPlayControlExplainEvent::MissingControl);
			++ledger.missingControlCount;
			continue;
		case iggy::NpcMapPlayControlFramePlanEntryStatus::MissingTraitSet:
			AddPlanEntryStatusRow(
				ledger,
				entry,
				iggy::NpcMapPlayControlExplainEvent::MissingTraitSet);
			++ledger.missingTraitCount;
			continue;
		case iggy::NpcMapPlayControlFramePlanEntryStatus::ActorNotPresent:
			AddPlanEntryStatusRow(
				ledger,
				entry,
				iggy::NpcMapPlayControlExplainEvent::ActorNotPresent);
			++ledger.actorNotPresentCount;
			continue;
		case iggy::NpcMapPlayControlFramePlanEntryStatus::RequestPrepared:
			break;
		}

		iggy::NpcMapPlayControlExplainRow hand = Row(
			ledger,
			iggy::NpcMapPlayControlExplainEvent::HandDrawn,
			entry.frame.actor.npcId);
		hand.hasFrameIndex = true;
		hand.frameIndex = entry.frameIndex;
		if (entry.requestIndex) {
			hand.hasRequestIndex = true;
			hand.requestIndex = *entry.requestIndex;
		}
		AddRow(ledger, hand);
		++ledger.handDrawnCount;
		ledger.handIssueCount += entry.hand.issues.size();
		if (!entry.hand.issues.empty()) {
			iggy::NpcMapPlayControlExplainRow issue = hand;
			issue.event = iggy::NpcMapPlayControlExplainEvent::HandIssueObserved;
			AddRow(ledger, issue);
		}

		iggy::NpcMapPlayControlExplainRow map = hand;
		map.event = iggy::NpcMapPlayControlExplainEvent::MapQueried;
		AddRow(ledger, map);
		++ledger.mapQueryCount;
	}
}

const iggy::NpcPlayControlFrameApplyEntry2D *FindApplyEntry(
	const iggy::NpcMapPlayControlFrameStep2DResult &step,
	std::size_t proposalIndex)
{
	for (const iggy::NpcPlayControlFrameApplyEntry2D &entry : step.apply.entries) {
		if (entry.proposalIndex == proposalIndex)
			return &entry;
	}
	return nullptr;
}

void AddStepRows(
	iggy::NpcMapPlayControlExplainLedger &ledger,
	const iggy::NpcMapPlayControlFrameStep2DResult &step)
{
	for (const iggy::NpcMapPlayControlFrameStep2DEntry &entry : step.entries) {
		iggy::NpcMapPlayControlExplainRow base = Row(
			ledger,
			iggy::NpcMapPlayControlExplainEvent::PlayFolded,
			entry.request.npcId);
		base.hasRequestIndex = true;
		base.requestIndex = entry.requestIndex;

		if (entry.mapPlay.mapChangedSelection) {
			iggy::NpcMapPlayControlExplainRow row = base;
			row.event = iggy::NpcMapPlayControlExplainEvent::MapSelectionChanged;
			row.actionTag = entry.mapPlay.mapSelectedActionTag;
			AddRow(ledger, row);
			++ledger.mapChangedSelectionCount;
		}

		{
			iggy::NpcMapPlayControlExplainRow row = base;
			row.event = entry.mapPlay.kept()
				? iggy::NpcMapPlayControlExplainEvent::PlayKept
				: iggy::NpcMapPlayControlExplainEvent::PlayFolded;
			row.actionTag = entry.mapPlay.fold.tell.play.selected.ent.actionTag;
			AddRow(ledger, row);
			if (entry.mapPlay.kept())
				++ledger.playKeptCount;
			else
				++ledger.playFoldedCount;
		}

		{
			iggy::NpcMapPlayControlExplainRow row = base;
			row.event = entry.proposal.hasProposal()
				? iggy::NpcMapPlayControlExplainEvent::ControlProposed
				: iggy::NpcMapPlayControlExplainEvent::ControlFailed;
			row.actionTag = entry.proposal.actionTag;
			AddRow(ledger, row);
			if (entry.proposal.hasProposal())
				++ledger.controlProposedCount;
			else
				++ledger.controlFailedCount;
		}

		const iggy::NpcPlayControlFrameApplyEntry2D *apply =
			FindApplyEntry(step, entry.requestIndex);
		if (apply != nullptr) {
			iggy::NpcMapPlayControlExplainRow row = base;
			row.event = apply->apply.applied()
				? iggy::NpcMapPlayControlExplainEvent::ControlApplied
				: iggy::NpcMapPlayControlExplainEvent::ControlApplyFailed;
			row.actionTag = apply->request.proposal.actionTag;
			AddRow(ledger, row);
			if (apply->apply.applied())
				++ledger.controlAppliedCount;
			else
				++ledger.controlApplyFailedCount;
		}
	}
}

void Finalize(iggy::NpcMapPlayControlExplainLedger &ledger)
{
	ledger.status = ledger.rows.empty()
		? iggy::NpcMapPlayControlExplainLedgerStatus::Empty
		: iggy::NpcMapPlayControlExplainLedgerStatus::Reported;
}

} // namespace

namespace iggy {

bool NpcMapPlayControlExplainLedger::empty() const
{
	return status == NpcMapPlayControlExplainLedgerStatus::Empty;
}

bool NpcMapPlayControlExplainLedger::hasRows() const
{
	return !rows.empty();
}

bool NpcMapPlayControlExplainLedger::hasControlChanges() const
{
	return controlAppliedCount > 0;
}

NpcMapPlayControlExplainLedger NpcMapPlayControlExplainLedgerReporter::report(
	const NpcMapPlayControlFramePlanResult &plan,
	const NpcMapPlayControlFrameStep2DResult &step) const
{
	NpcMapPlayControlExplainLedger ledger;
	ledger.plan = plan;
	ledger.step = step;
	AddPlanRows(ledger, plan);
	AddStepRows(ledger, step);
	Finalize(ledger);
	return ledger;
}

NpcMapPlayControlExplainLedger NpcMapPlayControlExplainLedgerReporter::report(
	const NpcAiProfileTraitResolveResult &profile,
	const NpcMapPlayControlFramePlanResult &plan,
	const NpcMapPlayControlFrameStep2DResult &step) const
{
	NpcMapPlayControlExplainLedger ledger;
	ledger.hasProfile = true;
	ledger.profile = profile;
	ledger.plan = plan;
	ledger.step = step;
	AddProfileRows(ledger, profile);
	AddPlanRows(ledger, plan);
	AddStepRows(ledger, step);
	Finalize(ledger);
	return ledger;
}

} // namespace iggy
