#include "RuntimeDebugArtifactBundleResultBuilder.hpp"

namespace dev {

RuntimeDebugArtifactBundleResultBuilder::RuntimeDebugArtifactBundleResultBuilder(const RuntimeDebugArtifactPaths &paths)
    : bundle_({
          .rootPath = paths.rootPath,
          .manifestPath = paths.manifestPath,
          .tracePath = paths.tracePath,
      })
{
}

void RuntimeDebugArtifactBundleResultBuilder::markRootPrepared()
{
	bundle_.rootPrepared = true;
}

void RuntimeDebugArtifactBundleResultBuilder::recordWrite(const RuntimeDebugArtifactWriteResult &write)
{
	bundle_.traceSaved = write.traceSaved;
	bundle_.manifestSaved = write.manifestSaved;
}

RuntimeDebugArtifactBundleResult RuntimeDebugArtifactBundleResultBuilder::result() const
{
	return bundle_;
}

} // namespace dev
