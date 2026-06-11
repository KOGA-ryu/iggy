#include "RuntimeFrameTrace.hpp"

#include "app/RuntimeFramePolicyText.hpp"
#include "app/RuntimeFrameTraceHeaderText.hpp"
#include "app/RuntimeFrameTraceSections.hpp"

namespace dev {

namespace {

void AppendLines(std::vector<std::string> &lines, const std::vector<std::string> &section)
{
	lines.insert(lines.end(), section.begin(), section.end());
}

} // namespace

std::vector<std::string> RuntimeFrameTrace::format(const RuntimeFrameReport &report) const
{
	std::vector<std::string> lines;
	lines.push_back(RuntimeFrameTraceHeaderText {}.format(report));

	{
		lines.push_back(RuntimeFramePolicyText {}.format(
		    "policy mode",
		    report.framePolicy,
		    RuntimeFramePolicyBoolStyle::Numeric));
	}

	RuntimeFrameTraceSections sections;
	AppendLines(lines, sections.formatRuntimeSources(report));
	AppendLines(lines, sections.formatLifecycleEvents(report));
	AppendLines(lines, sections.formatSimulationEvents(report));

	return lines;
}

} // namespace dev
