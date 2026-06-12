#include <cstdlib>
#include <iostream>
#include <string_view>

#include "core/resource/ResourceId.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

void TestResourceIdSplitsNamespaceAndLocalName()
{
	iggy::ResourceId id { "level:cellar_01" };

	Expect(id.value() == "level:cellar_01", "ResourceId should preserve full value");
	Expect(id.namespaceName() == "level", "ResourceId should expose namespace before colon");
	Expect(id.localName() == "cellar_01", "ResourceId should expose local name after colon");
}

void TestResourceIdHandlesUnqualifiedNames()
{
	iggy::ResourceId id { "cellar_01" };

	Expect(id.namespaceName().empty(), "unqualified ResourceId should have empty namespace");
	Expect(id.localName() == "cellar_01", "unqualified ResourceId local name should be full value");
	Expect(!id.empty(), "non-empty ResourceId should report non-empty");
	Expect(iggy::ResourceId {}.empty(), "default ResourceId should be empty");
}

void TestResourceIdComparesFullValue()
{
	Expect(iggy::ResourceId { "item:potion" } == iggy::ResourceId { "item:potion" }, "ResourceId equality should compare full values");
	Expect(iggy::ResourceId { "item:potion" } != iggy::ResourceId { "enemy:potion" }, "ResourceId inequality should compare full values");
}

} // namespace

int main()
{
	TestResourceIdSplitsNamespaceAndLocalName();
	TestResourceIdHandlesUnqualifiedNames();
	TestResourceIdComparesFullValue();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
