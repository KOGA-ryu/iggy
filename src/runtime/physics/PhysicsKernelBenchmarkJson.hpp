#pragma once

#include <string>

#include "runtime/physics/PhysicsKernelBenchmark.hpp"

namespace iggy3d {

std::string physicsKernelBenchmarkCaseToJson(
    const PhysicsKernelBenchmarkCaseResult& result);
std::string physicsKernelBenchmarkSuiteToJson(
    const PhysicsKernelBenchmarkSuiteResult& result);
std::string physicsKernelBenchmarkSuiteToJson(
    const PhysicsKernelBenchmarkConfig& config);

}  // namespace iggy3d
