#include "runtime/RuntimeGameplayProductPointerProjection.hpp"

#include <cstdlib>

#include "support/GeometryAssertions.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

using Status =
	iggy::runtime::RuntimeGameplayProductPointerProjectionStatus;

iggy::runtime::RuntimeGameplayProductPointerProjectionInput Input(
	iggy::Vec2 viewportPoint,
	iggy::Vec2 viewportSize = { 4.0F, 2.0F },
	iggy::Vec2 cameraPosition = { 10.0F, 20.0F },
	float zoom = 1.0F)
{
	iggy::runtime::RuntimeGameplayProductPointerProjectionInput input;
	input.viewportPoint = viewportPoint;
	input.viewportSize = viewportSize;
	input.camera.position = cameraPosition;
	input.cameraView.viewportSize = { 99.0F, 99.0F };
	input.cameraView.zoom = zoom;
	return input;
}

iggy::runtime::RuntimeGameplayProductPointerProjectionResult Project(
	const iggy::runtime::RuntimeGameplayProductPointerProjectionInput &input)
{
	return iggy::runtime::RuntimeGameplayProductPointerProjection {}.project(
		input);
}

void TestViewportCenterMapsToCameraPosition()
{
	const auto result = Project(Input({ 2.0F, 1.0F }));

	Expect(result.status == Status::Projected,
		"pointer projection should report projected status");
	Expect(NearVec(result.worldPoint, { 10.0F, 20.0F }),
		"viewport center should map to camera position");
	Expect(result.tile == iggy::TileCoord { 10, 20 },
		"center world point should project to tileForPoint tile");
}

void TestCornersMapThroughCameraViewHalfExtentsAtZoomOne()
{
	const auto topLeft = Project(Input({ 0.0F, 0.0F }));
	const auto bottomRight = Project(Input({ 4.0F, 2.0F }));

	Expect(NearVec(topLeft.cameraView.halfExtent, { 2.0F, 1.0F }),
		"zoom-one camera view should expose expected half extents");
	Expect(NearVec(topLeft.worldPoint, { 8.0F, 19.0F }),
		"top-left viewport point should map to visible bounds min");
	Expect(NearVec(bottomRight.worldPoint, { 12.0F, 21.0F }),
		"bottom-right viewport point should map to visible bounds max");
}

void TestZoomReducesWorldDeltaConsistentlyWithCameraView()
{
	const auto result = Project(Input(
		{ 4.0F, 2.0F },
		{ 4.0F, 2.0F },
		{ 10.0F, 20.0F },
		2.0F));

	Expect(result.cameraView.effectiveZoom == 2.0F,
		"positive zoom should be forwarded to CameraView");
	Expect(NearVec(result.cameraView.halfExtent, { 1.0F, 0.5F }),
		"zoom two should halve visible world half extent");
	Expect(NearVec(result.worldPoint, { 11.0F, 20.5F }),
		"zoom two bottom-right should use reduced world delta");
}

void TestNonPositiveZoomUsesCameraViewEffectiveZoomOne()
{
	const auto result = Project(Input(
		{ 4.0F, 2.0F },
		{ 4.0F, 2.0F },
		{ 10.0F, 20.0F },
		0.0F));

	Expect(result.cameraView.effectiveZoom == 1.0F,
		"non-positive zoom should normalize to CameraView effective zoom one");
	Expect(NearVec(result.worldPoint, { 12.0F, 21.0F }),
		"non-positive zoom should project like zoom one");
}

void TestNegativeViewportDimensionsNormalizeLikeCameraView()
{
	const auto result = Project(Input(
		{ 4.0F, 2.0F },
		{ -4.0F, -2.0F },
		{ 10.0F, 20.0F }));

	Expect(NearVec(result.cameraView.halfExtent, { 2.0F, 1.0F }),
		"negative viewport dimensions should normalize to positive half extents");
	Expect(NearVec(result.worldPoint, { 12.0F, 21.0F }),
		"negative viewport dimensions should still project against normalized viewport size");
}

void TestZeroViewportAxisProducesDegenerateAxis()
{
	const auto result = Project(Input(
		{ 99.0F, 2.0F },
		{ 0.0F, 4.0F },
		{ 10.0F, 20.0F }));

	Expect(result.degenerateViewportX && !result.degenerateViewportY,
		"zero viewport x axis should be reported as degenerate");
	Expect(NearVec(result.cameraView.halfExtent, { 0.0F, 2.0F }),
		"zero viewport x axis should produce degenerate x half extent");
	Expect(NearVec(result.worldPoint, { 10.0F, 20.0F }),
		"zero viewport x axis should not divide and center y should remain camera y");
}

void TestProjectedTileUsesTileForPointFloorSemantics()
{
	const auto result = Project(Input(
		{ 0.1F, 0.25F },
		{ 1.0F, 1.0F },
		{ -1.0F, -2.0F }));

	Expect(NearVec(result.worldPoint, { -1.4F, -2.25F }),
		"negative fractional projection setup should produce expected world point");
	Expect(result.tile == iggy::tileForPoint(result.worldPoint),
		"projected tile should be tileForPoint(worldPoint)");
	Expect(result.tile == iggy::TileCoord { -2, -3 },
		"negative fractional world point should use floor tile semantics");
}

void TestInputCameraAndConfigAreNotMutated()
{
	auto input = Input(
		{ 2.0F, 1.0F },
		{ 4.0F, 2.0F },
		{ 10.0F, 20.0F },
		2.0F);
	const auto before = input;

	(void) Project(input);

	Expect(NearVec(input.viewportPoint, before.viewportPoint) &&
			NearVec(input.viewportSize, before.viewportSize) &&
			NearVec(input.camera.position, before.camera.position) &&
			NearVec(input.cameraView.viewportSize,
				before.cameraView.viewportSize) &&
			input.cameraView.zoom == before.cameraView.zoom,
		"pointer projection should not mutate input camera or config");
}

} // namespace

int main()
{
	TestViewportCenterMapsToCameraPosition();
	TestCornersMapThroughCameraViewHalfExtentsAtZoomOne();
	TestZoomReducesWorldDeltaConsistentlyWithCameraView();
	TestNonPositiveZoomUsesCameraViewEffectiveZoomOne();
	TestNegativeViewportDimensionsNormalizeLikeCameraView();
	TestZeroViewportAxisProducesDegenerateAxis();
	TestProjectedTileUsesTileForPointFloorSemantics();
	TestInputCameraAndConfigAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
