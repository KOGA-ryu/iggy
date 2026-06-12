#include "runtime/RuntimeCollisionWorldProvider.hpp"

namespace iggy::runtime {

RuntimeCollisionWorldResult RuntimeCollisionWorldProvider::resolve(
	const RuntimeSessionState &session,
	RuntimeCollisionWorldRequest request) const
{
	RuntimeCollisionWorldResult result;
	if (request.explicitWorld != nullptr) {
		result.source = RuntimeCollisionWorldSource::Explicit;
		result.world = *request.explicitWorld;
		return result;
	}

	if (session.derivedCaches.hasCollisionCache) {
		result.source = RuntimeCollisionWorldSource::SessionCache;
		result.world = session.derivedCaches.collision.world;
	}

	return result;
}

} // namespace iggy::runtime
