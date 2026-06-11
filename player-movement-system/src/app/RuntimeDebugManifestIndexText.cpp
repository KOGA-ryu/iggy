#include "RuntimeDebugManifestIndexText.hpp"

namespace dev {

namespace {

const char *BoolText(bool value)
{
	return value ? "true" : "false";
}

} // namespace

std::vector<std::string> RuntimeDebugManifestIndexText::format(const RuntimeDebugManifestContext &context) const
{
	return {
		"bundle version=1",
		"trace=run.trace saved=" + std::string { BoolText(context.traceSaved) },
	};
}

} // namespace dev
