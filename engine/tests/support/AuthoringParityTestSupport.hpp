#ifndef IGGY_TEST_SUPPORT_AUTHORING_PARITY_TEST_SUPPORT_HPP
#define IGGY_TEST_SUPPORT_AUTHORING_PARITY_TEST_SUPPORT_HPP

#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

namespace iggy::test {

inline void ExpectAuthoringParity(
	bool condition,
	const std::string &message,
	int &failures)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++failures;
	}
}

inline std::string AuthoringParityMessage(
	std::string_view context,
	const char *suffix)
{
	return std::string(context) + suffix;
}

inline void ExpectTomlScenarioRunSummaryParity(
	const runtime::RuntimeGameplayTomlScenarioFacadeResult &source,
	const runtime::RuntimeGameplayTomlScenarioFacadeResult &packaged,
	std::string_view context,
	int &failures)
{
	ExpectAuthoringParity(
		packaged.runSummary.frameCount == source.runSummary.frameCount,
		AuthoringParityMessage(context, " should match frame count"),
		failures);
	ExpectAuthoringParity(
		packaged.runSummary.acceptedCommandCount ==
			source.runSummary.acceptedCommandCount,
		AuthoringParityMessage(context, " should match accepted command count"),
		failures);
	ExpectAuthoringParity(
		packaged.runSummary.pickedUpCount == source.runSummary.pickedUpCount,
		AuthoringParityMessage(context, " should match pickup count"),
		failures);
	ExpectAuthoringParity(
		packaged.runSummary.interactionChanged ==
			source.runSummary.interactionChanged,
		AuthoringParityMessage(context, " should match interaction changed flag"),
		failures);
	ExpectAuthoringParity(
		packaged.runSummary.npcMovedCount == source.runSummary.npcMovedCount,
		AuthoringParityMessage(context, " should match NPC moved count"),
		failures);
	ExpectAuthoringParity(
		packaged.runSummary.npcBlockedMovementCount ==
			source.runSummary.npcBlockedMovementCount,
		AuthoringParityMessage(context, " should match blocked NPC movement count"),
		failures);
	ExpectAuthoringParity(
		packaged.runSummary.finalRows == source.runSummary.finalRows,
		AuthoringParityMessage(context, " should match final rows"),
		failures);
}

inline void ExpectTomlScenarioExpectationParity(
	const runtime::RuntimeGameplayTomlScenarioFacadeResult &source,
	const runtime::RuntimeGameplayTomlScenarioFacadeResult &packaged,
	std::string_view context,
	int &failures)
{
	ExpectAuthoringParity(
		packaged.expectationComparison.present ==
			source.expectationComparison.present,
		AuthoringParityMessage(context, " should match expectation presence"),
		failures);
	ExpectAuthoringParity(
		packaged.expectationComparison.matched ==
			source.expectationComparison.matched,
		AuthoringParityMessage(context, " should match expectation result"),
		failures);
}

inline void ExpectTomlScenarioTraceParity(
	const runtime::RuntimeGameplayTomlScenarioFacadeResult &source,
	const runtime::RuntimeGameplayTomlScenarioFacadeResult &packaged,
	std::string_view context,
	int &failures)
{
	ExpectAuthoringParity(
		packaged.traceFrames.size() == source.traceFrames.size(),
		AuthoringParityMessage(context, " should match trace frame count"),
		failures);
	if (packaged.traceFrames.size() != source.traceFrames.size())
		return;

	for (std::size_t index = 0; index < source.traceFrames.size(); ++index) {
		ExpectAuthoringParity(
			packaged.traceFrames[index].frameId ==
				source.traceFrames[index].frameId,
			AuthoringParityMessage(context, " should match trace frame id"),
			failures);
		ExpectAuthoringParity(
			packaged.traceFrames[index].acceptedCommandCount ==
				source.traceFrames[index].acceptedCommandCount,
			AuthoringParityMessage(
				context,
				" should match trace accepted command count"),
			failures);
		ExpectAuthoringParity(
			packaged.traceFrames[index].pickedUpCount ==
				source.traceFrames[index].pickedUpCount,
			AuthoringParityMessage(context, " should match trace pickup count"),
			failures);
		ExpectAuthoringParity(
			packaged.traceFrames[index].interactionChanged ==
				source.traceFrames[index].interactionChanged,
			AuthoringParityMessage(context, " should match trace interaction flag"),
			failures);
		ExpectAuthoringParity(
			packaged.traceFrames[index].npcMovedCount ==
				source.traceFrames[index].npcMovedCount,
			AuthoringParityMessage(context, " should match trace NPC moved count"),
			failures);
		ExpectAuthoringParity(
			packaged.traceFrames[index].rows == source.traceFrames[index].rows,
			AuthoringParityMessage(context, " should match trace rows"),
			failures);
	}
}

} // namespace iggy::test

#endif
