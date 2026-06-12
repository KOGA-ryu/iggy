#include <cstdlib>

#include "scene/camera/CameraRig.hpp"
#include "scene/camera/CameraShake.hpp"
#include "scene/camera/CameraView.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

iggy::CameraViewResult View(iggy::Vec2 cameraPosition, iggy::Vec2 viewportSize, float zoom)
{
	return iggy::CameraView {}.visibleWorldBounds({ cameraPosition }, { viewportSize, zoom });
}

void TestZoomOnePositiveViewport()
{
	const iggy::CameraViewResult result = View({ 0.0F, 0.0F }, { 10.0F, 6.0F }, 1.0F);

	Expect(Near(result.effectiveZoom, 1.0F), "zoom one should preserve effective zoom");
	Expect(NearVec(result.halfExtent, { 5.0F, 3.0F }), "zoom one half extent should be viewport divided by two");
	ExpectBounds(result.bounds, { -5.0F, -3.0F }, { 5.0F, 3.0F }, "zoom one bounds should be centered on camera");
}

void TestZoomGreaterThanOneReducesHalfExtent()
{
	const iggy::CameraViewResult result = View({ 0.0F, 0.0F }, { 10.0F, 6.0F }, 2.0F);

	Expect(Near(result.effectiveZoom, 2.0F), "positive zoom should be effective zoom");
	Expect(NearVec(result.halfExtent, { 2.5F, 1.5F }), "zoom greater than one should reduce half extent");
	ExpectBounds(result.bounds, { -2.5F, -1.5F }, { 2.5F, 1.5F }, "zoom greater than one should reduce visible bounds");
}

void TestZoomLessThanOneIncreasesHalfExtent()
{
	const iggy::CameraViewResult result = View({ 0.0F, 0.0F }, { 10.0F, 6.0F }, 0.5F);

	Expect(Near(result.effectiveZoom, 0.5F), "fractional positive zoom should be effective zoom");
	Expect(NearVec(result.halfExtent, { 10.0F, 6.0F }), "zoom less than one should increase half extent");
	ExpectBounds(result.bounds, { -10.0F, -6.0F }, { 10.0F, 6.0F }, "zoom less than one should increase visible bounds");
}

void TestNonPositiveZoomNormalizesToOne()
{
	const iggy::CameraViewResult zero = View({ 0.0F, 0.0F }, { 10.0F, 6.0F }, 0.0F);
	const iggy::CameraViewResult negative = View({ 0.0F, 0.0F }, { 10.0F, 6.0F }, -2.0F);

	Expect(Near(zero.effectiveZoom, 1.0F) && Near(negative.effectiveZoom, 1.0F), "nonpositive zoom should normalize to one");
	Expect(NearVec(zero.halfExtent, { 5.0F, 3.0F }) && NearVec(negative.halfExtent, { 5.0F, 3.0F }), "nonpositive zoom should use zoom-one half extent");
}

void TestNegativeViewportDimensionsNormalize()
{
	const iggy::CameraViewResult result = View({ 0.0F, 0.0F }, { -10.0F, -6.0F }, 1.0F);

	Expect(NearVec(result.halfExtent, { 5.0F, 3.0F }), "negative viewport dimensions should normalize by absolute value");
	ExpectBounds(result.bounds, { -5.0F, -3.0F }, { 5.0F, 3.0F }, "negative viewport dimensions should produce ordered bounds");
}

void TestZeroViewportDimensionProducesDegenerateAxis()
{
	const iggy::CameraViewResult result = View({ 2.0F, 3.0F }, { 0.0F, 6.0F }, 1.0F);

	Expect(NearVec(result.halfExtent, { 0.0F, 3.0F }), "zero viewport width should produce zero x half extent");
	ExpectBounds(result.bounds, { 2.0F, 0.0F }, { 2.0F, 6.0F }, "zero viewport width should produce degenerate x bounds");
}

void TestCameraPositionIsCenter()
{
	const iggy::CameraViewResult result = View({ 10.0F, -4.0F }, { 8.0F, 4.0F }, 1.0F);

	Expect(NearVec(result.halfExtent, { 4.0F, 2.0F }), "nonzero camera position should not change half extent");
	ExpectBounds(result.bounds, { 6.0F, -6.0F }, { 14.0F, -2.0F }, "bounds should shift around camera center");
}

void TestRigShakeViewManualComposition()
{
	iggy::CameraRigConfig rigConfig;
	rigConfig.follow = { 10.0F, 0.0F };
	const iggy::CameraRigResult rig = iggy::CameraRig {}.update({ { 0.0F, 0.0F } }, { 2.0F, 0.0F }, rigConfig);
	const iggy::CameraShakeResult shake = iggy::CameraShake {}.step({ { 2.0F, 0.0F, 9 }, 0.25F, { 1.0F, 8.0F, 0.0F } });
	const iggy::CameraState presentation { rig.state.position + shake.offset };
	const iggy::CameraViewResult view = iggy::CameraView {}.visibleWorldBounds(presentation, { { 4.0F, 2.0F }, 1.0F });

	Expect(rig.followed, "composition rig should follow target");
	Expect(shake.active, "composition shake should be active");
	Expect(NearVec(rig.state.position, { 2.0F, 0.0F }), "view query should not mutate base rig state");
	Expect(NearVec(view.halfExtent, { 2.0F, 1.0F }), "composition view should use configured viewport");
	ExpectBounds(view.bounds, presentation.position - view.halfExtent, presentation.position + view.halfExtent, "view bounds should center on presentation state");
}

} // namespace

int main()
{
	TestZoomOnePositiveViewport();
	TestZoomGreaterThanOneReducesHalfExtent();
	TestZoomLessThanOneIncreasesHalfExtent();
	TestNonPositiveZoomNormalizesToOne();
	TestNegativeViewportDimensionsNormalize();
	TestZeroViewportDimensionProducesDegenerateAxis();
	TestCameraPositionIsCenter();
	TestRigShakeViewManualComposition();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
