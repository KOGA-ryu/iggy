#pragma once

#include "runtime/RuntimeGameplayAsciiSourcePlanProfileScenarioConverter.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanTomlFileReader.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanTomlReader.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanValidator.hpp"
#include "runtime/RuntimeGameplayProfileScenarioRunner.hpp"
#include "runtime/RuntimeGameplayProfileScenarioValidator.hpp"
#include "runtime/RuntimeGameplayScenarioAuthoringAdapter.hpp"
#include "runtime/RuntimeGameplayScenarioValidator.hpp"

namespace iggy::runtime {

[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanTomlFileReadStatus status);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanTomlReadStatus status);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode code);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanTomlReadIssueCode code);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanIssueCode code);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayScenarioAuthoringAdapterStatus status);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayScenarioAuthoringSource source);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayScenarioAuthoringAdapterIssueCode code);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus status);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode code);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayProfileScenarioIssueCode code);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayProfileScenarioValidationStatus status);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayScenarioIssueCode code);
[[nodiscard]] const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayProfileScenarioRunStatus status);

} // namespace iggy::runtime
