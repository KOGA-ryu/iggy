#include "RuntimeDebugManifestSetupText.hpp"

#include <sstream>

namespace dev {

namespace {

const char *BoolText(bool value)
{
	return value ? "true" : "false";
}

} // namespace

std::string RuntimeDebugManifestSetupText::format(const RuntimeSetupResult &setup) const
{
	std::ostringstream line;
	line << "setup startupScriptRan=" << BoolText(setup.startupScriptRan)
	     << " inventoryScriptRan=" << BoolText(setup.inventoryScriptRan)
	     << " movementScriptRan=" << BoolText(setup.movementScriptRan);
	return line.str();
}

} // namespace dev
